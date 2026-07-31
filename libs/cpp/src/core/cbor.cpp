#include "cardano/core/cbor.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>

namespace cardano::core::cbor {
namespace {

[[nodiscard]] bool same_number(double left, double right) noexcept {
  return std::bit_cast<std::uint64_t>(left) == std::bit_cast<std::uint64_t>(right) ||
         (std::isnan(left) && std::isnan(right));
}

[[nodiscard]] bool valid_utf8(ByteSpan bytes) noexcept {
  std::size_t index = 0;
  while (index < bytes.size()) {
    const auto first = std::to_integer<std::uint8_t>(bytes[index]);
    if (first <= 0x7fU) {
      ++index;
      continue;
    }
    std::size_t continuation = 0;
    std::uint32_t value = 0;
    std::uint32_t minimum = 0;
    if ((first & 0xe0U) == 0xc0U) {
      continuation = 1;
      value = first & 0x1fU;
      minimum = 0x80U;
    } else if ((first & 0xf0U) == 0xe0U) {
      continuation = 2;
      value = first & 0x0fU;
      minimum = 0x800U;
    } else if ((first & 0xf8U) == 0xf0U) {
      continuation = 3;
      value = first & 0x07U;
      minimum = 0x10000U;
    } else {
      return false;
    }
    if (index + continuation >= bytes.size()) {
      return false;
    }
    for (std::size_t offset = 1; offset <= continuation; ++offset) {
      const auto next = std::to_integer<std::uint8_t>(bytes[index + offset]);
      if ((next & 0xc0U) != 0x80U) {
        return false;
      }
      value = (value << 6U) | (next & 0x3fU);
    }
    if (value < minimum || value > 0x10ffffU || (value >= 0xd800U && value <= 0xdfffU)) {
      return false;
    }
    index += continuation + 1U;
  }
  return true;
}

[[nodiscard]] std::string bytes_to_string(ByteSpan bytes) {
  std::string output;
  output.reserve(bytes.size());
  for (const auto byte : bytes) {
    output.push_back(static_cast<char>(std::to_integer<unsigned char>(byte)));
  }
  return output;
}

[[nodiscard]] Bytes string_to_bytes(std::string_view value) {
  Bytes output;
  output.reserve(value.size());
  for (const unsigned char character : value) {
    output.push_back(static_cast<Byte>(character));
  }
  return output;
}

[[nodiscard]] std::uint16_t encode_half(double input) noexcept {
  if (std::isnan(input)) {
    return 0x7e00U;
  }
  const auto value = static_cast<float>(input);
  const auto bits = std::bit_cast<std::uint32_t>(value);
  const std::uint16_t sign = static_cast<std::uint16_t>((bits >> 16U) & 0x8000U);
  int exponent = static_cast<int>((bits >> 23U) & 0xffU) - 127 + 15;
  std::uint32_t fraction = bits & 0x7fffffU;
  if (exponent <= 0) {
    if (exponent < -10) {
      return sign;
    }
    fraction = (fraction | 0x800000U) >> static_cast<unsigned>(1 - exponent);
    return static_cast<std::uint16_t>(sign | ((fraction + 0x1000U) >> 13U));
  }
  if (exponent >= 31) {
    return static_cast<std::uint16_t>(sign | 0x7c00U);
  }
  const std::uint32_t rounded = fraction + 0x1000U;
  if ((rounded & 0x800000U) != 0U) {
    ++exponent;
    fraction = 0;
  }
  if (exponent >= 31) {
    return static_cast<std::uint16_t>(sign | 0x7c00U);
  }
  return static_cast<std::uint16_t>(sign | (static_cast<std::uint16_t>(exponent) << 10U) |
                                    ((fraction + 0x1000U) >> 13U));
}

[[nodiscard]] double decode_half(std::uint16_t bits) noexcept {
  const double sign = (bits & 0x8000U) == 0U ? 1.0 : -1.0;
  const std::uint16_t exponent = (bits >> 10U) & 0x1fU;
  const std::uint16_t fraction = bits & 0x03ffU;
  if (exponent == 0U) {
    return sign * std::ldexp(static_cast<double>(fraction) / 1024.0, -14);
  }
  if (exponent == 31U) {
    return fraction == 0U ? sign * std::numeric_limits<double>::infinity()
                          : std::numeric_limits<double>::quiet_NaN();
  }
  return sign * std::ldexp(1.0 + static_cast<double>(fraction) / 1024.0, exponent - 15);
}

[[nodiscard]] FloatWidth canonical_float_width(double value) noexcept {
  if (same_number(decode_half(encode_half(value)), value)) {
    return FloatWidth::half;
  }
  if (same_number(static_cast<double>(static_cast<float>(value)), value)) {
    return FloatWidth::single;
  }
  return FloatWidth::double_precision;
}

[[nodiscard]] std::size_t width_size(HeadWidth width) noexcept {
  return static_cast<std::size_t>(width);
}

[[nodiscard]] bool fits_width(std::uint64_t value, HeadWidth width) noexcept {
  switch (width) {
    case HeadWidth::inline_value:
      return value < 24U;
    case HeadWidth::one:
      return value <= std::numeric_limits<std::uint8_t>::max();
    case HeadWidth::two:
      return value <= std::numeric_limits<std::uint16_t>::max();
    case HeadWidth::four:
      return value <= std::numeric_limits<std::uint32_t>::max();
    case HeadWidth::eight:
      return true;
  }
  return false;
}

[[nodiscard]] HeadWidth minimal_width(std::uint64_t value) noexcept {
  if (value < 24U) {
    return HeadWidth::inline_value;
  }
  if (value <= std::numeric_limits<std::uint8_t>::max()) {
    return HeadWidth::one;
  }
  if (value <= std::numeric_limits<std::uint16_t>::max()) {
    return HeadWidth::two;
  }
  if (value <= std::numeric_limits<std::uint32_t>::max()) {
    return HeadWidth::four;
  }
  return HeadWidth::eight;
}

[[nodiscard]] VoidResult validate_semantic_tag(const BigInteger& tag, const Value& value);

class Reader {
 public:
  Reader(ByteSpan bytes, DecodeOptions options) : bytes_(bytes), limits_(options.limits) {}

