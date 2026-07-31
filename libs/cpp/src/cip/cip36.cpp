#include "cardano/cip/cip36.hpp"

#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <utility>

#include "cardano/crypto/primitives.hpp"

namespace cardano::cip::cip36 {
namespace {

using core::BigInteger;
using core::CardanoError;
using core::ErrorCode;
using core::Result;
using core::cbor::MapValue;
using core::cbor::Mode;
using core::cbor::Value;
using Json = nlohmann::json;

CardanoError structure_error(std::string message) {
  return CardanoError(ErrorCode::invalid_structure, std::move(message));
}

Result<const MapValue*> require_map(const Value& value, std::string_view name) {
  const auto* map = value.as_map();
  if (map == nullptr) {
    return std::unexpected(structure_error(std::string(name) + " must be a CBOR map"));
  }
  return map;
}

Result<std::uint64_t> unsigned_value(const Value& value, std::string_view name) {
  const auto* item = value.as_unsigned();
  if (item == nullptr) {
    return std::unexpected(structure_error(std::string(name) + " must be unsigned"));
  }
  auto number = item->value.to_uint64();
  if (!number) return std::unexpected(number.error());
  return *number;
}

Result<std::map<std::uint64_t, const Value*>> unsigned_map(
    const Value& value, std::string_view name, std::initializer_list<std::uint64_t> allowed,
    bool ignore_unrelated = false) {
  auto map = require_map(value, name);
  if (!map) return std::unexpected(map.error());
  std::map<std::uint64_t, const Value*> fields;
  for (const auto& [key, item] : (*map)->entries) {
    const auto candidate = unsigned_value(key, "map key");
    if (!candidate) {
      if (ignore_unrelated) continue;
      return std::unexpected(candidate.error());
    }
    bool accepted = false;
    for (const auto allowed_key : allowed) {
      if (*candidate == allowed_key) {
        accepted = true;
        break;
      }
    }
    if (!accepted) {
      if (ignore_unrelated) continue;
      return std::unexpected(structure_error(std::string(name) + " has an unknown key"));
    }
    if (!fields.emplace(*candidate, &item).second) {
      return std::unexpected(
          CardanoError(ErrorCode::duplicate_key, std::string(name) + " has a duplicate key"));
    }
  }
  return fields;
}

Value uint(std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); }

Result<crypto::PublicKey> public_key(const Value& value, std::string_view name) {
  const auto* bytes = value.as_byte_string();
  if (bytes == nullptr) {
    return std::unexpected(structure_error(std::string(name) + " must be bytes"));
  }
  return crypto::PublicKey::from_bytes(bytes->value);
}

Value public_key_value(const crypto::PublicKey& key) { return Value::byte_string(key.to_bytes()); }

Result<DelegationDistribution> parse_distribution(const Value& value) {
  if (const auto* bytes = value.as_byte_string(); bytes != nullptr) {
    auto key = crypto::PublicKey::from_bytes(bytes->value);
    if (!key) return std::unexpected(key.error());
    return DelegationDistribution::legacy(std::move(*key));
  }
  const auto* array = value.as_array();
  if (array == nullptr || array->values.empty()) {
    return std::unexpected(structure_error("weighted delegation must be a nonempty array"));
  }
  std::vector<Delegation> delegations;
  delegations.reserve(array->values.size());
  for (const auto& item : array->values) {
    const auto* pair = item.as_array();
    if (pair == nullptr || pair->values.size() != 2) {
      return std::unexpected(structure_error("delegation must be a two-item array"));
    }
    auto key = public_key(pair->values[0], "voting public key");
    auto weight = unsigned_value(pair->values[1], "delegation weight");
    if (!key) return std::unexpected(key.error());
    if (!weight) return std::unexpected(weight.error());
    if (*weight > std::numeric_limits<std::uint32_t>::max()) {
      return std::unexpected(
          CardanoError(ErrorCode::out_of_range, "delegation weight exceeds uint32"));
    }
    delegations.push_back({std::move(*key), static_cast<std::uint32_t>(*weight)});
  }
  return DelegationDistribution::weighted(std::move(delegations));
}

Value distribution_value(const DelegationDistribution& distribution) {
  if (distribution.kind() == DelegationDistributionKind::legacy) {
    return public_key_value(*distribution.legacy_key());
  }
  std::vector<Value> values;
  values.reserve(distribution.delegations()->size());
  for (const auto& delegation : *distribution.delegations()) {
    values.push_back(Value::array({
        public_key_value(delegation.voting_public_key),
        uint(delegation.weight),
    }));
  }
  return Value::array(std::move(values));
}

Result<Json> parse_json(std::string_view text) {
  try {
    return Json::parse(text);
  } catch (const std::exception& error) {
    return std::unexpected(CardanoError(ErrorCode::invalid_encoding, error.what()));
  }
}

Result<std::uint64_t> json_uint(const Json& value, std::string_view name) {
  if (value.is_number_unsigned()) return value.get<std::uint64_t>();
  if (value.is_number_integer()) {
    const auto number = value.get<std::int64_t>();
    if (number >= 0) return static_cast<std::uint64_t>(number);
  }
  if (value.is_string()) {
    const auto& text = value.get_ref<const std::string&>();
    if (text.empty()) {
      return std::unexpected(structure_error(std::string(name) + " is empty"));
    }
    for (const char character : text) {
      if (character < '0' || character > '9') {
        return std::unexpected(
            structure_error(std::string(name) + " must contain ASCII digits only"));
      }
    }
    auto integer = BigInteger::from_decimal(text);
    if (!integer) return std::unexpected(integer.error());
    return integer->to_uint64();
  }
  return std::unexpected(
      structure_error(std::string(name) + " must be a nonnegative integer or decimal string"));
}

Json json_uint_output(std::uint64_t value) {
  constexpr std::uint64_t max_safe = 9'007'199'254'740'991ULL;
  if (value <= max_safe) return Json(value);
  return Json(std::to_string(value));
}

Result<DelegationDistribution> distribution_from_json(const Json& json) {
  if (!json.is_object()) {
    return std::unexpected(structure_error("delegation JSON must be an object"));
  }
  if (json.contains("Legacy") && json.at("Legacy").is_string()) {
    auto key = crypto::PublicKey::from_hex(json.at("Legacy").get<std::string>());
    if (!key) return std::unexpected(key.error());
    return DelegationDistribution::legacy(std::move(*key));
  }
  if (!json.contains("Weighted")) {
    return std::unexpected(structure_error("delegation JSON has no recognized variant"));
  }
  const Json* array = &json.at("Weighted");
  if (array->is_object() && array->contains("weighted")) {
    array = &array->at("weighted");
  }
  if (!array->is_array() || array->empty()) {
    return std::unexpected(structure_error("weighted delegation JSON must be nonempty"));
  }
  std::vector<Delegation> delegations;
  for (const auto& item : *array) {
    if (!item.is_object() || !item.contains("voting_pub_key") || !item.contains("weight") ||
        !item.at("voting_pub_key").is_string()) {
      return std::unexpected(structure_error("invalid delegation JSON"));
    }
    auto key = crypto::PublicKey::from_hex(item.at("voting_pub_key").get<std::string>());
    auto weight = json_uint(item.at("weight"), "weight");
    if (!key) return std::unexpected(key.error());
    if (!weight) return std::unexpected(weight.error());
    if (*weight > std::numeric_limits<std::uint32_t>::max()) {
      return std::unexpected(CardanoError(ErrorCode::out_of_range, "weight exceeds uint32"));
    }
    delegations.push_back({std::move(*key), static_cast<std::uint32_t>(*weight)});
  }
  return DelegationDistribution::weighted(std::move(delegations));
}

Json distribution_json(const DelegationDistribution& distribution) {
  if (distribution.kind() == DelegationDistributionKind::legacy) {
    return Json{{"Legacy", distribution.legacy_key()->to_hex()}};
  }
  Json values = Json::array();
  for (const auto& delegation : *distribution.delegations()) {
    values.push_back({
        {"voting_pub_key", delegation.voting_public_key.to_hex()},
        {"weight", delegation.weight},
    });
  }
  return Json{{"Weighted", Json{{"weighted", std::move(values)}}}};
}

Result<RegistrationWitness> witness_from_json(const Json& json) {
  if (!json.is_object() || !json.contains("stake_witness") ||
      !json.at("stake_witness").is_string()) {
    return std::unexpected(structure_error("witness JSON requires stake_witness"));
  }
  auto signature = crypto::Ed25519Signature::from_hex(json.at("stake_witness").get<std::string>());
  if (!signature) return std::unexpected(signature.error());
  return RegistrationWitness(std::move(*signature));
}

Json witness_json(const RegistrationWitness& witness) {
  return Json{{"stake_witness", witness.signature().to_hex()}};
}

Result<Value> merge_metadata(const Value& metadata, std::vector<std::pair<Value, Value>> additions,
                             std::initializer_list<std::uint64_t> replaced_labels) {
  auto map = require_map(metadata, "metadata");
  if (!map) return std::unexpected(map.error());
  std::vector<std::pair<Value, Value>> entries;
  for (const auto& [key, item] : (*map)->entries) {
    const auto* number = key.as_unsigned();
    bool replaced = false;
    if (number != nullptr && number->value.fits_uint64()) {
      const auto label = number->value.to_uint64().value();
      for (const auto replacement : replaced_labels) {
        if (label == replacement) replaced = true;
      }
    }
    if (!replaced) entries.emplace_back(key, item);
  }
  for (auto& addition : additions) entries.push_back(std::move(addition));
  return Value::map(std::move(entries));
}

core::Bytes hash_proposal(std::uint64_t label, Value inner, bool canonical) {
  auto bytes = core::cbor::encode_cbor(Value::map({{uint(label), std::move(inner)}}),
                                       {.mode = canonical ? Mode::canonical : Mode::preserve});
  if (!bytes) return {};
  return crypto::blake2b256(*bytes);
}

}  // namespace

