#include "cardano/chain/builder.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <utility>

namespace cardano::chain {
namespace {

using core::BigInteger;
using core::CardanoError;
using core::ErrorCode;
using core::Result;
using CborValue = core::cbor::Value;

CardanoError argument_error(std::string message) {
  return CardanoError(ErrorCode::invalid_argument, std::move(message));
}

CardanoError balance_error(std::string message) {
  return CardanoError(ErrorCode::balance, std::move(message));
}

CborValue uint(std::uint64_t value) { return CborValue::unsigned_integer(BigInteger(value)); }

CborValue integer(std::int64_t value) {
  return value >= 0 ? CborValue::unsigned_integer(BigInteger(value))
                    : CborValue::negative_integer(BigInteger(value));
}

std::string asset_key(const crypto::ScriptHash& policy, const AssetName& asset) {
  return policy.to_hex() + ":" + asset.hex();
}

Result<std::vector<PolicyAssets>> normalize_assets(const std::vector<PolicyAssets>& input,
                                                   bool reject_zero) {
  std::vector<PolicyAssets> output;
  std::map<std::string, std::pair<std::size_t, std::size_t>> positions;
  for (const auto& policy : input) {
    if (policy.assets.empty()) {
      return std::unexpected(argument_error("asset policy must be nonempty"));
    }
    for (const auto& asset : policy.assets) {
      if (reject_zero && asset.quantity == 0) {
        return std::unexpected(argument_error("asset quantity cannot be zero"));
      }
      const auto identity = asset_key(policy.policy, asset.name);
      const auto existing = positions.find(identity);
      if (existing == positions.end()) {
        auto policy_position = std::find_if(output.begin(), output.end(), [&](const auto& item) {
          return item.policy == policy.policy;
        });
        if (policy_position == output.end()) {
          output.push_back({policy.policy, {}});
          policy_position = std::prev(output.end());
        }
        policy_position->assets.push_back(asset);
        positions.emplace(
            identity,
            std::pair{static_cast<std::size_t>(std::distance(output.begin(), policy_position)),
                      policy_position->assets.size() - 1});
      } else {
        auto& quantity = output[existing->second.first].assets[existing->second.second].quantity;
        if ((asset.quantity > 0 &&
             quantity > std::numeric_limits<std::int64_t>::max() - asset.quantity) ||
            (asset.quantity < 0 &&
             quantity < std::numeric_limits<std::int64_t>::min() - asset.quantity)) {
          return std::unexpected(CardanoError(ErrorCode::out_of_range, "asset quantity overflow"));
        }
        quantity += asset.quantity;
        if (reject_zero && quantity == 0) {
          return std::unexpected(argument_error("merged asset quantity cannot equal zero"));
        }
      }
    }
  }
  for (auto& policy : output) {
    std::erase_if(policy.assets, [](const auto& asset) { return asset.quantity == 0; });
  }
  std::erase_if(output, [](const auto& policy) { return policy.assets.empty(); });
  return output;
}

Result<Value> add_asset_delta(const Value& base, const crypto::ScriptHash& policy,
                              const AssetName& asset, std::int64_t delta) {
  std::vector<PolicyAssets> values = base.multiasset();
  auto policy_it = std::find_if(values.begin(), values.end(),
                                [&](const auto& item) { return item.policy == policy; });
  if (policy_it == values.end()) {
    values.push_back({policy, {{asset, delta}}});
  } else {
    auto asset_it = std::find_if(policy_it->assets.begin(), policy_it->assets.end(),
                                 [&](const auto& item) { return item.name == asset; });
    if (asset_it == policy_it->assets.end()) {
      policy_it->assets.push_back({asset, delta});
    } else {
      if ((delta > 0 && asset_it->quantity > std::numeric_limits<std::int64_t>::max() - delta) ||
          (delta < 0 && asset_it->quantity < std::numeric_limits<std::int64_t>::min() - delta)) {
        return std::unexpected(CardanoError(ErrorCode::out_of_range, "asset arithmetic overflow"));
      }
      asset_it->quantity += delta;
    }
  }
  auto normalized = normalize_assets(values, false);
  if (!normalized) return std::unexpected(normalized.error());
  return Value(base.coin(), std::move(*normalized));
}

CborValue tagged_set(std::vector<CborValue> values) {
  return CborValue::tag(BigInteger(std::uint64_t{258}), CborValue::array(std::move(values)),
                        core::cbor::HeadWidth::two);
}

Result<std::uint32_t> random_u32(core::SecureRandomSource& random) {
  auto bytes = random.random_bytes(4);
  if (!bytes) return std::unexpected(bytes.error());
  if (bytes->size() != 4) {
    return std::unexpected(CardanoError(ErrorCode::random_unavailable,
                                        "random source returned an invalid byte count"));
  }
  return std::to_integer<std::uint32_t>((*bytes)[0]) |
         (std::to_integer<std::uint32_t>((*bytes)[1]) << 8U) |
         (std::to_integer<std::uint32_t>((*bytes)[2]) << 16U) |
         (std::to_integer<std::uint32_t>((*bytes)[3]) << 24U);
}

template <typename T>
core::VoidResult shuffle(std::vector<T>& values, core::SecureRandomSource& random) {
  for (std::size_t index = values.size(); index > 1; --index) {
    auto draw = random_u32(random);
    if (!draw) return std::unexpected(draw.error());
    std::swap(values[index - 1], values[*draw % index]);
  }
  return std::monostate{};
}

std::string canonical_hex(const CborValue& value) {
  const auto encoded = core::cbor::encode_cbor(value, {.mode = core::cbor::Mode::canonical});
  return encoded ? core::bytes_to_hex(*encoded) : std::string{};
}

struct AddressRequirement {
  std::optional<crypto::Ed25519KeyHash> key;
  std::optional<crypto::ScriptHash> script;
};

Result<AddressRequirement> address_requirement(const Address& address, bool reward) {
  const auto raw = address.to_bytes();
  if (raw.empty() || address.kind() == AddressKind::byron) {
    return AddressRequirement{};
  }
  const auto variant = std::to_integer<std::uint8_t>(raw[0]) >> 4U;
  const bool allowed = reward ? (variant == 14 || variant == 15) : (variant <= 7);
  if (!allowed || raw.size() < 29) {
    return std::unexpected(argument_error(reward ? "withdrawal address is not a reward address"
                                                 : "input address has no payment credential"));
  }
  const auto hash = core::ByteSpan(raw).subspan(1, 28);
  const bool script = reward ? variant == 15 : (variant & 1U) != 0;
  if (script) {
    auto value = crypto::ScriptHash::from_bytes(hash);
    return value ? Result<AddressRequirement>(AddressRequirement{.script = std::move(*value)})
                 : std::unexpected(value.error());
  }
  auto value = crypto::Ed25519KeyHash::from_bytes(hash);
  return value ? Result<AddressRequirement>(AddressRequirement{.key = std::move(*value)})
               : std::unexpected(value.error());
}

void add_script_requirement(RequiredWitnessSet& required,
                            const InputAggregateWitnessData& aggregate) {
  const auto identity = aggregate.script_hash().to_hex();
  if (aggregate.kind() == InputAggregateWitnessData::Kind::native) {
    required.native_scripts.insert(identity);
  } else {
    const auto language = aggregate.plutus_witness()->script().script()
                              ? aggregate.plutus_witness()->script().script()->language()
                              : std::uint8_t{1};
    if (language == 1) required.plutus_v1_scripts.insert(identity);
    if (language == 2) required.plutus_v2_scripts.insert(identity);
    if (language == 3) required.plutus_v3_scripts.insert(identity);
  }
  for (const auto& signer : aggregate.signers()) {
    required.vkeys.insert(signer.to_hex());
  }
}

Result<RequiredWitnessSet> certificate_requirements(const CborValue& certificate) {
  RequiredWitnessSet required;
  const auto* array = certificate.as_array();
  if (array == nullptr || array->values.empty() || array->values[0].as_unsigned() == nullptr) {
    return std::unexpected(argument_error("certificate must be a tagged array"));
  }
  auto tag_value = array->values[0].as_unsigned()->value.to_uint64();
  if (!tag_value) return std::unexpected(tag_value.error());
  const auto tag = *tag_value;
  const auto add_key = [&](const CborValue& value) -> core::VoidResult {
    const auto* bytes_value = value.as_byte_string();
    if (bytes_value == nullptr) {
      return std::unexpected(argument_error("certificate key must be bytes"));
    }
    auto hash = crypto::Ed25519KeyHash::from_bytes(bytes_value->value);
    if (!hash) return std::unexpected(hash.error());
    required.vkeys.insert(hash->to_hex());
    return std::monostate{};
  };
  if (tag == 3) {
    if (array->values.size() < 2 || array->values[1].as_array() == nullptr) {
      return std::unexpected(argument_error("pool registration is malformed"));
    }
    const auto& pool = array->values[1].as_array()->values;
    if (pool.size() < 7) {
      return std::unexpected(argument_error("pool registration is incomplete"));
    }
    auto status = add_key(pool[0]);
    if (!status) return std::unexpected(status.error());
    const auto* owners = pool[6].as_array();
    const auto* owner_values = owners;
    if (owner_values == nullptr && pool[6].as_tag() != nullptr &&
        pool[6].as_tag()->value != nullptr) {
      owner_values = pool[6].as_tag()->value->as_array();
    }
    if (owner_values == nullptr) {
      return std::unexpected(argument_error("pool owners must be a set"));
    }
    for (const auto& owner : owner_values->values) {
      status = add_key(owner);
      if (!status) return std::unexpected(status.error());
    }
    return required;
  }
  if (tag == 4) {
    if (array->values.size() < 2) {
      return std::unexpected(argument_error("pool retirement is incomplete"));
    }
    auto status = add_key(array->values[1]);
    return status ? Result<RequiredWitnessSet>(std::move(required))
                  : std::unexpected(status.error());
  }
  const std::set<std::uint64_t> credential_tags{1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  if (!credential_tags.contains(tag)) return required;
  if (array->values.size() < 2 || array->values[1].as_array() == nullptr) {
    return std::unexpected(argument_error("certificate credential is malformed"));
  }
  const auto& credential = array->values[1].as_array()->values;
  if (credential.size() != 2 || credential[0].as_unsigned() == nullptr ||
      credential[1].as_byte_string() == nullptr) {
    return std::unexpected(argument_error("certificate credential is malformed"));
  }
  auto kind = credential[0].as_unsigned()->value.to_uint64();
  if (!kind || *kind > 1) {
    return std::unexpected(argument_error("certificate credential kind is invalid"));
  }
  if (*kind == 0) {
    auto hash = crypto::Ed25519KeyHash::from_bytes(credential[1].as_byte_string()->value);
    if (!hash) return std::unexpected(hash.error());
    required.vkeys.insert(hash->to_hex());
  } else {
    auto hash = crypto::ScriptHash::from_bytes(credential[1].as_byte_string()->value);
    if (!hash) return std::unexpected(hash.error());
    required.native_scripts.insert(hash->to_hex());
  }
  return required;
}

Result<std::vector<crypto::Ed25519KeyHash>> native_script_signers(const CborValue& script,
                                                                  std::size_t depth = 0) {
  if (depth > 128) {
    return std::unexpected(CardanoError(ErrorCode::depth_limit_exceeded,
                                        "native script signer discovery exceeds depth 128"));
  }
  const auto* fields = script.as_array();
  if (fields == nullptr || fields->values.empty() || fields->values[0].as_unsigned() == nullptr) {
    return std::unexpected(argument_error("native script must be a discriminated array"));
  }
  auto kind = fields->values[0].as_unsigned()->value.to_uint64();
  if (!kind || *kind > 5) {
    return std::unexpected(argument_error("native script discriminator must be in 0..5"));
  }
  if (*kind == 0) {
    if (fields->values.size() != 2 || fields->values[1].as_byte_string() == nullptr) {
      return std::unexpected(argument_error("native public-key script must be [0, bytes28]"));
    }
    auto hash = crypto::Ed25519KeyHash::from_bytes(fields->values[1].as_byte_string()->value);
    if (!hash) return std::unexpected(hash.error());
    return std::vector<crypto::Ed25519KeyHash>{std::move(*hash)};
  }
  if (*kind == 4 || *kind == 5) {
    if (fields->values.size() != 2 || fields->values[1].as_unsigned() == nullptr) {
      return std::unexpected(
          argument_error("native timelock script must contain one unsigned slot"));
    }
    return std::vector<crypto::Ed25519KeyHash>{};
  }
  const std::size_t list_index = *kind == 3 ? 2 : 1;
  if (fields->values.size() != list_index + 1 ||
      (*kind == 3 && fields->values[1].as_unsigned() == nullptr)) {
    return std::unexpected(argument_error("native script collection has an invalid shape"));
  }
  const auto* scripts = fields->values[list_index].as_array();
  if (scripts == nullptr) {
    return std::unexpected(argument_error("native script children must be an array"));
  }
  std::vector<crypto::Ed25519KeyHash> result;
  std::set<std::string> seen;
  for (const auto& child : scripts->values) {
    auto child_signers = native_script_signers(child, depth + 1);
    if (!child_signers) return std::unexpected(child_signers.error());
    for (auto& signer : *child_signers) {
      if (seen.insert(signer.to_hex()).second) {
        result.push_back(std::move(signer));
      }
    }
  }
  return result;
}

core::VoidResult apply_aggregate(TransactionWitnessSetBuilder& witnesses,
                                 RedeemerSetBuilder& redeemers, RedeemerWitnessKey redeemer_key,
                                 const InputAggregateWitnessData& aggregate) {
  for (const auto& signer : aggregate.signers()) {
    witnesses.require_vkey(signer);
  }
  if (aggregate.kind() == InputAggregateWitnessData::Kind::native) {
    witnesses.require_native_script(aggregate.script_hash());
    auto script_status =
        witnesses.add_native_script(aggregate.script_hash(), *aggregate.native_script());
    if (!script_status) return script_status;
    auto discovered = native_script_signers(*aggregate.native_script());
    if (!discovered) return std::unexpected(discovered.error());
    std::vector<crypto::Ed25519KeyHash> signers;
    switch (aggregate.native_info()->mode()) {
      case NativeScriptWitnessMode::vkeys:
        signers = aggregate.native_info()->hashes();
        break;
      case NativeScriptWitnessMode::signature_count: {
        const auto count = std::min(aggregate.native_info()->count(), discovered->size());
        signers.insert(signers.end(), discovered->begin(), discovered->begin() + count);
        break;
      }
      case NativeScriptWitnessMode::assume_signature_count:
        signers = std::move(*discovered);
        break;
    }
    for (const auto& signer : signers) {
      witnesses.require_vkey(signer);
    }
    return std::monostate{};
  }
  const auto& partial = *aggregate.plutus_witness();
  const auto& script = partial.script();
  const auto language = script.script() ? script.script()->language() : std::uint8_t{1};
  witnesses.require_plutus_script(language, script.hash());
  if (script.script()) {
    auto status = witnesses.add_plutus_script(language, script.hash(), script.script()->bytes());
    if (!status) return status;
  }
  if (aggregate.datum()) {
    auto hash = hash_plutus_data(*aggregate.datum());
    if (!hash) return std::unexpected(hash.error());
    witnesses.require_datum(*hash);
    witnesses.add_datum(*hash, *aggregate.datum());
  }
  witnesses.require_redeemer(redeemer_key);
  witnesses.add_redeemer(redeemer_key, UntaggedRedeemer{partial.data(), std::nullopt});
  redeemers.add(std::move(redeemer_key), UntaggedRedeemer{partial.data(), std::nullopt});
  return std::monostate{};
}

}  // namespace

NativeScriptWitnessInfo::NativeScriptWitnessInfo(NativeScriptWitnessMode mode, std::size_t count,
                                                 std::vector<crypto::Ed25519KeyHash> hashes)
    : mode_(mode), count_(count), hashes_(std::move(hashes)) {}
NativeScriptWitnessInfo NativeScriptWitnessInfo::signature_count(std::size_t count) {
  return NativeScriptWitnessInfo(NativeScriptWitnessMode::signature_count, count, {});
}
NativeScriptWitnessInfo NativeScriptWitnessInfo::vkeys(std::vector<crypto::Ed25519KeyHash> hashes) {
  return NativeScriptWitnessInfo(NativeScriptWitnessMode::vkeys, 0, std::move(hashes));
}
NativeScriptWitnessInfo NativeScriptWitnessInfo::assume_signature_count() {
  return NativeScriptWitnessInfo(NativeScriptWitnessMode::assume_signature_count, 0, {});
}
NativeScriptWitnessMode NativeScriptWitnessInfo::mode() const noexcept { return mode_; }
std::size_t NativeScriptWitnessInfo::count() const noexcept { return count_; }
const std::vector<crypto::Ed25519KeyHash>& NativeScriptWitnessInfo::hashes() const noexcept {
  return hashes_;
}

PlutusScript::PlutusScript(std::uint8_t language, core::Bytes bytes)
    : language_(language), bytes_(std::move(bytes)) {}
Result<PlutusScript> PlutusScript::create(std::uint8_t language, core::ByteSpan bytes) {
  if (language < 1 || language > 3) {
    return std::unexpected(argument_error("Plutus language must be 1, 2, or 3"));
  }
  return PlutusScript(language, core::copy_bytes(bytes));
}
std::uint8_t PlutusScript::language() const noexcept { return language_; }
const core::Bytes& PlutusScript::bytes() const noexcept { return bytes_; }
crypto::ScriptHash PlutusScript::hash() const { return hash_script(language_, bytes_); }
CborValue PlutusScript::to_script() const {
  return CborValue::array({
      uint(language_),
      CborValue::byte_string(bytes_),
  });
}

PlutusScriptWitness::PlutusScriptWitness(crypto::ScriptHash hash,
                                         std::optional<PlutusScript> script)
    : hash_(std::move(hash)), script_(std::move(script)) {}
PlutusScriptWitness PlutusScriptWitness::reference(crypto::ScriptHash hash) {
  return PlutusScriptWitness(std::move(hash), std::nullopt);
}
PlutusScriptWitness PlutusScriptWitness::inline_script(PlutusScript script) {
  auto hash = script.hash();
  return PlutusScriptWitness(std::move(hash), std::move(script));
}
const crypto::ScriptHash& PlutusScriptWitness::hash() const noexcept { return hash_; }
const std::optional<PlutusScript>& PlutusScriptWitness::script() const noexcept { return script_; }

PartialPlutusWitness::PartialPlutusWitness(PlutusScriptWitness script, CborValue data)
    : script_(std::move(script)), data_(std::move(data)) {}
const PlutusScriptWitness& PartialPlutusWitness::script() const noexcept { return script_; }
const CborValue& PartialPlutusWitness::data() const noexcept { return data_; }

InputAggregateWitnessData::InputAggregateWitnessData(
    Kind kind, crypto::ScriptHash script_hash, std::optional<CborValue> native_script,
    std::optional<NativeScriptWitnessInfo> native_info,
    std::optional<PartialPlutusWitness> plutus_witness, std::vector<crypto::Ed25519KeyHash> signers,
    std::optional<CborValue> datum)
    : kind_(kind),
      script_hash_(std::move(script_hash)),
      native_script_(std::move(native_script)),
      native_info_(std::move(native_info)),
      plutus_witness_(std::move(plutus_witness)),
      signers_(std::move(signers)),
      datum_(std::move(datum)) {}
InputAggregateWitnessData InputAggregateWitnessData::native(crypto::ScriptHash hash,
                                                            CborValue script,
                                                            NativeScriptWitnessInfo info) {
  return InputAggregateWitnessData(Kind::native, std::move(hash), std::move(script),
                                   std::move(info), std::nullopt, {}, std::nullopt);
}
InputAggregateWitnessData InputAggregateWitnessData::plutus(
    PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers,
    std::optional<CborValue> datum) {
  auto hash = witness.script().hash();
  return InputAggregateWitnessData(Kind::plutus, std::move(hash), std::nullopt, std::nullopt,
                                   std::move(witness), std::move(signers), std::move(datum));
}
InputAggregateWitnessData::Kind InputAggregateWitnessData::kind() const noexcept { return kind_; }
const crypto::ScriptHash& InputAggregateWitnessData::script_hash() const noexcept {
  return script_hash_;
}
const std::optional<CborValue>& InputAggregateWitnessData::native_script() const noexcept {
  return native_script_;
}
const std::optional<NativeScriptWitnessInfo>& InputAggregateWitnessData::native_info()
    const noexcept {
  return native_info_;
}
const std::optional<PartialPlutusWitness>& InputAggregateWitnessData::plutus_witness()
    const noexcept {
  return plutus_witness_;
}
const std::vector<crypto::Ed25519KeyHash>& InputAggregateWitnessData::signers() const noexcept {
  return signers_;
}
const std::optional<CborValue>& InputAggregateWitnessData::datum() const noexcept { return datum_; }

AssetName::AssetName(core::Bytes bytes) : bytes_(std::move(bytes)) {}
Result<AssetName> AssetName::from_bytes(core::ByteSpan bytes) {
  if (bytes.size() > 32) {
    return std::unexpected(CardanoError(ErrorCode::out_of_range, "asset name exceeds 32 bytes"));
  }
  return AssetName(core::copy_bytes(bytes));
}
Result<AssetName> AssetName::from_hex(std::string_view hex) {
  auto bytes = core::hex_to_bytes(hex);
  if (!bytes) return std::unexpected(bytes.error());
  return from_bytes(*bytes);
}
const core::Bytes& AssetName::bytes() const noexcept { return bytes_; }
std::string AssetName::hex() const { return core::bytes_to_hex(bytes_); }

Value::Value(std::uint64_t coin) : coin_(coin) {}
Value::Value(std::uint64_t coin, std::vector<PolicyAssets> multiasset)
    : coin_(coin), multiasset_(std::move(multiasset)) {}
std::uint64_t Value::coin() const noexcept { return coin_; }
void Value::set_coin(std::uint64_t coin) noexcept { coin_ = coin; }
const std::vector<PolicyAssets>& Value::multiasset() const noexcept { return multiasset_; }
bool Value::has_assets() const noexcept {
  for (const auto& policy : multiasset_) {
    if (!policy.assets.empty()) return true;
  }
  return false;
}
std::int64_t Value::asset_quantity(const crypto::ScriptHash& policy,
                                   const AssetName& asset) const noexcept {
  for (const auto& policy_value : multiasset_) {
    if (policy_value.policy != policy) continue;
    for (const auto& asset_value : policy_value.assets) {
      if (asset_value.name == asset) return asset_value.quantity;
    }
  }
  return 0;
}
Result<Value> Value::checked_add(const Value& other) const {
  if (other.coin_ > std::numeric_limits<std::uint64_t>::max() - coin_) {
    return std::unexpected(CardanoError(ErrorCode::out_of_range, "coin overflow"));
  }
  Value output(coin_ + other.coin_, multiasset_);
  for (const auto& policy : other.multiasset_) {
    for (const auto& asset : policy.assets) {
      auto next = add_asset_delta(output, policy.policy, asset.name, asset.quantity);
      if (!next) return std::unexpected(next.error());
      output = std::move(*next);
    }
  }
  return output;
}
Result<Value> Value::checked_sub(const Value& other) const {
  if (coin_ < other.coin_) {
    return std::unexpected(balance_error("insufficient coin"));
  }
  Value output(coin_ - other.coin_, multiasset_);
  for (const auto& policy : other.multiasset_) {
    for (const auto& asset : policy.assets) {
      const auto current = output.asset_quantity(policy.policy, asset.name);
      if (asset.quantity < 0 || current < asset.quantity) {
        return std::unexpected(balance_error("insufficient native asset"));
      }
      auto next = add_asset_delta(output, policy.policy, asset.name, -asset.quantity);
      if (!next) return std::unexpected(next.error());
      output = std::move(*next);
    }
  }
  return output;
}
bool Value::covers(const Value& other) const noexcept {
  if (coin_ < other.coin_) return false;
  for (const auto& policy : other.multiasset_) {
    for (const auto& asset : policy.assets) {
      if (asset.quantity > 0 && asset_quantity(policy.policy, asset.name) < asset.quantity) {
        return false;
      }
    }
  }
  return true;
}
CborValue Value::to_cbor_value(bool signed_assets) const {
  if (!has_assets()) return uint(coin_);
  auto policies = multiasset_;
  std::ranges::sort(policies, {}, [](const auto& policy) { return policy.policy.to_hex(); });
  std::vector<std::pair<CborValue, CborValue>> policy_entries;
  for (auto& policy : policies) {
    std::ranges::sort(policy.assets, {}, [](const auto& asset) { return asset.name.hex(); });
    std::vector<std::pair<CborValue, CborValue>> asset_entries;
    for (const auto& asset : policy.assets) {
      if (asset.quantity == 0) continue;
      asset_entries.emplace_back(CborValue::byte_string(asset.name.bytes()),
                                 signed_assets ? integer(asset.quantity)
                                               : uint(static_cast<std::uint64_t>(asset.quantity)));
    }
    if (!asset_entries.empty()) {
      policy_entries.emplace_back(CborValue::byte_string(policy.policy.to_bytes()),
                                  CborValue::map(std::move(asset_entries)));
    }
  }
  return CborValue::array({
      uint(coin_),
      CborValue::map(std::move(policy_entries)),
  });
}
Result<core::Bytes> Value::to_cbor() const {
  return core::cbor::encode_cbor(to_cbor_value(), {.mode = core::cbor::Mode::canonical});
}

std::string Value::to_json() const {
  nlohmann::json assets = nlohmann::json::object();
  for (const auto& policy : multiasset_) {
    nlohmann::json quantities = nlohmann::json::object();
    for (const auto& asset : policy.assets) {
      quantities[asset.name.hex()] = static_cast<double>(asset.quantity);
    }
    assets[policy.policy.to_hex()] = std::move(quantities);
  }
  return nlohmann::json{
      {"coin", static_cast<double>(coin_)},
      {"multiasset", std::move(assets)},
  }
      .dump();
}

TransactionInput::TransactionInput(crypto::TransactionHash transaction_id, std::uint64_t index)
    : transaction_id_(std::move(transaction_id)), index_(index) {}

Result<TransactionInput> TransactionInput::from_json(std::string_view json) {
  try {
    const auto parsed = nlohmann::json::parse(json);
    if (!parsed.is_object() || parsed.size() != 2U || !parsed.contains("transaction_id") ||
        !parsed.at("transaction_id").is_string() || !parsed.contains("index") ||
        !parsed.at("index").is_number()) {
      return std::unexpected(argument_error("transaction input JSON has invalid fields"));
    }
    const auto index_number = parsed.at("index").get<double>();
    if (!std::isfinite(index_number) || index_number < 0.0 ||
        index_number > 9'007'199'254'740'991.0 || std::trunc(index_number) != index_number) {
      return std::unexpected(argument_error("transaction input JSON index is not a safe integer"));
    }
    const auto index = static_cast<std::uint64_t>(index_number);
    auto transaction_id =
        crypto::TransactionHash::from_hex(parsed.at("transaction_id").get<std::string>());
    if (!transaction_id) return std::unexpected(transaction_id.error());
    return TransactionInput(*transaction_id, index);
  } catch (const nlohmann::json::exception& error) {
    return std::unexpected(
        argument_error(std::string("invalid transaction input JSON: ") + error.what()));
  }
}
const crypto::TransactionHash& TransactionInput::transaction_id() const noexcept {
  return transaction_id_;
}
std::uint64_t TransactionInput::index() const noexcept { return index_; }
CborValue TransactionInput::to_cbor_value() const {
  return CborValue::array({
      CborValue::byte_string(transaction_id_.to_bytes()),
      uint(index_),
  });
}
std::string TransactionInput::canonical_identity() const {
  const auto bytes =
      core::cbor::encode_cbor(to_cbor_value(), {.mode = core::cbor::Mode::canonical});
  return bytes ? core::bytes_to_hex(*bytes) : std::string{};
}

std::string TransactionInput::to_json() const {
  return nlohmann::json{
      {"transaction_id", transaction_id_.to_hex()},
      {"index", static_cast<double>(index_)},
  }
      .dump();
}

TransactionOutput::TransactionOutput(Address address, Value amount)
    : address_(std::move(address)), amount_(std::move(amount)) {}
void TransactionOutput::set_datum_option(CborValue value) {
  datum_option_ = std::move(value);
  communication_datum_.reset();
}
Result<std::monostate> TransactionOutput::set_communication_datum(const CborValue& datum) {
  auto hash = hash_plutus_data(datum);
  if (!hash) return std::unexpected(hash.error());
  datum_option_ = CborValue::array({
      uint(0),
      CborValue::byte_string(hash->to_bytes()),
  });
  communication_datum_ = datum;
  return std::monostate{};
}
void TransactionOutput::set_script_reference(CborValue value) {
  script_reference_ = std::move(value);
}
const Address& TransactionOutput::address() const noexcept { return address_; }
const Value& TransactionOutput::amount() const noexcept { return amount_; }
Value& TransactionOutput::amount() noexcept { return amount_; }
const std::optional<CborValue>& TransactionOutput::datum_option() const noexcept {
  return datum_option_;
}
const std::optional<CborValue>& TransactionOutput::script_reference() const noexcept {
  return script_reference_;
}
const std::optional<CborValue>& TransactionOutput::communication_datum() const noexcept {
  return communication_datum_;
}
CborValue TransactionOutput::to_cbor_value() const {
  std::vector<std::pair<CborValue, CborValue>> entries{
      {uint(0), CborValue::byte_string(address_.to_bytes())},
      {uint(1), amount_.to_cbor_value()},
  };
  if (datum_option_) entries.emplace_back(uint(2), *datum_option_);
  if (script_reference_) entries.emplace_back(uint(3), *script_reference_);
  return CborValue::map(std::move(entries));
}

#define CARDANO_CONFIG_SETTER(name, field, bit, type)                                  \
  TransactionBuilderConfigBuilder& TransactionBuilderConfigBuilder::name(type value) { \
    config_.field = std::move(value);                                                  \
    required_mask_ |= static_cast<std::uint16_t>(1U << bit);                           \
    return *this;                                                                      \
  }

CARDANO_CONFIG_SETTER(linear_fee, linear_fee, 0, LinearFee)
CARDANO_CONFIG_SETTER(reference_script_cost_per_byte, reference_script_cost_per_byte, 1,
                      std::uint64_t)
CARDANO_CONFIG_SETTER(pool_deposit, pool_deposit, 2, std::uint64_t)
CARDANO_CONFIG_SETTER(key_deposit, key_deposit, 3, std::uint64_t)
CARDANO_CONFIG_SETTER(max_value_size, max_value_size, 4, std::uint32_t)
CARDANO_CONFIG_SETTER(max_transaction_size, max_transaction_size, 5, std::uint32_t)
CARDANO_CONFIG_SETTER(coins_per_utxo_byte, coins_per_utxo_byte, 6, std::uint64_t)
CARDANO_CONFIG_SETTER(ex_unit_prices, ex_unit_prices, 7, ExUnitPrices)
CARDANO_CONFIG_SETTER(collateral_percentage, collateral_percentage, 8, std::uint32_t)
CARDANO_CONFIG_SETTER(max_collateral_inputs, max_collateral_inputs, 9, std::uint32_t)

#undef CARDANO_CONFIG_SETTER

TransactionBuilderConfigBuilder& TransactionBuilderConfigBuilder::cost_models(CborValue value) {
  config_.cost_models = std::move(value);
  return *this;
}
TransactionBuilderConfigBuilder& TransactionBuilderConfigBuilder::prefer_pure_change(bool value) {
  config_.prefer_pure_change = value;
  return *this;
}
Result<TransactionBuilderConfig> TransactionBuilderConfigBuilder::build() const {
  constexpr std::uint16_t all_required = (1U << 10U) - 1U;
  if (required_mask_ != all_required) {
    return std::unexpected(argument_error("transaction builder configuration is incomplete"));
  }
  if (config_.ex_unit_prices.memory_denominator == 0 ||
      config_.ex_unit_prices.steps_denominator == 0 || config_.max_value_size == 0 ||
      config_.max_transaction_size == 0 || config_.coins_per_utxo_byte == 0) {
    return std::unexpected(
        argument_error("transaction builder configuration has invalid zero bounds"));
  }
  return config_;
}

TransactionOutputBuilder& TransactionOutputBuilder::with_address(Address address) {
  address_ = std::move(address);
  return *this;
}
TransactionOutputBuilder& TransactionOutputBuilder::with_data(CborValue datum) {
  datum_ = std::move(datum);
  communication_datum_.reset();
  return *this;
}
TransactionOutputBuilder& TransactionOutputBuilder::with_communication_data(CborValue datum) {
  auto hash = hash_plutus_data(datum);
  if (hash) {
    datum_ = CborValue::array({
        uint(0),
        CborValue::byte_string(hash->to_bytes()),
    });
    communication_datum_ = std::move(datum);
  }
  return *this;
}
TransactionOutputBuilder& TransactionOutputBuilder::with_reference_script(CborValue script) {
  script_reference_ = std::move(script);
  return *this;
}
Result<TransactionOutputAmountBuilder> TransactionOutputBuilder::next() const {
  if (!address_) {
    return std::unexpected(argument_error("transaction output address is missing"));
  }
  return TransactionOutputAmountBuilder(*address_, datum_, script_reference_, communication_datum_);
}

TxRedeemerBuilder::TxRedeemerBuilder(RedeemerWitnessKey key, CborValue data)
    : key_(std::move(key)), redeemer_{std::move(data), std::nullopt} {}
TxRedeemerBuilder& TxRedeemerBuilder::ex_units(ExUnits value) {
  redeemer_.ex_units = value;
  return *this;
}
RedeemerWitnessKey TxRedeemerBuilder::key() const { return key_; }
UntaggedRedeemer TxRedeemerBuilder::build() const { return redeemer_; }

void RedeemerSetBuilder::add(RedeemerWitnessKey key, UntaggedRedeemer redeemer) {
  const auto existing = std::find_if(redeemers_.begin(), redeemers_.end(),
                                     [&](const auto& item) { return item.first == key; });
  if (existing != redeemers_.end()) {
    existing->second = std::move(redeemer);
  } else {
    redeemers_.emplace_back(std::move(key), std::move(redeemer));
  }
}
void RedeemerSetBuilder::set_ex_units(RedeemerPurpose purpose, std::uint64_t final_index,
                                      ExUnits ex_units) {
  overrides_[std::to_string(static_cast<unsigned>(purpose)) + ":" + std::to_string(final_index)] =
      ex_units;
}
Result<CborValue> RedeemerSetBuilder::build(bool dummy_ex_units) const {
  auto sorted = redeemers_;
  std::stable_sort(sorted.begin(), sorted.end(), [](const auto& left, const auto& right) {
    if (left.first.purpose != right.first.purpose) {
      return left.first.purpose < right.first.purpose;
    }
    return left.first.sort_key < right.first.sort_key;
  });
  std::vector<CborValue> values;
  std::optional<RedeemerPurpose> previous;
  std::uint64_t index = 0;
  for (const auto& [key, redeemer] : sorted) {
    if (!previous || *previous != key.purpose) {
      previous = key.purpose;
      index = 0;
    }
    const auto identity =
        std::to_string(static_cast<unsigned>(key.purpose)) + ":" + std::to_string(index);
    std::optional<ExUnits> units;
    if (const auto override = overrides_.find(identity); override != overrides_.end()) {
      units = override->second;
    } else {
      units = redeemer.ex_units;
    }
    if (!units && !dummy_ex_units) {
      return std::unexpected(CardanoError(
          ErrorCode::evaluation, "redeemer is missing ExUnits for final index " + identity));
    }
    const auto effective = units.value_or(ExUnits{});
    if (effective.memory < 0 || effective.steps < 0) {
      return std::unexpected(argument_error("redeemer ExUnits cannot be negative"));
    }
    values.push_back(CborValue::array({
        uint(static_cast<std::uint8_t>(key.purpose)),
        uint(index),
        redeemer.data,
        CborValue::array({
            uint(static_cast<std::uint64_t>(effective.memory)),
            uint(static_cast<std::uint64_t>(effective.steps)),
        }),
    }));
    ++index;
  }
  return CborValue::array(std::move(values));
}
bool RedeemerSetBuilder::empty() const noexcept { return redeemers_.empty(); }

bool RequiredWitnessSet::empty() const noexcept {
  return vkeys.empty() && bootstraps.empty() && native_scripts.empty() &&
         plutus_v1_scripts.empty() && plutus_v2_scripts.empty() && plutus_v3_scripts.empty() &&
         datums.empty() && redeemers.empty();
}
void RequiredWitnessSet::merge(const RequiredWitnessSet& other) {
  vkeys.insert(other.vkeys.begin(), other.vkeys.end());
  bootstraps.insert(other.bootstraps.begin(), other.bootstraps.end());
  native_scripts.insert(other.native_scripts.begin(), other.native_scripts.end());
  plutus_v1_scripts.insert(other.plutus_v1_scripts.begin(), other.plutus_v1_scripts.end());
  plutus_v2_scripts.insert(other.plutus_v2_scripts.begin(), other.plutus_v2_scripts.end());
  plutus_v3_scripts.insert(other.plutus_v3_scripts.begin(), other.plutus_v3_scripts.end());
  datums.insert(other.datums.begin(), other.datums.end());
  redeemers.insert(other.redeemers.begin(), other.redeemers.end());
  script_references.insert(other.script_references.begin(), other.script_references.end());
}

SingleInputBuilder::SingleInputBuilder(TransactionUnspentOutput utxo) : utxo_(std::move(utxo)) {}
SingleInputBuilder::SingleInputBuilder(TransactionInput input, TransactionOutput output)
    : utxo_{std::move(input), std::move(output)} {}

Result<InputBuilderResult> SingleInputBuilder::payment_key() const {
  auto requirement = address_requirement(utxo_.output.address(), false);
  if (!requirement) return std::unexpected(requirement.error());
  if (requirement->script) {
    return std::unexpected(argument_error("UTxO payment credential is a script"));
  }
  return InputBuilderResult{
      .utxo = utxo_,
      .required_vkey = requirement->key,
      .aggregate = std::nullopt,
  };
}

Result<InputBuilderResult> SingleInputBuilder::native_script(crypto::ScriptHash hash,
                                                             CborValue script,
                                                             NativeScriptWitnessInfo info) const {
  auto requirement = address_requirement(utxo_.output.address(), false);
  if (!requirement) return std::unexpected(requirement.error());
  if (!requirement->script || *requirement->script != hash) {
    return std::unexpected(
        argument_error("native script does not satisfy the input payment credential"));
  }
  auto aggregate =
      InputAggregateWitnessData::native(std::move(hash), std::move(script), std::move(info));
  return InputBuilderResult{
      .utxo = utxo_,
      .required_vkey = std::nullopt,
      .aggregate = std::move(aggregate),
  };
}

Result<InputBuilderResult> SingleInputBuilder::plutus_script(
    PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers,
    std::optional<CborValue> datum) const {
  auto requirement = address_requirement(utxo_.output.address(), false);
  if (!requirement) return std::unexpected(requirement.error());
  if (!requirement->script || *requirement->script != witness.script().hash()) {
    return std::unexpected(
        argument_error("Plutus script does not satisfy the input payment credential"));
  }
  const auto& option = utxo_.output.datum_option();
  if (datum) {
    if (!option || option->as_array() == nullptr || option->as_array()->values.size() != 2 ||
        option->as_array()->values[0].as_unsigned() == nullptr ||
        option->as_array()->values[0].as_unsigned()->value != BigInteger(std::uint64_t{0}) ||
        option->as_array()->values[1].as_byte_string() == nullptr) {
      return std::unexpected(argument_error("external datum requires a datum-hash option"));
    }
    auto hash = hash_plutus_data(*datum);
    if (!hash || hash->to_bytes() != option->as_array()->values[1].as_byte_string()->value) {
      return std::unexpected(hash ? argument_error("Plutus datum does not satisfy the input")
                                  : hash.error());
    }
  } else if (option && option->as_array() != nullptr && option->as_array()->values.size() == 2 &&
             option->as_array()->values[0].as_unsigned() != nullptr &&
             option->as_array()->values[0].as_unsigned()->value == BigInteger(std::uint64_t{0})) {
    return std::unexpected(argument_error("datum-hash input requires an external datum"));
  }
  auto aggregate =
      InputAggregateWitnessData::plutus(std::move(witness), std::move(signers), std::move(datum));
  return InputBuilderResult{
      .utxo = utxo_,
      .required_vkey = std::nullopt,
      .aggregate = std::move(aggregate),
  };
}

SingleMintBuilder::SingleMintBuilder(std::vector<AssetQuantity> assets)
    : assets_(std::move(assets)) {}
Result<SingleMintBuilder> SingleMintBuilder::create(std::vector<AssetQuantity> assets) {
  if (assets.empty() ||
      std::ranges::any_of(assets, [](const auto& item) { return item.quantity == 0; })) {
    return std::unexpected(argument_error("mint assets must be nonempty and nonzero"));
  }
  std::set<std::string> identities;
  for (const auto& asset : assets) {
    if (!identities.insert(asset.name.hex()).second) {
      return std::unexpected(CardanoError(ErrorCode::duplicate_key, "duplicate mint asset name"));
    }
  }
  return SingleMintBuilder(std::move(assets));
}
Result<MintBuilderResult> SingleMintBuilder::native_script(crypto::ScriptHash hash,
                                                           CborValue script,
                                                           NativeScriptWitnessInfo info) const {
  auto aggregate = InputAggregateWitnessData::native(hash, std::move(script), std::move(info));
  RequiredWitnessSet required;
  add_script_requirement(required, aggregate);
  return MintBuilderResult{
      .mint = PolicyAssets{std::move(hash), assets_},
      .aggregate = std::move(aggregate),
      .required = std::move(required),
  };
}
Result<MintBuilderResult> SingleMintBuilder::plutus_script(
    PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers) const {
  auto hash = witness.script().hash();
  auto aggregate = InputAggregateWitnessData::plutus(std::move(witness), std::move(signers));
  RequiredWitnessSet required;
  add_script_requirement(required, aggregate);
  return MintBuilderResult{
      .mint = PolicyAssets{std::move(hash), assets_},
      .aggregate = std::move(aggregate),
      .required = std::move(required),
  };
}

SingleWithdrawalBuilder::SingleWithdrawalBuilder(Address reward_address, std::uint64_t amount)
    : reward_address_(std::move(reward_address)), amount_(amount) {}
Result<WithdrawalBuilderResult> SingleWithdrawalBuilder::payment_key() const {
  auto requirement = address_requirement(reward_address_, true);
  if (!requirement) return std::unexpected(requirement.error());
  if (requirement->script) {
    return std::unexpected(argument_error("withdrawal credential is a script"));
  }
  RequiredWitnessSet required;
  if (requirement->key) required.vkeys.insert(requirement->key->to_hex());
  return WithdrawalBuilderResult{reward_address_, amount_, std::nullopt, std::move(required)};
}
Result<WithdrawalBuilderResult> SingleWithdrawalBuilder::native_script(
    crypto::ScriptHash hash, CborValue script, NativeScriptWitnessInfo info) const {
  auto requirement = address_requirement(reward_address_, true);
  if (!requirement) return std::unexpected(requirement.error());
  if (!requirement->script || *requirement->script != hash) {
    return std::unexpected(argument_error("native script does not satisfy withdrawal"));
  }
  auto aggregate = InputAggregateWitnessData::native(hash, std::move(script), std::move(info));
  RequiredWitnessSet required;
  add_script_requirement(required, aggregate);
  return WithdrawalBuilderResult{reward_address_, amount_, std::move(aggregate),
                                 std::move(required)};
}
Result<WithdrawalBuilderResult> SingleWithdrawalBuilder::plutus_script(
    PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers) const {
  auto requirement = address_requirement(reward_address_, true);
  if (!requirement) return std::unexpected(requirement.error());
  if (!requirement->script || *requirement->script != witness.script().hash()) {
    return std::unexpected(argument_error("Plutus script does not satisfy withdrawal"));
  }
  auto aggregate = InputAggregateWitnessData::plutus(std::move(witness), std::move(signers));
  RequiredWitnessSet required;
  add_script_requirement(required, aggregate);
  return WithdrawalBuilderResult{reward_address_, amount_, std::move(aggregate),
                                 std::move(required)};
}

SingleCertificateBuilder::SingleCertificateBuilder(CborValue certificate)
    : certificate_(std::move(certificate)) {}
Result<CertificateBuilderResult> SingleCertificateBuilder::skip_witness() const {
  auto required = certificate_requirements(certificate_);
  return required ? Result<CertificateBuilderResult>(
                        CertificateBuilderResult{certificate_, std::nullopt, *required})
                  : std::unexpected(required.error());
}
Result<CertificateBuilderResult> SingleCertificateBuilder::payment_key() const {
  auto required = certificate_requirements(certificate_);
  if (!required) return std::unexpected(required.error());
  if (!required->native_scripts.empty()) {
    return std::unexpected(argument_error("certificate requires a script"));
  }
  return CertificateBuilderResult{certificate_, std::nullopt, *required};
}
Result<CertificateBuilderResult> SingleCertificateBuilder::native_script(
    crypto::ScriptHash hash, CborValue script, NativeScriptWitnessInfo info) const {
  auto required = certificate_requirements(certificate_);
  if (!required) return std::unexpected(required.error());
  if (!required->native_scripts.empty() && !required->native_scripts.contains(hash.to_hex())) {
    return std::unexpected(argument_error("native script does not satisfy certificate"));
  }
  auto aggregate = InputAggregateWitnessData::native(hash, std::move(script), std::move(info));
  add_script_requirement(*required, aggregate);
  return CertificateBuilderResult{certificate_, std::move(aggregate), std::move(*required)};
}
Result<CertificateBuilderResult> SingleCertificateBuilder::plutus_script(
    PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers) const {
  auto required = certificate_requirements(certificate_);
  if (!required) return std::unexpected(required.error());
  const auto identity = witness.script().hash().to_hex();
  if (!required->native_scripts.empty() && !required->native_scripts.contains(identity)) {
    return std::unexpected(argument_error("Plutus script does not satisfy certificate"));
  }
  required->native_scripts.erase(identity);
  auto aggregate = InputAggregateWitnessData::plutus(std::move(witness), std::move(signers));
  add_script_requirement(*required, aggregate);
  return CertificateBuilderResult{certificate_, std::move(aggregate), std::move(*required)};
}

ProposalBuilder& ProposalBuilder::add(CborValue proposal) {
  entries_.push_back({std::move(proposal), std::nullopt});
  return *this;
}
ProposalBuilder& ProposalBuilder::add_native(CborValue proposal, crypto::ScriptHash hash,
                                             CborValue script, NativeScriptWitnessInfo info) {
  entries_.push_back({
      std::move(proposal),
      InputAggregateWitnessData::native(std::move(hash), std::move(script), std::move(info)),
  });
  return *this;
}
ProposalBuilder& ProposalBuilder::add_plutus(CborValue proposal, PartialPlutusWitness witness,
                                             std::vector<crypto::Ed25519KeyHash> signers,
                                             std::optional<CborValue> datum) {
  entries_.push_back({
      std::move(proposal),
      InputAggregateWitnessData::plutus(std::move(witness), std::move(signers), std::move(datum)),
  });
  return *this;
}
ProposalBuilderResult ProposalBuilder::build() const { return {entries_}; }

void VoteBuilder::insert(VoteBuilderEntry entry) {
  const auto identity = canonical_hex(entry.voter) + ":" + canonical_hex(entry.action);
  const auto existing = std::find_if(entries_.begin(), entries_.end(), [&](const auto& item) {
    return canonical_hex(item.voter) + ":" + canonical_hex(item.action) == identity;
  });
  if (existing == entries_.end()) {
    entries_.push_back(std::move(entry));
  } else {
    *existing = std::move(entry);
  }
}
VoteBuilder& VoteBuilder::add(CborValue voter, CborValue action, CborValue procedure) {
  insert({std::move(voter), std::move(action), std::move(procedure), std::nullopt});
  return *this;
}
VoteBuilder& VoteBuilder::add_native(CborValue voter, CborValue action, CborValue procedure,
                                     crypto::ScriptHash hash, CborValue script,
                                     NativeScriptWitnessInfo info) {
  insert({
      std::move(voter),
      std::move(action),
      std::move(procedure),
      InputAggregateWitnessData::native(std::move(hash), std::move(script), std::move(info)),
  });
  return *this;
}
VoteBuilder& VoteBuilder::add_plutus(CborValue voter, CborValue action, CborValue procedure,
                                     PartialPlutusWitness witness,
                                     std::vector<crypto::Ed25519KeyHash> signers,
                                     std::optional<CborValue> datum) {
  insert({
      std::move(voter),
      std::move(action),
      std::move(procedure),
      InputAggregateWitnessData::plutus(std::move(witness), std::move(signers), std::move(datum)),
  });
  return *this;
}
VoteBuilderResult VoteBuilder::build() const { return {entries_}; }

void TransactionWitnessSetBuilder::require_vkey(crypto::Ed25519KeyHash hash) {
  missing_.vkeys.insert(hash.to_hex());
}
void TransactionWitnessSetBuilder::require_bootstrap(core::ByteSpan public_key) {
  missing_.bootstraps.insert(core::bytes_to_hex(public_key));
}
void TransactionWitnessSetBuilder::require_native_script(crypto::ScriptHash hash) {
  const auto identity = hash.to_hex();
  if (!missing_.script_references.contains(identity)) {
    missing_.native_scripts.insert(std::move(identity));
  }
}
void TransactionWitnessSetBuilder::require_plutus_script(std::uint8_t language,
                                                         crypto::ScriptHash hash) {
  const auto identity = hash.to_hex();
  if (missing_.script_references.contains(identity)) return;
  switch (language) {
    case 1:
      missing_.plutus_v1_scripts.insert(identity);
      break;
    case 2:
      missing_.plutus_v2_scripts.insert(identity);
      break;
    case 3:
      missing_.plutus_v3_scripts.insert(identity);
      break;
    default:
      break;
  }
}
void TransactionWitnessSetBuilder::require_datum(crypto::DatumHash hash) {
  missing_.datums.insert(hash.to_hex());
}
void TransactionWitnessSetBuilder::require_redeemer(RedeemerWitnessKey key) {
  missing_.redeemers.insert(std::to_string(static_cast<unsigned>(key.purpose)) + ":" +
                            key.sort_key);
}
core::VoidResult TransactionWitnessSetBuilder::satisfy_script_reference(std::uint8_t language,
                                                                        crypto::ScriptHash hash) {
  if (language > 3) {
    return std::unexpected(argument_error("script language must be in 0..3"));
  }
  const auto identity = hash.to_hex();
  missing_.script_references.insert(identity);
  missing_.native_scripts.erase(identity);
  missing_.plutus_v1_scripts.erase(identity);
  missing_.plutus_v2_scripts.erase(identity);
  missing_.plutus_v3_scripts.erase(identity);
  std::erase_if(native_scripts_, [&](const auto& item) { return item.first == identity; });
  std::erase_if(plutus_v1_scripts_, [&](const auto& item) { return item.first == identity; });
  std::erase_if(plutus_v2_scripts_, [&](const auto& item) { return item.first == identity; });
  std::erase_if(plutus_v3_scripts_, [&](const auto& item) { return item.first == identity; });
  return std::monostate{};
}

core::VoidResult TransactionWitnessSetBuilder::add_vkey_witness(CborValue witness) {
  const auto* array = witness.as_array();
  if (array == nullptr || array->values.size() != 2 ||
      array->values[0].as_byte_string() == nullptr ||
      array->values[1].as_byte_string() == nullptr ||
      array->values[0].as_byte_string()->value.size() != 32 ||
      array->values[1].as_byte_string()->value.size() != 64) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_structure, "vkey witness must be [bytes32,bytes64]"));
  }
  auto public_key = crypto::PublicKey::from_bytes(array->values[0].as_byte_string()->value);
  if (!public_key) return std::unexpected(public_key.error());
  const auto identity = public_key->hash().to_hex();
  const auto existing = std::find_if(vkeys_.begin(), vkeys_.end(),
                                     [&](const auto& item) { return item.first == identity; });
  if (existing == vkeys_.end()) {
    vkeys_.emplace_back(identity, std::move(witness));
  } else {
    existing->second = std::move(witness);
  }
  missing_.vkeys.erase(identity);
  return std::monostate{};
}
void TransactionWitnessSetBuilder::add_bootstrap_witness(core::ByteSpan public_key,
                                                         CborValue witness) {
  const auto identity = core::bytes_to_hex(public_key);
  const auto existing = std::find_if(bootstraps_.begin(), bootstraps_.end(),
                                     [&](const auto& item) { return item.first == identity; });
  if (existing == bootstraps_.end()) {
    bootstraps_.emplace_back(identity, std::move(witness));
  } else {
    existing->second = std::move(witness);
  }
  missing_.bootstraps.erase(identity);
}
core::VoidResult TransactionWitnessSetBuilder::add_native_script(crypto::ScriptHash hash,
                                                                 CborValue script) {
  const auto identity = hash.to_hex();
  if (missing_.script_references.contains(identity)) return std::monostate{};
  auto encoded = core::cbor::encode_cbor(script);
  if (!encoded) return std::unexpected(encoded.error());
  if (hash_script(0, *encoded) != hash) {
    return std::unexpected(
        CardanoError(ErrorCode::integrity, "native script hash does not match script bytes"));
  }
  const auto existing = std::find_if(native_scripts_.begin(), native_scripts_.end(),
                                     [&](const auto& item) { return item.first == identity; });
  if (existing == native_scripts_.end()) {
    native_scripts_.emplace_back(identity, std::move(script));
  } else {
    existing->second = std::move(script);
  }
  missing_.native_scripts.erase(identity);
  return std::monostate{};
}
core::VoidResult TransactionWitnessSetBuilder::add_plutus_script(std::uint8_t language,
                                                                 crypto::ScriptHash hash,
                                                                 core::Bytes script) {
  auto* values = &plutus_v1_scripts_;
  auto* missing = &missing_.plutus_v1_scripts;
  if (language == 2) {
    values = &plutus_v2_scripts_;
    missing = &missing_.plutus_v2_scripts;
  } else if (language == 3) {
    values = &plutus_v3_scripts_;
    missing = &missing_.plutus_v3_scripts;
  } else if (language != 1) {
    return std::unexpected(argument_error("Plutus language must be 1, 2, or 3"));
  }
  if (hash_script(language, script) != hash) {
    return std::unexpected(
        CardanoError(ErrorCode::integrity, "Plutus script hash does not match script bytes"));
  }
  const auto identity = hash.to_hex();
  if (missing_.script_references.contains(identity)) return std::monostate{};
  const auto existing = std::find_if(values->begin(), values->end(),
                                     [&](const auto& item) { return item.first == identity; });
  if (existing == values->end()) {
    values->emplace_back(identity, std::move(script));
  } else {
    existing->second = std::move(script);
  }
  missing->erase(identity);
  return std::monostate{};
}
void TransactionWitnessSetBuilder::add_datum(crypto::DatumHash hash, CborValue datum) {
  const auto identity = hash.to_hex();
  const auto existing = std::find_if(datums_.begin(), datums_.end(),
                                     [&](const auto& item) { return item.first == identity; });
  if (existing == datums_.end()) {
    datums_.emplace_back(identity, std::move(datum));
  } else {
    existing->second = std::move(datum);
  }
  missing_.datums.erase(identity);
}
void TransactionWitnessSetBuilder::add_redeemer(RedeemerWitnessKey key, UntaggedRedeemer redeemer) {
  missing_.redeemers.erase(std::to_string(static_cast<unsigned>(key.purpose)) + ":" + key.sort_key);
  redeemers_.add(std::move(key), std::move(redeemer));
}
void TransactionWitnessSetBuilder::set_redeemer_ex_units(RedeemerPurpose purpose,
                                                         std::uint64_t final_index,
                                                         ExUnits ex_units) {
  redeemers_.set_ex_units(purpose, final_index, ex_units);
}