  [[nodiscard]] Result<Value> read() {
    auto value = read_value(0);
    if (!value) {
      return value;
    }
    if (offset_ != bytes_.size()) {
      return std::unexpected(
          error(ErrorCode::trailing_data, "trailing data follows the CBOR item", offset_));
    }
    return value;
  }

 private:
  struct Head {
    std::uint64_t value{};
    HeadWidth width{HeadWidth::inline_value};
  };

  [[nodiscard]] CardanoError error(ErrorCode code, std::string message, std::size_t offset) const {
    return CardanoError(code, std::move(message), {}, offset);
  }

  [[nodiscard]] Result<std::uint8_t> take() {
    if (offset_ >= bytes_.size()) {
      return std::unexpected(
          error(ErrorCode::truncated_input, "unexpected end of CBOR input", offset_));
    }
    return std::to_integer<std::uint8_t>(bytes_[offset_++]);
  }

  [[nodiscard]] std::optional<std::uint8_t> peek() const noexcept {
    if (offset_ >= bytes_.size()) {
      return std::nullopt;
    }
    return std::to_integer<std::uint8_t>(bytes_[offset_]);
  }

  [[nodiscard]] Result<Bytes> take_bytes(std::size_t length) {
    if (length > bytes_.size() - offset_) {
      return std::unexpected(
          error(ErrorCode::truncated_input, "CBOR byte range exceeds available input", offset_));
    }
    Bytes output(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
                 bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + length));
    offset_ += length;
    return output;
  }

  [[nodiscard]] Result<Head> read_head(std::uint8_t additional, std::size_t start) {
    if (additional < 24U) {
      return Head{additional, HeadWidth::inline_value};
    }
    HeadWidth width{};
    if (additional == 24U) {
      width = HeadWidth::one;
    } else if (additional == 25U) {
      width = HeadWidth::two;
    } else if (additional == 26U) {
      width = HeadWidth::four;
    } else if (additional == 27U) {
      width = HeadWidth::eight;
    } else {
      return std::unexpected(
          error(ErrorCode::invalid_cbor, "invalid CBOR additional information", start));
    }
    const auto count = width_size(width);
    auto bytes = take_bytes(count);
    if (!bytes) {
      return std::unexpected(bytes.error());
    }
    std::uint64_t value = 0;
    for (const auto byte : *bytes) {
      value = (value << 8U) | std::to_integer<std::uint8_t>(byte);
    }
    return Head{value, width};
  }

  [[nodiscard]] Result<std::size_t> checked_length(std::uint64_t value, std::size_t maximum,
                                                   std::string_view kind,
                                                   std::size_t offset) const {
    if (value > maximum || value > std::numeric_limits<std::size_t>::max()) {
      return std::unexpected(error(ErrorCode::resource_limit_exceeded,
                                   std::string(kind) + " exceeds its configured limit", offset));
    }
    return static_cast<std::size_t>(value);
  }

