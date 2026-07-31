#include "cardano/cip/cip8.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <utility>

#include "cardano/crypto/primitives.hpp"

namespace cardano::cip::cip8 {
namespace {

using core::BigInteger;
using core::CardanoError;
using core::ErrorCode;
using core::Result;
using core::cbor::Mode;
using core::cbor::Value;

CardanoError structure_error(std::string message) {
  return CardanoError(ErrorCode::invalid_structure, std::move(message));
}

Value integer_value(const BigInteger& integer) {
  if (integer.is_negative()) {
    return Value::negative_integer(integer);
  }
  return Value::unsigned_integer(integer);
}

bool label_equal(const Value& left, const Label& right) {
  return left.semantic_equal(right.to_cbor_value());
}

Result<const core::cbor::MapValue*> require_map(const Value& value, std::string_view name) {
  const auto* map = value.as_map();
  if (map == nullptr) {
    return std::unexpected(structure_error(std::string(name) + " must be a map"));
  }
  return map;
}

Result<const core::cbor::ArrayValue*> require_array(const Value& value, std::size_t length,
                                                    std::string_view name) {
  const auto* array = value.as_array();
  if (array == nullptr || array->values.size() != length) {
    return std::unexpected(structure_error(std::string(name) + " must be a " +
                                           std::to_string(length) + "-item array"));
  }
  return array;
}

Result<core::Bytes> require_bytes(const Value& value, std::string_view name) {
  const auto* bytes = value.as_byte_string();
  if (bytes == nullptr) {
    return std::unexpected(structure_error(std::string(name) + " must be bytes"));
  }
  return bytes->value;
}

Result<Headers> parse_headers(const Value& protected_value, const Value& unprotected_value) {
  auto protected_bytes = require_bytes(protected_value, "protected headers");
  if (!protected_bytes) return std::unexpected(protected_bytes.error());
  auto protected_headers = ProtectedHeaderMap::from_bytes(*protected_bytes);
  auto unprotected = HeaderMap::from_cbor_value(unprotected_value);
  if (!protected_headers) return std::unexpected(protected_headers.error());
  if (!unprotected) return std::unexpected(unprotected.error());
  return Headers{std::move(*protected_headers), std::move(*unprotected)};
}

std::pair<Value, Value> encode_headers(const Headers& headers, Mode mode) {
  return {
      Value::byte_string(headers.protected_headers.bytes()),
      headers.unprotected.to_cbor_value(mode),
  };
}

Result<std::optional<core::Bytes>> parse_optional_bytes(const Value& value, std::string_view name) {
  if (std::holds_alternative<core::cbor::NullValue>(value.node())) {
    return std::optional<core::Bytes>{};
  }
  auto bytes = require_bytes(value, name);
  if (!bytes) return std::unexpected(bytes.error());
  return std::optional<core::Bytes>(std::move(*bytes));
}

std::string context_name(SigContext context) {
  switch (context) {
    case SigContext::signature:
      return "Signature";
    case SigContext::signature1:
      return "Signature1";
    case SigContext::counter_signature:
      return "CounterSignature";
  }
  return {};
}

Result<SigContext> parse_context(const Value& value) {
  const auto* text = value.as_text_string();
  if (text == nullptr) {
    return std::unexpected(structure_error("signature context must be text"));
  }
  if (text->value == "Signature") return SigContext::signature;
  if (text->value == "Signature1") return SigContext::signature1;
  if (text->value == "CounterSignature") return SigContext::counter_signature;
  return std::unexpected(structure_error("unknown signature context"));
}

std::uint32_t fnv1a32(core::ByteSpan bytes) {
  std::uint32_t hash = 0x811c9dc5U;
  for (const auto byte : bytes) {
    hash ^= std::to_integer<std::uint8_t>(byte);
    hash *= 0x01000193U;
  }
  return hash;
}

std::string base64url_encode(core::ByteSpan input) {
  constexpr std::string_view alphabet =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  std::string output;
  output.reserve((input.size() * 4 + 2) / 3);
  std::size_t offset = 0;
  while (offset + 3 <= input.size()) {
    const auto value = (std::to_integer<std::uint32_t>(input[offset]) << 16U) |
                       (std::to_integer<std::uint32_t>(input[offset + 1]) << 8U) |
                       std::to_integer<std::uint32_t>(input[offset + 2]);
    output.push_back(alphabet[(value >> 18U) & 63U]);
    output.push_back(alphabet[(value >> 12U) & 63U]);
    output.push_back(alphabet[(value >> 6U) & 63U]);
    output.push_back(alphabet[value & 63U]);
    offset += 3;
  }
  const auto remaining = input.size() - offset;
  if (remaining == 1) {
    const auto value = std::to_integer<std::uint32_t>(input[offset]) << 16U;
    output.push_back(alphabet[(value >> 18U) & 63U]);
    output.push_back(alphabet[(value >> 12U) & 63U]);
  } else if (remaining == 2) {
    const auto value = (std::to_integer<std::uint32_t>(input[offset]) << 16U) |
                       (std::to_integer<std::uint32_t>(input[offset + 1]) << 8U);
    output.push_back(alphabet[(value >> 18U) & 63U]);
    output.push_back(alphabet[(value >> 12U) & 63U]);
    output.push_back(alphabet[(value >> 6U) & 63U]);
  }
  return output;
}

Result<core::Bytes> base64url_decode(std::string_view input) {
  while (!input.empty() && input.back() == '=') input.remove_suffix(1);
  if (input.size() % 4 == 1) {
    return std::unexpected(CardanoError(ErrorCode::invalid_encoding, "invalid base64url length"));
  }
  auto digit = [](char value) -> int {
    if (value >= 'A' && value <= 'Z') return value - 'A';
    if (value >= 'a' && value <= 'z') return value - 'a' + 26;
    if (value >= '0' && value <= '9') return value - '0' + 52;
    if (value == '-') return 62;
    if (value == '_') return 63;
    return -1;
  };
  core::Bytes output;
  std::uint32_t accumulator = 0;
  unsigned bits = 0;
  for (const char character : input) {
    if (character == '=') {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_encoding, "base64url padding is not terminal"));
    }
    const int value = digit(character);
    if (value < 0) {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_encoding, "invalid base64url character"));
    }
    accumulator = (accumulator << 6U) | static_cast<std::uint32_t>(value);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      output.push_back(static_cast<core::Byte>((accumulator >> bits) & 0xffU));
    }
  }
  if (bits != 0 && (accumulator & ((1U << bits) - 1U)) != 0) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_encoding, "nonzero base64url padding bits"));
  }
  return output;
}

}  // namespace

