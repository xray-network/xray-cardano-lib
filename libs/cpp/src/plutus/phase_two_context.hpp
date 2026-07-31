#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "cardano/core/cbor.hpp"
#include "cardano/plutus/uplc.hpp"

namespace cardano::plutus::detail {

struct ContextRedeemer {
  std::uint8_t tag{};
  std::uint64_t index{};
  core::cbor::Value data;
};

struct ContextUtxo {
  core::cbor::Value input;
  core::cbor::Value output;
};

[[nodiscard]] core::Result<Data> make_script_context(
    const ContextRedeemer& current, const std::vector<ContextRedeemer>& redeemers,
    const core::cbor::MapValue& body, const core::cbor::MapValue& witnesses,
    const std::vector<ContextUtxo>& utxos, std::optional<core::cbor::Value> current_datum,
    std::array<std::int64_t, 3> slot_config, std::uint64_t protocol, std::uint8_t language);

}  // namespace cardano::plutus::detail
