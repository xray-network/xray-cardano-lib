#include "cardano/chain/plutus_data.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace cardano::chain {
namespace {

[[nodiscard]] core::Result<PlutusData> decode_node(const core::cbor::Value& value,
                                                   std::size_t depth, std::size_t max_depth,
                                                   bool enforce_wire_limit) {
  if (depth > max_depth) {
    return std::unexpected(core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                                              "Plutus Data nesting exceeds the configured limit"));
  }
  if (const auto* integer = value.as_unsigned()) {
    return PlutusData::integer(integer->value);
  }
  if (const auto* integer = value.as_negative()) {
    return PlutusData::integer(integer->value);
  }
  if (const auto* bytes = value.as_byte_string()) {
    const auto valid_wire =
        !enforce_wire_limit ||
        (!bytes->encoding.indefinite
             ? bytes->value.size() <= 64U
             : std::ranges::all_of(bytes->encoding.chunks, [](const core::cbor::ByteChunk& chunk) {
                 return chunk.value.size() <= 64U;
               }));
    if (!valid_wire) {
      return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                                "PlutusData byte chunks are limited to 64 bytes"));
    }
    return PlutusData::bytes(bytes->value);
  }
  if (const auto* list = value.as_array()) {
    PlutusData::List decoded;
    decoded.reserve(list->values.size());
    for (std::size_t index = 0; index < list->values.size(); ++index) {
      auto item = decode_node(list->values[index], depth + 1, max_depth, enforce_wire_limit);
      if (!item) {
        return std::unexpected(item.error().at(index));
      }
      decoded.push_back(std::move(*item));
    }
    return PlutusData::list(std::move(decoded));
  }
  if (const auto* map = value.as_map()) {
    std::vector<std::pair<PlutusData, PlutusData>> decoded;
    decoded.reserve(map->entries.size());
    for (std::size_t index = 0; index < map->entries.size(); ++index) {
      auto key = decode_node(map->entries[index].first, depth + 1, max_depth, enforce_wire_limit);
      auto item = decode_node(map->entries[index].second, depth + 1, max_depth, enforce_wire_limit);
      if (!key || !item) {
        return std::unexpected((!key ? key.error() : item.error()).at(index));
      }
      decoded.emplace_back(std::move(*key), std::move(*item));
    }
    return PlutusData::map(std::move(decoded));
  }
  const auto* tag = value.as_tag();
  if (tag == nullptr || tag->value == nullptr) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                              "CBOR node is not valid Plutus Data"));
  }
  const auto tag_number = tag->tag.to_uint64();
  if (tag_number && (*tag_number == 2U || *tag_number == 3U)) {
    const auto* bytes = tag->value->as_byte_string();
    if (bytes == nullptr || bytes->value.empty() || bytes->value.front() == core::Byte{0}) {
      return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                                "Plutus Data bignum must be minimally encoded"));
    }
    auto magnitude = core::BigInteger::from_unsigned_bytes_be(bytes->value);
    return PlutusData::integer(*tag_number == 2U ? std::move(magnitude)
                                                 : core::BigInteger(std::int64_t{-1}) - magnitude);
  }
  core::BigInteger alternative;
  const core::cbor::ArrayValue* fields = nullptr;
  if (tag_number && *tag_number >= 121 && *tag_number <= 127) {
    alternative = core::BigInteger(*tag_number - 121);
    fields = tag->value->as_array();
  } else if (tag_number && *tag_number >= 1280 && *tag_number <= 1400) {
    alternative = core::BigInteger(*tag_number - 1280 + 7);
    fields = tag->value->as_array();
  } else if (tag_number && *tag_number == 102) {
    const auto* shape = tag->value->as_array();
    if (shape == nullptr || shape->values.size() != 2 ||
        shape->values[0].as_unsigned() == nullptr) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::invalid_structure,
                             "general constructor must be tag 102 over [alternative, fields]"));
    }
    alternative = shape->values[0].as_unsigned()->value;
    fields = shape->values[1].as_array();
  } else {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure, "unsupported tag in Plutus Data"));
  }
  if (fields == nullptr) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                              "constructor fields must be a CBOR array"));
  }
  PlutusData::List decoded_fields;
  decoded_fields.reserve(fields->values.size());
  for (std::size_t index = 0; index < fields->values.size(); ++index) {
    auto item = decode_node(fields->values[index], depth + 1, max_depth, enforce_wire_limit);
    if (!item) {
      return std::unexpected(item.error().at(index));
    }
    decoded_fields.push_back(std::move(*item));
  }
  return PlutusData::constr(std::move(alternative), std::move(decoded_fields));
}