DelegationDistribution::DelegationDistribution(
    std::variant<crypto::PublicKey, std::vector<Delegation>> value)
    : value_(std::move(value)) {}

DelegationDistribution DelegationDistribution::legacy(crypto::PublicKey voting_public_key) {
  return DelegationDistribution(std::move(voting_public_key));
}

Result<DelegationDistribution> DelegationDistribution::weighted(
    std::vector<Delegation> delegations) {
  if (delegations.empty()) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_structure, "weighted delegations must be nonempty"));
  }
  return DelegationDistribution(std::move(delegations));
}

DelegationDistributionKind DelegationDistribution::kind() const noexcept {
  return std::holds_alternative<crypto::PublicKey>(value_) ? DelegationDistributionKind::legacy
                                                           : DelegationDistributionKind::weighted;
}

const crypto::PublicKey* DelegationDistribution::legacy_key() const noexcept {
  return std::get_if<crypto::PublicKey>(&value_);
}

const std::vector<Delegation>* DelegationDistribution::delegations() const noexcept {
  return std::get_if<std::vector<Delegation>>(&value_);
}

RegistrationWitness::RegistrationWitness(crypto::Ed25519Signature signature)
    : signature_(std::move(signature)) {}

RegistrationWitness::RegistrationWitness(crypto::Ed25519Signature signature,
                                         std::shared_ptr<const Value> preserved)
    : signature_(std::move(signature)), preserved_(std::move(preserved)) {}