Label::Label(BigInteger integer) : value_(std::move(integer)) {}
Label::Label(std::string text) : value_(std::move(text)) {}
Label::Label(std::int64_t integer) : value_(BigInteger(integer)) {}
Label::Label(AlgorithmId value) : Label(static_cast<std::int64_t>(value)) {}
Label::Label(KeyType value) : Label(static_cast<std::int64_t>(value)) {}
Label::Label(ECKey value) : Label(static_cast<std::int64_t>(value)) {}
Label::Label(CurveType value) : Label(static_cast<std::int64_t>(value)) {}
Label::Label(KeyOperation value) : Label(static_cast<std::int64_t>(value)) {}

LabelKind Label::kind() const noexcept {
  return std::holds_alternative<BigInteger>(value_) ? LabelKind::integer : LabelKind::text;
}
const BigInteger* Label::as_integer() const noexcept { return std::get_if<BigInteger>(&value_); }
const std::string* Label::as_text() const noexcept { return std::get_if<std::string>(&value_); }
Value Label::to_cbor_value() const {
  if (const auto* integer = as_integer(); integer != nullptr) {
    return integer_value(*integer);
  }
  return Value::text_string(*as_text());
}
Result<Label> Label::from_cbor_value(const Value& value) {
  if (const auto* integer = value.as_unsigned(); integer != nullptr) {
    return Label(integer->value);
  }
  if (const auto* integer = value.as_negative(); integer != nullptr) {
    return Label(integer->value);
  }
  if (const auto* text = value.as_text_string(); text != nullptr) {
    return Label(text->value);
  }
  return std::unexpected(structure_error("COSE label must be integer or text"));
}

