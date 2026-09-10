#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cardano/core/bech32.hpp"
#include "cardano/core/big_integer.hpp"
#include "cardano/core/bytes.hpp"
#include "cardano/core/error.hpp"
#include "cardano/core/network.hpp"
#include "cardano/crypto/fixed_bytes.hpp"
#include "cardano/crypto/keys.hpp"

namespace cardano::chain {

enum class AddressKind : std::uint8_t { base, pointer, enterprise, reward, byron };

enum class AddressHeaderKind : std::uint8_t {
  base_payment_key_stake_key = 0,
  base_payment_script_stake_key = 1,
  base_payment_key_stake_script = 2,
  base_payment_script_stake_script = 3,
  pointer_key = 4,
  pointer_script = 5,
  enterprise_key = 6,
  enterprise_script = 7,
  byron = 8,
  reward_key = 14,
  reward_script = 15
};

enum class CredentialKind : std::uint8_t { key, script };

class Credential {
 public:
  [[nodiscard]] static Credential key(crypto::Ed25519KeyHash hash);
  [[nodiscard]] static Credential script(crypto::ScriptHash hash);
  [[nodiscard]] static core::Result<Credential> from_json(std::string_view json);
  [[nodiscard]] CredentialKind kind() const noexcept;
  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] std::string to_json() const;
  friend bool operator==(const Credential&, const Credential&) = default;

 private:
  Credential(CredentialKind kind, std::array<core::Byte, 28> bytes);
  CredentialKind kind_;
  std::array<core::Byte, 28> bytes_;
};

struct Pointer {
  core::BigInteger slot;
  core::BigInteger transaction_index;
  core::BigInteger certificate_index;
  friend bool operator==(const Pointer&, const Pointer&) = default;
};

class Address {
 public:
  [[nodiscard]] static core::Result<Address> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<Address> from_hex(std::string_view hex);
  [[nodiscard]] static core::Result<Address> from_bech32(std::string_view encoded);
  [[nodiscard]] static core::Result<Address> from_bech32_payload_compatible(
      std::string_view encoded);
  [[nodiscard]] static bool is_valid(std::string_view encoded) noexcept;

  [[nodiscard]] AddressKind kind() const noexcept;
  [[nodiscard]] core::Result<std::uint8_t> network_id() const;
  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] std::string to_hex() const;
  [[nodiscard]] core::Result<std::string> to_bech32(
      std::optional<std::string_view> hrp = std::nullopt) const;
  [[nodiscard]] std::string to_json() const;

  friend bool operator==(const Address&, const Address&) = default;

 private:
  Address(AddressKind kind, core::Bytes bytes, std::size_t semantic_length);
  AddressKind kind_;
  core::Bytes bytes_;
  std::size_t semantic_length_;

  friend class BaseAddress;
  friend class PointerAddress;
  friend class EnterpriseAddress;
  friend class RewardAddress;
};

class BaseAddress {
 public:
  BaseAddress(std::uint8_t network, Credential payment, Credential stake);
  [[nodiscard]] static core::Result<BaseAddress> from_address(const Address& address);
  [[nodiscard]] Address to_address() const;
  [[nodiscard]] std::uint8_t network_id() const noexcept;
  [[nodiscard]] const Credential& payment_credential() const noexcept;
  [[nodiscard]] const Credential& stake_credential() const noexcept;

 private:
  std::uint8_t network_;
  Credential payment_;
  Credential stake_;
};

class PointerAddress {
 public:
  PointerAddress(std::uint8_t network, Credential payment, Pointer pointer);
  [[nodiscard]] static core::Result<PointerAddress> from_address(const Address& address);
  [[nodiscard]] core::Result<Address> to_address() const;
  [[nodiscard]] const Pointer& pointer() const noexcept;

 private:
  std::uint8_t network_;
  Credential payment_;
  Pointer pointer_;
};

class EnterpriseAddress {
 public:
  EnterpriseAddress(std::uint8_t network, Credential payment);
  [[nodiscard]] static core::Result<EnterpriseAddress> from_address(const Address& address);
  [[nodiscard]] Address to_address() const;