Result<RegistrationWitness> RegistrationWitness::from_cbor_value(const Value& value) {
  auto fields = unsigned_map(value, "CIP-36 witness", {1});
  if (!fields) return std::unexpected(fields.error());
  if (!fields->contains(1)) {
    return std::unexpected(structure_error("CIP-36 witness is missing key 1"));
  }
  const auto* bytes = fields->at(1)->as_byte_string();
  if (bytes == nullptr) {
    return std::unexpected(structure_error("CIP-36 signature must be bytes"));
  }
  auto signature = crypto::Ed25519Signature::from_bytes(bytes->value);
  if (!signature) return std::unexpected(signature.error());
  return RegistrationWitness(std::move(*signature), std::make_shared<const Value>(value));
}

Value RegistrationWitness::to_cbor_value(Mode mode) const {
  if (mode == Mode::preserve && preserved_) return *preserved_;
  return Value::map({{uint(1), Value::byte_string(signature_.to_bytes())}});
}

const crypto::Ed25519Signature& RegistrationWitness::signature() const noexcept {
  return signature_;
}

KeyRegistration::KeyRegistration(DelegationDistribution delegation,
                                 crypto::PublicKey stake_credential, chain::Address payment_address,
                                 std::uint64_t nonce, std::uint64_t voting_purpose,
                                 bool purpose_explicit, bool legacy_locked,
                                 std::shared_ptr<const Value> preserved)
    : delegation_(std::move(delegation)),
      stake_credential_(std::move(stake_credential)),
      payment_address_(std::move(payment_address)),
      nonce_(nonce),
      voting_purpose_(voting_purpose),
      purpose_explicit_(purpose_explicit),
      legacy_locked_(legacy_locked),
      preserved_(std::move(preserved)) {}