HeaderMap::HeaderMap() : value_(Value::map({})) {}
HeaderMap::HeaderMap(Value value) : value_(std::move(value)) {}

Result<HeaderMap> HeaderMap::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  return from_cbor_value(*value);
}

Result<HeaderMap> HeaderMap::from_cbor_value(const Value& value) {
  auto map = require_map(value, "header map");
  if (!map) return std::unexpected(map.error());
  std::vector<Label> labels;
  labels.reserve((*map)->entries.size());
  for (const auto& [key, item] : (*map)->entries) {
    static_cast<void>(item);
    auto label = Label::from_cbor_value(key);
    if (!label) return std::unexpected(label.error());
    for (const auto& existing : labels) {
      if (existing.to_cbor_value().semantic_equal(label->to_cbor_value())) {
        return std::unexpected(
            CardanoError(ErrorCode::duplicate_key, "header map contains duplicate label"));
      }
    }
    labels.push_back(std::move(*label));
  }
  return HeaderMap(value);
}

std::optional<Value> HeaderMap::header(const Label& label) const {
  for (const auto& [key, value] : value_.as_map()->entries) {
    if (label_equal(key, label)) return value;
  }
  return std::nullopt;
}

core::VoidResult HeaderMap::set_header(Label label, Value value) {
  std::vector<std::pair<Value, Value>> entries = value_.as_map()->entries;
  for (auto& [key, existing] : entries) {
    if (label_equal(key, label)) {
      existing = std::move(value);
      value_ = Value::map(std::move(entries));
      return std::monostate{};
    }
  }
  entries.emplace_back(label.to_cbor_value(), std::move(value));
  value_ = Value::map(std::move(entries));
  return std::monostate{};
}

Labels HeaderMap::keys() const {
  Labels output;
  for (const auto& [key, value] : value_.as_map()->entries) {
    static_cast<void>(value);
    output.push_back(Label::from_cbor_value(key).value());
  }
  return output;
}

void HeaderMap::set_algorithm_id(Label value) {
  static_cast<void>(set_header(Label(std::int64_t{1}), value.to_cbor_value()));
}
std::optional<Label> HeaderMap::algorithm_id() const {
  auto value = header(Label(std::int64_t{1}));
  if (!value) return std::nullopt;
  return Label::from_cbor_value(*value).value();
}
void HeaderMap::set_criticality(Labels value) {
  std::vector<Value> labels;
  for (const auto& label : value) labels.push_back(label.to_cbor_value());
  static_cast<void>(set_header(Label(std::int64_t{2}), Value::array(std::move(labels))));
}
std::optional<Labels> HeaderMap::criticality() const {
  const auto value = header(Label(std::int64_t{2}));
  if (!value || value->as_array() == nullptr) return std::nullopt;
  Labels labels;
  for (const auto& item : value->as_array()->values) {
    auto label = Label::from_cbor_value(item);
    if (!label) return std::nullopt;
    labels.push_back(std::move(*label));
  }
  return labels;
}
void HeaderMap::set_content_type(Label value) {
  static_cast<void>(set_header(Label(std::int64_t{3}), value.to_cbor_value()));
}
std::optional<Label> HeaderMap::content_type() const {
  auto value = header(Label(std::int64_t{3}));
  if (!value) return std::nullopt;
  return Label::from_cbor_value(*value).value();
}
void HeaderMap::set_key_id(core::Bytes value) {
  static_cast<void>(set_header(Label(std::int64_t{4}), Value::byte_string(std::move(value))));
}
std::optional<core::Bytes> HeaderMap::key_id() const {
  const auto value = header(Label(std::int64_t{4}));
  if (!value || value->as_byte_string() == nullptr) return std::nullopt;
  return value->as_byte_string()->value;
}
Value HeaderMap::to_cbor_value(Mode mode) const {
  if (mode == Mode::preserve) return value_;
  const auto encoded = core::cbor::encode_cbor(value_, {.mode = Mode::canonical});
  return core::cbor::decode_cbor(*encoded).value();
}
Result<core::Bytes> HeaderMap::to_bytes(Mode mode) const {
  return core::cbor::encode_cbor(value_, {.mode = mode});
}

