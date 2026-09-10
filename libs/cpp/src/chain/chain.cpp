#include "cardano/chain/chain.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <utility>

#include "cardano/core/cbor.hpp"

namespace cardano::chain {
namespace {

constexpr std::array<std::string_view, 8> ACCEPTED_SUFFIXES{
    "cb57afb0b35fc89c63061c9914e055001a518c7516",
    "13d5f4a3fe0478b2241e0168e3cba5001a22c15a11",
    "00",
    "6a33306635616d6b776877716134777666796a64657a7961656c6d6e6e676436643465",
    "35616379327230656b7270717a71646b386c7a716e357234356e",
    "061d070c0d041b07020f0b0d0b0f020912051d1c100911040e1f0713110301000b101600",
    "126e7735333567367673703778376668787071327074736839676b72",
    "2c"};
constexpr std::string_view BASE58_ALPHABET =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

[[nodiscard]] bool accepted_suffix(core::ByteSpan suffix) {
  if (suffix.empty()) {
    return true;
  }
  const auto hex = core::bytes_to_hex(suffix);
  return std::ranges::find(ACCEPTED_SUFFIXES, hex) != ACCEPTED_SUFFIXES.end();
}

[[nodiscard]] core::Result<Credential> credential_from(CredentialKind kind, core::ByteSpan bytes) {
  if (kind == CredentialKind::key) {
    auto value = crypto::Ed25519KeyHash::from_bytes(bytes);
    if (!value) {
      return std::unexpected(value.error());
    }
    return Credential::key(*value);
  }
  auto value = crypto::ScriptHash::from_bytes(bytes);
  if (!value) {
    return std::unexpected(value.error());
  }
  return Credential::script(*value);
}

[[nodiscard]] std::uint8_t header_variant(std::uint8_t base, CredentialKind first,
                                          std::optional<CredentialKind> second = std::nullopt) {
  std::uint8_t variant = base;
  if (first == CredentialKind::script) {
    variant |= second ? 0x01U : 0x01U;
  }
  if (second && *second == CredentialKind::script) {
    variant |= 0x02U;
  }
  return variant;
}

[[nodiscard]] core::Result<std::uint64_t> unsigned_value(const core::cbor::Value& value) {
  const auto* node = value.as_unsigned();
  if (node == nullptr) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure, "expected an unsigned CBOR value"));
  }
  return node->value.to_uint64();
}

}  // namespace

void enforce_linkage() noexcept {}

Credential::Credential(CredentialKind kind, std::array<core::Byte, 28> bytes)
    : kind_(kind), bytes_(std::move(bytes)) {}
Credential Credential::key(crypto::Ed25519KeyHash hash) {
  std::array<core::Byte, 28> bytes{};
  std::ranges::copy(hash.span(), bytes.begin());
  return Credential(CredentialKind::key, bytes);
}
Credential Credential::script(crypto::ScriptHash hash) {
  std::array<core::Byte, 28> bytes{};
  std::ranges::copy(hash.span(), bytes.begin());
  return Credential(CredentialKind::script, bytes);
}
core::Result<Credential> Credential::from_json(std::string_view json) {
  try {
    const auto parsed = nlohmann::json::parse(json);
    if (!parsed.is_object() || parsed.size() != 1U) {
      return std::unexpected(core::CardanoError(core::ErrorCode::invalid_argument,
                                                "credential JSON must have one variant"));
    }
    const bool key_variant = parsed.contains("PubKey");
    const auto variant = key_variant ? "PubKey" : "Script";
    if ((!key_variant && !parsed.contains("Script")) || !parsed.at(variant).is_object() ||
        parsed.at(variant).size() != 1U || !parsed.at(variant).contains("hash") ||
        !parsed.at(variant).at("hash").is_string()) {
      return std::unexpected(core::CardanoError(core::ErrorCode::invalid_argument,
                                                "credential JSON variant is invalid"));
    }
    auto bytes = core::hex_to_bytes(parsed.at(variant).at("hash").get<std::string>());
    return bytes
               ? credential_from(key_variant ? CredentialKind::key : CredentialKind::script, *bytes)
               : std::unexpected(bytes.error());
  } catch (const nlohmann::json::exception& error) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding,
                           std::string("invalid credential JSON: ") + error.what()));
  }
}
CredentialKind Credential::kind() const noexcept { return kind_; }
core::Bytes Credential::to_bytes() const { return {bytes_.begin(), bytes_.end()}; }
std::string Credential::to_json() const {
  const auto variant = kind_ == CredentialKind::key ? "PubKey" : "Script";
  return nlohmann::json{{variant, nlohmann::json{{"hash", core::bytes_to_hex(bytes_)}}}}.dump();
}