Result<CborValue> TransactionWitnessSetBuilder::build() const {
  if (!missing_.empty()) {
    return std::unexpected(
        CardanoError(ErrorCode::missing_witness, "required transaction witnesses are missing"));
  }
  return build_unchecked(false, false);
}
Result<CborValue> TransactionWitnessSetBuilder::build_unchecked(bool fake_vkeys,
                                                                bool dummy_ex_units) const {
  std::vector<std::pair<CborValue, CborValue>> fields;
  auto append_values = [&](std::uint64_t key, const auto& entries, auto convert) {
    std::vector<CborValue> values;
    for (const auto& entry : entries) values.push_back(convert(entry.second));
    if (!values.empty()) fields.emplace_back(uint(key), tagged_set(std::move(values)));
  };
  append_values(0, vkeys_, [](const CborValue& value) { return value; });
  if (fake_vkeys && !missing_.vkeys.empty()) {
    std::vector<CborValue> values;
    if (!fields.empty() &&
        fields.front().first.as_unsigned()->value == BigInteger(std::uint64_t{0})) {
      values = fields.front().second.as_tag()->value->as_array()->values;
      fields.erase(fields.begin());
    }
    std::uint8_t marker = 0;
    for (const auto& missing : missing_.vkeys) {
      static_cast<void>(missing);
      core::Bytes public_key(32);
      public_key[0] = static_cast<core::Byte>(marker++);
      values.push_back(CborValue::array({
          CborValue::byte_string(std::move(public_key)),
          CborValue::byte_string(core::Bytes(64)),
      }));
    }
    fields.insert(fields.begin(), {uint(0), tagged_set(std::move(values))});
  }
  append_values(1, native_scripts_, [](const CborValue& value) { return value; });
  append_values(2, bootstraps_, [](const CborValue& value) { return value; });
  append_values(3, plutus_v1_scripts_,
                [](const core::Bytes& value) { return CborValue::byte_string(value); });
  append_values(4, datums_, [](const CborValue& value) { return value; });
  if (!redeemers_.empty()) {
    auto redeemers = redeemers_.build(dummy_ex_units);
    if (!redeemers) return std::unexpected(redeemers.error());
    fields.emplace_back(uint(5), std::move(*redeemers));
  }
  append_values(6, plutus_v2_scripts_,
                [](const core::Bytes& value) { return CborValue::byte_string(value); });
  append_values(7, plutus_v3_scripts_,
                [](const core::Bytes& value) { return CborValue::byte_string(value); });
  std::ranges::sort(fields, {}, [](const auto& field) {
    return field.first.as_unsigned()->value.to_uint64().value();
  });
  return CborValue::map(std::move(fields));
}
const RequiredWitnessSet& TransactionWitnessSetBuilder::missing() const noexcept {
  return missing_;
}