ProtectedHeaderMap::ProtectedHeaderMap() = default;
ProtectedHeaderMap::ProtectedHeaderMap(core::Bytes bytes, bool) : bytes_(std::move(bytes)) {}
ProtectedHeaderMap::ProtectedHeaderMap(const HeaderMap& headers) {
  if (!headers.keys().empty()) bytes_ = headers.to_bytes().value();
}
Result<ProtectedHeaderMap> ProtectedHeaderMap::from_bytes(core::ByteSpan serialized_map) {
  if (serialized_map.empty()) return ProtectedHeaderMap();
  auto headers = HeaderMap::from_bytes(serialized_map);
  if (!headers) return std::unexpected(headers.error());
  return ProtectedHeaderMap(core::copy_bytes(serialized_map), true);
}
const core::Bytes& ProtectedHeaderMap::bytes() const noexcept { return bytes_; }
Result<HeaderMap> ProtectedHeaderMap::deserialized_headers() const {
  if (bytes_.empty()) return HeaderMap();
  return HeaderMap::from_bytes(bytes_);
}

SigStructure::SigStructure(SigContext context, ProtectedHeaderMap body_protected,
                           core::Bytes external_aad, core::Bytes payload)
    : context_(context),
      body_protected_(std::move(body_protected)),
      external_aad_(std::move(external_aad)),
      payload_(std::move(payload)) {}
void SigStructure::set_sign_protected(ProtectedHeaderMap value) {
  sign_protected_ = std::move(value);
}
SigContext SigStructure::context() const noexcept { return context_; }
const ProtectedHeaderMap& SigStructure::body_protected() const noexcept { return body_protected_; }
const std::optional<ProtectedHeaderMap>& SigStructure::sign_protected() const noexcept {
  return sign_protected_;
}
const core::Bytes& SigStructure::external_aad() const noexcept { return external_aad_; }
const core::Bytes& SigStructure::payload() const noexcept { return payload_; }
Value SigStructure::to_cbor_value() const {
  std::vector<Value> values{
      Value::text_string(context_name(context_)),
      Value::byte_string(body_protected_.bytes()),
  };
  if (sign_protected_) values.push_back(Value::byte_string(sign_protected_->bytes()));
  values.push_back(Value::byte_string(external_aad_));
  values.push_back(Value::byte_string(payload_));
  return Value::array(std::move(values));
}
Result<core::Bytes> SigStructure::to_bytes() const {
  return core::cbor::encode_cbor(to_cbor_value(), {.mode = Mode::canonical});
}
Result<SigStructure> SigStructure::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  const auto* array = value->as_array();
  if (array == nullptr || (array->values.size() != 4 && array->values.size() != 5)) {
    return std::unexpected(structure_error("SigStructure must be a four- or five-item array"));
  }
  auto context = parse_context(array->values[0]);
  auto body = require_bytes(array->values[1], "body protected");
  if (!context) return std::unexpected(context.error());
  if (!body) return std::unexpected(body.error());
  auto protected_headers = ProtectedHeaderMap::from_bytes(*body);
  if (!protected_headers) return std::unexpected(protected_headers.error());
  std::size_t offset = 2;
  SigStructure output(*context, std::move(*protected_headers), {}, {});
  if (array->values.size() == 5) {
    auto sign = require_bytes(array->values[offset++], "sign protected");
    if (!sign) return std::unexpected(sign.error());
    auto parsed = ProtectedHeaderMap::from_bytes(*sign);
    if (!parsed) return std::unexpected(parsed.error());
    output.set_sign_protected(std::move(*parsed));
  }
  auto aad = require_bytes(array->values[offset++], "external aad");
  auto payload = require_bytes(array->values[offset], "payload");
  if (!aad) return std::unexpected(aad.error());
  if (!payload) return std::unexpected(payload.error());
  output.external_aad_ = std::move(*aad);
  output.payload_ = std::move(*payload);
  return output;
}

