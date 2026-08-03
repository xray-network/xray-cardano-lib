#include "cardano/crypto/identity.hpp"

#include "cardano/core/bech32.hpp"

namespace cardano::crypto {
namespace {
std::string stem(KeyTextRole role) {
  switch (role) {
    case KeyTextRole::root:
      return "root";
    case KeyTextRole::account:
      return "acct";
    case KeyTextRole::payment:
      return "addr";
    case KeyTextRole::stake:
      return "stake";
    case KeyTextRole::drep:
      return "drep";
    case KeyTextRole::cc_cold:
      return "cc_cold";
    case KeyTextRole::cc_hot:
      return "cc_hot";
  }
  return {};
}
core::Result<core::Bytes> decode(KeyTextRole role, std::string_view suffix,
                                 std::string_view value) {
  auto result = core::decode_bech32(value);
  if (!result) return std::unexpected(result.error());
  if (result->prefix != stem(role) + std::string(suffix))
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "Cardano key role or form mismatch"));
  return result->bytes;
}
}  // namespace
core::Result<std::string> encode_cardano_public_key(KeyTextRole role, const PublicKey& key) {
  return core::encode_bech32(stem(role) + "_vk", key.to_bytes());
}
core::Result<PublicKey> decode_cardano_public_key(KeyTextRole role, std::string_view value) {
  auto bytes = decode(role, "_vk", value);
  return bytes ? PublicKey::from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Result<std::string> encode_cardano_private_key(KeyTextRole role, const PrivateKey& key) {
  if (key.form() == PrivateKeyForm::extended && role != KeyTextRole::drep &&
      role != KeyTextRole::cc_cold && role != KeyTextRole::cc_hot)
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_argument,
                           "64-byte extended *_sk is limited to DRep and committee roles"));
  return core::encode_bech32(stem(role) + "_sk", key.to_bytes());
}
core::Result<PrivateKey> decode_cardano_private_key(KeyTextRole role, PrivateKeyForm form,
                                                    std::string_view value) {
  if (form == PrivateKeyForm::extended && role != KeyTextRole::drep &&
      role != KeyTextRole::cc_cold && role != KeyTextRole::cc_hot)
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_argument,
                           "64-byte extended *_sk is limited to DRep and committee roles"));
  auto bytes = decode(role, "_sk", value);
  if (!bytes) return std::unexpected(bytes.error());
  return form == PrivateKeyForm::normal ? PrivateKey::from_normal_bytes(*bytes)
                                        : PrivateKey::from_extended_bytes(*bytes);
}
core::Result<std::string> encode_cardano_bip32_public_key(KeyTextRole role,
                                                          const Bip32PublicKey& key) {
  return core::encode_bech32(stem(role) + "_xvk", key.to_bytes());
}
core::Result<Bip32PublicKey> decode_cardano_bip32_public_key(KeyTextRole role,
                                                             std::string_view value) {
  auto bytes = decode(role, "_xvk", value);
  return bytes ? Bip32PublicKey::from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Result<std::string> encode_cardano_bip32_private_key(KeyTextRole role,
                                                           const Bip32PrivateKey& key) {
  return core::encode_bech32(stem(role) + "_xsk", key.to_bytes());
}
core::Result<Bip32PrivateKey> decode_cardano_bip32_private_key(KeyTextRole role,
                                                               std::string_view value) {
  auto bytes = decode(role, "_xsk", value);
  return bytes ? Bip32PrivateKey::from_bytes(*bytes) : std::unexpected(bytes.error());
}
}  // namespace cardano::crypto