  [[nodiscard]] Result<Value> read_value(std::size_t depth) {
    if (depth > limits_.max_depth) {
      return std::unexpected(
          error(ErrorCode::depth_limit_exceeded, "CBOR nesting depth limit exceeded", offset_));
    }
    ++tokens_;
    if (tokens_ > limits_.max_tokens) {
      return std::unexpected(
          error(ErrorCode::resource_limit_exceeded, "CBOR token limit exceeded", offset_));
    }
    const std::size_t start = offset_;
    auto initial_result = take();
    if (!initial_result) {
      return std::unexpected(initial_result.error());
    }
    const auto initial = *initial_result;
    if (initial == 0xffU) {
      return std::unexpected(error(ErrorCode::invalid_structure, "unexpected CBOR break", start));
    }
    const std::uint8_t major = initial >> 5U;
    const std::uint8_t additional = initial & 31U;

    if (major == 0U || major == 1U || major == 6U) {
      if (additional == 31U) {
        return std::unexpected(error(ErrorCode::invalid_cbor,
                                     "indefinite marker is invalid for this CBOR type", start));
      }
      auto head = read_head(additional, start);
      if (!head) {
        return std::unexpected(head.error());
      }
      if (major == 0U) {
        return Value::unsigned_integer(BigInteger(head->value), head->width)
            .with_span({start, offset_});
      }
      if (major == 1U) {
        return Value::negative_integer(BigInteger(std::int64_t{-1}) - BigInteger(head->value),
                                       head->width)
            .with_span({start, offset_});
      }
      auto nested = read_value(depth + 1U);
      if (!nested) {
        return std::unexpected(nested.error().at("tag"));
      }
      auto validation = validate_semantic_tag(BigInteger(head->value), *nested);
      if (!validation) {
        return std::unexpected(validation.error());
      }
      return Value::tag(BigInteger(head->value), std::move(*nested), head->width)
          .with_span({start, offset_});
    }

    if (major == 2U || major == 3U) {
      if (additional == 31U) {
        std::size_t total = 0;
        Bytes byte_value;
        std::string text_value;
        std::vector<ByteChunk> byte_chunks;
        std::vector<TextChunk> text_chunks;
        while (peek() != std::optional<std::uint8_t>(0xffU)) {
          const auto chunk_start = offset_;
          auto chunk_initial_result = take();
          if (!chunk_initial_result) {
            return std::unexpected(error(ErrorCode::truncated_input,
                                         "indefinite CBOR string is missing its break", offset_));
          }
          const auto chunk_initial = *chunk_initial_result;
          if ((chunk_initial >> 5U) != major || (chunk_initial & 31U) == 31U) {
            return std::unexpected(error(ErrorCode::invalid_structure,
                                         "indefinite CBOR string contains an invalid chunk",
                                         chunk_start));
          }
          auto chunk_head = read_head(chunk_initial & 31U, chunk_start);
          if (!chunk_head) {
            return std::unexpected(chunk_head.error());
          }
          auto chunk_length = checked_length(chunk_head->value, limits_.max_string_bytes,
                                             "CBOR string", chunk_start);
          if (!chunk_length || *chunk_length > limits_.max_string_bytes - total) {
            return std::unexpected(error(ErrorCode::resource_limit_exceeded,
                                         "indefinite CBOR string exceeds its configured limit",
                                         chunk_start));
          }
          total += *chunk_length;
          auto chunk_bytes = take_bytes(*chunk_length);
          if (!chunk_bytes) {
            return std::unexpected(chunk_bytes.error());
          }
          if (major == 2U) {
            byte_value.insert(byte_value.end(), chunk_bytes->begin(), chunk_bytes->end());
            byte_chunks.push_back(ByteChunk{std::move(*chunk_bytes), chunk_head->width});
          } else {
            if (!valid_utf8(*chunk_bytes)) {
              return std::unexpected(error(ErrorCode::invalid_utf8,
                                           "CBOR text chunk is not valid UTF-8", chunk_start));
            }
            auto text = bytes_to_string(*chunk_bytes);
            text_value += text;
            text_chunks.push_back(TextChunk{std::move(text), chunk_head->width});
          }
        }
        auto break_result = take();
        if (!break_result) {
          return std::unexpected(break_result.error());
        }
        if (major == 2U) {
          return Value::byte_string(
                     std::move(byte_value),
                     ByteStringEncoding{true, HeadWidth::inline_value, std::move(byte_chunks)})
              .with_span({start, offset_});
        }
        return Value::text_string(
                   std::move(text_value),
                   TextStringEncoding{true, HeadWidth::inline_value, std::move(text_chunks)})
            .with_span({start, offset_});
      }
      auto head = read_head(additional, start);
      if (!head) {
        return std::unexpected(head.error());
      }
      auto length = checked_length(head->value, limits_.max_string_bytes, "CBOR string", start);
      if (!length) {
        return std::unexpected(length.error());
      }
      auto contents = take_bytes(*length);
      if (!contents) {
        return std::unexpected(contents.error());
      }
      if (major == 2U) {
        return Value::byte_string(std::move(*contents), ByteStringEncoding{false, head->width, {}})
            .with_span({start, offset_});
      }
      if (!valid_utf8(*contents)) {
        return std::unexpected(
            error(ErrorCode::invalid_utf8, "CBOR text string is not valid UTF-8", start));
      }
      return Value::text_string(bytes_to_string(*contents),
                                TextStringEncoding{false, head->width, {}})
          .with_span({start, offset_});
    }

    if (major == 4U || major == 5U) {
      const bool indefinite = additional == 31U;
      Head head{};
      std::optional<std::size_t> length;
      if (!indefinite) {
        auto result = read_head(additional, start);
        if (!result) {
          return std::unexpected(result.error());
        }
        head = *result;
        auto checked =
            checked_length(head.value, limits_.max_collection_length, "CBOR collection", start);
        if (!checked) {
          return std::unexpected(checked.error());
        }
        length = *checked;
      }
      const LengthEncoding encoding{indefinite, indefinite ? HeadWidth::inline_value : head.width};
      if (major == 4U) {
        std::vector<Value> values;
        if (length) {
          values.reserve(*length);
        }
        while (!length || values.size() < *length) {
          if (!length && peek() == std::optional<std::uint8_t>(0xffU)) {
            auto ignored = take();
            (void)ignored;
            break;
          }
          if (!length && !peek()) {
            return std::unexpected(error(ErrorCode::truncated_input,
                                         "indefinite CBOR array is missing its break", offset_));
          }
          if (values.size() >= limits_.max_collection_length) {
            return std::unexpected(error(ErrorCode::resource_limit_exceeded,
                                         "CBOR array exceeds its configured limit", offset_));
          }
          auto nested = read_value(depth + 1U);
          if (!nested) {
            return std::unexpected(nested.error().at(values.size()));
          }
          values.push_back(std::move(*nested));
        }
        return Value::array(std::move(values), encoding).with_span({start, offset_});
      }
      std::vector<std::pair<Value, Value>> entries;
      if (length) {
        entries.reserve(*length);
      }
      while (!length || entries.size() < *length) {
        if (!length && peek() == std::optional<std::uint8_t>(0xffU)) {
          auto ignored = take();
          (void)ignored;
          break;
        }
        if (!length && !peek()) {
          return std::unexpected(error(ErrorCode::truncated_input,
                                       "indefinite CBOR map is missing its break", offset_));
        }
        if (entries.size() >= limits_.max_collection_length) {
          return std::unexpected(error(ErrorCode::resource_limit_exceeded,
                                       "CBOR map exceeds its configured limit", offset_));
        }
        auto key = read_value(depth + 1U);
        if (!key) {
          return std::unexpected(key.error().at("key"));
        }
        if (!length && peek() == std::optional<std::uint8_t>(0xffU)) {
          return std::unexpected(
              error(ErrorCode::invalid_structure,
                    "indefinite CBOR map break appears where a value is required", offset_));
        }
        auto item = read_value(depth + 1U);
        if (!item) {
          return std::unexpected(item.error().at(entries.size()));
        }
        entries.emplace_back(std::move(*key), std::move(*item));
      }
      return Value::map(std::move(entries), encoding).with_span({start, offset_});
    }

    if (major == 7U) {
      if (additional == 20U || additional == 21U) {
        return Value::boolean(additional == 21U).with_span({start, offset_});
      }
      if (additional == 22U) {
        return Value::null().with_span({start, offset_});
      }
      if (additional == 23U) {
        return Value::undefined().with_span({start, offset_});
      }
      if (additional < 20U) {
        return Value::simple(additional).with_span({start, offset_});
      }
      if (additional == 24U) {
        auto simple = take();
        if (!simple) {
          return std::unexpected(simple.error());
        }
        return Value::simple(*simple, HeadWidth::one).with_span({start, offset_});
      }
      if (additional == 25U) {
        auto bytes = take_bytes(2);
        if (!bytes) {
          return std::unexpected(bytes.error());
        }
        const auto bits =
            static_cast<std::uint16_t>((std::to_integer<std::uint16_t>((*bytes)[0]) << 8U) |
                                       std::to_integer<std::uint16_t>((*bytes)[1]));
        return Value::floating(decode_half(bits), FloatWidth::half).with_span({start, offset_});
      }
      if (additional == 26U || additional == 27U) {
        const std::size_t width = additional == 26U ? 4U : 8U;
        auto bytes = take_bytes(width);
        if (!bytes) {
          return std::unexpected(bytes.error());
        }
        std::uint64_t bits = 0;
        for (const auto byte : *bytes) {
          bits = (bits << 8U) | std::to_integer<std::uint8_t>(byte);
        }
        double value = 0;
        if (width == 4U) {
          value = static_cast<double>(std::bit_cast<float>(static_cast<std::uint32_t>(bits)));
        } else {
          value = std::bit_cast<double>(bits);
        }
        return Value::floating(value,
                               width == 4U ? FloatWidth::single : FloatWidth::double_precision)
            .with_span({start, offset_});
      }
      return std::unexpected(error(ErrorCode::invalid_cbor, "invalid CBOR simple value", start));
    }
    return std::unexpected(error(ErrorCode::invalid_cbor, "unknown CBOR major type", start));
  }

