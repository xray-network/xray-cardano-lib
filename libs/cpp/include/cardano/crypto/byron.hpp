#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "cardano/core/big_integer.hpp"
#include "cardano/core/bytes.hpp"
#include "cardano/core/error.hpp"
#include "cardano/crypto/fixed_bytes.hpp"
#include "cardano/crypto/keys.hpp"

namespace cardano::crypto {

class LegacyDaedalusPrivateKey {
 public:
  LegacyDaedalusPrivateKey(const LegacyDaedalusPrivateKey&) = delete;
  LegacyDaedalusPrivateKey& operator=(const LegacyDaedalusPrivateKey&) = delete;
  LegacyDaedalusPrivateKey(LegacyDaedalusPrivateKey&& other) noexcept;
  LegacyDaedalusPrivateKey& operator=(LegacyDaedalusPrivateKey&& other) noexcept;
  ~LegacyDaedalusPrivateKey();

  [[nodiscard]] static core::Result<LegacyDaedalusPrivateKey> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] core::Result<core::Bytes> to_bytes() const;
  [[nodiscard]] core::Result<core::Bytes> chain_code() const;
  void clear() noexcept;

 private:
  explicit LegacyDaedalusPrivateKey(std::array<core::Byte, 96> bytes);
  std::array<core::Byte, 96> bytes_{};
  bool cleared_{false};

  friend core::Result<core::Bytes> legacy_public_key(const LegacyDaedalusPrivateKey&);
  friend core::Result<Ed25519Signature> legacy_sign(const LegacyDaedalusPrivateKey&,
                                                    core::ByteSpan);
};

[[nodiscard]] core::Result<core::Bytes> legacy_public_key(const LegacyDaedalusPrivateKey& key);
[[nodiscard]] core::Result<Ed25519Signature> legacy_sign(const LegacyDaedalusPrivateKey& key,
                                                         core::ByteSpan message);

enum class ByronSigningTag : std::uint8_t {
  transaction = 0x01,
  redeem_transaction = 0x02,
  vss_certificate = 0x03,
  update_proposal = 0x04,
  commitment = 0x05,
  update_vote = 0x06,
  main_block = 0x07,
  main_block_light = 0x08,
  main_block_heavy = 0x09,
  proxy_secret_key = 0x0a
};

[[nodiscard]] core::Bytes byron_proxy_signing_data(const Bip32PublicKey& delegate,
                                                   std::uint64_t omega,
                                                   std::uint32_t protocol_magic);
[[nodiscard]] core::Result<Ed25519Signature> sign_byron_proxy_certificate(
    const Bip32PrivateKey& issuer, const Bip32PublicKey& delegate, std::uint64_t omega,
    std::uint32_t protocol_magic);
[[nodiscard]] bool verify_byron_proxy_certificate(const Bip32PublicKey& issuer,
                                                  const Bip32PublicKey& delegate,
                                                  std::uint64_t omega, std::uint32_t protocol_magic,
                                                  const Ed25519Signature& certificate) noexcept;

class AborEncoder {
 public:
  [[nodiscard]] core::VoidResult u8(std::uint8_t value);
  [[nodiscard]] core::VoidResult u16(std::uint16_t value);
  [[nodiscard]] core::VoidResult u32(std::uint32_t value);
  [[nodiscard]] core::VoidResult u64(std::uint64_t value);
  [[nodiscard]] core::VoidResult u128(const core::BigInteger& value);
  [[nodiscard]] core::VoidResult bytes(core::ByteSpan value);
  [[nodiscard]] core::VoidResult struct_start();
  [[nodiscard]] core::VoidResult struct_end();
  [[nodiscard]] core::Result<core::Bytes> finalize() const;

 private:
  struct ArrayMarker {
    std::size_t byte_offset{};
    std::size_t token_start{};
  };
  [[nodiscard]] core::VoidResult integer(std::uint8_t tag, const core::BigInteger& value,
                                         std::size_t width);
  core::Bytes bytes_;
  std::vector<ArrayMarker> arrays_;
  std::size_t tokens_{};
};

class AborDecoder {
 public:
  explicit AborDecoder(core::ByteSpan bytes);
  [[nodiscard]] core::Result<std::uint8_t> u8();
  [[nodiscard]] core::Result<std::uint16_t> u16();
  [[nodiscard]] core::Result<std::uint32_t> u32();
  [[nodiscard]] core::Result<std::uint64_t> u64();
  [[nodiscard]] core::Result<core::BigInteger> u128();
  [[nodiscard]] core::Result<core::Bytes> bytes();
  [[nodiscard]] core::Result<std::uint8_t> array();
  [[nodiscard]] core::VoidResult end() const;

 private:
  [[nodiscard]] core::Result<core::ByteSpan> take(std::size_t length);
  [[nodiscard]] core::Result<core::BigInteger> integer(std::uint8_t tag, std::size_t width);
  core::Bytes bytes_;
  std::size_t offset_{};
};

}  // namespace cardano::crypto
