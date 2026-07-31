#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cardano/core/big_integer.hpp"
#include "cardano/core/bytes.hpp"

namespace cardano::core::cbor {

enum class HeadWidth : std::uint8_t { inline_value = 0, one = 1, two = 2, four = 4, eight = 8 };

enum class FloatWidth : std::uint8_t { half = 2, single = 4, double_precision = 8 };

enum class Mode { preserve, canonical };

struct Span {
  std::size_t start{};
  std::size_t end{};
  friend bool operator==(const Span&, const Span&) = default;
};

struct LengthEncoding {
  bool indefinite{};
  HeadWidth width{HeadWidth::inline_value};
  friend bool operator==(const LengthEncoding&, const LengthEncoding&) = default;
};

struct ByteChunk {
  Bytes value;
  HeadWidth width{HeadWidth::inline_value};
};

struct TextChunk {
  std::string value;
  HeadWidth width{HeadWidth::inline_value};
};

struct ByteStringEncoding {
  bool indefinite{};
  HeadWidth width{HeadWidth::inline_value};
  std::vector<ByteChunk> chunks;
};

struct TextStringEncoding {
  bool indefinite{};
  HeadWidth width{HeadWidth::inline_value};
  std::vector<TextChunk> chunks;
};

struct UnsignedValue {
  BigInteger value;
  HeadWidth width{HeadWidth::inline_value};
};

struct NegativeValue {
  BigInteger value;
  HeadWidth width{HeadWidth::inline_value};
};

struct ByteStringValue {
  Bytes value;
  ByteStringEncoding encoding;
};

struct TextStringValue {
  std::string value;
  TextStringEncoding encoding;
};

struct BooleanValue {
  bool value{};
};

struct NullValue {};
struct UndefinedValue {};

struct SimpleValue {
  std::uint8_t value{};
  HeadWidth width{HeadWidth::inline_value};
};

struct FloatingValue {
  double value{};
  FloatWidth width{FloatWidth::double_precision};
};

class Value;

struct ArrayValue {
  std::vector<Value> values;
  LengthEncoding encoding;
};

struct MapValue {
  std::vector<std::pair<Value, Value>> entries;
  LengthEncoding encoding;
};

struct TagValue {
  BigInteger tag;
  std::shared_ptr<Value> value;
  HeadWidth width{HeadWidth::inline_value};
};

class Value {
 public:
  using Node = std::variant<UnsignedValue, NegativeValue, ByteStringValue, TextStringValue,
                            std::shared_ptr<ArrayValue>, std::shared_ptr<MapValue>,
                            std::shared_ptr<TagValue>, BooleanValue, NullValue, UndefinedValue,
                            SimpleValue, FloatingValue>;

  explicit Value(Node node, std::optional<Span> span = std::nullopt);

  [[nodiscard]] static Value unsigned_integer(BigInteger value,
                                              HeadWidth width = HeadWidth::inline_value);
  [[nodiscard]] static Value negative_integer(BigInteger value,
                                              HeadWidth width = HeadWidth::inline_value);
  [[nodiscard]] static Value byte_string(Bytes value, ByteStringEncoding encoding = {});
  [[nodiscard]] static Value text_string(std::string value, TextStringEncoding encoding = {});
  [[nodiscard]] static Value array(std::vector<Value> values, LengthEncoding encoding = {});
  [[nodiscard]] static Value map(std::vector<std::pair<Value, Value>> entries,
                                 LengthEncoding encoding = {});
  [[nodiscard]] static Value tag(BigInteger tag, Value value,
                                 HeadWidth width = HeadWidth::inline_value);
  [[nodiscard]] static Value boolean(bool value);
  [[nodiscard]] static Value null();
  [[nodiscard]] static Value undefined();
  [[nodiscard]] static Value simple(std::uint8_t value, HeadWidth width = HeadWidth::inline_value);
  [[nodiscard]] static Value floating(double value,
                                      FloatWidth width = FloatWidth::double_precision);

  [[nodiscard]] const Node& node() const noexcept;
  [[nodiscard]] const std::optional<Span>& span() const noexcept;
  [[nodiscard]] const ArrayValue* as_array() const noexcept;
  [[nodiscard]] const MapValue* as_map() const noexcept;
  [[nodiscard]] const TagValue* as_tag() const noexcept;
  [[nodiscard]] const ByteStringValue* as_byte_string() const noexcept;
  [[nodiscard]] const TextStringValue* as_text_string() const noexcept;
  [[nodiscard]] const UnsignedValue* as_unsigned() const noexcept;
  [[nodiscard]] const NegativeValue* as_negative() const noexcept;

  [[nodiscard]] Value with_span(Span span) const;
  [[nodiscard]] bool semantic_equal(const Value& other) const;

 private:
  Node node_;
  std::optional<Span> span_;
};

struct DecoderLimits {
  std::size_t max_depth{512};
  std::size_t max_collection_length{1'000'000};
  std::size_t max_string_bytes{64U * 1024U * 1024U};
  std::size_t max_tokens{2'000'000};
};

inline constexpr DecoderLimits DEFAULT_CBOR_LIMITS{};

struct DecodeOptions {
  DecoderLimits limits{DEFAULT_CBOR_LIMITS};
};

struct EncodeOptions {
  Mode mode{Mode::preserve};
};

[[nodiscard]] Result<Value> decode_cbor(ByteSpan bytes, DecodeOptions options = {});
[[nodiscard]] Result<Bytes> encode_cbor(const Value& value, EncodeOptions options = {});
[[nodiscard]] Result<Value> decode_embedded_cbor(const Value& tagged, DecodeOptions options = {});

}  // namespace cardano::core::cbor