core::Bytes encode_variable_natural(const core::BigInteger& value) {
  if (value.is_negative()) {
    return {};
  }
  const auto base256 = value.to_unsigned_bytes_be();
  if (base256.empty()) {
    return {core::Byte{0}};
  }
  core::Bytes groups;
  std::uint16_t accumulator = 0;
  unsigned bits = 0;
  for (auto iterator = base256.rbegin(); iterator != base256.rend(); ++iterator) {
    accumulator |= static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(*iterator) << bits);
    bits += 8;
    while (bits >= 7) {
      groups.push_back(static_cast<core::Byte>(accumulator & 0x7fU));
      accumulator >>= 7U;
      bits -= 7;
    }
  }
  if (bits != 0) {
    groups.push_back(static_cast<core::Byte>(accumulator & 0x7fU));
  }
  while (groups.size() > 1 && groups.back() == core::Byte{0}) {
    groups.pop_back();
  }
  core::Bytes output;
  output.reserve(groups.size());
  for (auto iterator = groups.rbegin(); iterator != groups.rend(); ++iterator) {
    auto byte = *iterator;
    if (std::next(iterator) != groups.rend()) {
      byte |= core::Byte{0x80};
    }
    output.push_back(byte);
  }
  return output;
}

core::Result<std::pair<core::BigInteger, std::size_t>> decode_variable_natural(
    core::ByteSpan bytes) {
  core::BigInteger value(std::uint64_t{0});
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    const auto byte = std::to_integer<std::uint8_t>(bytes[index]);
    value *= core::BigInteger(std::uint64_t{128});
    value += core::BigInteger(static_cast<std::uint64_t>(byte & 0x7fU));
    if ((byte & 0x80U) == 0) {
      return std::pair(std::move(value), index + 1);
    }
  }
  return std::unexpected(
      core::CardanoError(core::ErrorCode::truncated_input, "unterminated variable natural"));
}

Address::Address(AddressKind kind, core::Bytes bytes, std::size_t semantic_length)
    : kind_(kind), bytes_(std::move(bytes)), semantic_length_(semantic_length) {}

core::Result<Address> Address::from_bytes(core::ByteSpan bytes) {
  if (bytes.empty()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_length, "address cannot be empty"));
  }
  const auto variant = std::to_integer<std::uint8_t>(bytes.front()) >> 4U;
  if (variant <= 3) {
    if (bytes.size() < 57 || !accepted_suffix(bytes.subspan(57))) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::invalid_encoding,
                             "base address has an invalid length or compatibility suffix"));
    }
    return Address(AddressKind::base, core::Bytes(bytes.begin(), bytes.end()), 57);
  }
  if (variant == 4 || variant == 5) {
    if (bytes.size() < 32) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::truncated_input, "pointer address is truncated"));
    }
    std::size_t offset = 29;
    for (unsigned component = 0; component < 3; ++component) {
      auto decoded = decode_variable_natural(bytes.subspan(offset));
      if (!decoded) {
        return std::unexpected(decoded.error().at(component));
      }
      offset += decoded->second;
    }
    if (!accepted_suffix(bytes.subspan(offset))) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::invalid_encoding,
                             "pointer address has an unrecognized compatibility suffix"));
    }
    return Address(AddressKind::pointer, core::Bytes(bytes.begin(), bytes.end()), offset);
  }
  if (variant == 6 || variant == 7 || variant == 14 || variant == 15) {
    if (bytes.size() < 29 || !accepted_suffix(bytes.subspan(29))) {
      return std::unexpected(core::CardanoError(
          core::ErrorCode::invalid_encoding,
          "single-credential address has an invalid length or compatibility suffix"));
    }
    return Address(variant >= 14 ? AddressKind::reward : AddressKind::enterprise,
                   core::Bytes(bytes.begin(), bytes.end()), 29);
  }
  if (variant == 8) {
    return Address(AddressKind::byron, core::Bytes(bytes.begin(), bytes.end()), bytes.size());
  }
  return std::unexpected(
      core::CardanoError(core::ErrorCode::invalid_encoding, "address header variant is reserved"));
}