Value COSESignature::to_cbor_value(Mode mode) const {
  auto [protected_value, unprotected_value] = encode_headers(headers, mode);
  return Value::array({
      std::move(protected_value),
      std::move(unprotected_value),
      Value::byte_string(signature),
  });
}
Result<COSESignature> COSESignature::from_cbor_value(const Value& value) {
  auto array = require_array(value, 3, "COSESignature");
  if (!array) return std::unexpected(array.error());
  auto headers = parse_headers((*array)->values[0], (*array)->values[1]);
  auto signature = require_bytes((*array)->values[2], "signature");
  if (!headers) return std::unexpected(headers.error());
  if (!signature) return std::unexpected(signature.error());
  return COSESignature{std::move(*headers), std::move(*signature)};
}

CounterSignature::CounterSignature(COSESignature signature) : signatures_{std::move(signature)} {}
CounterSignature::CounterSignature(COSESignatures signatures)
    : signatures_(std::move(signatures)) {}
const COSESignatures& CounterSignature::signatures() const noexcept { return signatures_; }
Value CounterSignature::to_cbor_value(Mode mode) const {
  if (signatures_.size() == 1) return signatures_.front().to_cbor_value(mode);
  std::vector<Value> values;
  for (const auto& signature : signatures_) {
    values.push_back(signature.to_cbor_value(mode));
  }
  return Value::array(std::move(values));
}
Result<CounterSignature> CounterSignature::from_cbor_value(const Value& value) {
  if (const auto* array = value.as_array(); array != nullptr && array->values.size() == 3 &&
                                            array->values[0].as_byte_string() != nullptr) {
    auto signature = COSESignature::from_cbor_value(value);
    if (!signature) return std::unexpected(signature.error());
    return CounterSignature(std::move(*signature));
  }
  const auto* array = value.as_array();
  if (array == nullptr) {
    return std::unexpected(structure_error("counter signature must be signature or array"));
  }
  COSESignatures signatures;
  for (const auto& item : array->values) {
    auto signature = COSESignature::from_cbor_value(item);
    if (!signature) return std::unexpected(signature.error());
    signatures.push_back(std::move(*signature));
  }
  return CounterSignature(std::move(signatures));
}

COSESign1::COSESign1(Headers headers, std::optional<core::Bytes> payload, core::Bytes signature)
    : headers_(std::move(headers)),
      payload_(std::move(payload)),
      signature_(std::move(signature)) {}