KeyRegistration KeyRegistration::legacy(DelegationDistribution delegation,
                                        crypto::PublicKey stake_credential,
                                        chain::Address payment_address, std::uint64_t nonce) {
  return KeyRegistration(std::move(delegation), std::move(stake_credential),
                         std::move(payment_address), nonce, 0, false, true);
}

Result<KeyRegistration> KeyRegistration::weighted(DelegationDistribution delegation,
                                                  crypto::PublicKey stake_credential,
                                                  chain::Address payment_address,
                                                  std::uint64_t nonce) {
  if (delegation.kind() != DelegationDistributionKind::weighted) {
    return std::unexpected(structure_error("weighted registration requires weighted delegation"));
  }
  return KeyRegistration(std::move(delegation), std::move(stake_credential),
                         std::move(payment_address), nonce, 0, true, false);
}

Result<KeyRegistration> KeyRegistration::from_cbor_value(const Value& value) {
  auto fields = unsigned_map(value, "CIP-36 key registration", {1, 2, 3, 4, 5});
  if (!fields) return std::unexpected(fields.error());
  for (const auto required : {1ULL, 2ULL, 3ULL, 4ULL}) {
    if (!fields->contains(required)) {
      return std::unexpected(structure_error("CIP-36 key registration is incomplete"));
    }
  }
  auto delegation = parse_distribution(*fields->at(1));
  auto stake = public_key(*fields->at(2), "stake credential");
  const auto* payment_bytes = fields->at(3)->as_byte_string();
  if (!delegation) return std::unexpected(delegation.error());
  if (!stake) return std::unexpected(stake.error());
  if (payment_bytes == nullptr) {
    return std::unexpected(structure_error("payment address must be bytes"));
  }
  auto payment = chain::Address::from_bytes(payment_bytes->value);
  auto nonce = unsigned_value(*fields->at(4), "nonce");
  if (!payment) return std::unexpected(payment.error());
  if (!nonce) return std::unexpected(nonce.error());
  std::uint64_t purpose = 0;
  if (fields->contains(5)) {
    auto parsed = unsigned_value(*fields->at(5), "voting purpose");
    if (!parsed) return std::unexpected(parsed.error());
    purpose = *parsed;
  }
  return KeyRegistration(std::move(*delegation), std::move(*stake), std::move(*payment), *nonce,
                         purpose, fields->contains(5),
                         delegation->kind() == DelegationDistributionKind::legacy,
                         std::make_shared<const Value>(value));
}

Result<KeyRegistration> KeyRegistration::from_json(std::string_view text) {
  auto json = parse_json(text);
  if (!json) return std::unexpected(json.error());
  if (!json->is_object() || !json->contains("delegation") || !json->contains("stake_credential") ||
      !json->contains("payment_address") || !json->contains("nonce")) {
    return std::unexpected(structure_error("registration JSON is incomplete"));
  }
  auto distribution = distribution_from_json(json->at("delegation"));
  if (!distribution) return std::unexpected(distribution.error());
  if (!json->at("stake_credential").is_string() || !json->at("payment_address").is_string()) {
    return std::unexpected(structure_error("registration JSON strings are invalid"));
  }
  auto stake = crypto::PublicKey::from_hex(json->at("stake_credential").get<std::string>());
  auto payment = chain::Address::from_bech32(json->at("payment_address").get<std::string>());
  auto nonce = json_uint(json->at("nonce"), "nonce");
  if (!stake) return std::unexpected(stake.error());
  if (!payment) return std::unexpected(payment.error());
  if (!nonce) return std::unexpected(nonce.error());
  const bool explicit_purpose = json->contains("voting_purpose");
  std::uint64_t purpose = 0;
  if (explicit_purpose) {
    auto parsed = json_uint(json->at("voting_purpose"), "voting purpose");
    if (!parsed) return std::unexpected(parsed.error());
    purpose = *parsed;
  }
  return KeyRegistration(std::move(*distribution), std::move(*stake), std::move(*payment), *nonce,
                         purpose, explicit_purpose,
                         distribution->kind() == DelegationDistributionKind::legacy);
}