SignedTxBuilder::SignedTxBuilder(CborValue body, TransactionWitnessSetBuilder witnesses,
                                 std::optional<CborValue> auxiliary_data)
    : body_(std::move(body)),
      witnesses_(std::move(witnesses)),
      auxiliary_data_(std::move(auxiliary_data)) {}
core::VoidResult SignedTxBuilder::sign(const crypto::PrivateKey& private_key) {
  auto body_hash = hash_transaction(body_);
  if (!body_hash) return std::unexpected(body_hash.error());
  auto witness = make_vkey_witness(*body_hash, private_key);
  if (!witness) return std::unexpected(witness.error());
  return witnesses_.add_vkey_witness(std::move(*witness));
}
Result<CborValue> SignedTxBuilder::build() const {
  auto witnesses = witnesses_.build();
  if (!witnesses) return std::unexpected(witnesses.error());
  return CborValue::array({
      body_,
      std::move(*witnesses),
      CborValue::boolean(true),
      auxiliary_data_.value_or(CborValue::null()),
  });
}
Result<CborValue> SignedTxBuilder::build_unchecked() const {
  auto witnesses = witnesses_.build_unchecked();
  if (!witnesses) return std::unexpected(witnesses.error());
  return CborValue::array({
      body_,
      std::move(*witnesses),
      CborValue::boolean(true),
      auxiliary_data_.value_or(CborValue::null()),
  });
}
Result<SingleOutputBuilderResult> TransactionOutputBuilder::next(Value amount) const {
  if (!address_) {
    return std::unexpected(argument_error("transaction output address is missing"));
  }
  TransactionOutput output(*address_, std::move(amount));
  if (datum_) output.set_datum_option(*datum_);
  if (script_reference_) output.set_script_reference(*script_reference_);
  if (communication_datum_) {
    auto status = output.set_communication_datum(*communication_datum_);
    if (!status) return std::unexpected(status.error());
  }
  return SingleOutputBuilderResult{std::move(output)};
}