Result<COSESign1> COSESign1::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  auto parsed = from_cbor_value(*value);
  if (parsed) parsed->preserved_ = std::move(*value);
  return parsed;
}
Result<COSESign1> COSESign1::from_cbor_value(const Value& value) {
  auto array = require_array(value, 4, "COSESign1");
  if (!array) return std::unexpected(array.error());
  auto headers = parse_headers((*array)->values[0], (*array)->values[1]);
  auto payload = parse_optional_bytes((*array)->values[2], "payload");
  auto signature = require_bytes((*array)->values[3], "signature");
  if (!headers) return std::unexpected(headers.error());
  if (!payload) return std::unexpected(payload.error());
  if (!signature) return std::unexpected(signature.error());
  COSESign1 output(std::move(*headers), std::move(*payload), std::move(*signature));
  output.preserved_ = value;
  return output;
}
const Headers& COSESign1::headers() const noexcept { return headers_; }
const std::optional<core::Bytes>& COSESign1::payload() const noexcept { return payload_; }
const core::Bytes& COSESign1::signature() const noexcept { return signature_; }
Result<SigStructure> COSESign1::signed_data(core::ByteSpan external_aad,
                                            std::optional<core::Bytes> external_payload) const {
  core::Bytes payload;
  if (external_payload) {
    payload = std::move(*external_payload);
  } else if (payload_) {
    payload = *payload_;
  } else {
    return std::unexpected(CardanoError(
        ErrorCode::invalid_argument, "payload is detached but no external payload was supplied"));
  }
  return SigStructure(SigContext::signature1, headers_.protected_headers,
                      core::copy_bytes(external_aad), std::move(payload));
}
Value COSESign1::to_cbor_value(Mode mode) const {
  if (mode == Mode::preserve && preserved_) return *preserved_;
  auto [protected_value, unprotected_value] = encode_headers(headers_, mode);
  return Value::array({
      std::move(protected_value),
      std::move(unprotected_value),
      payload_ ? Value::byte_string(*payload_) : Value::null(),
      Value::byte_string(signature_),
  });
}
Result<core::Bytes> COSESign1::to_bytes(Mode mode) const {
  return core::cbor::encode_cbor(to_cbor_value(mode), {.mode = mode});
}

COSESign::COSESign(Headers headers, std::optional<core::Bytes> payload, COSESignatures signatures)
    : headers_(std::move(headers)),
      payload_(std::move(payload)),
      signatures_(std::move(signatures)) {}
Result<COSESign> COSESign::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  return from_cbor_value(*value);
}
Result<COSESign> COSESign::from_cbor_value(const Value& value) {
  auto array = require_array(value, 4, "COSESign");
  if (!array) return std::unexpected(array.error());
  auto headers = parse_headers((*array)->values[0], (*array)->values[1]);
  auto payload = parse_optional_bytes((*array)->values[2], "payload");
  const auto* signatures_value = (*array)->values[3].as_array();
  if (!headers) return std::unexpected(headers.error());
  if (!payload) return std::unexpected(payload.error());
  if (signatures_value == nullptr) {
    return std::unexpected(structure_error("COSESign signatures must be an array"));
  }
  COSESignatures signatures;
  for (const auto& item : signatures_value->values) {
    auto signature = COSESignature::from_cbor_value(item);
    if (!signature) return std::unexpected(signature.error());
    signatures.push_back(std::move(*signature));
  }
  COSESign output(std::move(*headers), std::move(*payload), std::move(signatures));
  output.preserved_ = value;
  return output;
}
const Headers& COSESign::headers() const noexcept { return headers_; }
const std::optional<core::Bytes>& COSESign::payload() const noexcept { return payload_; }
const COSESignatures& COSESign::signatures() const noexcept { return signatures_; }
Value COSESign::to_cbor_value(Mode mode) const {
  if (mode == Mode::preserve && preserved_) return *preserved_;
  auto [protected_value, unprotected_value] = encode_headers(headers_, mode);
  std::vector<Value> signatures;
  for (const auto& signature : signatures_) {
    signatures.push_back(signature.to_cbor_value(mode));
  }
  return Value::array({
      std::move(protected_value),
      std::move(unprotected_value),
      payload_ ? Value::byte_string(*payload_) : Value::null(),
      Value::array(std::move(signatures)),
  });
}
Result<core::Bytes> COSESign::to_bytes(Mode mode) const {
  return core::cbor::encode_cbor(to_cbor_value(mode), {.mode = mode});
}

