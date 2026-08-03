#include "cardano/cip/cip67.hpp"

namespace cardano::cip::experimental::cip67 {
namespace {
std::uint8_t crc8(std::uint8_t high, std::uint8_t low) noexcept {
  std::uint8_t crc = 0;
  for (const auto byte : {high, low}) {
    crc ^= byte;
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = static_cast<std::uint8_t>((crc & 0x80U) != 0U ? (crc << 1U) ^ 0x07U : crc << 1U);
  }
  return crc;
}
std::uint8_t byte(core::Byte value) noexcept { return static_cast<std::uint8_t>(value); }
}  // namespace

std::array<core::Byte, 4> encode(AssetNameLabel label) noexcept {
  const auto value = label.value();
  const auto high = static_cast<std::uint8_t>(value >> 8U);
  const auto low = static_cast<std::uint8_t>(value);
  const auto checksum = crc8(high, low);
  return {static_cast<core::Byte>(high >> 4U),
          static_cast<core::Byte>(((high & 0x0fU) << 4U) | (low >> 4U)),
          static_cast<core::Byte>(((low & 0x0fU) << 4U) | (checksum >> 4U)),
          static_cast<core::Byte>((checksum & 0x0fU) << 4U)};
}
core::Result<AssetNameLabel> decode(core::ByteSpan bytes) {
  if (bytes.size() != 4)
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "CIP-67 label must contain four bytes"));
  if ((byte(bytes[0]) & 0xf0U) != 0U || (byte(bytes[3]) & 0x0fU) != 0U)
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "CIP-67 bracket mismatch"));
  const auto high =
      static_cast<std::uint8_t>(((byte(bytes[0]) & 0x0fU) << 4U) | (byte(bytes[1]) >> 4U));
  const auto low =
      static_cast<std::uint8_t>(((byte(bytes[1]) & 0x0fU) << 4U) | (byte(bytes[2]) >> 4U));
  const auto checksum =
      static_cast<std::uint8_t>(((byte(bytes[2]) & 0x0fU) << 4U) | (byte(bytes[3]) >> 4U));
  if (checksum != crc8(high, low))
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "CIP-67 checksum mismatch"));
  return AssetNameLabel(static_cast<std::uint16_t>((high << 8U) | low));
}
core::Result<chain::AssetName> make_asset_name(AssetNameLabel label, core::ByteSpan content) {
  if (content.size() > 28)
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_length, "CIP-67 content exceeds 28 bytes"));
  const auto prefix = encode(label);
  core::Bytes bytes(prefix.begin(), prefix.end());
  bytes.insert(bytes.end(), content.begin(), content.end());
  return chain::AssetName::from_bytes(bytes);
}
core::Result<LabelledAssetName> split_asset_name(const chain::AssetName& name) {
  if (name.bytes().size() < 4)
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_length, "CIP-67 asset name is too short"));
  auto label = decode(core::ByteSpan(name.bytes()).first(4));
  if (!label) return std::unexpected(label.error());
  return LabelledAssetName{*label, core::Bytes(name.bytes().begin() + 4, name.bytes().end())};
}
}  // namespace cardano::cip::experimental::cip67