const DelegationDistribution& KeyRegistration::delegation() const noexcept { return delegation_; }
const crypto::PublicKey& KeyRegistration::stake_credential() const noexcept {
  return stake_credential_;
}
const chain::Address& KeyRegistration::payment_address() const noexcept { return payment_address_; }
std::uint64_t KeyRegistration::nonce() const noexcept { return nonce_; }
std::uint64_t KeyRegistration::voting_purpose() const noexcept { return voting_purpose_; }
bool KeyRegistration::has_explicit_voting_purpose() const noexcept { return purpose_explicit_; }

void KeyRegistration::set_voting_purpose(std::uint64_t purpose) {
  voting_purpose_ = purpose;
  if (legacy_locked_) {
    purpose_explicit_ = false;
  } else if (purpose != 0 || purpose_explicit_) {
    purpose_explicit_ = true;
  }
  mutated_ = true;
}

core::VoidResult KeyRegistration::verify() const {
  if (delegation_.kind() == DelegationDistributionKind::weighted) {
    for (const auto& delegation : *delegation_.delegations()) {
      if (delegation.weight != 0) {
        return std::unexpected(
            CardanoError(ErrorCode::invalid_structure,
                         "CIP-36 compatibility check rejects nonzero delegation weight"));
      }
    }
  }
  return std::monostate{};
}

Value KeyRegistration::to_cbor_value(Mode mode) const {
  if (mode == Mode::preserve && preserved_ && !mutated_) return *preserved_;
  std::vector<std::pair<Value, Value>> entries{
      {uint(1), distribution_value(delegation_)},
      {uint(2), public_key_value(stake_credential_)},
      {uint(3), Value::byte_string(payment_address_.to_bytes())},
      {uint(4), uint(nonce_)},
  };
  if (purpose_explicit_) entries.emplace_back(uint(5), uint(voting_purpose_));
  return Value::map(std::move(entries));
}

std::string KeyRegistration::to_json() const {
  const auto payment = payment_address_.to_bech32();
  return Json{
      {"delegation", distribution_json(delegation_)},
      {"stake_credential", stake_credential_.to_hex()},
      {"payment_address", payment ? *payment : ""},
      {"nonce", json_uint_output(nonce_)},
      {"voting_purpose", json_uint_output(voting_purpose_)},
  }
      .dump();
}

KeyDeregistration::KeyDeregistration(crypto::PublicKey stake_credential, std::uint64_t nonce)
    : stake_credential_(std::move(stake_credential)), nonce_(nonce) {}

KeyDeregistration::KeyDeregistration(crypto::PublicKey stake_credential, std::uint64_t nonce,
                                     std::uint64_t voting_purpose, bool purpose_explicit,
                                     std::shared_ptr<const Value> preserved)
    : stake_credential_(std::move(stake_credential)),
      nonce_(nonce),
      voting_purpose_(voting_purpose),
      purpose_explicit_(purpose_explicit),
      preserved_(std::move(preserved)) {}

Result<KeyDeregistration> KeyDeregistration::from_cbor_value(const Value& value) {
  auto fields = unsigned_map(value, "CIP-36 key deregistration", {1, 2, 3});
  if (!fields) return std::unexpected(fields.error());
  if (!fields->contains(1) || !fields->contains(2)) {
    return std::unexpected(structure_error("CIP-36 key deregistration is incomplete"));
  }
  auto stake = public_key(*fields->at(1), "stake credential");
  auto nonce = unsigned_value(*fields->at(2), "nonce");
  if (!stake) return std::unexpected(stake.error());
  if (!nonce) return std::unexpected(nonce.error());
  std::uint64_t purpose = 0;
  if (fields->contains(3)) {
    auto parsed = unsigned_value(*fields->at(3), "voting purpose");
    if (!parsed) return std::unexpected(parsed.error());
    purpose = *parsed;
  }
  return KeyDeregistration(std::move(*stake), *nonce, purpose, fields->contains(3),
                           std::make_shared<const Value>(value));
}

Result<KeyDeregistration> KeyDeregistration::from_json(std::string_view text) {
  auto json = parse_json(text);
  if (!json) return std::unexpected(json.error());
  if (!json->is_object() || !json->contains("stake_credential") || !json->contains("nonce") ||
      !json->at("stake_credential").is_string()) {
    return std::unexpected(structure_error("deregistration JSON is incomplete"));
  }
  auto stake = crypto::PublicKey::from_hex(json->at("stake_credential").get<std::string>());
  auto nonce = json_uint(json->at("nonce"), "nonce");
  if (!stake) return std::unexpected(stake.error());
  if (!nonce) return std::unexpected(nonce.error());
  const bool explicit_purpose = json->contains("voting_purpose");
  std::uint64_t purpose = 0;
  if (explicit_purpose) {
    auto parsed = json_uint(json->at("voting_purpose"), "voting purpose");
    if (!parsed) return std::unexpected(parsed.error());
    purpose = *parsed;
  }
  return KeyDeregistration(std::move(*stake), *nonce, purpose, explicit_purpose, nullptr);
}