  ByteSpan bytes_;
  DecoderLimits limits_;
  std::size_t offset_{};
  std::size_t tokens_{};
};

[[nodiscard]] bool is_semantic_integer(const Value& value) {
  if (value.as_unsigned() != nullptr || value.as_negative() != nullptr) {
    return true;
  }
  const auto* tag = value.as_tag();
  if (tag == nullptr || tag->value == nullptr) {
    return false;
  }
  const auto number = tag->tag.to_uint64();
  return number && (*number == 2U || *number == 3U) && tag->value->as_byte_string() != nullptr;
}

[[nodiscard]] bool is_positive_semantic_integer(const Value& value) {
  if (const auto* integer = value.as_unsigned()) {
    return !integer->value.is_zero();
  }
  const auto* tag = value.as_tag();
  if (tag == nullptr || tag->value == nullptr || tag->tag != BigInteger(std::uint64_t{2})) {
    return false;
  }
  const auto* bytes = tag->value->as_byte_string();
  return bytes != nullptr && !bytes->value.empty() && bytes->value.front() != Byte{0};
}

[[nodiscard]] VoidResult validate_semantic_tag(const BigInteger& tag, const Value& value) {
  const auto number = tag.to_uint64();
  if (!number) {
    return std::monostate{};
  }
  if (*number == 2U || *number == 3U) {
    const auto* bytes = value.as_byte_string();
    if (bytes == nullptr) {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_structure, "CBOR bignum tag requires a byte string"));
    }
  } else if (*number == 30U) {
    const auto* rational = value.as_array();
    if (rational == nullptr || rational->values.size() != 2U ||
        !is_semantic_integer(rational->values[0]) ||
        !is_positive_semantic_integer(rational->values[1])) {
      return std::unexpected(CardanoError(
          ErrorCode::invalid_structure, "CBOR rational tag requires [integer, positive-integer]"));
    }
  } else if (*number == 258U && value.as_array() == nullptr) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_structure, "CBOR set tag requires an array"));
  }
  return std::monostate{};
}

