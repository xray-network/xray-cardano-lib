#include "cardano/chain/genesis.hpp"

#include <array>
#include <limits>
#include <nlohmann/json.hpp>
#include <span>
#include <utility>

#include "cardano/chain/address.hpp"
#include "cardano/crypto/keys.hpp"
#include "cardano/crypto/primitives.hpp"

namespace cardano::chain {
namespace {

using Json = nlohmann::ordered_json;

[[nodiscard]] core::CardanoError invalid(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::Result<const Json*> object_field(const Json& object, std::string_view field) {
  if (!object.is_object()) {
    return std::unexpected(invalid("genesis value must be an object"));
  }
  const auto found = object.find(std::string(field));
  if (found == object.end() || !found->is_object()) {
    return std::unexpected(
        invalid(std::string("genesis field ") + std::string(field) + " must be an object"));
  }
  return &*found;
}

[[nodiscard]] core::Result<std::string> string_field(const Json& object, std::string_view field) {
  const auto found = object.find(std::string(field));
  if (found == object.end() || !found->is_string()) {
    return std::unexpected(
        invalid(std::string("genesis field ") + std::string(field) + " must be a string"));
  }
  return found->get<std::string>();
}

[[nodiscard]] core::Result<core::BigInteger> unsigned_integer(const Json& value,
                                                              std::string_view name) {
  std::string decimal;
  if (value.is_number_unsigned()) {
    decimal = std::to_string(value.get<std::uint64_t>());
  } else if (value.is_number_integer()) {
    const auto number = value.get<std::int64_t>();
    if (number < 0) {
      return std::unexpected(invalid(std::string(name) + " must be unsigned"));
    }
    decimal = std::to_string(number);
  } else if (value.is_string()) {
    decimal = value.get<std::string>();
    if (decimal.empty() || decimal.find_first_not_of("0123456789") != std::string::npos) {
      return std::unexpected(invalid(std::string(name) + " must be an unsigned decimal integer"));
    }
  } else {
    return std::unexpected(invalid(std::string(name) + " must be an unsigned integer"));
  }
  return core::BigInteger::from_decimal(decimal);
}

[[nodiscard]] core::Result<std::uint64_t> uint64_field(const Json& object, std::string_view field) {
  const auto found = object.find(std::string(field));
  if (found == object.end()) {
    return std::unexpected(invalid(std::string("missing genesis field ") + std::string(field)));
  }
  auto value = unsigned_integer(*found, field);
  return value ? value->to_uint64() : std::unexpected(value.error());
}

[[nodiscard]] core::Result<core::Bytes> decode_base64(std::string_view encoded, bool url_safe) {
  std::array<std::int16_t, 256> table{};
  table.fill(-1);
  const std::string_view alphabet =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (std::size_t index = 0; index < alphabet.size(); ++index) {
    table[static_cast<unsigned char>(alphabet[index])] = static_cast<std::int16_t>(index);
  }
  if (url_safe) {
    table[static_cast<unsigned char>('-')] = 62;
    table[static_cast<unsigned char>('_')] = 63;
  }
  core::Bytes output;
  std::uint32_t accumulator = 0;
  unsigned bits = 0;
  bool padding = false;
  for (const char raw : encoded) {
    const auto character = static_cast<unsigned char>(raw);
    if (character == '=') {
      padding = true;
      continue;
    }
    if (padding || table[character] < 0) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::invalid_encoding, "invalid Base64 value"));
    }
    accumulator = (accumulator << 6U) | static_cast<std::uint32_t>(table[character]);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      output.push_back(static_cast<core::Byte>((accumulator >> bits) & 0xffU));
    }
  }
  if (bits >= 6 || (bits != 0 && (accumulator & ((1U << bits) - 1U)) != 0)) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "invalid Base64 padding"));
  }
  return output;
}

[[nodiscard]] core::Result<Json> parse_json(std::string_view json) {
  try {
    auto parsed = Json::parse(json);
    if (!parsed.is_object()) {
      return std::unexpected(invalid("genesis JSON must be an object"));
    }
    return parsed;
  } catch (const nlohmann::json::exception& error) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::invalid_encoding, std::string("invalid genesis JSON: ") + error.what()));
  }
}

}  // namespace