const crypto::PublicKey& KeyDeregistration::stake_credential() const noexcept {
  return stake_credential_;
}
std::uint64_t KeyDeregistration::nonce() const noexcept { return nonce_; }
std::uint64_t KeyDeregistration::voting_purpose() const noexcept { return voting_purpose_; }
bool KeyDeregistration::has_explicit_voting_purpose() const noexcept { return purpose_explicit_; }
void KeyDeregistration::set_voting_purpose(std::uint64_t purpose) {
  voting_purpose_ = purpose;
  if (purpose != 0 || purpose_explicit_) purpose_explicit_ = true;
  mutated_ = true;
}

Value KeyDeregistration::to_cbor_value(Mode mode) const {
  if (mode == Mode::preserve && preserved_ && !mutated_) return *preserved_;
  std::vector<std::pair<Value, Value>> entries{
      {uint(1), public_key_value(stake_credential_)},
      {uint(2), uint(nonce_)},
  };
  if (purpose_explicit_) entries.emplace_back(uint(3), uint(voting_purpose_));
  return Value::map(std::move(entries));
}

std::string KeyDeregistration::to_json() const {
  return Json{
      {"stake_credential", stake_credential_.to_hex()},
      {"nonce", json_uint_output(nonce_)},
      {"voting_purpose", json_uint_output(voting_purpose_)},
  }
      .dump();
}

RegistrationCbor::RegistrationCbor(KeyRegistration registration, RegistrationWitness witness)
    : registration_(std::move(registration)), witness_(std::move(witness)) {}

Result<RegistrationCbor> RegistrationCbor::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  return from_cbor_value(*value);
}

Result<RegistrationCbor> RegistrationCbor::from_cbor_value(const Value& value) {
  auto fields = unsigned_map(value, "CIP-36 registration metadata",
                             {REGISTRATION_LABEL, WITNESS_LABEL}, true);
  if (!fields) return std::unexpected(fields.error());
  if (!fields->contains(REGISTRATION_LABEL) || !fields->contains(WITNESS_LABEL)) {
    return std::unexpected(structure_error("registration metadata is incomplete"));
  }
  auto registration = KeyRegistration::from_cbor_value(*fields->at(REGISTRATION_LABEL));
  auto witness = RegistrationWitness::from_cbor_value(*fields->at(WITNESS_LABEL));
  if (!registration) return std::unexpected(registration.error());
  if (!witness) return std::unexpected(witness.error());
  return RegistrationCbor(std::move(*registration), std::move(*witness));
}

Result<RegistrationCbor> RegistrationCbor::from_json(std::string_view text) {
  auto json = parse_json(text);
  if (!json) return std::unexpected(json.error());
  if (!json->is_object() || !json->contains("key_registration") ||
      !json->contains("registration_witness")) {
    return std::unexpected(structure_error("registration view JSON is incomplete"));
  }
  auto registration = KeyRegistration::from_json(json->at("key_registration").dump());
  auto witness = witness_from_json(json->at("registration_witness"));
  if (!registration) return std::unexpected(registration.error());
  if (!witness) return std::unexpected(witness.error());
  return RegistrationCbor(std::move(*registration), std::move(*witness));
}

const KeyRegistration& RegistrationCbor::registration() const noexcept { return registration_; }
KeyRegistration& RegistrationCbor::registration() noexcept { return registration_; }
const RegistrationWitness& RegistrationCbor::witness() const noexcept { return witness_; }

core::Bytes RegistrationCbor::hash_to_sign(bool force_canonical) const {
  return hash_proposal(
      REGISTRATION_LABEL,
      registration_.to_cbor_value(force_canonical ? Mode::canonical : Mode::preserve),
      force_canonical);
}

Result<core::Bytes> RegistrationCbor::to_bytes(Mode mode) const {
  auto verified = registration_.verify();
  if (!verified) return std::unexpected(verified.error());
  return core::cbor::encode_cbor(Value::map({
                                     {uint(REGISTRATION_LABEL), registration_.to_cbor_value(mode)},
                                     {uint(WITNESS_LABEL), witness_.to_cbor_value(mode)},
                                 }),
                                 {.mode = mode});
}