TransactionOutputAmountBuilder::TransactionOutputAmountBuilder(
    Address address, std::optional<CborValue> datum, std::optional<CborValue> script_reference,
    std::optional<CborValue> communication_datum)
    : address_(std::move(address)),
      datum_(std::move(datum)),
      script_reference_(std::move(script_reference)),
      communication_datum_(std::move(communication_datum)) {}
TransactionOutputAmountBuilder& TransactionOutputAmountBuilder::with_value(Value value) {
  amount_ = std::move(value);
  return *this;
}
core::VoidResult TransactionOutputAmountBuilder::with_asset_and_min_required_coin(
    std::vector<PolicyAssets> assets, std::uint64_t coins_per_utxo_byte) {
  TransactionOutput output(address_, Value(0, assets));
  if (datum_) output.set_datum_option(*datum_);
  if (script_reference_) output.set_script_reference(*script_reference_);
  auto first = min_ada_required(output.to_cbor_value(), coins_per_utxo_byte);
  if (!first) return std::unexpected(first.error());
  output.amount().set_coin(*first);
  auto second = min_ada_required(output.to_cbor_value(), coins_per_utxo_byte);
  if (!second) return std::unexpected(second.error());
  amount_ = Value(*second, std::move(assets));
  return std::monostate{};
}
Result<SingleOutputBuilderResult> TransactionOutputAmountBuilder::build() const {
  if (!amount_) {
    return std::unexpected(argument_error("transaction output amount is missing"));
  }
  TransactionOutput output(address_, *amount_);
  if (datum_) output.set_datum_option(*datum_);
  if (script_reference_) output.set_script_reference(*script_reference_);
  if (communication_datum_) {
    auto status = output.set_communication_datum(*communication_datum_);
    if (!status) return std::unexpected(status.error());
  }
  return SingleOutputBuilderResult{std::move(output)};
}