core::Result<Address> Address::from_hex(std::string_view hex) {
  auto bytes = core::hex_to_bytes(hex);
  return bytes ? from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Result<Address> Address::from_bech32(std::string_view encoded) {
  auto decoded = core::decode_bech32(encoded);
  if (!decoded) return std::unexpected(decoded.error());
  auto address = from_bytes(decoded->bytes);
  if (!address) return std::unexpected(address.error());
  if (address->kind() == AddressKind::byron)
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "Byron addresses are not Shelley Bech32 addresses"));
  const auto network = *address->network_id();
  const std::string expected = address->kind() == AddressKind::reward
                                   ? (network == 1 ? "stake" : "stake_test")
                                   : (network == 1 ? "addr" : "addr_test");
  if (decoded->prefix != expected)
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "address HRP does not match its class and network"));
  return address;
}
core::Result<Address> Address::from_bech32_payload_compatible(std::string_view encoded) {
  auto decoded = core::decode_bech32(encoded);
  return decoded ? from_bytes(decoded->bytes) : std::unexpected(decoded.error());
}
bool Address::is_valid(std::string_view encoded) noexcept {
  try {
    if (from_bech32(encoded)) {
      return true;
    }
    return ByronAddress::from_base58(encoded).has_value();
  } catch (...) {
    return false;
  }
}
AddressKind Address::kind() const noexcept { return kind_; }
core::Result<std::uint8_t> Address::network_id() const {
  if (kind_ == AddressKind::byron) {
    return std::unexpected(core::CardanoError(core::ErrorCode::unsupported,
                                              "generic Byron address has no direct network id"));
  }
  return std::to_integer<std::uint8_t>(bytes_[0]) & 0x0fU;
}
core::Bytes Address::to_bytes() const { return bytes_; }
std::string Address::to_hex() const { return core::bytes_to_hex(bytes_); }
core::Result<std::string> Address::to_bech32(std::optional<std::string_view> hrp) const {
  if (kind_ == AddressKind::byron && !hrp) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::unsupported, "generic Byron address requires an explicit Bech32 HRP"));
  }
  std::string selected;
  if (hrp) {
    selected = *hrp;
  } else {
    selected = kind_ == AddressKind::reward ? "stake" : "addr";
    if (*network_id() != 1) {
      selected += "_test";
    }
  }
  return core::encode_bech32(selected, bytes_);
}
std::string Address::to_json() const {
  const auto encoded = to_bech32();
  return encoded ? "\"" + *encoded + "\"" : std::string{};
}

BaseAddress::BaseAddress(std::uint8_t network, Credential payment, Credential stake)
    : network_(network), payment_(std::move(payment)), stake_(std::move(stake)) {}
core::Result<BaseAddress> BaseAddress::from_address(const Address& address) {
  if (address.kind_ != AddressKind::base) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure, "address is not a base address"));
  }
  const auto header = std::to_integer<std::uint8_t>(address.bytes_[0]);
  auto payment =
      credential_from((header & 0x10U) != 0 ? CredentialKind::script : CredentialKind::key,
                      core::ByteSpan(address.bytes_).subspan(1, 28));
  auto stake = credential_from((header & 0x20U) != 0 ? CredentialKind::script : CredentialKind::key,
                               core::ByteSpan(address.bytes_).subspan(29, 28));
  if (!payment || !stake) {
    return std::unexpected(!payment ? payment.error() : stake.error());
  }
  return BaseAddress(header & 0x0fU, *payment, *stake);
}
Address BaseAddress::to_address() const {
  const auto variant = header_variant(0, payment_.kind(), stake_.kind());
  core::Bytes bytes{static_cast<core::Byte>((variant << 4U) | (network_ & 0x0fU))};
  const auto payment = payment_.to_bytes();
  const auto stake = stake_.to_bytes();
  bytes.insert(bytes.end(), payment.begin(), payment.end());
  bytes.insert(bytes.end(), stake.begin(), stake.end());
  return Address(AddressKind::base, std::move(bytes), 57);
}
std::uint8_t BaseAddress::network_id() const noexcept { return network_; }
const Credential& BaseAddress::payment_credential() const noexcept { return payment_; }
const Credential& BaseAddress::stake_credential() const noexcept { return stake_; }