[[nodiscard]] core::Result<core::cbor::Value> encode_node(const PlutusData& data) {
  return std::visit(
      [](const auto& node) -> core::Result<core::cbor::Value> {
        using Node = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<Node, std::shared_ptr<ConstrPlutusData>>) {
          std::vector<core::cbor::Value> fields;
          fields.reserve(node->fields.size());
          for (const auto& field : node->fields) {
            auto encoded = encode_node(field);
            if (!encoded) {
              return std::unexpected(encoded.error());
            }
            fields.push_back(std::move(*encoded));
          }
          core::cbor::Value payload = core::cbor::Value::array(std::move(fields));
          auto alternative = node->alternative.to_uint64();
          if (alternative && *alternative <= 6) {
            return core::cbor::Value::tag(core::BigInteger(std::uint64_t{121} + *alternative),
                                          std::move(payload));
          }
          if (alternative && *alternative >= 7 && *alternative <= 127) {
            return core::cbor::Value::tag(core::BigInteger(std::uint64_t{1280} + *alternative - 7),
                                          std::move(payload));
          }
          if (node->alternative.is_negative()) {
            return std::unexpected(core::CardanoError(
                core::ErrorCode::out_of_range, "constructor alternative must be nonnegative"));
          }
          std::vector<core::cbor::Value> general;
          general.push_back(core::cbor::Value::unsigned_integer(node->alternative));
          general.push_back(std::move(payload));
          return core::cbor::Value::tag(core::BigInteger(std::uint64_t{102}),
                                        core::cbor::Value::array(std::move(general)));
        } else if constexpr (std::is_same_v<Node, std::shared_ptr<PlutusMap>>) {
          std::vector<std::pair<core::cbor::Value, core::cbor::Value>> entries;
          entries.reserve(node->entries.size());
          for (const auto& [key, value] : node->entries) {
            auto encoded_key = encode_node(key);
            auto encoded_value = encode_node(value);
            if (!encoded_key || !encoded_value) {
              return std::unexpected(!encoded_key ? encoded_key.error() : encoded_value.error());
            }
            entries.emplace_back(std::move(*encoded_key), std::move(*encoded_value));
          }
          return core::cbor::Value::map(std::move(entries));
        } else if constexpr (std::is_same_v<Node, std::shared_ptr<PlutusData::List>>) {
          std::vector<core::cbor::Value> items;
          items.reserve(node->size());
          for (const auto& item : *node) {
            auto encoded = encode_node(item);
            if (!encoded) {
              return std::unexpected(encoded.error());
            }
            items.push_back(std::move(*encoded));
          }
          return core::cbor::Value::array(std::move(items));
        } else if constexpr (std::is_same_v<Node, core::BigInteger>) {
          return node.is_negative() ? core::cbor::Value::negative_integer(node)
                                    : core::cbor::Value::unsigned_integer(node);
        } else {
          if (node.size() <= 64U) {
            return core::cbor::Value::byte_string(node);
          }
          core::cbor::ByteStringEncoding encoding;
          encoding.indefinite = true;
          for (std::size_t offset = 0U; offset < node.size(); offset += 64U) {
            const auto length = std::min<std::size_t>(64U, node.size() - offset);
            encoding.chunks.push_back(
                {core::Bytes(node.begin() + static_cast<std::ptrdiff_t>(offset),
                             node.begin() + static_cast<std::ptrdiff_t>(offset + length)),
                 core::cbor::HeadWidth::inline_value});
          }
          return core::cbor::Value::byte_string(node, std::move(encoding));
        }
      },
      data.node());
}

