#pragma once

#include "cardano/crypto/keys.hpp"

namespace cardano::crypto {

enum class Cip1852Role : std::uint32_t {
  external = 0,
  internal = 1,
  staking = 2,
  drep = 3,
  cc_cold = 4,
  cc_hot = 5
};

class Cip1852Path {
 public:
  [[nodiscard]] static core::Result<Cip1852Path> make(std::uint32_t account, Cip1852Role role,
                                                      std::uint32_t index);
  [[nodiscard]] std::uint32_t account() const noexcept;
  [[nodiscard]] Cip1852Role role() const noexcept;
  [[nodiscard]] std::uint32_t index() const noexcept;
  [[nodiscard]] core::Result<Bip32PrivateKey> derive_private(const Bip32PrivateKey& root) const;

 private:
  Cip1852Path(std::uint32_t account, Cip1852Role role, std::uint32_t index);
  std::uint32_t account_{}, index_{};
  Cip1852Role role_{};
};

class Cip1852Account {
 public:
  [[nodiscard]] static core::Result<Cip1852Account> from_root(const Bip32PrivateKey& root,
                                                              std::uint32_t account);
  [[nodiscard]] core::Result<Bip32PrivateKey> derive_private(Cip1852Role role,
                                                             std::uint32_t index) const;
  [[nodiscard]] core::Result<Bip32PublicKey> derive_public(Cip1852Role role,
                                                           std::uint32_t index) const;
  [[nodiscard]] core::Result<Bip32PublicKey> public_key() const;

 private:
  Cip1852Account(Bip32PrivateKey private_key);
  Bip32PrivateKey private_key_;
};

[[nodiscard]] core::Result<Bip32PublicKey> derive_cip1852_public(const Bip32PublicKey& account,
                                                                 Cip1852Role role,
                                                                 std::uint32_t index);

}  // namespace cardano::crypto