PointerAddress::PointerAddress(std::uint8_t network, Credential payment, Pointer pointer)
    : network_(network), payment_(std::move(payment)), pointer_(std::move(pointer)) {}
core::Result<PointerAddress> PointerAddress::from_address(const Address& address) {
  if (address.kind_ != AddressKind::pointer) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure, "address is not a pointer address"));
  }
  const auto header = std::to_integer<std::uint8_t>(address.bytes_[0]);
  auto payment =
      credential_from((header & 0x10U) != 0 ? CredentialKind::script : CredentialKind::key,
                      core::ByteSpan(address.bytes_).subspan(1, 28));
  if (!payment) {
    return std::unexpected(payment.error());
  }
  std::size_t offset = 29;
  auto slot = decode_variable_natural(core::ByteSpan(address.bytes_).subspan(offset));
  offset += slot->second;
  auto transaction = decode_variable_natural(core::ByteSpan(address.bytes_).subspan(offset));
  offset += transaction->second;
  auto certificate = decode_variable_natural(core::ByteSpan(address.bytes_).subspan(offset));
  return PointerAddress(header & 0x0fU, *payment,
                        Pointer{slot->first, transaction->first, certificate->first});
}
core::Result<Address> PointerAddress::to_address() const {
  if (pointer_.slot.is_negative() || pointer_.transaction_index.is_negative() ||
      pointer_.certificate_index.is_negative()) {
    return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                              "pointer components must be nonnegative"));
  }
  const auto variant = header_variant(4, payment_.kind());
  core::Bytes bytes{static_cast<core::Byte>((variant << 4U) | (network_ & 0x0fU))};
  const auto payment = payment_.to_bytes();
  bytes.insert(bytes.end(), payment.begin(), payment.end());
  for (const auto* component :
       {&pointer_.slot, &pointer_.transaction_index, &pointer_.certificate_index}) {
    const auto encoded = encode_variable_natural(*component);
    bytes.insert(bytes.end(), encoded.begin(), encoded.end());
  }
  const auto semantic_length = bytes.size();
  return Address(AddressKind::pointer, std::move(bytes), semantic_length);
}
const Pointer& PointerAddress::pointer() const noexcept { return pointer_; }

EnterpriseAddress::EnterpriseAddress(std::uint8_t network, Credential payment)
    : network_(network), payment_(std::move(payment)) {}
core::Result<EnterpriseAddress> EnterpriseAddress::from_address(const Address& address) {
  if (address.kind_ != AddressKind::enterprise) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                              "address is not an enterprise address"));
  }
  const auto header = std::to_integer<std::uint8_t>(address.bytes_[0]);
  auto payment =
      credential_from((header & 0x10U) != 0 ? CredentialKind::script : CredentialKind::key,
                      core::ByteSpan(address.bytes_).subspan(1, 28));
  return payment ? core::Result<EnterpriseAddress>(EnterpriseAddress(header & 0x0fU, *payment))
                 : std::unexpected(payment.error());
}
Address EnterpriseAddress::to_address() const {
  const auto variant = header_variant(6, payment_.kind());
  core::Bytes bytes{static_cast<core::Byte>((variant << 4U) | (network_ & 0x0fU))};
  const auto payment = payment_.to_bytes();
  bytes.insert(bytes.end(), payment.begin(), payment.end());
  return Address(AddressKind::enterprise, std::move(bytes), 29);
}

RewardAddress::RewardAddress(std::uint8_t network, Credential stake)
    : network_(network), stake_(std::move(stake)) {}
core::Result<RewardAddress> RewardAddress::from_address(const Address& address) {
  if (address.kind_ != AddressKind::reward) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure, "address is not a reward address"));
  }
  const auto header = std::to_integer<std::uint8_t>(address.bytes_[0]);
  auto stake = credential_from((header & 0x10U) != 0 ? CredentialKind::script : CredentialKind::key,
                               core::ByteSpan(address.bytes_).subspan(1, 28));
  return stake ? core::Result<RewardAddress>(RewardAddress(header & 0x0fU, *stake))
               : std::unexpected(stake.error());
}
core::Result<RewardAddress> RewardAddress::from_json(std::string_view json) {
  if (json.size() < 2 || json.front() != '"' || json.back() != '"') {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                              "reward address JSON must be a compact JSON string"));
  }
  auto address = Address::from_bech32(json.substr(1, json.size() - 2));
  return address ? from_address(*address) : std::unexpected(address.error());
}
Address RewardAddress::to_address() const {
  const auto variant = header_variant(14, stake_.kind());
  core::Bytes bytes{static_cast<core::Byte>((variant << 4U) | (network_ & 0x0fU))};
  const auto stake = stake_.to_bytes();
  bytes.insert(bytes.end(), stake.begin(), stake.end());
  return Address(AddressKind::reward, std::move(bytes), 29);
}
std::string RewardAddress::to_json() const { return to_address().to_json(); }

