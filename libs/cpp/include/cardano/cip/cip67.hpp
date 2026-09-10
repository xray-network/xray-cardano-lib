#pragma once

#include <array>
#include <cstdint>

#include "cardano/chain/builder.hpp"

namespace cardano::cip::experimental::cip67 {

class AssetNameLabel {
 public:
  explicit AssetNameLabel(std::uint16_t value) : value_(value) {}
  [[nodiscard]] std::uint16_t value() const noexcept { return value_; }
  friend bool operator==(const AssetNameLabel&, const AssetNameLabel&) = default;

 private:
  std::uint16_t value_{};
};

struct LabelledAssetName {
  AssetNameLabel label;
  core::Bytes content;
  friend bool operator==(const LabelledAssetName&, const LabelledAssetName&) = default;
};

[[nodiscard]] std::array<core::Byte, 4> encode(AssetNameLabel label) noexcept;
[[nodiscard]] core::Result<AssetNameLabel> decode(core::ByteSpan bytes);
[[nodiscard]] core::Result<chain::AssetName> make_asset_name(AssetNameLabel label,
                                                             core::ByteSpan content);
[[nodiscard]] core::Result<LabelledAssetName> split_asset_name(const chain::AssetName& name);

}  // namespace cardano::cip::experimental::cip67
