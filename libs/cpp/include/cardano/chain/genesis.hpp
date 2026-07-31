#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>

#include "cardano/chain/ledger.hpp"
#include "cardano/core/big_integer.hpp"
#include "cardano/core/error.hpp"
#include "cardano/core/network.hpp"
#include "cardano/crypto/fixed_bytes.hpp"

namespace cardano::chain {

struct ParsedByronGenesis {
  crypto::BlockHeaderHash genesis_previous;
  std::uint64_t epoch_stability_depth{};
  core::ProtocolMagic protocol_magic{0};
  LinearFee fee_policy;
  std::uint64_t start_time{};
  std::uint64_t slot_duration_milliseconds{};
  std::map<std::string, core::BigInteger> avvm_distribution;
  std::map<std::string, core::BigInteger> non_avvm_balances;
  std::set<std::string> boot_stakeholders;
};

struct ShelleyGenesisDelegation {
  crypto::Ed25519KeyHash delegate;
  crypto::VRFKeyHash vrf;
};

struct ParsedShelleyGenesis {
  core::BigInteger epoch_length;
  std::map<std::string, core::BigInteger> initial_funds;
  std::uint8_t network_id{};
  std::uint32_t network_magic{};
  std::string system_start;
  std::map<std::string, ShelleyGenesisDelegation> genesis_delegations;
  std::string raw_json;
};

[[nodiscard]] core::Result<ParsedByronGenesis> parse_byron_genesis(std::string_view json);
[[nodiscard]] core::Result<ParsedShelleyGenesis> parse_shelley_genesis(std::string_view json);

}  // namespace cardano::chain
