#pragma once

#include <array>
#include <compare>
#include <string>
#include <string_view>

#include "cardano/chain/builder.hpp"
#include "cardano/core/error.hpp"
#include "cardano/crypto/fixed_bytes.hpp"

namespace cardano::cip {

class AssetFingerprint {
 public:
  [[nodiscard]] static AssetFingerprint from_asset(const crypto::ScriptHash& policy,
                                                   const chain::AssetName& asset_name);
  [[nodiscard]] static core::Result<AssetFingerprint> from_bech32(std::string_view value);
  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] std::string to_bech32() const;
  friend bool operator==(const AssetFingerprint&, const AssetFingerprint&) = default;
  friend auto operator<=>(const AssetFingerprint&, const AssetFingerprint&) = default;

 private:
  explicit AssetFingerprint(std::array<core::Byte, 20> bytes);
  std::array<core::Byte, 20> bytes_{};
};

}  // namespace cardano::cip