std::uint32_t crc32(core::ByteSpan bytes) noexcept {
  std::uint32_t crc = 0xffffffffU;
  for (const auto byte : bytes) {
    crc ^= std::to_integer<std::uint8_t>(byte);
    for (unsigned bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1U) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
  }
  return ~crc;
}

std::string encode_base58(core::ByteSpan bytes) {
  std::size_t zeros = 0;
  while (zeros < bytes.size() && bytes[zeros] == core::Byte{0}) {
    ++zeros;
  }
  std::vector<std::uint8_t> digits((bytes.size() - zeros) * 138 / 100 + 1);
  std::size_t length = 0;
  for (std::size_t index = zeros; index < bytes.size(); ++index) {
    unsigned carry = std::to_integer<std::uint8_t>(bytes[index]);
    std::size_t position = 0;
    for (auto iterator = digits.rbegin();
         (carry != 0 || position < length) && iterator != digits.rend(); ++iterator, ++position) {
      carry += 256U * *iterator;
      *iterator = static_cast<std::uint8_t>(carry % 58U);
      carry /= 58U;
    }
    length = position;
  }
  auto iterator = digits.begin() + static_cast<std::ptrdiff_t>(digits.size() - length);
  while (iterator != digits.end() && *iterator == 0) {
    ++iterator;
  }
  std::string output(zeros, '1');
  for (; iterator != digits.end(); ++iterator) {
    output.push_back(BASE58_ALPHABET[*iterator]);
  }
  return output;
}

core::Result<core::Bytes> decode_base58(std::string_view encoded) {
  if (encoded.empty()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "Base58 string cannot be empty"));
  }
  std::array<std::int8_t, 128> map{};
  map.fill(-1);
  for (std::size_t index = 0; index < BASE58_ALPHABET.size(); ++index) {
    map[static_cast<unsigned char>(BASE58_ALPHABET[index])] = static_cast<std::int8_t>(index);
  }
  std::size_t zeros = 0;
  while (zeros < encoded.size() && encoded[zeros] == '1') {
    ++zeros;
  }
  std::vector<std::uint8_t> bytes((encoded.size() - zeros) * 733 / 1000 + 1);
  std::size_t length = 0;
  for (std::size_t index = zeros; index < encoded.size(); ++index) {
    const auto character = static_cast<unsigned char>(encoded[index]);
    if (character >= map.size() || map[character] < 0) {
      return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                                "Base58 string contains an invalid character"));
    }
    unsigned carry = static_cast<unsigned>(map[character]);
    std::size_t position = 0;
    for (auto iterator = bytes.rbegin();
         (carry != 0 || position < length) && iterator != bytes.rend(); ++iterator, ++position) {
      carry += 58U * *iterator;
      *iterator = static_cast<std::uint8_t>(carry & 0xffU);
      carry >>= 8U;
    }
    length = position;
  }
  auto iterator = bytes.begin() + static_cast<std::ptrdiff_t>(bytes.size() - length);
  while (iterator != bytes.end() && *iterator == 0) {
    ++iterator;
  }
  core::Bytes output(zeros, core::Byte{0});
  for (; iterator != bytes.end(); ++iterator) {
    output.push_back(static_cast<core::Byte>(*iterator));
  }
  return output;
}

ByronAddress::ByronAddress(core::Bytes envelope, std::uint32_t protocol_magic,
                           std::uint32_t checksum)
    : envelope_(std::move(envelope)), protocol_magic_(protocol_magic), checksum_(checksum) {}
