#include "cardano/cip/cip14.hpp"

#include <algorithm>
#include <cctype>

#include "cardano/core/bech32.hpp"
#include "cardano/crypto/primitives.hpp"

namespace cardano::cip {

AssetFingerprint::AssetFingerprint(std::array<core::Byte, 20> bytes) : bytes_(bytes) {}

AssetFingerprint AssetFingerprint::from_asset(const crypto::ScriptHash& policy,
                                              const chain::AssetName& asset_name) {
  core::Bytes input;
  input.reserve(policy.span().size() + asset_name.bytes().size());
  input.insert(input.end(), policy.span().begin(), policy.span().end());
  input.insert(input.end(), asset_name.bytes().begin(), asset_name.bytes().end());
  const auto digest = crypto::blake2b160(input);
  std::array<core::Byte, 20> bytes{};
  std::ranges::copy(digest, bytes.begin());
  return AssetFingerprint(bytes);
}

core::Result<AssetFingerprint> AssetFingerprint::from_bech32(std::string_view value) {
  if (std::ranges::any_of(value, [](char character) {
        return std::isupper(static_cast<unsigned char>(character)) != 0;
      })) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "CIP-14 fingerprint must be lowercase"));
  }
  auto decoded = core::decode_bech32(value);
  if (!decoded) return std::unexpected(decoded.error());
  if (decoded->prefix != "asset") {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "CIP-14 fingerprint HRP must be asset"));
  }
  if (decoded->bytes.size() != 20) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "CIP-14 fingerprint must contain 20 bytes"));
  }
  std::array<core::Byte, 20> bytes{};
  std::ranges::copy(decoded->bytes, bytes.begin());
  return AssetFingerprint(bytes);
}

core::Bytes AssetFingerprint::to_bytes() const { return {bytes_.begin(), bytes_.end()}; }
std::string AssetFingerprint::to_bech32() const { return *core::encode_bech32("asset", bytes_); }

}  // namespace cardano::cip
