#pragma once

#include "cardano/crypto/keys.hpp"

namespace cardano::crypto {

enum class KeyTextRole : std::uint8_t { root, account, payment, stake, drep, cc_cold, cc_hot };

[[nodiscard]] core::Result<std::string> encode_cardano_public_key(KeyTextRole role,
                                                                  const PublicKey& key);
[[nodiscard]] core::Result<PublicKey> decode_cardano_public_key(KeyTextRole role,
                                                                std::string_view value);
[[nodiscard]] core::Result<std::string> encode_cardano_private_key(KeyTextRole role,
                                                                   const PrivateKey& key);
[[nodiscard]] core::Result<PrivateKey> decode_cardano_private_key(KeyTextRole role,
                                                                  PrivateKeyForm form,
                                                                  std::string_view value);
[[nodiscard]] core::Result<std::string> encode_cardano_bip32_public_key(KeyTextRole role,
                                                                        const Bip32PublicKey& key);
[[nodiscard]] core::Result<Bip32PublicKey> decode_cardano_bip32_public_key(KeyTextRole role,
                                                                           std::string_view value);
[[nodiscard]] core::Result<std::string> encode_cardano_bip32_private_key(
    KeyTextRole role, const Bip32PrivateKey& key);
[[nodiscard]] core::Result<Bip32PrivateKey> decode_cardano_bip32_private_key(
    KeyTextRole role, std::string_view value);

}  // namespace cardano::crypto
