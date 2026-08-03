#include "cardano/cip/cip129.hpp"

#include "cardano/core/bech32.hpp"

namespace cardano::cip::experimental::cip129 {
namespace {
std::string_view hrp(CredentialRole role) {
  switch (role) {
    case CredentialRole::committee_hot:
      return "cc_hot";
    case CredentialRole::committee_cold:
      return "cc_cold";
    case CredentialRole::drep:
      return "drep";
  }
  return "";
}
std::uint8_t high(CredentialRole role) {
  return role == CredentialRole::committee_hot ? 0 : role == CredentialRole::committee_cold ? 1 : 2;
}
core::Result<CredentialRole> parse_role(std::uint8_t header) {
  switch (header >> 4U) {
    case 0:
      return CredentialRole::committee_hot;
    case 1:
      return CredentialRole::committee_cold;
    case 2:
      return CredentialRole::drep;
    default:
      return std::unexpected(
          core::CardanoError(core::ErrorCode::invalid_encoding, "unknown CIP-129 role"));
  }
}
}  // namespace
CredentialId::CredentialId(CredentialRole role, CredentialKind kind, core::Bytes hash)
    : role_(role), kind_(kind), hash_(std::move(hash)) {}
CredentialId CredentialId::key(CredentialRole role, crypto::Ed25519KeyHash hash) {
  return CredentialId(role, CredentialKind::key_hash, hash.to_bytes());
}
CredentialId CredentialId::script(CredentialRole role, crypto::ScriptHash hash) {
  return CredentialId(role, CredentialKind::script_hash, hash.to_bytes());
}
core::Result<CredentialId> CredentialId::from_bech32(std::string_view value) {
  auto decoded = core::decode_bech32(value);
  if (!decoded) return std::unexpected(decoded.error());
  if (decoded->bytes.size() != 29)
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "CIP-129 credential must contain 29 bytes"));
  const auto header = static_cast<std::uint8_t>(decoded->bytes[0]);
  const auto low = static_cast<std::uint8_t>(header & 0x0fU);
  auto parsed_role = parse_role(header);
  if (!parsed_role || (low != 2U && low != 3U) || decoded->prefix != hrp(*parsed_role))
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "CIP-129 role, kind, and HRP must agree"));
  return CredentialId(*parsed_role,
                      low == 2U ? CredentialKind::key_hash : CredentialKind::script_hash,
                      core::Bytes(decoded->bytes.begin() + 1, decoded->bytes.end()));
}
std::string CredentialId::to_bech32() const {
  core::Bytes bytes{
      static_cast<core::Byte>((high(role_) << 4U) | (kind_ == CredentialKind::key_hash ? 2U : 3U))};
  bytes.insert(bytes.end(), hash_.begin(), hash_.end());
  return *core::encode_bech32(hrp(role_), bytes);
}
CredentialRole CredentialId::role() const noexcept { return role_; }
CredentialKind CredentialId::kind() const noexcept { return kind_; }
core::Bytes CredentialId::hash_bytes() const { return hash_; }
GovernanceActionId::GovernanceActionId(crypto::TransactionHash transaction, std::uint8_t index)
    : transaction_(transaction), index_(index) {}
core::Result<GovernanceActionId> GovernanceActionId::make(crypto::TransactionHash transaction,
                                                          std::uint64_t index) {
  if (index > 255U)
    return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                              "CIP-129 governance action index must be in 0..255"));
  return GovernanceActionId(transaction, static_cast<std::uint8_t>(index));
}
core::Result<GovernanceActionId> GovernanceActionId::from_bech32(std::string_view value) {
  auto decoded = core::decode_bech32(value);
  if (!decoded) return std::unexpected(decoded.error());
  if (decoded->prefix != "gov_action" || decoded->bytes.size() != 33)
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "invalid CIP-129 governance action"));
  auto tx = crypto::TransactionHash::from_bytes(core::ByteSpan(decoded->bytes).first(32));
  if (!tx) return std::unexpected(tx.error());
  return GovernanceActionId(*tx, static_cast<std::uint8_t>(decoded->bytes[32]));
}
std::string GovernanceActionId::to_bech32() const {
  auto bytes = transaction_.to_bytes();
  bytes.push_back(static_cast<core::Byte>(index_));
  return *core::encode_bech32("gov_action", bytes);
}
const crypto::TransactionHash& GovernanceActionId::transaction() const noexcept {
  return transaction_;
}
std::uint8_t GovernanceActionId::index() const noexcept { return index_; }
core::Result<LegacyCip105Credential> parse_legacy_cip105(std::string_view value) {
  auto decoded = core::decode_bech32(value);
  if (!decoded) return std::unexpected(decoded.error());
  CredentialRole parsed;
  if (decoded->prefix == "cc_hot")
    parsed = CredentialRole::committee_hot;
  else if (decoded->prefix == "cc_cold")
    parsed = CredentialRole::committee_cold;
  else if (decoded->prefix == "drep")
    parsed = CredentialRole::drep;
  else
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "unknown CIP-105 HRP"));
  if (decoded->bytes.size() != 28)
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "CIP-105 credential must contain 28 bytes"));
  return LegacyCip105Credential{parsed, decoded->bytes};
}
}  // namespace cardano::cip::experimental::cip129