class Writer {
 public:
  explicit Writer(Mode mode) : mode_(mode) {}

  [[nodiscard]] Result<Bytes> write(const Value& value) {
    auto status = write_value(value);
    if (!status) {
      return std::unexpected(status.error());
    }
    return bytes_;
  }

 private:
  [[nodiscard]] VoidResult raw(ByteSpan bytes) {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return std::monostate{};
  }

  [[nodiscard]] VoidResult head(std::uint8_t major, const BigInteger& integer,
                                std::optional<HeadWidth> preferred = std::nullopt) {
    auto value_result = integer.to_uint64();
    if (!value_result) {
      return std::unexpected(
          CardanoError(ErrorCode::out_of_range, "CBOR head value must fit uint64"));
    }
    const auto value = *value_result;
    const HeadWidth width = mode_ == Mode::preserve && preferred && fits_width(value, *preferred)
                                ? *preferred
                                : minimal_width(value);
    const auto width_bytes = width_size(width);
    const std::uint8_t additional = width == HeadWidth::inline_value
                                        ? static_cast<std::uint8_t>(value)
                                    : width == HeadWidth::one  ? 24U
                                    : width == HeadWidth::two  ? 25U
                                    : width == HeadWidth::four ? 26U
                                                               : 27U;
    bytes_.push_back(static_cast<Byte>((major << 5U) | additional));
    for (std::size_t index = width_bytes; index > 0U; --index) {
      const auto shift = static_cast<unsigned>((index - 1U) * 8U);
      bytes_.push_back(static_cast<Byte>((value >> shift) & 0xffU));
    }
    return std::monostate{};
  }

  [[nodiscard]] VoidResult length(std::uint8_t major, std::size_t size,
                                  const LengthEncoding& encoding, bool& indefinite) {
    indefinite = mode_ == Mode::preserve && encoding.indefinite;
    if (indefinite) {
      bytes_.push_back(static_cast<Byte>((major << 5U) | 31U));
      return std::monostate{};
    }
    return head(major, BigInteger(static_cast<std::uint64_t>(size)), encoding.width);
  }