TransactionBuilder::TransactionBuilder(TransactionBuilderConfig config)
    : config_(std::move(config)) {}
bool TransactionBuilder::contains_input(const TransactionInput& input) const {
  const auto identity = input.canonical_identity();
  return std::ranges::any_of(
      inputs_, [&](const auto& item) { return item.input.canonical_identity() == identity; });
}
core::VoidResult TransactionBuilder::add_input(TransactionUnspentOutput input) {
  if (contains_input(input.input)) {
    return std::unexpected(
        CardanoError(ErrorCode::duplicate_key, "duplicate explicit transaction input"));
  }
  inputs_.push_back(std::move(input));
  return std::monostate{};
}
core::VoidResult TransactionBuilder::add_input(InputBuilderResult input) {
  if (input.required_vkey) witnesses_.require_vkey(*input.required_vkey);
  if (input.aggregate) {
    auto status = apply_aggregate(
        witnesses_, redeemers_,
        RedeemerWitnessKey{RedeemerPurpose::spend, input.utxo.input.canonical_identity()},
        *input.aggregate);
    if (!status) return status;
  }
  return add_input(std::move(input.utxo));
}
void TransactionBuilder::add_utxo(TransactionUnspentOutput candidate) {
  const auto identity = candidate.input.canonical_identity();
  if (contains_input(candidate.input) || std::ranges::any_of(candidates_, [&](const auto& item) {
        return item.input.canonical_identity() == identity;
      })) {
    return;
  }
  candidates_.push_back(std::move(candidate));
}
core::VoidResult TransactionBuilder::add_output(TransactionOutput output) {
  auto value_bytes = output.amount().to_cbor();
  if (!value_bytes) return std::unexpected(value_bytes.error());
  if (value_bytes->size() > config_.max_value_size) {
    return std::unexpected(
        CardanoError(ErrorCode::out_of_range, "transaction output Value exceeds maximum size"));
  }
  auto minimum = min_ada_required(output.to_cbor_value(), config_.coins_per_utxo_byte);
  if (!minimum) return std::unexpected(minimum.error());
  if (output.amount().coin() < *minimum) {
    return std::unexpected(balance_error("transaction output coin is below minimum ADA"));
  }
  if (output.communication_datum()) {
    auto hash = hash_plutus_data(*output.communication_datum());
    if (!hash) return std::unexpected(hash.error());
    witnesses_.require_datum(*hash);
    witnesses_.add_datum(*hash, *output.communication_datum());
  }
  outputs_.push_back(std::move(output));
  return std::monostate{};
}
void TransactionBuilder::add_reference_input(TransactionInput input) {
  const auto identity = input.canonical_identity();
  if (!std::ranges::any_of(reference_inputs_, [&](const auto& existing) {
        return existing.canonical_identity() == identity;
      })) {
    reference_inputs_.push_back(std::move(input));
  }
}
core::VoidResult TransactionBuilder::add_reference_input(TransactionUnspentOutput input) {
  const auto identity = input.input.canonical_identity();
  if (std::ranges::any_of(reference_inputs_, [&](const auto& existing) {
        return existing.canonical_identity() == identity;
      })) {
    return std::monostate{};
  }
  if (input.output.script_reference()) {
    const auto* tag = input.output.script_reference()->as_tag();
    if (tag != nullptr && tag->value != nullptr && tag->value->as_byte_string() != nullptr) {
      const auto& embedded = tag->value->as_byte_string()->value;
      auto script = core::cbor::decode_cbor(embedded);
      if (!script) return std::unexpected(script.error());
      const auto* fields = script->as_array();
      if (fields == nullptr || fields->values.size() != 2 ||
          fields->values[0].as_unsigned() == nullptr) {
        return std::unexpected(
            argument_error("reference ScriptRef must contain a decodable Script"));
      }
      auto language = fields->values[0].as_unsigned()->value.to_uint64();
      if (!language || *language > 3) {
        return std::unexpected(argument_error("reference Script language must be in 0..3"));
      }
      std::optional<crypto::ScriptHash> hash;
      if (*language == 0) {
        auto native = core::cbor::encode_cbor(fields->values[1]);
        if (!native) return std::unexpected(native.error());
        hash.emplace(hash_script(0, *native));
      } else {
        const auto* bytes = fields->values[1].as_byte_string();
        if (bytes == nullptr) {
          return std::unexpected(argument_error("reference Plutus script must contain bytes"));
        }
        hash.emplace(hash_script(static_cast<std::uint8_t>(*language), bytes->value));
      }
      auto status = witnesses_.satisfy_script_reference(static_cast<std::uint8_t>(*language),
                                                        std::move(*hash));
      if (!status) return status;
    }
  }
  reference_inputs_.push_back(input.input);
  reference_utxos_.push_back(std::move(input));
  return std::monostate{};
}
core::VoidResult TransactionBuilder::set_mint(std::vector<PolicyAssets> mint) {
  if (mint.empty()) {
    return std::unexpected(argument_error("mint must be nonempty"));
  }
  auto normalized = normalize_assets(mint, true);
  if (!normalized) return std::unexpected(normalized.error());
  if (normalized->empty()) {
    return std::unexpected(argument_error("mint must be nonempty"));
  }
  mint_ = std::move(*normalized);
  return std::monostate{};
}
core::VoidResult TransactionBuilder::add_mint(MintBuilderResult mint) {
  auto combined = mint_;
  combined.push_back(mint.mint);
  auto normalized = normalize_assets(combined, true);
  if (!normalized) return std::unexpected(normalized.error());
  auto status = apply_aggregate(
      witnesses_, redeemers_, RedeemerWitnessKey{RedeemerPurpose::mint, mint.mint.policy.to_hex()},
      mint.aggregate);
  if (!status) return status;
  mint_ = std::move(*normalized);
  return std::monostate{};
}
core::VoidResult TransactionBuilder::add_withdrawal(Address reward_address, std::uint64_t coin) {
  if (reward_address.kind() != AddressKind::reward) {
    return std::unexpected(argument_error("withdrawal address must be a reward address"));
  }
  const auto identity = reward_address.to_hex();
  if (std::ranges::any_of(withdrawals_, [&](const auto& withdrawal) {
        return withdrawal.first.to_hex() == identity;
      })) {
    return std::unexpected(CardanoError(ErrorCode::duplicate_key, "duplicate withdrawal address"));
  }
  withdrawals_.emplace_back(std::move(reward_address), coin);
  return std::monostate{};
}
core::VoidResult TransactionBuilder::add_withdrawal(WithdrawalBuilderResult withdrawal) {
  if (withdrawal.aggregate) {
    auto status =
        apply_aggregate(witnesses_, redeemers_,
                        RedeemerWitnessKey{RedeemerPurpose::reward, withdrawal.address.to_hex()},
                        *withdrawal.aggregate);
    if (!status) return status;
  }
  for (const auto& key : withdrawal.required.vkeys) {
    auto hash = crypto::Ed25519KeyHash::from_hex(key);
    if (!hash) return std::unexpected(hash.error());
    witnesses_.require_vkey(*hash);
  }
  return add_withdrawal(std::move(withdrawal.address), withdrawal.amount);
}
void TransactionBuilder::add_certificate(CborValue value) {
  certificates_.push_back(std::move(value));
}
core::VoidResult TransactionBuilder::add_certificate(CertificateBuilderResult certificate) {
  const auto ordinal =
      std::count_if(certificates_.begin(), certificates_.end(), [](const auto&) { return true; });
  if (certificate.aggregate) {
    auto status = apply_aggregate(
        witnesses_, redeemers_,
        RedeemerWitnessKey{
            RedeemerPurpose::certificate,
            std::string(10 - std::min<std::size_t>(10, std::to_string(ordinal).size()), '0') +
                std::to_string(ordinal)},
        *certificate.aggregate);
    if (!status) return status;
  }
  for (const auto& key : certificate.required.vkeys) {
    auto hash = crypto::Ed25519KeyHash::from_hex(key);
    if (hash) witnesses_.require_vkey(*hash);
  }
  certificates_.push_back(std::move(certificate.certificate));
  return std::monostate{};
}
void TransactionBuilder::add_proposal(CborValue value) { proposals_.push_back(std::move(value)); }
core::VoidResult TransactionBuilder::add_proposals(ProposalBuilderResult proposals) {
  for (std::size_t index = 0; index < proposals.entries.size(); ++index) {
    auto& entry = proposals.entries[index];
    if (entry.aggregate) {
      const auto decimal = std::to_string(index);
      auto status = apply_aggregate(
          witnesses_, redeemers_,
          RedeemerWitnessKey{
              RedeemerPurpose::proposing,
              std::string(10 - std::min<std::size_t>(10, decimal.size()), '0') + decimal},
          *entry.aggregate);
      if (!status) return status;
    }
    proposals_.push_back(std::move(entry.proposal));
  }
  return std::monostate{};
}
core::VoidResult TransactionBuilder::add_votes(VoteBuilderResult votes) {
  for (auto& entry : votes.entries) {
    if (entry.aggregate) {
      auto status = apply_aggregate(
          witnesses_, redeemers_,
          RedeemerWitnessKey{RedeemerPurpose::voting,
                             canonical_hex(entry.voter) + ":" + canonical_hex(entry.action)},
          *entry.aggregate);
      if (!status) return status;
    }
    votes_.push_back(std::move(entry));
  }
  return std::monostate{};
}
core::VoidResult TransactionBuilder::add_collateral(InputBuilderResult collateral) {
  if (collateral.aggregate) {
    return std::unexpected(
        argument_error("collateral cannot require native or Plutus script witnesses"));
  }
  if (collateral_.size() >= config_.max_collateral_inputs) {
    return std::unexpected(
        CardanoError(ErrorCode::out_of_range, "maximum collateral inputs exceeded"));
  }
  if (collateral.required_vkey) {
    witnesses_.require_vkey(*collateral.required_vkey);
  }
  collateral_.push_back(std::move(collateral));
  return std::monostate{};
}
void TransactionBuilder::set_collateral_return(TransactionOutput output) {
  collateral_return_ = std::move(output);
}
void TransactionBuilder::add_required_signer(crypto::Ed25519KeyHash signer) {
  const auto identity = signer.to_hex();
  if (!std::ranges::any_of(required_signers_,
                           [&](const auto& existing) { return existing.to_hex() == identity; })) {
    witnesses_.require_vkey(signer);
    required_signers_.push_back(std::move(signer));
  }
}
void TransactionBuilder::set_current_treasury_value(std::uint64_t value) {
  current_treasury_value_ = value;
}
void TransactionBuilder::set_fee(std::uint64_t value) { fee_ = value; }
void TransactionBuilder::set_redeemer_ex_units(RedeemerPurpose purpose, std::uint64_t final_index,
                                               ExUnits ex_units) {
  redeemers_.set_ex_units(purpose, final_index, ex_units);
  witnesses_.set_redeemer_ex_units(purpose, final_index, ex_units);
}
void TransactionBuilder::set_donation(std::uint64_t value) { donation_ = value; }
void TransactionBuilder::set_ttl(std::uint64_t value) { ttl_ = value; }
void TransactionBuilder::set_validity_start(std::uint64_t value) { validity_start_ = value; }
void TransactionBuilder::set_network_id(std::uint8_t value) { network_id_ = value; }
void TransactionBuilder::set_auxiliary_data(CborValue value) { auxiliary_data_ = std::move(value); }
void TransactionBuilder::merge_auxiliary_data(CborValue value) {
  if (!auxiliary_data_ || auxiliary_data_->as_map() == nullptr || value.as_map() == nullptr) {
    auxiliary_data_ = std::move(value);
    return;
  }
  auto entries = auxiliary_data_->as_map()->entries;
  for (auto& [key, item] : value.as_map()->entries) {
    const auto identity = canonical_hex(key);
    const auto existing = std::ranges::find(
        entries, identity, [](const auto& entry) { return canonical_hex(entry.first); });
    if (existing == entries.end()) {
      entries.emplace_back(std::move(key), std::move(item));
    } else {
      existing->second = std::move(item);
    }
  }
  auxiliary_data_ = CborValue::map(std::move(entries));
}