core::Result<ParsedByronGenesis> parse_byron_genesis(std::string_view json) {
  auto raw = parse_json(json);
  if (!raw) {
    return std::unexpected(raw.error());
  }
  auto protocol = object_field(*raw, "protocolConsts");
  auto block = object_field(*raw, "blockVersionData");
  if (!protocol) {
    return std::unexpected(protocol.error());
  }
  if (!block) {
    return std::unexpected(block.error());
  }
  auto fees = object_field(**block, "txFeePolicy");
  auto avvm = object_field(*raw, "avvmDistr");
  auto non_avvm = object_field(*raw, "nonAvvmBalances");
  auto stakeholders = object_field(*raw, "bootStakeholders");
  auto heavy = object_field(*raw, "heavyDelegation");
  if (!fees) return std::unexpected(fees.error());
  if (!avvm) return std::unexpected(avvm.error());
  if (!non_avvm) return std::unexpected(non_avvm.error());
  if (!stakeholders) return std::unexpected(stakeholders.error());
  if (!heavy) return std::unexpected(heavy.error());

  auto stability = uint64_field(**protocol, "k");
  auto magic = uint64_field(**protocol, "protocolMagic");
  auto multiplier = uint64_field(**fees, "multiplier");
  auto summand = uint64_field(**fees, "summand");
  auto start_time = uint64_field(*raw, "startTime");
  auto slot_duration = uint64_field(**block, "slotDuration");
  if (!stability) return std::unexpected(stability.error());
  if (!magic || *magic > std::numeric_limits<std::uint32_t>::max()) {
    return std::unexpected(magic ? invalid("protocolMagic exceeds uint32") : magic.error());
  }
  if (!multiplier) return std::unexpected(multiplier.error());
  if (!summand) return std::unexpected(summand.error());
  if (!start_time) return std::unexpected(start_time.error());
  if (!slot_duration) return std::unexpected(slot_duration.error());

  std::map<std::string, core::BigInteger> avvm_distribution;
  for (const auto& [key, value] : (*avvm)->items()) {
    auto decoded = decode_base64(key, true);
    auto amount = unsigned_integer(value, "avvm distribution amount");
    if (!decoded) return std::unexpected(decoded.error());
    if (!crypto::PublicKey::from_bytes(*decoded)) {
      return std::unexpected(invalid("AVVM key must decode to 32 bytes"));
    }
    if (!amount) return std::unexpected(amount.error());
    avvm_distribution.emplace(key, std::move(*amount));
  }

  std::map<std::string, core::BigInteger> non_avvm_balances;
  for (const auto& [key, value] : (*non_avvm)->items()) {
    auto address = ByronAddress::from_base58(key);
    auto amount = unsigned_integer(value, "non-AVVM balance");
    if (!address) return std::unexpected(address.error());
    if (!amount) return std::unexpected(amount.error());
    non_avvm_balances.emplace(key, std::move(*amount));
  }

  std::set<std::string> boot_stakeholders;
  for (const auto& [key, unused] : (*stakeholders)->items()) {
    static_cast<void>(unused);
    auto hash = crypto::Ed25519KeyHash::from_hex(key);
    if (!hash) return std::unexpected(hash.error());
    boot_stakeholders.insert(key);
    const auto entry = (*heavy)->find(key);
    if (entry == (*heavy)->end() || !entry->is_object()) {
      return std::unexpected(invalid("heavyDelegation must contain every boot stakeholder"));
    }
    for (const auto* field : {"issuerPk", "delegatePk"}) {
      auto encoded = string_field(*entry, field);
      if (!encoded) return std::unexpected(encoded.error());
      auto decoded = decode_base64(*encoded, false);
      if (!decoded) return std::unexpected(decoded.error());
      if (decoded->size() != 64) {
        return std::unexpected(
            core::CardanoError(core::ErrorCode::invalid_length,
                               std::string(field) + " must be a 64-byte extended public key"));
      }
    }
  }

  const auto normalized_json = raw->dump();
  const auto normalized_bytes = std::as_bytes(std::span(normalized_json));
  auto hash = crypto::BlockHeaderHash::from_bytes(crypto::blake2b256(normalized_bytes));
  if (!hash) {
    return std::unexpected(hash.error());
  }
  return ParsedByronGenesis{
      .genesis_previous = std::move(*hash),
      .epoch_stability_depth = *stability,
      .protocol_magic = core::ProtocolMagic(static_cast<std::uint32_t>(*magic)),
      .fee_policy = LinearFee{*multiplier, *summand},
      .start_time = *start_time,
      .slot_duration_milliseconds = *slot_duration,
      .avvm_distribution = std::move(avvm_distribution),
      .non_avvm_balances = std::move(non_avvm_balances),
      .boot_stakeholders = std::move(boot_stakeholders),
  };
}