  [[nodiscard]] VoidResult bignum(std::uint64_t tag, const BigInteger& magnitude) {
    const auto encoded = magnitude.to_unsigned_bytes_be();
    if (encoded.empty() || encoded.front() == Byte{0}) {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_structure,
                       "CBOR bignum magnitude must be positive and minimally encoded"));
    }
    auto status = head(6U, BigInteger(tag));
    if (!status) {
      return status;
    }
    status = head(2U, BigInteger(static_cast<std::uint64_t>(encoded.size())));
    if (!status) {
      return status;
    }
    return raw(encoded);
  }

  [[nodiscard]] VoidResult write_float(double value, FloatWidth width) {
    if (width == FloatWidth::half) {
      const auto bits = encode_half(value);
      bytes_.push_back(Byte{0xf9});
      bytes_.push_back(static_cast<Byte>(bits >> 8U));
      bytes_.push_back(static_cast<Byte>(bits & 0xffU));
      return std::monostate{};
    }
    if (width == FloatWidth::single) {
      bytes_.push_back(Byte{0xfa});
      const auto bits = std::bit_cast<std::uint32_t>(static_cast<float>(value));
      for (int shift = 24; shift >= 0; shift -= 8) {
        bytes_.push_back(static_cast<Byte>((bits >> shift) & 0xffU));
      }
      return std::monostate{};
    }
    bytes_.push_back(Byte{0xfb});
    const auto bits = std::bit_cast<std::uint64_t>(value);
    for (int shift = 56; shift >= 0; shift -= 8) {
      bytes_.push_back(static_cast<Byte>((bits >> shift) & 0xffU));
    }
    return std::monostate{};
  }

  [[nodiscard]] Result<Bytes> canonical_bytes(const Value& value) const {
    Writer writer(Mode::canonical);
    return writer.write(value);
  }

  [[nodiscard]] VoidResult write_value(const Value& value) {
    return std::visit(
        [&](const auto& node) -> VoidResult {
          using Node = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<Node, UnsignedValue>) {
            return node.value.fits_uint64() ? head(0U, node.value, node.width)
                                            : bignum(2U, node.value);
          } else if constexpr (std::is_same_v<Node, NegativeValue>) {
            if (!node.value.is_negative()) {
              return std::unexpected(CardanoError(ErrorCode::out_of_range,
                                                  "negative CBOR value must be less than zero"));
            }
            const auto magnitude = BigInteger(std::int64_t{-1}) - node.value;
            return magnitude.fits_uint64() ? head(1U, magnitude, node.width)
                                           : bignum(3U, magnitude);
          } else if constexpr (std::is_same_v<Node, ByteStringValue>) {
            if (mode_ == Mode::preserve && node.encoding.indefinite) {
              Bytes combined;
              for (const auto& chunk : node.encoding.chunks) {
                combined.insert(combined.end(), chunk.value.begin(), chunk.value.end());
              }
              if (bytes_equal(combined, node.value)) {
                bytes_.push_back(Byte{0x5f});
                for (const auto& chunk : node.encoding.chunks) {
                  auto status = head(2U, BigInteger(static_cast<std::uint64_t>(chunk.value.size())),
                                     chunk.width);
                  if (!status) {
                    return status;
                  }
                  auto raw_status = raw(chunk.value);
                  if (!raw_status) {
                    return raw_status;
                  }
                }
                bytes_.push_back(Byte{0xff});
                return std::monostate{};
              }
            }
            auto status = head(2U, BigInteger(static_cast<std::uint64_t>(node.value.size())),
                               node.encoding.width);
            if (!status) {
              return status;
            }
            return raw(node.value);
          } else if constexpr (std::is_same_v<Node, TextStringValue>) {
            if (!valid_utf8(string_to_bytes(node.value))) {
              return std::unexpected(
                  CardanoError(ErrorCode::invalid_utf8, "CBOR text string is not valid UTF-8"));
            }
            if (mode_ == Mode::preserve && node.encoding.indefinite) {
              std::string combined;
              for (const auto& chunk : node.encoding.chunks) {
                combined += chunk.value;
              }
              if (combined == node.value) {
                bytes_.push_back(Byte{0x7f});
                for (const auto& chunk : node.encoding.chunks) {
                  const auto chunk_bytes = string_to_bytes(chunk.value);
                  auto status = head(3U, BigInteger(static_cast<std::uint64_t>(chunk_bytes.size())),
                                     chunk.width);
                  if (!status) {
                    return status;
                  }
                  auto raw_status = raw(chunk_bytes);
                  if (!raw_status) {
                    return raw_status;
                  }
                }
                bytes_.push_back(Byte{0xff});
                return std::monostate{};
              }
            }
            const auto encoded = string_to_bytes(node.value);
            auto status = head(3U, BigInteger(static_cast<std::uint64_t>(encoded.size())),
                               node.encoding.width);
            if (!status) {
              return status;
            }
            return raw(encoded);
          } else if constexpr (std::is_same_v<Node, std::shared_ptr<ArrayValue>>) {
            bool indefinite = false;
            auto status = length(4U, node->values.size(), node->encoding, indefinite);
            if (!status) {
              return status;
            }
            for (const auto& item : node->values) {
              status = write_value(item);
              if (!status) {
                return status;
              }
            }
            if (indefinite) {
              bytes_.push_back(Byte{0xff});
            }
            return std::monostate{};
          } else if constexpr (std::is_same_v<Node, std::shared_ptr<MapValue>>) {
            std::vector<const std::pair<Value, Value>*> entries;
            entries.reserve(node->entries.size());
            for (const auto& entry : node->entries) {
              entries.push_back(&entry);
            }
            if (mode_ == Mode::canonical) {
              std::stable_sort(
                  entries.begin(), entries.end(), [&](const auto* left, const auto* right) {
                    auto left_bytes = canonical_bytes(left->first);
                    auto right_bytes = canonical_bytes(right->first);
                    if (!left_bytes || !right_bytes) {
                      return false;
                    }
                    if (left_bytes->size() != right_bytes->size()) {
                      return left_bytes->size() < right_bytes->size();
                    }
                    return std::lexicographical_compare(left_bytes->begin(), left_bytes->end(),
                                                        right_bytes->begin(), right_bytes->end());
                  });
            }
            bool indefinite = false;
            auto status = length(5U, entries.size(), node->encoding, indefinite);
            if (!status) {
              return status;
            }
            for (const auto* entry : entries) {
              status = write_value(entry->first);
              if (!status) {
                return status;
              }
              status = write_value(entry->second);
              if (!status) {
                return status;
              }
            }
            if (indefinite) {
              bytes_.push_back(Byte{0xff});
            }
            return std::monostate{};
          } else if constexpr (std::is_same_v<Node, std::shared_ptr<TagValue>>) {
            if (node->tag.is_negative()) {
              return std::unexpected(
                  CardanoError(ErrorCode::out_of_range, "CBOR tag must be nonnegative"));
            }
            if (node->value == nullptr) {
              return std::unexpected(
                  CardanoError(ErrorCode::invalid_structure, "CBOR tag must contain a value"));
            }
            auto validation = validate_semantic_tag(node->tag, *node->value);
            if (!validation) {
              return validation;
            }
            const auto tag_number = node->tag.to_uint64();
            const auto* magnitude_bytes = node->value->as_byte_string();
            if (mode_ == Mode::canonical && tag_number &&
                (*tag_number == 2U || *tag_number == 3U) && magnitude_bytes != nullptr) {
              const auto magnitude = BigInteger::from_unsigned_bytes_be(magnitude_bytes->value);
              if (magnitude.fits_uint64()) {
                return head(*tag_number == 2U ? 0U : 1U, magnitude);
              }
              const auto minimal = magnitude.to_unsigned_bytes_be();
              auto status = head(6U, node->tag);
              if (!status) {
                return status;
              }
              status = head(2U, BigInteger(static_cast<std::uint64_t>(minimal.size())));
              if (!status) {
                return status;
              }
              return raw(minimal);
            }
            auto status = head(6U, node->tag, node->width);
            if (!status) {
              return status;
            }
            return write_value(*node->value);
          } else if constexpr (std::is_same_v<Node, BooleanValue>) {
            bytes_.push_back(node.value ? Byte{0xf5} : Byte{0xf4});
            return std::monostate{};
          } else if constexpr (std::is_same_v<Node, NullValue>) {
            bytes_.push_back(Byte{0xf6});
            return std::monostate{};
          } else if constexpr (std::is_same_v<Node, UndefinedValue>) {
            bytes_.push_back(Byte{0xf7});
            return std::monostate{};
          } else if constexpr (std::is_same_v<Node, SimpleValue>) {
            if ((mode_ == Mode::preserve && node.width == HeadWidth::one) || node.value >= 24U) {
              bytes_.push_back(Byte{0xf8});
              bytes_.push_back(static_cast<Byte>(node.value));
            } else {
              bytes_.push_back(static_cast<Byte>(0xe0U | node.value));
            }
            return std::monostate{};
          } else if constexpr (std::is_same_v<Node, FloatingValue>) {
            return write_float(node.value, mode_ == Mode::preserve
                                               ? node.width
                                               : canonical_float_width(node.value));
          }
          return std::unexpected(CardanoError(ErrorCode::internal, "unknown CBOR value variant"));
        },
        value.node());
  }

  Mode mode_;
  Bytes bytes_;
};