Result<Value> TransactionBuilder::total_input() const {
  Value total;
  for (const auto& input : inputs_) {
    auto next = total.checked_add(input.output.amount());
    if (!next) return std::unexpected(next.error());
    total = std::move(*next);
  }
  std::vector<std::pair<CborValue, CborValue>> accounting_fields;
  if (!withdrawals_.empty()) {
    std::vector<std::pair<CborValue, CborValue>> values;
    for (const auto& withdrawal : withdrawals_) {
      values.emplace_back(CborValue::byte_string(withdrawal.first.to_bytes()),
                          uint(withdrawal.second));
    }
    accounting_fields.emplace_back(uint(5), CborValue::map(std::move(values)));
  }
  if (!certificates_.empty()) {
    accounting_fields.emplace_back(uint(4), tagged_set(certificates_));
  }
  auto implicit = get_implicit_input(CborValue::map(std::move(accounting_fields)),
                                     config_.key_deposit, config_.pool_deposit);
  if (!implicit) return std::unexpected(implicit.error());
  auto next = total.checked_add(Value(*implicit));
  if (!next) return std::unexpected(next.error());
  total = std::move(*next);
  for (const auto& policy : mint_) {
    for (const auto& asset : policy.assets) {
      if (asset.quantity > 0) {
        auto added = add_asset_delta(total, policy.policy, asset.name, asset.quantity);
        if (!added) return std::unexpected(added.error());
        total = std::move(*added);
      }
    }
  }
  return total;
}
Result<Value> TransactionBuilder::total_output() const {
  Value total;
  for (const auto& output : outputs_) {
    auto next = total.checked_add(output.amount());
    if (!next) return std::unexpected(next.error());
    total = std::move(*next);
  }
  if (donation_) {
    auto next = total.checked_add(Value(*donation_));
    if (!next) return std::unexpected(next.error());
    total = std::move(*next);
  }
  std::vector<std::pair<CborValue, CborValue>> accounting_fields;
  if (!certificates_.empty()) {
    accounting_fields.emplace_back(uint(4), tagged_set(certificates_));
  }
  if (!proposals_.empty()) {
    accounting_fields.emplace_back(uint(20), tagged_set(proposals_));
  }
  auto deposits = get_deposit(CborValue::map(std::move(accounting_fields)), config_.key_deposit,
                              config_.pool_deposit);
  if (!deposits) return std::unexpected(deposits.error());
  auto with_deposits = total.checked_add(Value(*deposits));
  if (!with_deposits) return std::unexpected(with_deposits.error());
  total = std::move(*with_deposits);
  for (const auto& policy : mint_) {
    for (const auto& asset : policy.assets) {
      if (asset.quantity < 0) {
        if (asset.quantity == std::numeric_limits<std::int64_t>::min()) {
          return std::unexpected(
              CardanoError(ErrorCode::out_of_range, "burn quantity magnitude overflow"));
        }
        auto added = add_asset_delta(total, policy.policy, asset.name, -asset.quantity);
        if (!added) return std::unexpected(added.error());
        total = std::move(*added);
      }
    }
  }
  return total;
}
Result<bool> TransactionBuilder::covered() const {
  auto input = total_input();
  auto output = total_output();
  auto calculated_fee = min_fee(true);
  if (!input || !output || !calculated_fee) return false;
  auto required = output->checked_add(Value(*calculated_fee));
  if (!required) return false;
  return input->covers(*required);
}
core::VoidResult TransactionBuilder::select_candidate(std::size_t index) {
  if (index >= candidates_.size()) {
    return std::unexpected(argument_error("candidate index is out of range"));
  }
  inputs_.push_back(std::move(candidates_[index]));
  candidates_.erase(candidates_.begin() + static_cast<std::ptrdiff_t>(index));
  return std::monostate{};
}
core::VoidResult TransactionBuilder::select_utxos(CoinSelectionStrategyCIP2 strategy,
                                                  core::SecureRandomSource* random) {
  if ((strategy == CoinSelectionStrategyCIP2::random_improve ||
       strategy == CoinSelectionStrategyCIP2::random_improve_multi_asset) &&
      random == nullptr) {
    return std::unexpected(
        argument_error("random-improve selection requires an explicit random source"));
  }
  if (strategy == CoinSelectionStrategyCIP2::random_improve &&
      std::ranges::any_of(outputs_,
                          [](const auto& output) { return output.amount().has_assets(); })) {
    return std::unexpected(
        argument_error("plain random-improve does not support native-asset outputs"));
  }

  const bool multiasset = strategy == CoinSelectionStrategyCIP2::largest_first_multi_asset ||
                          strategy == CoinSelectionStrategyCIP2::random_improve_multi_asset;
  const bool randomized = strategy == CoinSelectionStrategyCIP2::random_improve ||
                          strategy == CoinSelectionStrategyCIP2::random_improve_multi_asset;

  auto target = total_output();
  if (!target) return std::unexpected(target.error());
  if (multiasset && !randomized) {
    for (const auto& policy : target->multiasset()) {
      for (const auto& asset : policy.assets) {
        while (true) {
          auto current = total_input();
          if (!current) return std::unexpected(current.error());
          if (current->asset_quantity(policy.policy, asset.name) >= asset.quantity) {
            break;
          }
          std::vector<std::size_t> eligible;
          for (std::size_t index = 0; index < candidates_.size(); ++index) {
            if (candidates_[index].output.amount().asset_quantity(policy.policy, asset.name) > 0) {
              eligible.push_back(index);
            }
          }
          if (eligible.empty()) {
            return std::unexpected(balance_error("insufficient native assets for coin selection"));
          }
          std::ranges::sort(eligible, [&](std::size_t left, std::size_t right) {
            const auto lq =
                candidates_[left].output.amount().asset_quantity(policy.policy, asset.name);
            const auto rq =
                candidates_[right].output.amount().asset_quantity(policy.policy, asset.name);
            if (lq != rq) return lq > rq;
            return candidates_[left].input.canonical_identity() <
                   candidates_[right].input.canonical_identity();
          });
          auto status = select_candidate(eligible.front());
          if (!status) return status;
        }
      }
    }
  }

  if (randomized) {
    const auto random_improve_by = [&](const auto& quantity,
                                       std::vector<std::uint64_t> targets) -> core::VoidResult {
      std::erase(targets, std::uint64_t{0});
      std::ranges::stable_sort(targets, std::greater{});
      if (targets.empty()) return std::monostate{};

      std::vector<std::string> shuffled;
      for (const auto& candidate : candidates_) {
        if (quantity(candidate) > 0) {
          shuffled.push_back(candidate.input.canonical_identity());
        }
      }
      auto status = shuffle(shuffled, *random);
      if (!status) return status;

      struct SelectionGroup {
        std::uint64_t target;
        std::vector<TransactionUnspentOutput> values;
      };
      std::vector<SelectionGroup> groups;
      groups.reserve(targets.size());
      for (const auto amount : targets) {
        SelectionGroup group{amount, {}};
        std::uint64_t sum = 0;
        while (sum < amount) {
          if (shuffled.empty()) {
            return std::unexpected(
                balance_error("insufficient quantity for random-improve selection"));
          }
          const auto identity = std::move(shuffled.back());
          shuffled.pop_back();
          const auto found = std::ranges::find(candidates_, identity, [](const auto& candidate) {
            return candidate.input.canonical_identity();
          });
          if (found == candidates_.end()) {
            return std::unexpected(
                CardanoError(ErrorCode::internal, "random-improve candidate identity was lost"));
          }
          const auto candidate_quantity = quantity(*found);
          if (candidate_quantity > std::numeric_limits<std::uint64_t>::max() - sum) {
            return std::unexpected(
                CardanoError(ErrorCode::out_of_range, "random-improve group quantity overflow"));
          }
          sum += candidate_quantity;
          group.values.push_back(std::move(*found));
          candidates_.erase(found);
        }
        groups.push_back(std::move(group));
      }

      std::vector<std::string> improvements;
      for (const auto& candidate : candidates_) {
        if (quantity(candidate) > 0) {
          improvements.push_back(candidate.input.canonical_identity());
        }
      }
      status = shuffle(improvements, *random);
      if (!status) return status;

      std::size_t cursor = 0;
      for (auto& group : groups) {
        const BigInteger ideal = BigInteger(group.target) * BigInteger(std::uint64_t{2});
        const BigInteger maximum = BigInteger(group.target) * BigInteger(std::uint64_t{3});
        for (auto& current : group.values) {
          if (improvements.empty()) break;
          const auto improvement_index = cursor % improvements.size();
          ++cursor;
          const auto found = std::ranges::find(
              candidates_, improvements[improvement_index],
              [](const auto& candidate) { return candidate.input.canonical_identity(); });
          if (found == candidates_.end()) {
            return std::unexpected(
                CardanoError(ErrorCode::internal, "random-improve replacement identity was lost"));
          }
          const BigInteger current_quantity(quantity(current));
          const BigInteger candidate_quantity(quantity(*found));
          const auto current_distance =
              current_quantity < ideal ? ideal - current_quantity : current_quantity - ideal;
          const auto candidate_distance =
              candidate_quantity < ideal ? ideal - candidate_quantity : candidate_quantity - ideal;
          if (candidate_distance < current_distance && candidate_quantity < maximum) {
            auto returned = std::move(current);
            current = std::move(*found);
            *found = std::move(returned);
            improvements[improvement_index] = found->input.canonical_identity();
          }
        }
      }

      for (auto& group : groups) {
        for (auto& candidate : group.values) {
          inputs_.push_back(std::move(candidate));
        }
      }
      return std::monostate{};
    };

    if (multiasset) {
      for (const auto& policy : target->multiasset()) {
        for (const auto& asset : policy.assets) {
          std::vector<std::uint64_t> targets;
          for (const auto& output : outputs_) {
            const auto quantity = output.amount().asset_quantity(policy.policy, asset.name);
            if (quantity > 0) {
              targets.push_back(static_cast<std::uint64_t>(quantity));
            }
          }
          auto status = random_improve_by(
              [&](const TransactionUnspentOutput& candidate) {
                const auto quantity =
                    candidate.output.amount().asset_quantity(policy.policy, asset.name);
                return quantity > 0 ? static_cast<std::uint64_t>(quantity) : std::uint64_t{0};
              },
              std::move(targets));
          if (!status) return status;
        }
      }
    }

    std::vector<std::uint64_t> coin_targets;
    for (const auto& output : outputs_) {
      if (output.amount().coin() > 0) {
        coin_targets.push_back(output.amount().coin());
      }
    }
    auto status = random_improve_by(
        [](const TransactionUnspentOutput& candidate) { return candidate.output.amount().coin(); },
        std::move(coin_targets));
    if (!status) return status;

    std::vector<std::string> remaining;
    remaining.reserve(candidates_.size());
    for (const auto& candidate : candidates_) {
      remaining.push_back(candidate.input.canonical_identity());
    }
    status = shuffle(remaining, *random);
    if (!status) return status;
    while (true) {
      auto coverage = covered();
      if (coverage && *coverage) return std::monostate{};
      if (remaining.empty()) {
        return std::unexpected(balance_error("available inputs do not cover outputs and fees"));
      }
      const auto identity = std::move(remaining.back());
      remaining.pop_back();
      const auto found = std::ranges::find(candidates_, identity, [](const auto& candidate) {
        return candidate.input.canonical_identity();
      });
      if (found == candidates_.end()) {
        return std::unexpected(
            CardanoError(ErrorCode::internal, "random-improve final candidate identity was lost"));
      }
      inputs_.push_back(std::move(*found));
      candidates_.erase(found);
    }
  }

  while (true) {
    auto status = covered();
    if (status && *status) return std::monostate{};
    if (candidates_.empty()) {
      return std::unexpected(balance_error("available inputs do not cover outputs and fees"));
    }
    std::size_t selected = 0;
    for (std::size_t index = 1; index < candidates_.size(); ++index) {
      const auto left_coin = candidates_[index].output.amount().coin();
      const auto right_coin = candidates_[selected].output.amount().coin();
      if (left_coin > right_coin ||
          (left_coin == right_coin && candidates_[index].input.canonical_identity() <
                                          candidates_[selected].input.canonical_identity())) {
        selected = index;
      }
    }
    auto selected_status = select_candidate(selected);
    if (!selected_status) return selected_status;
  }
}