Result<Value> RegistrationCbor::add_to_metadata(const Value& metadata) const {
  auto verified = registration_.verify();
  if (!verified) return std::unexpected(verified.error());
  return merge_metadata(metadata,
                        {
                            {uint(REGISTRATION_LABEL), registration_.to_cbor_value()},
                            {uint(WITNESS_LABEL), witness_.to_cbor_value()},
                        },
                        {REGISTRATION_LABEL, WITNESS_LABEL});
}

std::string RegistrationCbor::to_json() const {
  return Json{
      {"key_registration", Json::parse(registration_.to_json())},
      {"registration_witness", witness_json(witness_)},
  }
      .dump();
}

DeregistrationCbor::DeregistrationCbor(KeyDeregistration deregistration,
                                       DeregistrationWitness witness)
    : deregistration_(std::move(deregistration)), witness_(std::move(witness)) {}

Result<DeregistrationCbor> DeregistrationCbor::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  return from_cbor_value(*value);
}

Result<DeregistrationCbor> DeregistrationCbor::from_cbor_value(const Value& value) {
  auto fields = unsigned_map(value, "CIP-36 deregistration metadata",
                             {DEREGISTRATION_LABEL, WITNESS_LABEL}, true);
  if (!fields) return std::unexpected(fields.error());
  if (!fields->contains(DEREGISTRATION_LABEL) || !fields->contains(WITNESS_LABEL)) {
    return std::unexpected(structure_error("deregistration metadata is incomplete"));
  }
  auto deregistration = KeyDeregistration::from_cbor_value(*fields->at(DEREGISTRATION_LABEL));
  auto witness = DeregistrationWitness::from_cbor_value(*fields->at(WITNESS_LABEL));
  if (!deregistration) return std::unexpected(deregistration.error());
  if (!witness) return std::unexpected(witness.error());
  return DeregistrationCbor(std::move(*deregistration), std::move(*witness));
}

Result<DeregistrationCbor> DeregistrationCbor::from_json(std::string_view text) {
  auto json = parse_json(text);
  if (!json) return std::unexpected(json.error());
  if (!json->is_object() || !json->contains("key_deregistration") ||
      !json->contains("deregistration_witness")) {
    return std::unexpected(structure_error("deregistration view JSON is incomplete"));
  }
  auto deregistration = KeyDeregistration::from_json(json->at("key_deregistration").dump());
  auto witness = witness_from_json(json->at("deregistration_witness"));
  if (!deregistration) return std::unexpected(deregistration.error());
  if (!witness) return std::unexpected(witness.error());
  return DeregistrationCbor(std::move(*deregistration), std::move(*witness));
}

const KeyDeregistration& DeregistrationCbor::deregistration() const noexcept {
  return deregistration_;
}
KeyDeregistration& DeregistrationCbor::deregistration() noexcept { return deregistration_; }
const DeregistrationWitness& DeregistrationCbor::witness() const noexcept { return witness_; }

core::Bytes DeregistrationCbor::hash_to_sign(bool force_canonical) const {
  return hash_proposal(
      DEREGISTRATION_LABEL,
      deregistration_.to_cbor_value(force_canonical ? Mode::canonical : Mode::preserve),
      force_canonical);
}

Result<core::Bytes> DeregistrationCbor::to_bytes(Mode mode) const {
  return core::cbor::encode_cbor(
      Value::map({
          {uint(WITNESS_LABEL), witness_.to_cbor_value(mode)},
          {uint(DEREGISTRATION_LABEL), deregistration_.to_cbor_value(mode)},
      }),
      {.mode = mode});
}

Result<Value> DeregistrationCbor::add_to_metadata(const Value& metadata) const {
  return merge_metadata(metadata,
                        {
                            {uint(DEREGISTRATION_LABEL), deregistration_.to_cbor_value()},
                            {uint(WITNESS_LABEL), witness_.to_cbor_value()},
                        },
                        {DEREGISTRATION_LABEL, WITNESS_LABEL});
}

std::string DeregistrationCbor::to_json() const {
  return Json{
      {"key_deregistration", Json::parse(deregistration_.to_json())},
      {"deregistration_witness", witness_json(witness_)},
  }
      .dump();
}

}  // namespace cardano::cip::cip36