COSEKey::COSEKey(Label key_type) {
  static_cast<void>(map_.set_header(Label(std::int64_t{1}), key_type.to_cbor_value()));
}
Result<COSEKey> COSEKey::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  return from_cbor_value(*value);
}
Result<COSEKey> COSEKey::from_cbor_value(const Value& value) {
  auto map = HeaderMap::from_cbor_value(value);
  if (!map) return std::unexpected(map.error());
  const auto key_type_value = map->header(Label(std::int64_t{1}));
  if (!key_type_value) {
    return std::unexpected(structure_error("COSEKey is missing key type"));
  }
  auto key_type = Label::from_cbor_value(*key_type_value);
  if (!key_type) return std::unexpected(key_type.error());
  COSEKey output(std::move(*key_type));
  output.map_ = std::move(*map);
  return output;
}
std::optional<Value> COSEKey::header(const Label& label) const { return map_.header(label); }
core::VoidResult COSEKey::set_header(Label label, Value value) {
  return map_.set_header(std::move(label), std::move(value));
}
Result<core::Bytes> COSEKey::to_bytes(Mode mode) const { return map_.to_bytes(mode); }
Value COSEKey::to_cbor_value(Mode mode) const { return map_.to_cbor_value(mode); }

EdDSA25519Key::EdDSA25519Key(core::Bytes public_key) : public_key_(std::move(public_key)) {}
void EdDSA25519Key::is_for_signing() noexcept { for_signing_ = true; }
void EdDSA25519Key::is_for_verifying() noexcept { for_verifying_ = true; }
Result<COSEKey> EdDSA25519Key::build() const {
  if (public_key_.size() != 32) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_length, "Ed25519 COSE public key must be 32 bytes"));
  }
  COSEKey key{Label(KeyType::okp)};
  static_cast<void>(key.set_header(Label(ECKey::crv), Label(CurveType::ed25519).to_cbor_value()));
  static_cast<void>(key.set_header(Label(ECKey::x), Value::byte_string(public_key_)));
  static_cast<void>(
      key.set_header(Label(std::int64_t{3}), Label(AlgorithmId::ed_dsa).to_cbor_value()));
  if (for_signing_ || for_verifying_) {
    std::vector<Value> operations;
    if (for_signing_) operations.push_back(Label(KeyOperation::sign).to_cbor_value());
    if (for_verifying_) operations.push_back(Label(KeyOperation::verify).to_cbor_value());
    static_cast<void>(key.set_header(Label(std::int64_t{4}), Value::array(std::move(operations))));
  }
  return key;
}

COSESign1Builder::COSESign1Builder(Headers headers, core::Bytes payload, bool payload_external)
    : headers_(std::move(headers)),
      payload_(std::move(payload)),
      payload_external_(payload_external) {
  static_cast<void>(
      headers_.unprotected.set_header(Label(std::string("hashed")), Value::boolean(false)));
}
void COSESign1Builder::hash_payload() {
  if (hashed_) return;
  hashed_ = true;
  payload_ = crypto::blake2b224(payload_);
  static_cast<void>(
      headers_.unprotected.set_header(Label(std::string("hashed")), Value::boolean(true)));
}
void COSESign1Builder::set_external_aad(core::Bytes value) { external_aad_ = std::move(value); }
SigStructure COSESign1Builder::make_data_to_sign() const {
  return SigStructure(SigContext::signature1, headers_.protected_headers, external_aad_, payload_);
}
COSESign1 COSESign1Builder::build(core::Bytes signature) const {
  return COSESign1(headers_,
                   payload_external_ ? std::nullopt : std::optional<core::Bytes>(payload_),
                   std::move(signature));
}

COSESignBuilder::COSESignBuilder(Headers headers, core::Bytes payload, bool payload_external)
    : headers_(std::move(headers)),
      payload_(std::move(payload)),
      payload_external_(payload_external) {}
