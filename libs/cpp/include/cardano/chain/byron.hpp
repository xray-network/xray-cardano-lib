#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "cardano/chain/address.hpp"
#include "cardano/core/error.hpp"
#include "cardano/crypto/byron.hpp"
#include "cardano/crypto/fixed_bytes.hpp"
#include "cardano/crypto/keys.hpp"

namespace cardano::chain {

class Vkeywitness {
 public:
  Vkeywitness(crypto::PublicKey public_key, crypto::Ed25519Signature signature);
  [[nodiscard]] static core::Result<Vkeywitness> from_cbor(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<Vkeywitness> from_json(std::string_view json);
  [[nodiscard]] const crypto::PublicKey& public_key() const noexcept;
  [[nodiscard]] const crypto::Ed25519Signature& signature() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_cbor() const;
  [[nodiscard]] std::string to_json() const;
  friend bool operator==(const Vkeywitness&, const Vkeywitness&) = default;

 private:
  crypto::PublicKey public_key_;
  crypto::Ed25519Signature signature_;
};

using VkeywitnessList = std::vector<Vkeywitness>;
using NonEmptyVkeywitnessList = std::vector<Vkeywitness>;

class BootstrapWitness {
 public:
  [[nodiscard]] static core::Result<BootstrapWitness> create(crypto::PublicKey public_key,
                                                             crypto::Ed25519Signature signature,
                                                             core::ByteSpan chain_code,
                                                             AddrAttributes attributes);
  [[nodiscard]] static core::Result<BootstrapWitness> from_cbor(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<BootstrapWitness> from_json(std::string_view json);

  [[nodiscard]] const crypto::PublicKey& public_key() const noexcept;
  [[nodiscard]] const crypto::Ed25519Signature& signature() const noexcept;
  [[nodiscard]] core::Bytes chain_code() const;
  [[nodiscard]] const AddrAttributes& attributes() const noexcept;
  [[nodiscard]] core::Result<AddressContent> to_address_content() const;
  [[nodiscard]] core::Result<core::Bytes> to_cbor() const;
  [[nodiscard]] std::string to_json() const;
  friend bool operator==(const BootstrapWitness&, const BootstrapWitness&) = default;

 private:
  BootstrapWitness(crypto::PublicKey public_key, crypto::Ed25519Signature signature,
                   std::array<core::Byte, 32> chain_code, AddrAttributes attributes);

  crypto::PublicKey public_key_;
  crypto::Ed25519Signature signature_;
  std::array<core::Byte, 32> chain_code_;
  AddrAttributes attributes_;
};

using BootstrapWitnessList = std::vector<BootstrapWitness>;
using NonEmptyBootstrapWitnessList = std::vector<BootstrapWitness>;

class ByronGenesisRedeem {
 public:
  ByronGenesisRedeem(crypto::TransactionHash transaction_id, ByronAddress address);
  [[nodiscard]] const crypto::TransactionHash& transaction_id() const noexcept;
  [[nodiscard]] const ByronAddress& address() const noexcept;

 private:
  crypto::TransactionHash transaction_id_;
  ByronAddress address_;
};

[[nodiscard]] core::Result<ByronGenesisRedeem> genesis_txid_byron(
    const crypto::PublicKey& public_key,
    std::optional<core::ProtocolMagic> protocol_magic = std::nullopt);
[[nodiscard]] core::Result<BootstrapWitness> make_icarus_bootstrap_witness(
    const crypto::TransactionHash& transaction_body_hash, const ByronAddress& address,
    const crypto::Bip32PrivateKey& key);
[[nodiscard]] core::Result<BootstrapWitness> make_daedalus_bootstrap_witness(
    const crypto::TransactionHash& transaction_body_hash, const ByronAddress& address,
    const crypto::LegacyDaedalusPrivateKey& key);

}  // namespace cardano::chain
