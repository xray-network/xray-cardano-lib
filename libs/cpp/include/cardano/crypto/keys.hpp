#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "cardano/core/bytes.hpp"
#include "cardano/core/error.hpp"
#include "cardano/core/random.hpp"
#include "cardano/crypto/fixed_bytes.hpp"

namespace cardano::crypto {

class PublicKey {
 public:
  [[nodiscard]] static core::Result<PublicKey> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<PublicKey> from_hex(std::string_view hex);
  [[nodiscard]] static core::Result<PublicKey> from_bech32(std::string_view encoded);

  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] std::string to_hex() const;
  [[nodiscard]] core::Result<std::string> to_bech32() const;
  [[nodiscard]] Ed25519KeyHash hash() const;
  [[nodiscard]] bool verify(core::ByteSpan message,
                            const Ed25519Signature& signature) const noexcept;
  friend bool operator==(const PublicKey&, const PublicKey&) = default;

 private:
  explicit PublicKey(std::array<core::Byte, 32> bytes);
  std::array<core::Byte, 32> bytes_;
};

class PrivateKey {
 public:
  PrivateKey(const PrivateKey&) = delete;
  PrivateKey& operator=(const PrivateKey&) = delete;
  PrivateKey(PrivateKey&& other) noexcept;
  PrivateKey& operator=(PrivateKey&& other) noexcept;
  ~PrivateKey();

  [[nodiscard]] static core::Result<PrivateKey> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<PrivateKey> from_hex(std::string_view hex);
  [[nodiscard]] static core::Result<PrivateKey> from_bech32(std::string_view encoded);
  [[nodiscard]] static core::Result<PrivateKey> generate(core::SecureRandomSource& random);
  [[nodiscard]] static core::Result<PrivateKey> generate();

  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] std::string to_hex() const;
  [[nodiscard]] core::Result<std::string> to_bech32() const;
  [[nodiscard]] core::Result<PublicKey> public_key() const;
  [[nodiscard]] core::Result<Ed25519Signature> sign(core::ByteSpan message) const;
  void clear() noexcept;

 private:
  explicit PrivateKey(std::array<core::Byte, 32> bytes);
  std::array<core::Byte, 32> bytes_{};
  bool cleared_{false};
};

class Bip32PublicKey {
 public:
  [[nodiscard]] static core::Result<Bip32PublicKey> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<Bip32PublicKey> from_hex(std::string_view hex);
  [[nodiscard]] static core::Result<Bip32PublicKey> from_bech32(std::string_view encoded);

  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] std::string to_hex() const;
  [[nodiscard]] core::Result<std::string> to_bech32() const;
  [[nodiscard]] core::Bytes chain_code() const;
  [[nodiscard]] PublicKey public_key() const;
  [[nodiscard]] core::Result<Bip32PublicKey> derive(std::uint32_t index) const;
  friend bool operator==(const Bip32PublicKey&, const Bip32PublicKey&) = default;

 private:
  explicit Bip32PublicKey(std::array<core::Byte, 64> bytes);
  std::array<core::Byte, 64> bytes_;
};

class Bip32PrivateKey {
 public:
  Bip32PrivateKey(const Bip32PrivateKey&) = delete;
  Bip32PrivateKey& operator=(const Bip32PrivateKey&) = delete;
  Bip32PrivateKey(Bip32PrivateKey&& other) noexcept;
  Bip32PrivateKey& operator=(Bip32PrivateKey&& other) noexcept;
  ~Bip32PrivateKey();

  [[nodiscard]] static core::Result<Bip32PrivateKey> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<Bip32PrivateKey> from_hex(std::string_view hex);
  [[nodiscard]] static core::Result<Bip32PrivateKey> from_bech32(std::string_view encoded);
  [[nodiscard]] static core::Result<Bip32PrivateKey> from_128_xprv(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<Bip32PrivateKey> from_entropy(core::ByteSpan entropy,
                                                                  core::ByteSpan password);
  [[nodiscard]] static core::Result<Bip32PrivateKey> generate(core::SecureRandomSource& random);
  [[nodiscard]] static core::Result<Bip32PrivateKey> generate();

  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] core::Result<core::Bytes> to_128_xprv() const;
  [[nodiscard]] std::string to_hex() const;
  [[nodiscard]] core::Result<std::string> to_bech32() const;
  [[nodiscard]] core::Bytes chain_code() const;
  [[nodiscard]] core::Result<Bip32PublicKey> public_key() const;
  [[nodiscard]] core::Result<Bip32PrivateKey> derive(std::uint32_t index) const;
  [[nodiscard]] core::Result<Ed25519Signature> sign(core::ByteSpan message) const;
  void clear() noexcept;

 private:
  explicit Bip32PrivateKey(std::array<core::Byte, 96> bytes);
  std::array<core::Byte, 96> bytes_{};
  bool cleared_{false};
};

}  // namespace cardano::crypto