void COSESignBuilder::hash_payload() {
  if (hashed_) return;
  hashed_ = true;
  payload_ = crypto::blake2b224(payload_);
}
void COSESignBuilder::set_external_aad(core::Bytes value) { external_aad_ = std::move(value); }
SigStructure COSESignBuilder::make_data_to_sign() const {
  return SigStructure(SigContext::signature, headers_.protected_headers, external_aad_, payload_);
}
COSESign COSESignBuilder::build(COSESignatures signatures) const {
  return COSESign(headers_, payload_external_ ? std::nullopt : std::optional<core::Bytes>(payload_),
                  std::move(signatures));
}

SignedMessage::SignedMessage(COSESign sign) : value_(std::move(sign)) {}
SignedMessage::SignedMessage(COSESign1 sign1) : value_(std::move(sign1)) {}
Result<SignedMessage> SignedMessage::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  const auto* array = value->as_array();
  if (array == nullptr || array->values.size() != 4) {
    return std::unexpected(structure_error("signed message must be a four-item array"));
  }
  if (array->values[3].as_byte_string() != nullptr) {
    auto sign1 = COSESign1::from_cbor_value(*value);
    if (!sign1) return std::unexpected(sign1.error());
    return SignedMessage(std::move(*sign1));
  }
  auto sign = COSESign::from_cbor_value(*value);
  if (!sign) return std::unexpected(sign.error());
  return SignedMessage(std::move(*sign));
}
Result<SignedMessage> SignedMessage::from_user_facing_encoding(std::string_view encoded) {
  if (!encoded.starts_with("cms_")) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_encoding, "signed message must start with cms_"));
  }
  encoded.remove_prefix(4);
  while (!encoded.empty() && encoded.back() == '=') encoded.remove_suffix(1);
  if (encoded.size() < 8) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_encoding, "signed message is missing checksum"));
  }
  const auto split = encoded.size() - 6;
  auto body = base64url_decode(encoded.substr(0, split));
  auto checksum = base64url_decode(encoded.substr(split));
  if (!body) return std::unexpected(body.error());
  if (!checksum) return std::unexpected(checksum.error());
  if (checksum->size() != 4) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_length, "signed message checksum must be four bytes"));
  }
  const auto expected = (std::to_integer<std::uint32_t>((*checksum)[0]) << 24U) |
                        (std::to_integer<std::uint32_t>((*checksum)[1]) << 16U) |
                        (std::to_integer<std::uint32_t>((*checksum)[2]) << 8U) |
                        std::to_integer<std::uint32_t>((*checksum)[3]);
  if (expected != fnv1a32(*body)) {
    return std::unexpected(
        CardanoError(ErrorCode::checksum_mismatch, "signed message checksum does not match"));
  }
  return from_bytes(*body);
}
SignedMessageKind SignedMessage::kind() const noexcept {
  return std::holds_alternative<COSESign>(value_) ? SignedMessageKind::cose_sign
                                                  : SignedMessageKind::cose_sign1;
}
const COSESign* SignedMessage::as_cose_sign() const noexcept {
  return std::get_if<COSESign>(&value_);
}
const COSESign1* SignedMessage::as_cose_sign1() const noexcept {
  return std::get_if<COSESign1>(&value_);
}
Result<core::Bytes> SignedMessage::to_bytes(Mode mode) const {
  if (const auto* sign = as_cose_sign(); sign != nullptr) return sign->to_bytes(mode);
  return as_cose_sign1()->to_bytes(mode);
}
Result<std::string> SignedMessage::to_user_facing_encoding() const {
  auto body = to_bytes();
  if (!body) return std::unexpected(body.error());
  const auto checksum = fnv1a32(*body);
  const std::array<core::Byte, 4> checksum_bytes{
      static_cast<core::Byte>((checksum >> 24U) & 0xffU),
      static_cast<core::Byte>((checksum >> 16U) & 0xffU),
      static_cast<core::Byte>((checksum >> 8U) & 0xffU),
      static_cast<core::Byte>(checksum & 0xffU),
  };
  return "cms_" + base64url_encode(*body) + base64url_encode(checksum_bytes);
}

}  // namespace cardano::cip::cip8