Result<std::vector<TransactionOutput>> TransactionBuilder::make_change(const Address& address,
                                                                       const Value& change) const {
  if (!change.has_assets()) {
    TransactionOutput output(address, change);
    auto minimum = min_ada_required(output.to_cbor_value(), config_.coins_per_utxo_byte);
    if (!minimum) return std::unexpected(minimum.error());
    if (change.coin() < *minimum) return std::vector<TransactionOutput>{};
    return std::vector<TransactionOutput>{std::move(output)};
  }

  std::vector<std::vector<PolicyAssets>> bundles;
  std::vector<PolicyAssets> current;
  for (const auto& policy : change.multiasset()) {
    for (const auto& asset : policy.assets) {
      if (asset.quantity <= 0) continue;
      auto trial = current;
      auto policy_it = std::find_if(trial.begin(), trial.end(),
                                    [&](const auto& item) { return item.policy == policy.policy; });
      if (policy_it == trial.end()) {
        trial.push_back({policy.policy, {asset}});
      } else {
        policy_it->assets.push_back(asset);
      }
      auto size = Value(0, trial).to_cbor();
      if (!size) return std::unexpected(size.error());
      if (size->size() > config_.max_value_size) {
        if (current.empty()) {
          return std::unexpected(CardanoError(ErrorCode::out_of_range,
                                              "one native asset exceeds the maximum Value size"));
        }
        bundles.push_back(std::move(current));
        current = {{policy.policy, {asset}}};
        size = Value(0, current).to_cbor();
        if (!size || size->size() > config_.max_value_size) {
          return std::unexpected(CardanoError(ErrorCode::out_of_range,
                                              "one native asset exceeds the maximum Value size"));
        }
      } else {
        current = std::move(trial);
      }
    }
  }
  if (!current.empty()) bundles.push_back(std::move(current));

  std::vector<std::uint64_t> minima;
  std::uint64_t minimum_total = 0;
  for (const auto& bundle : bundles) {
    TransactionOutput first(address, Value(0, bundle));
    auto first_minimum = min_ada_required(first.to_cbor_value(), config_.coins_per_utxo_byte);
    if (!first_minimum) return std::unexpected(first_minimum.error());
    first.amount().set_coin(*first_minimum);
    auto second_minimum = min_ada_required(first.to_cbor_value(), config_.coins_per_utxo_byte);
    if (!second_minimum) return std::unexpected(second_minimum.error());
    if (*second_minimum > std::numeric_limits<std::uint64_t>::max() - minimum_total) {
      return std::unexpected(CardanoError(ErrorCode::out_of_range, "change minimum coin overflow"));
    }
    minimum_total += *second_minimum;
    minima.push_back(*second_minimum);
  }
  if (change.coin() < minimum_total) {
    return std::unexpected(balance_error("change coin does not cover native-asset minimum ADA"));
  }
  auto remainder = change.coin() - minimum_total;
  std::vector<TransactionOutput> outputs;
  if (config_.prefer_pure_change && remainder > 0) {
    TransactionOutput pure(address, Value(remainder));
    auto minimum = min_ada_required(pure.to_cbor_value(), config_.coins_per_utxo_byte);
    if (!minimum) return std::unexpected(minimum.error());
    if (remainder >= *minimum) {
      outputs.push_back(std::move(pure));
      remainder = 0;
    }
  }
  for (std::size_t index = 0; index < bundles.size(); ++index) {
    outputs.emplace_back(address,
                         Value(minima[index] + (index == 0 ? remainder : 0), bundles[index]));
  }
  return outputs;
}

Result<bool> TransactionBuilder::add_change_if_needed(const Address& address,
                                                      ChangeSelectionAlgo algorithm) {
  static_cast<void>(algorithm);
  const auto original_size = outputs_.size();
  std::uint64_t current_fee = fee_.value_or(0);
  for (std::size_t iteration = 0; iteration < 8; ++iteration) {
    outputs_.erase(outputs_.begin() + static_cast<std::ptrdiff_t>(original_size), outputs_.end());
    auto input = total_input();
    auto output = total_output();
    if (!input || !output) {
      return std::unexpected(!input ? input.error() : output.error());
    }
    auto required = output->checked_add(Value(current_fee));
    if (!required) return std::unexpected(required.error());
    auto change = input->checked_sub(*required);
    if (!change) return std::unexpected(change.error());
    auto change_outputs = make_change(address, *change);
    if (!change_outputs) return std::unexpected(change_outputs.error());
    outputs_.insert(outputs_.end(), std::make_move_iterator(change_outputs->begin()),
                    std::make_move_iterator(change_outputs->end()));
    fee_ = current_fee;
    auto next_fee = min_fee(true);
    if (!next_fee) return std::unexpected(next_fee.error());
    if (*next_fee == current_fee) break;
    current_fee = *next_fee;
  }

  outputs_.erase(outputs_.begin() + static_cast<std::ptrdiff_t>(original_size), outputs_.end());
  auto input = total_input();
  auto output = total_output();
  if (!input || !output) {
    return std::unexpected(!input ? input.error() : output.error());
  }
  auto required = output->checked_add(Value(current_fee));
  if (!required) return std::unexpected(required.error());
  auto change = input->checked_sub(*required);
  if (!change) return std::unexpected(change.error());
  auto final_outputs = make_change(address, *change);
  if (!final_outputs) return std::unexpected(final_outputs.error());
  const bool emitted = !final_outputs->empty();
  if (!emitted && (change->coin() != 0 || change->has_assets())) {
    if (change->has_assets()) {
      return std::unexpected(balance_error("native-asset change cannot be absorbed into the fee"));
    }
    if (change->coin() > std::numeric_limits<std::uint64_t>::max() - current_fee) {
      return std::unexpected(CardanoError(ErrorCode::out_of_range, "change fee overflow"));
    }
    current_fee += change->coin();
  } else {
    outputs_.insert(outputs_.end(), std::make_move_iterator(final_outputs->begin()),
                    std::make_move_iterator(final_outputs->end()));
  }
  fee_ = current_fee;
  return emitted;
}