[[nodiscard]] core::cbor::Value chunk_data_bytes(const core::cbor::Value& value) {
  if (const auto* bytes = value.as_byte_string()) {
    if (bytes->value.size() <= 64U) {
      return core::cbor::Value::byte_string(bytes->value);
    }
    core::cbor::ByteStringEncoding encoding;
    encoding.indefinite = true;
    for (std::size_t offset = 0U; offset < bytes->value.size(); offset += 64U) {
      const auto length = std::min<std::size_t>(64U, bytes->value.size() - offset);
      encoding.chunks.push_back(
          {core::Bytes(bytes->value.begin() + static_cast<std::ptrdiff_t>(offset),
                       bytes->value.begin() + static_cast<std::ptrdiff_t>(offset + length)),
           core::cbor::HeadWidth::inline_value});
    }
    return core::cbor::Value::byte_string(bytes->value, std::move(encoding));
  }
  if (const auto* array = value.as_array()) {
    std::vector<core::cbor::Value> values;
    values.reserve(array->values.size());
    for (const auto& item : array->values) {
      values.push_back(chunk_data_bytes(item));
    }
    return core::cbor::Value::array(std::move(values));
  }
  if (const auto* map = value.as_map()) {
    std::vector<std::pair<core::cbor::Value, core::cbor::Value>> entries;
    entries.reserve(map->entries.size());
    for (const auto& [key, item] : map->entries) {
      entries.emplace_back(chunk_data_bytes(key), chunk_data_bytes(item));
    }
    return core::cbor::Value::map(std::move(entries));
  }
  if (const auto* tag = value.as_tag()) {
    return core::cbor::Value::tag(tag->tag, chunk_data_bytes(*tag->value));
  }
  if (const auto* positive = value.as_unsigned()) {
    return core::cbor::Value::unsigned_integer(positive->value);
  }
  if (const auto* negative = value.as_negative()) {
    return core::cbor::Value::negative_integer(negative->value);
  }
  return value;
}

}  // namespace

PlutusData::PlutusData(Node node, std::shared_ptr<const core::cbor::Value> preserved)
    : node_(std::move(node)), preserved_(std::move(preserved)) {}
PlutusData PlutusData::constr(core::BigInteger alternative, List fields) {
  return PlutusData(std::make_shared<ConstrPlutusData>(
      ConstrPlutusData{std::move(alternative), std::move(fields)}));
}
PlutusData PlutusData::map(std::vector<std::pair<PlutusData, PlutusData>> entries) {
  return PlutusData(std::make_shared<PlutusMap>(PlutusMap{std::move(entries)}));
}
PlutusData PlutusData::list(List values) {
  return PlutusData(std::make_shared<List>(std::move(values)));
}
PlutusData PlutusData::integer(core::BigInteger value) { return PlutusData(std::move(value)); }
PlutusData PlutusData::bytes(core::Bytes value) { return PlutusData(std::move(value)); }
core::Result<PlutusData> PlutusData::from_cbor(core::ByteSpan bytes,
                                               core::cbor::DecodeOptions options) {
  auto value = core::cbor::decode_cbor(bytes, options);
  if (!value) {
    return std::unexpected(value.error());
  }
  auto decoded = decode_node(*value, 0, options.limits.max_depth, true);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  decoded->preserved_ = std::make_shared<core::cbor::Value>(std::move(*value));
  return decoded;
}
core::Result<PlutusData> PlutusData::from_cbor_value(const core::cbor::Value& value,
                                                     std::size_t max_depth,
                                                     bool enforce_wire_limit) {
  return decode_node(value, 0, max_depth, enforce_wire_limit);
}
core::Result<core::cbor::Value> PlutusData::to_cbor_value() const { return encode_node(*this); }
core::Result<core::Bytes> PlutusData::to_cbor(core::cbor::Mode mode) const {
  if (mode == core::cbor::Mode::preserve && preserved_) {
    return core::cbor::encode_cbor(*preserved_,
                                   core::cbor::EncodeOptions{.mode = core::cbor::Mode::preserve});
  }
  auto value = encode_node(*this);
  if (!value) {
    return std::unexpected(value.error());
  }
  if (mode == core::cbor::Mode::preserve) {
    return core::cbor::encode_cbor(*value, core::cbor::EncodeOptions{.mode = mode});
  }
  auto canonical = core::cbor::encode_cbor(
      *value, core::cbor::EncodeOptions{.mode = core::cbor::Mode::canonical});
  if (!canonical) {
    return std::unexpected(canonical.error());
  }
  auto decoded = core::cbor::decode_cbor(*canonical);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  return core::cbor::encode_cbor(chunk_data_bytes(*decoded),
                                 core::cbor::EncodeOptions{.mode = core::cbor::Mode::preserve});
}
const PlutusData::Node& PlutusData::node() const noexcept { return node_; }
bool operator==(const PlutusData& left, const PlutusData& right) {
  const auto left_bytes = left.to_cbor(core::cbor::Mode::canonical);
  const auto right_bytes = right.to_cbor(core::cbor::Mode::canonical);
  return left_bytes && right_bytes && *left_bytes == *right_bytes;
}
core::Result<PlutusData> validate_plutus_data_node(const core::cbor::Value& value,
                                                   std::size_t max_depth, bool enforce_wire_limit) {
  return PlutusData::from_cbor_value(value, max_depth, enforce_wire_limit);
}

}  // namespace cardano::chain
