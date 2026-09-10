#include "cardano/crypto/derivation.hpp"

namespace cardano::crypto {
namespace {
bool soft(std::uint32_t value) { return value < 0x80000000U; }
bool role_ok(Cip1852Role role) { return static_cast<std::uint32_t>(role) <= 5U; }
}  // namespace
Cip1852Path::Cip1852Path(std::uint32_t account, Cip1852Role role, std::uint32_t index)
    : account_(account), index_(index), role_(role) {}
core::Result<Cip1852Path> Cip1852Path::make(std::uint32_t account, Cip1852Role role,
                                            std::uint32_t index) {
  if (!soft(account) || !soft(index) || !role_ok(role))
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_argument,
                                              "invalid CIP-1852 account, role, or index"));
  return Cip1852Path(account, role, index);
}
std::uint32_t Cip1852Path::account() const noexcept { return account_; }
Cip1852Role Cip1852Path::role() const noexcept { return role_; }
std::uint32_t Cip1852Path::index() const noexcept { return index_; }
core::Result<Bip32PrivateKey> Cip1852Path::derive_private(const Bip32PrivateKey& root) const {
  auto purpose = root.derive(0x80000000U | 1852U);
  if (!purpose) return std::unexpected(purpose.error());
  auto coin = purpose->derive(0x80000000U | 1815U);
  if (!coin) return std::unexpected(coin.error());
  auto account = coin->derive(0x80000000U | account_);
  if (!account) return std::unexpected(account.error());
  auto role = account->derive(static_cast<std::uint32_t>(role_));
  if (!role) return std::unexpected(role.error());
  return role->derive(index_);
}
Cip1852Account::Cip1852Account(Bip32PrivateKey private_key)
    : private_key_(std::move(private_key)) {}
core::Result<Cip1852Account> Cip1852Account::from_root(const Bip32PrivateKey& root,
                                                       std::uint32_t account) {
  if (!soft(account))
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_argument,
                                              "CIP-1852 account must be below 2^31"));
  auto purpose = root.derive(0x80000000U | 1852U);
  if (!purpose) return std::unexpected(purpose.error());
  auto coin = purpose->derive(0x80000000U | 1815U);
  if (!coin) return std::unexpected(coin.error());
  auto key = coin->derive(0x80000000U | account);
  return key ? core::Result<Cip1852Account>(Cip1852Account(std::move(*key)))
             : std::unexpected(key.error());
}
core::Result<Bip32PrivateKey> Cip1852Account::derive_private(Cip1852Role role,
                                                             std::uint32_t index) const {
  if (!role_ok(role) || !soft(index))
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_argument, "invalid CIP-1852 role or index"));
  auto role_key = private_key_.derive(static_cast<std::uint32_t>(role));
  return role_key ? role_key->derive(index) : std::unexpected(role_key.error());
}
core::Result<Bip32PublicKey> Cip1852Account::derive_public(Cip1852Role role,
                                                           std::uint32_t index) const {
  auto account = private_key_.public_key();
  return account ? derive_cip1852_public(*account, role, index) : std::unexpected(account.error());
}
core::Result<Bip32PublicKey> Cip1852Account::public_key() const {
  return private_key_.public_key();
}
core::Result<Bip32PublicKey> derive_cip1852_public(const Bip32PublicKey& account, Cip1852Role role,
                                                   std::uint32_t index) {
  if (!role_ok(role) || !soft(index))
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_argument, "invalid CIP-1852 role or index"));
  auto role_key = account.derive(static_cast<std::uint32_t>(role));
  return role_key ? role_key->derive(index) : std::unexpected(role_key.error());
}
}  // namespace cardano::crypto