Result<std::uint64_t> TransactionBuilder::min_fee(bool include_exunits) const {
  auto body = build_body_with_fee(std::numeric_limits<std::uint64_t>::max(), false);
  if (!body) return std::unexpected(body.error());
  auto witnesses = witnesses_.build_unchecked(true, !include_exunits);
  if (!witnesses) return std::unexpected(witnesses.error());
  const auto transaction = CborValue::array({
      *body,
      *witnesses,
      CborValue::boolean(true),
      auxiliary_data_.value_or(CborValue::null()),
  });
  auto no_script = min_no_script_fee(transaction, config_.linear_fee);
  if (!no_script) return std::unexpected(no_script.error());
  if (!include_exunits) return no_script;
  ExUnits total{};
  if (const auto* map = witnesses->as_map()) {
    for (const auto& [key, value] : map->entries) {
      const auto* number = key.as_unsigned();
      if (number != nullptr && number->value == BigInteger(std::uint64_t{5})) {
        auto ex_units = compute_total_ex_units(value);
        if (!ex_units) return std::unexpected(ex_units.error());
        total = *ex_units;
      }
    }
  }
  auto script_fee = min_script_fee(total, config_.ex_unit_prices);
  if (!script_fee) return std::unexpected(script_fee.error());
  std::uint64_t reference_size = 0;
  for (const auto& input : inputs_) {
    if (!input.output.script_reference()) continue;
    const auto& reference = *input.output.script_reference();
    std::size_t size = 0;
    if (const auto* tag = reference.as_tag();
        tag != nullptr && tag->value != nullptr && tag->value->as_byte_string() != nullptr) {
      size = tag->value->as_byte_string()->value.size();
    } else {
      auto encoded = core::cbor::encode_cbor(reference);
      if (!encoded) return std::unexpected(encoded.error());
      size = encoded->size();
    }
    if (size > std::numeric_limits<std::uint64_t>::max() - reference_size) {
      return std::unexpected(
          CardanoError(ErrorCode::out_of_range, "reference script size overflow"));
    }
    reference_size += static_cast<std::uint64_t>(size);
  }
  for (const auto& input : reference_utxos_) {
    if (!input.output.script_reference()) continue;
    const auto& reference = *input.output.script_reference();
    std::size_t size = 0;
    if (const auto* tag = reference.as_tag();
        tag != nullptr && tag->value != nullptr && tag->value->as_byte_string() != nullptr) {
      size = tag->value->as_byte_string()->value.size();
    } else {
      auto encoded = core::cbor::encode_cbor(reference);
      if (!encoded) return std::unexpected(encoded.error());
      size = encoded->size();
    }
    if (size > std::numeric_limits<std::uint64_t>::max() - reference_size) {
      return std::unexpected(
          CardanoError(ErrorCode::out_of_range, "reference script size overflow"));
    }
    reference_size += static_cast<std::uint64_t>(size);
  }
  auto reference_fee =
      min_reference_script_fee(reference_size, config_.reference_script_cost_per_byte);
  if (!reference_fee) return std::unexpected(reference_fee.error());
  return cardano::chain::min_fee(transaction, config_.linear_fee, *script_fee, *reference_fee);
}

Result<CborValue> TransactionBuilder::build_body_with_fee(std::uint64_t fee,
                                                          bool enforce_size) const {
  if (donation_ && *donation_ == 0) {
    return std::unexpected(argument_error("donation cannot be zero"));
  }
  auto sorted_inputs = inputs_;
  std::ranges::sort(sorted_inputs, {},
                    [](const auto& input) { return input.input.canonical_identity(); });
  std::vector<CborValue> input_values;
  for (const auto& input : sorted_inputs) {
    input_values.push_back(input.input.to_cbor_value());
  }
  std::vector<CborValue> output_values;
  for (const auto& output : outputs_) output_values.push_back(output.to_cbor_value());
  std::vector<std::pair<CborValue, CborValue>> fields{
      {uint(0), tagged_set(std::move(input_values))},
      {uint(1), CborValue::array(std::move(output_values))},
      {uint(2), uint(fee)},
  };
  if (ttl_) fields.emplace_back(uint(3), uint(*ttl_));
  if (!certificates_.empty()) fields.emplace_back(uint(4), tagged_set(certificates_));
  if (!withdrawals_.empty()) {
    auto sorted = withdrawals_;
    std::ranges::sort(sorted, {}, [](const auto& withdrawal) { return withdrawal.first.to_hex(); });
    std::vector<std::pair<CborValue, CborValue>> values;
    for (const auto& withdrawal : sorted) {
      values.emplace_back(CborValue::byte_string(withdrawal.first.to_bytes()),
                          uint(withdrawal.second));
    }
    fields.emplace_back(uint(5), CborValue::map(std::move(values)));
  }
  if (auxiliary_data_) {
    auto hash = hash_auxiliary_data(*auxiliary_data_);
    if (!hash) return std::unexpected(hash.error());
    fields.emplace_back(uint(7), CborValue::byte_string(hash->to_bytes()));
  }
  if (validity_start_) fields.emplace_back(uint(8), uint(*validity_start_));
  if (!mint_.empty()) {
    Value mint_value(0, mint_);
    const auto mint_cbor = mint_value.to_cbor_value(true);
    const auto* array = mint_cbor.as_array();
    fields.emplace_back(uint(9), array->values[1]);
  }
  if (!redeemers_.empty()) {
    auto witness = witnesses_.build_unchecked(false, !enforce_size);
    if (!witness) return std::unexpected(witness.error());
    auto script_data_hash = calc_script_data_hash_from_witness(*witness, config_.cost_models);
    if (!script_data_hash) {
      return std::unexpected(script_data_hash.error());
    }
    if (*script_data_hash) {
      fields.emplace_back(uint(11), CborValue::byte_string((*script_data_hash)->to_bytes()));
    }
  }
  if (!collateral_.empty()) {
    std::vector<CborValue> values;
    for (const auto& collateral : collateral_) {
      values.push_back(collateral.utxo.input.to_cbor_value());
    }
    fields.emplace_back(uint(13), tagged_set(std::move(values)));
  }
  if (!required_signers_.empty()) {
    std::vector<CborValue> values;
    for (const auto& signer : required_signers_) {
      values.push_back(CborValue::byte_string(signer.to_bytes()));
    }
    fields.emplace_back(uint(14), tagged_set(std::move(values)));
  }
  if (network_id_) fields.emplace_back(uint(15), uint(*network_id_));
  if (collateral_return_) {
    std::uint64_t collateral_coin = 0;
    for (const auto& collateral : collateral_) {
      const auto coin = collateral.utxo.output.amount().coin();
      if (coin > std::numeric_limits<std::uint64_t>::max() - collateral_coin) {
        return std::unexpected(
            CardanoError(ErrorCode::out_of_range, "collateral coin total overflow"));
      }
      collateral_coin += coin;
    }
    if (collateral_return_->amount().coin() > collateral_coin) {
      return std::unexpected(balance_error("collateral return exceeds collateral input coin"));
    }
    fields.emplace_back(uint(16), collateral_return_->to_cbor_value());
    fields.emplace_back(uint(17), uint(collateral_coin - collateral_return_->amount().coin()));
  }
  if (!reference_inputs_.empty()) {
    std::vector<CborValue> values;
    for (const auto& input : reference_inputs_) values.push_back(input.to_cbor_value());
    fields.emplace_back(uint(18), tagged_set(std::move(values)));
  }
  if (!votes_.empty()) {
    std::vector<std::pair<std::string, std::vector<const VoteBuilderEntry*>>> groups;
    for (const auto& vote : votes_) {
      const auto identity = canonical_hex(vote.voter);
      const auto found = std::find_if(groups.begin(), groups.end(),
                                      [&](const auto& group) { return group.first == identity; });
      if (found == groups.end()) {
        groups.push_back({identity, {&vote}});
      } else {
        found->second.push_back(&vote);
      }
    }
    std::ranges::sort(groups, {}, [](const auto& group) { return group.first; });
    std::vector<std::pair<CborValue, CborValue>> voters;
    for (auto& [identity, actions] : groups) {
      static_cast<void>(identity);
      std::ranges::sort(actions, [](const auto* left, const auto* right) {
        auto left_bytes =
            core::cbor::encode_cbor(left->action, {.mode = core::cbor::Mode::canonical});
        auto right_bytes =
            core::cbor::encode_cbor(right->action, {.mode = core::cbor::Mode::canonical});
        if (left_bytes->size() != right_bytes->size()) {
          return left_bytes->size() < right_bytes->size();
        }
        return std::lexicographical_compare(left_bytes->begin(), left_bytes->end(),
                                            right_bytes->begin(), right_bytes->end());
      });
      std::vector<std::pair<CborValue, CborValue>> procedures;
      for (const auto* action : actions) {
        procedures.emplace_back(action->action, action->procedure);
      }
      voters.emplace_back(actions.front()->voter, CborValue::map(std::move(procedures)));
    }
    fields.emplace_back(uint(19), CborValue::map(std::move(voters)));
  }
  if (!proposals_.empty()) fields.emplace_back(uint(20), tagged_set(proposals_));
  if (current_treasury_value_) {
    fields.emplace_back(uint(21), uint(*current_treasury_value_));
  }
  if (donation_) fields.emplace_back(uint(22), uint(*donation_));
  std::ranges::sort(fields, {}, [](const auto& field) {
    return field.first.as_unsigned()->value.to_uint64().value();
  });
  auto body = CborValue::map(std::move(fields));
  if (enforce_size) {
    auto witness = witnesses_.build_unchecked(true, false);
    if (!witness) return std::unexpected(witness.error());
    const auto transaction = CborValue::array({
        body,
        *witness,
        CborValue::boolean(true),
        auxiliary_data_.value_or(CborValue::null()),
    });
    auto bytes = core::cbor::encode_cbor(transaction);
    if (!bytes) return std::unexpected(bytes.error());
    if (bytes->size() > config_.max_transaction_size) {
      return std::unexpected(
          CardanoError(ErrorCode::out_of_range, "transaction exceeds maximum size"));
    }
  }
  return body;
}

Result<CborValue> TransactionBuilder::build_body() const {
  if (!fee_) {
    return std::unexpected(argument_error("transaction fee has not been set"));
  }
  return build_body_with_fee(*fee_, true);
}
Result<CborValue> TransactionBuilder::build_for_evaluation() const {
  if (!fee_) {
    return std::unexpected(argument_error("transaction fee has not been set"));
  }
  auto body = build_body_with_fee(*fee_, false);
  if (!body) return std::unexpected(body.error());
  auto witness = witnesses_.build_unchecked(true, true);
  if (!witness) return std::unexpected(witness.error());
  auto transaction = CborValue::array({
      *body,
      *witness,
      CborValue::boolean(true),
      auxiliary_data_.value_or(CborValue::null()),
  });
  auto bytes = core::cbor::encode_cbor(transaction);
  if (!bytes) return std::unexpected(bytes.error());
  if (bytes->size() > config_.max_transaction_size) {
    return std::unexpected(
        CardanoError(ErrorCode::out_of_range, "evaluation transaction exceeds maximum size"));
  }
  return transaction;
}
Result<CborValue> TransactionBuilder::build_transaction() const {
  auto body = build_body();
  if (!body) return std::unexpected(body.error());
  auto witness = witnesses_.build_unchecked(false, false);
  if (!witness) return std::unexpected(witness.error());
  auto transaction = CborValue::array({
      *body,
      *witness,
      CborValue::boolean(true),
      auxiliary_data_.value_or(CborValue::null()),
  });
  auto bytes = core::cbor::encode_cbor(transaction);
  if (!bytes) return std::unexpected(bytes.error());
  if (bytes->size() > config_.max_transaction_size) {
    return std::unexpected(
        CardanoError(ErrorCode::out_of_range, "transaction exceeds maximum size"));
  }
  return transaction;
}
Result<SignedTxBuilder> TransactionBuilder::build_signed() const {
  auto body = build_body();
  if (!body) return std::unexpected(body.error());
  return SignedTxBuilder(*body, witnesses_, auxiliary_data_);
}
const std::vector<TransactionUnspentOutput>& TransactionBuilder::inputs() const noexcept {
  return inputs_;
}
const std::vector<TransactionOutput>& TransactionBuilder::outputs() const noexcept {
  return outputs_;
}
std::optional<std::uint64_t> TransactionBuilder::fee() const noexcept { return fee_; }

}  // namespace cardano::chain
