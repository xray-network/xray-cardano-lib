#pragma once

#include <cstdint>

namespace cardano::core {

inline constexpr std::uint32_t BYRON_MAINNET_NETWORK_MAGIC = 764824073U;
inline constexpr std::uint32_t BYRON_TESTNET_NETWORK_MAGIC = 1097911063U;
inline constexpr std::uint32_t PREVIEW_NETWORK_MAGIC = 2U;
inline constexpr std::uint32_t PREPROD_NETWORK_MAGIC = 1U;
inline constexpr std::uint32_t SANCHO_TESTNET_NETWORK_MAGIC = 4U;

class ProtocolMagic {
 public:
  explicit constexpr ProtocolMagic(std::uint32_t value) noexcept : value_(value) {}
  [[nodiscard]] constexpr std::uint32_t value() const noexcept { return value_; }
  friend bool operator==(const ProtocolMagic&, const ProtocolMagic&) = default;

 private:
  std::uint32_t value_;
};

enum class NetworkId : std::uint8_t { testnet = 0, mainnet = 1 };

}  // namespace cardano::core