 private:
  std::uint8_t network_;
  Credential payment_;
};

class RewardAddress {
 public:
  RewardAddress(std::uint8_t network, Credential stake);
  [[nodiscard]] static core::Result<RewardAddress> from_address(const Address& address);
  [[nodiscard]] static core::Result<RewardAddress> from_json(std::string_view json);
  [[nodiscard]] Address to_address() const;
  [[nodiscard]] std::string to_json() const;

 private:
  std::uint8_t network_;
  Credential stake_;
};

[[nodiscard]] core::Bytes encode_variable_natural(const core::BigInteger& value);
[[nodiscard]] core::Result<std::pair<core::BigInteger, std::size_t>> decode_variable_natural(
    core::ByteSpan bytes);

[[nodiscard]] std::uint32_t crc32(core::ByteSpan bytes) noexcept;
[[nodiscard]] std::string encode_base58(core::ByteSpan bytes);
[[nodiscard]] core::Result<core::Bytes> decode_base58(std::string_view encoded);

enum class ByronAddrType : std::uint8_t { public_key = 0, script = 1, redeem = 2 };

enum class SpendingDataKind : std::uint8_t { public_key = 0, script = 1, redeem = 2 };

enum class StakeDistributionKind : std::uint8_t { single_key = 0, bootstrap_era = 1 };

struct ByronScriptDomain;
struct StakeholderIdDomain;
struct AddressIdDomain;
using ByronScript = crypto::FixedBytes<32, ByronScriptDomain>;
using StakeholderId = crypto::FixedBytes<28, StakeholderIdDomain>;
using AddressId = crypto::FixedBytes<28, AddressIdDomain>;

class HDAddressPayload {
 public:
  explicit HDAddressPayload(core::Bytes bytes);
  [[nodiscard]] core::Bytes get() const;
  friend bool operator==(const HDAddressPayload&, const HDAddressPayload&) = default;

 private:
  core::Bytes bytes_;
};

class SpendingData {
 public:
  [[nodiscard]] static SpendingData public_key(const crypto::Bip32PublicKey& value);
  [[nodiscard]] static SpendingData script(const ByronScript& value);
  [[nodiscard]] static SpendingData redeem(const crypto::PublicKey& value);
  [[nodiscard]] static core::Result<SpendingData> from_cbor(core::ByteSpan bytes);

  [[nodiscard]] SpendingDataKind kind() const noexcept;
  [[nodiscard]] core::Bytes bytes() const;
  [[nodiscard]] core::Result<core::Bytes> to_cbor() const;
  friend bool operator==(const SpendingData&, const SpendingData&) = default;

 private:
  SpendingData(SpendingDataKind kind, core::Bytes bytes);
  SpendingDataKind kind_;
  core::Bytes bytes_;
};

class StakeDistribution {
 public:
  [[nodiscard]] static StakeDistribution single_key(StakeholderId stakeholder);
  [[nodiscard]] static StakeDistribution bootstrap_era();
  [[nodiscard]] static core::Result<StakeDistribution> from_cbor(core::ByteSpan bytes);

  [[nodiscard]] StakeDistributionKind kind() const noexcept;
  [[nodiscard]] const std::optional<StakeholderId>& stakeholder() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_cbor() const;
  friend bool operator==(const StakeDistribution&, const StakeDistribution&) = default;

 private:
  explicit StakeDistribution(std::optional<StakeholderId> stakeholder);
  std::optional<StakeholderId> stakeholder_;
};

class AddrAttributes {
 public:
  AddrAttributes() = default;
  [[nodiscard]] static AddrAttributes bootstrap_era(
      std::optional<HDAddressPayload> derivation_path = std::nullopt,
      std::optional<core::ProtocolMagic> protocol_magic = std::nullopt);
  [[nodiscard]] static AddrAttributes single_key(const crypto::Bip32PublicKey& public_key,
                                                 std::optional<HDAddressPayload> derivation_path,
                                                 core::ProtocolMagic protocol_magic);
  [[nodiscard]] static core::Result<AddrAttributes> from_cbor(core::ByteSpan bytes);

