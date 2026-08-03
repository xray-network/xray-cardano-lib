#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "cardano/crypto/fixed_bytes.hpp"

namespace cardano::cip::experimental::cip129 {

enum class CredentialRole : std::uint8_t { committee_hot, committee_cold, drep };
enum class CredentialKind : std::uint8_t { key_hash, script_hash };

class CredentialId {
 public:
  [[nodiscard]] static CredentialId key(CredentialRole role, crypto::Ed25519KeyHash hash);
  [[nodiscard]] static CredentialId script(CredentialRole role, crypto::ScriptHash hash);
  [[nodiscard]] static core::Result<CredentialId> from_bech32(std::string_view value);
  [[nodiscard]] std::string to_bech32() const;
  [[nodiscard]] CredentialRole role() const noexcept;
  [[nodiscard]] CredentialKind kind() const noexcept;
  [[nodiscard]] core::Bytes hash_bytes() const;

 private:
  CredentialId(CredentialRole role, CredentialKind kind, core::Bytes hash);
  CredentialRole role_{};
  CredentialKind kind_{};
  core::Bytes hash_;
};

class GovernanceActionId {
 public:
  [[nodiscard]] static core::Result<GovernanceActionId> make(crypto::TransactionHash transaction,
                                                             std::uint64_t index);
  [[nodiscard]] static core::Result<GovernanceActionId> from_bech32(std::string_view value);
  [[nodiscard]] std::string to_bech32() const;
  [[nodiscard]] const crypto::TransactionHash& transaction() const noexcept;
  [[nodiscard]] std::uint8_t index() const noexcept;

 private:
  GovernanceActionId(crypto::TransactionHash transaction, std::uint8_t index);
  crypto::TransactionHash transaction_;
  std::uint8_t index_{};
};

struct LegacyCip105Credential {
  CredentialRole role;
  core::Bytes hash;
};
[[nodiscard]] core::Result<LegacyCip105Credential> parse_legacy_cip105(std::string_view value);

}  // namespace cardano::cip::experimental::cip129