[[nodiscard]] bool equal_nodes(const Value& left, const Value& right);

[[nodiscard]] bool equal_nodes(const Value& left, const Value& right) {
  if (left.node().index() != right.node().index()) {
    return false;
  }
  return std::visit(
      [&](const auto& left_node) -> bool {
        using Node = std::decay_t<decltype(left_node)>;
        const auto* right_node = std::get_if<Node>(&right.node());
        if (right_node == nullptr) {
          return false;
        }
        if constexpr (std::is_same_v<Node, UnsignedValue> || std::is_same_v<Node, NegativeValue>) {
          return left_node.value == right_node->value;
        } else if constexpr (std::is_same_v<Node, ByteStringValue>) {
          return bytes_equal(left_node.value, right_node->value);
        } else if constexpr (std::is_same_v<Node, TextStringValue>) {
          return left_node.value == right_node->value;
        } else if constexpr (std::is_same_v<Node, std::shared_ptr<ArrayValue>>) {
          if (left_node->values.size() != (*right_node)->values.size()) {
            return false;
          }
          for (std::size_t index = 0; index < left_node->values.size(); ++index) {
            if (!equal_nodes(left_node->values[index], (*right_node)->values[index])) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<Node, std::shared_ptr<MapValue>>) {
          if (left_node->entries.size() != (*right_node)->entries.size()) {
            return false;
          }
          for (std::size_t index = 0; index < left_node->entries.size(); ++index) {
            if (!equal_nodes(left_node->entries[index].first,
                             (*right_node)->entries[index].first) ||
                !equal_nodes(left_node->entries[index].second,
                             (*right_node)->entries[index].second)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<Node, std::shared_ptr<TagValue>>) {
          return left_node->tag == (*right_node)->tag &&
                 equal_nodes(*left_node->value, *(*right_node)->value);
        } else if constexpr (std::is_same_v<Node, BooleanValue>) {
          return left_node.value == right_node->value;
        } else if constexpr (std::is_same_v<Node, NullValue> ||
                             std::is_same_v<Node, UndefinedValue>) {
          return true;
        } else if constexpr (std::is_same_v<Node, SimpleValue>) {
          return left_node.value == right_node->value;
        } else if constexpr (std::is_same_v<Node, FloatingValue>) {
          return same_number(left_node.value, right_node->value);
        }
        return false;
      },
      left.node());
}

}  // namespace

Value::Value(Node node, std::optional<Span> span) : node_(std::move(node)), span_(span) {}

Value Value::unsigned_integer(BigInteger value, HeadWidth width) {
  return Value(UnsignedValue{std::move(value), width});
}

Value Value::negative_integer(BigInteger value, HeadWidth width) {
  return Value(NegativeValue{std::move(value), width});
}

Value Value::byte_string(Bytes value, ByteStringEncoding encoding) {
  return Value(ByteStringValue{std::move(value), std::move(encoding)});
}

Value Value::text_string(std::string value, TextStringEncoding encoding) {
  return Value(TextStringValue{std::move(value), std::move(encoding)});
}

Value Value::array(std::vector<Value> values, LengthEncoding encoding) {
  return Value(std::make_shared<ArrayValue>(ArrayValue{std::move(values), encoding}));
}

Value Value::map(std::vector<std::pair<Value, Value>> entries, LengthEncoding encoding) {
  return Value(std::make_shared<MapValue>(MapValue{std::move(entries), encoding}));
}

Value Value::tag(BigInteger tag_value, Value value, HeadWidth width) {
  return Value(std::make_shared<TagValue>(
      TagValue{std::move(tag_value), std::make_shared<Value>(std::move(value)), width}));
}

Value Value::boolean(bool value) { return Value(BooleanValue{value}); }
Value Value::null() { return Value(NullValue{}); }
Value Value::undefined() { return Value(UndefinedValue{}); }
Value Value::simple(std::uint8_t value, HeadWidth width) {
  return Value(SimpleValue{value, width});
}
Value Value::floating(double value, FloatWidth width) { return Value(FloatingValue{value, width}); }

const Value::Node& Value::node() const noexcept { return node_; }
const std::optional<Span>& Value::span() const noexcept { return span_; }

const ArrayValue* Value::as_array() const noexcept {
  const auto* pointer = std::get_if<std::shared_ptr<ArrayValue>>(&node_);
  return pointer == nullptr ? nullptr : pointer->get();
}

const MapValue* Value::as_map() const noexcept {
  const auto* pointer = std::get_if<std::shared_ptr<MapValue>>(&node_);
  return pointer == nullptr ? nullptr : pointer->get();
}

const TagValue* Value::as_tag() const noexcept {
  const auto* pointer = std::get_if<std::shared_ptr<TagValue>>(&node_);
  return pointer == nullptr ? nullptr : pointer->get();
}

const ByteStringValue* Value::as_byte_string() const noexcept {
  return std::get_if<ByteStringValue>(&node_);
}

const TextStringValue* Value::as_text_string() const noexcept {
  return std::get_if<TextStringValue>(&node_);
}

const UnsignedValue* Value::as_unsigned() const noexcept {
  return std::get_if<UnsignedValue>(&node_);
}

const NegativeValue* Value::as_negative() const noexcept {
  return std::get_if<NegativeValue>(&node_);
}

Value Value::with_span(Span span) const {
  Value copy(*this);
  copy.span_ = span;
  return copy;
}

bool Value::semantic_equal(const Value& other) const { return equal_nodes(*this, other); }

Result<Value> decode_cbor(ByteSpan bytes, DecodeOptions options) {
  Reader reader(bytes, options);
  return reader.read();
}

Result<Bytes> encode_cbor(const Value& value, EncodeOptions options) {
  Writer writer(options.mode);
  return writer.write(value);
}

Result<Value> decode_embedded_cbor(const Value& tagged, DecodeOptions options) {
  const auto* tag = tagged.as_tag();
  if (tag == nullptr || tag->tag != BigInteger(std::uint64_t{24})) {
    return std::unexpected(CardanoError(ErrorCode::invalid_structure, "expected CBOR tag 24"));
  }
  const auto* bytes = tag->value->as_byte_string();
  if (bytes == nullptr) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_structure, "CBOR tag 24 must contain a byte string"));
  }
  return decode_cbor(bytes->value, options);
}

}  // namespace cardano::core::cbor