  [[nodiscard]] const std::optional<StakeDistribution>& stake_distribution() const noexcept;
  [[nodiscard]] const std::optional<HDAddressPayload>& derivation_path() const noexcept;
  [[nodiscard]] std::optional<core::ProtocolMagic> protocol_magic() const noexcept;
  void set_stake_distribution(StakeDistribution value);
  void set_derivation_path(HDAddressPayload value);
  void set_protocol_magic(core::ProtocolMagic value);
  [[nodiscard]] core::Result<core::Bytes> to_cbor() const;
  friend bool operator==(const AddrAttributes&, const AddrAttributes&) = default;

 private:
  std::optional<StakeDistribution> stake_distribution_;
  std::optional<HDAddressPayload> derivation_path_;
  std::optional<std::uint32_t> protocol_magic_;
};

class AddressContent {
 public:
  [[nodiscard]] static core::Result<AddressContent> from_cbor(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<AddressContent> create(AddressId address_id,
                                                           AddrAttributes attributes,
                                                           ByronAddrType kind);
  [[nodiscard]] static core::Result<AddressContent> hash_and_create(
      ByronAddrType kind, const SpendingData& spending_data, const AddrAttributes& attributes);
  [[nodiscard]] static core::Result<AddressContent> redeem(
      const crypto::PublicKey& public_key,
      std::optional<core::ProtocolMagic> protocol_magic = std::nullopt);
  [[nodiscard]] static core::Result<AddressContent> simple(
      const crypto::Bip32PublicKey& public_key,
      std::optional<core::ProtocolMagic> protocol_magic = std::nullopt);
  [[nodiscard]] static core::Result<AddressContent> icarus_from_key(
      const crypto::Bip32PublicKey& public_key, core::ProtocolMagic protocol_magic);

  [[nodiscard]] const AddressId& address_id() const noexcept;
  [[nodiscard]] const AddrAttributes& attributes() const noexcept;
  [[nodiscard]] ByronAddrType type() const noexcept;
  [[nodiscard]] core::ProtocolMagic protocol_magic() const noexcept;
  [[nodiscard]] std::optional<std::uint8_t> network_id() const noexcept;
  [[nodiscard]] core::Result<bool> identical_with(const crypto::Bip32PublicKey& public_key) const;
  [[nodiscard]] core::Result<core::Bytes> to_cbor() const;
  friend bool operator==(const AddressContent&, const AddressContent&) = default;

 private:
  AddressContent(AddressId address_id, AddrAttributes attributes, ByronAddrType kind);
  AddressId address_id_;
  AddrAttributes attributes_;
  ByronAddrType kind_;
};

class Crc32 {
 public:
  Crc32() = default;
  void update(core::ByteSpan bytes) noexcept;
  [[nodiscard]] std::uint32_t finalize() const noexcept;

 private:
  std::uint32_t state_{0xffffffffU};
};

class ByronAddress {
 public:
  [[nodiscard]] static core::Result<ByronAddress> create(const AddressContent& content,
                                                         std::uint32_t checksum);
  [[nodiscard]] static core::Result<ByronAddress> from_content(const AddressContent& content);
  [[nodiscard]] static core::Result<ByronAddress> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<ByronAddress> from_base58(std::string_view encoded);
  [[nodiscard]] static core::Result<ByronAddress> from_address(const Address& address);

  [[nodiscard]] core::Result<AddressContent> content() const;
  [[nodiscard]] std::uint32_t checksum() const noexcept;
  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] std::string to_base58() const;
  [[nodiscard]] Address to_address() const;
  [[nodiscard]] std::uint32_t protocol_magic() const noexcept;
  [[nodiscard]] std::optional<std::uint8_t> network_id() const noexcept;

 private:
  ByronAddress(core::Bytes envelope, std::uint32_t protocol_magic, std::uint32_t checksum);
  core::Bytes envelope_;
  std::uint32_t protocol_magic_;
  std::uint32_t checksum_;
};

}  // namespace cardano::chain