core::Result<ByronAddress> ByronAddress::from_bytes(core::ByteSpan bytes) {
  auto outer = core::cbor::decode_cbor(bytes);
  if (!outer) {
    return std::unexpected(outer.error());
  }
  const auto* array = outer->as_array();
  if (array == nullptr || array->values.size() != 2) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                              "Byron envelope must be a two-item array"));
  }
  const auto* tag = array->values[0].as_tag();
  if (tag == nullptr || tag->tag != core::BigInteger(std::uint64_t{24}) || tag->value == nullptr ||
      tag->value->as_byte_string() == nullptr) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure,
                           "Byron envelope requires tag 24 containing address content"));
  }
  auto expected_crc = unsigned_value(array->values[1]);
  if (!expected_crc || *expected_crc > std::numeric_limits<std::uint32_t>::max()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::out_of_range, "Byron CRC must fit uint32"));
  }
  const auto& content_bytes = tag->value->as_byte_string()->value;
  if (crc32(content_bytes) != *expected_crc) {
    return std::unexpected(core::CardanoError(core::ErrorCode::checksum_mismatch,
                                              "Byron address CRC32 does not match"));
  }
  auto content = core::cbor::decode_cbor(content_bytes);
  if (!content) {
    return std::unexpected(content.error());
  }
  const auto* content_array = content->as_array();
  if (content_array == nullptr || content_array->values.size() != 3 ||
      content_array->values[0].as_byte_string() == nullptr ||
      content_array->values[0].as_byte_string()->value.size() != 28) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                              "Byron address content has an invalid shape"));
  }
  auto address_type = unsigned_value(content_array->values[2]);
  if (!address_type || *address_type > 2) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::out_of_range, "Byron address type must be 0, 1, or 2"));
  }
  const auto* attributes = content_array->values[1].as_map();
  if (attributes == nullptr) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                              "Byron address attributes must be a map"));
  }
  std::set<std::uint64_t> seen;
  std::uint32_t protocol_magic = core::BYRON_MAINNET_NETWORK_MAGIC;
  for (const auto& [key_node, value_node] : attributes->entries) {
    auto key = unsigned_value(key_node);
    if (!key || *key > 2 || !seen.insert(*key).second || value_node.as_byte_string() == nullptr) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::invalid_structure,
                             "Byron address attribute key/value is invalid or duplicated"));
    }
    const auto& embedded = value_node.as_byte_string()->value;
    auto decoded = core::cbor::decode_cbor(embedded);
    if (!decoded) {
      return std::unexpected(decoded.error());
    }
    if (*key == 2) {
      auto magic = unsigned_value(*decoded);
      if (!magic || *magic > std::numeric_limits<std::uint32_t>::max()) {
        return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                                  "Byron protocol magic must fit uint32"));
      }
      protocol_magic = static_cast<std::uint32_t>(*magic);
    } else if (*key == 0) {
      const auto* stake = decoded->as_array();
      if (stake == nullptr || stake->values.empty()) {
        return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                                  "Byron stake distribution attribute is invalid"));
      }
    } else if (decoded->as_byte_string() == nullptr) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::invalid_structure,
                             "Byron HD payload attribute must embed a byte string"));
    }
  }
  return ByronAddress(core::Bytes(bytes.begin(), bytes.end()), protocol_magic,
                      static_cast<std::uint32_t>(*expected_crc));
}
core::Result<ByronAddress> ByronAddress::from_base58(std::string_view encoded) {
  auto bytes = decode_base58(encoded);
  return bytes ? from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Result<ByronAddress> ByronAddress::from_address(const Address& address) {
  if (address.kind() != AddressKind::byron) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure, "address is not a Byron envelope"));
  }
  return from_bytes(address.to_bytes());
}
core::Bytes ByronAddress::to_bytes() const { return envelope_; }
std::string ByronAddress::to_base58() const { return encode_base58(envelope_); }
Address ByronAddress::to_address() const { return *Address::from_bytes(envelope_); }
std::uint32_t ByronAddress::protocol_magic() const noexcept { return protocol_magic_; }
std::uint32_t ByronAddress::checksum() const noexcept { return checksum_; }
std::optional<std::uint8_t> ByronAddress::network_id() const noexcept {
  if (protocol_magic_ == core::BYRON_MAINNET_NETWORK_MAGIC) {
    return 1;
  }
  if (protocol_magic_ == core::BYRON_TESTNET_NETWORK_MAGIC || protocol_magic_ == 1 ||
      protocol_magic_ == 2 || protocol_magic_ == 4) {
    return 0;
  }
  return std::nullopt;
}

}  // namespace cardano::chain