core::Result<ParsedShelleyGenesis> parse_shelley_genesis(std::string_view json) {
  auto raw = parse_json(json);
  if (!raw) {
    return std::unexpected(raw.error());
  }
  auto network = string_field(*raw, "networkId");
  auto system_start = string_field(*raw, "systemStart");
  auto epoch = raw->find("epochLength");
  auto network_magic = uint64_field(*raw, "networkMagic");
  auto funds = object_field(*raw, "initialFunds");
  auto delegations = object_field(*raw, "genDelegs");
  if (!network) return std::unexpected(network.error());
  if (*network != "Mainnet" && *network != "Testnet") {
    return std::unexpected(invalid("networkId must be Mainnet or Testnet"));
  }
  if (!system_start) return std::unexpected(system_start.error());
  if (epoch == raw->end()) return std::unexpected(invalid("missing epochLength"));
  auto epoch_length = unsigned_integer(*epoch, "epochLength");
  if (!epoch_length) return std::unexpected(epoch_length.error());
  if (!network_magic || *network_magic > std::numeric_limits<std::uint32_t>::max()) {
    return std::unexpected(network_magic ? invalid("networkMagic exceeds uint32")
                                         : network_magic.error());
  }
  if (!funds) return std::unexpected(funds.error());
  if (!delegations) return std::unexpected(delegations.error());

  std::map<std::string, core::BigInteger> initial_funds;
  for (const auto& [encoded, value] : (*funds)->items()) {
    auto address = Address::from_hex(encoded);
    auto amount = unsigned_integer(value, "initial fund");
    if (!address) return std::unexpected(address.error());
    if (!amount) return std::unexpected(amount.error());
    initial_funds.emplace(address->to_hex(), std::move(*amount));
  }

  std::map<std::string, ShelleyGenesisDelegation> genesis_delegations;
  for (const auto& [hash_text, value] : (*delegations)->items()) {
    auto hash = crypto::Ed25519KeyHash::from_hex(hash_text);
    if (!hash || !value.is_object()) {
      return std::unexpected(hash ? invalid("genesis delegation must be an object") : hash.error());
    }
    auto delegate_text = string_field(value, "delegate");
    auto vrf_text = string_field(value, "vrf");
    if (!delegate_text) return std::unexpected(delegate_text.error());
    if (!vrf_text) return std::unexpected(vrf_text.error());
    auto delegate = crypto::Ed25519KeyHash::from_hex(*delegate_text);
    auto vrf = crypto::VRFKeyHash::from_hex(*vrf_text);
    if (!delegate) return std::unexpected(delegate.error());
    if (!vrf) return std::unexpected(vrf.error());
    genesis_delegations.emplace(hash_text,
                                ShelleyGenesisDelegation{std::move(*delegate), std::move(*vrf)});
  }

  const auto staking = raw->find("staking");
  if (staking != raw->end() && !staking->is_null()) {
    if (!staking->is_object()) {
      return std::unexpected(invalid("staking must be an object"));
    }
    auto pools = object_field(*staking, "pools");
    if (!pools) return std::unexpected(pools.error());
    for (const auto& [unused, pool] : (*pools)->items()) {
      static_cast<void>(unused);
      if (!pool.is_object() || !pool.contains("owners") || !pool.at("owners").is_array() ||
          !pool.contains("relays") || !pool.at("relays").is_array()) {
        return std::unexpected(invalid("pool owners and relays must be arrays"));
      }
    }
  }

  return ParsedShelleyGenesis{
      .epoch_length = std::move(*epoch_length),
      .initial_funds = std::move(initial_funds),
      .network_id = static_cast<std::uint8_t>(*network == "Mainnet" ? 1 : 0),
      .network_magic = static_cast<std::uint32_t>(*network_magic),
      .system_start = std::move(*system_start),
      .genesis_delegations = std::move(genesis_delegations),
      .raw_json = raw->dump(),
  };
}

}  // namespace cardano::chain
