#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cardano/chain/address.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/crypto/fixed_bytes.hpp"
#include "cardano/crypto/keys.hpp"

namespace cardano::cip::cip36 {

inline constexpr std::uint64_t REGISTRATION_LABEL = 61284;
inline constexpr std::uint64_t WITNESS_LABEL = 61285;
inline constexpr std::uint64_t DEREGISTRATION_LABEL = 61286;

struct Delegation {
  crypto::PublicKey voting_public_key;
  std::uint32_t weight{};
  friend bool operator==(const Delegation&, const Delegation&) = default;
};

enum class DelegationDistributionKind { legacy, weighted };

class DelegationDistribution {
 public:
  [[nodiscard]] static DelegationDistribution legacy(crypto::PublicKey voting_public_key);
  [[nodiscard]] static core::Result<DelegationDistribution> weighted(
      std::vector<Delegation> delegations);

  [[nodiscard]] DelegationDistributionKind kind() const noexcept;
  [[nodiscard]] const crypto::PublicKey* legacy_key() const noexcept;
  [[nodiscard]] const std::vector<Delegation>* delegations() const noexcept;
  friend bool operator==(const DelegationDistribution&, const DelegationDistribution&) = default;

 private:
  explicit DelegationDistribution(std::variant<crypto::PublicKey, std::vector<Delegation>> value);
  std::variant<crypto::PublicKey, std::vector<Delegation>> value_;
};

class RegistrationWitness {
 public:
  explicit RegistrationWitness(crypto::Ed25519Signature signature);
  [[nodiscard]] static core::Result<RegistrationWitness> from_cbor_value(
      const core::cbor::Value& value);
  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] const crypto::Ed25519Signature& signature() const noexcept;

 private:
  RegistrationWitness(crypto::Ed25519Signature signature,
                      std::shared_ptr<const core::cbor::Value> preserved);
  crypto::Ed25519Signature signature_;
  std::shared_ptr<const core::cbor::Value> preserved_;
};

using DeregistrationWitness = RegistrationWitness;

class KeyRegistration {
 public:
  [[nodiscard]] static KeyRegistration legacy(DelegationDistribution delegation,
                                              crypto::PublicKey stake_credential,
                                              chain::Address payment_address, std::uint64_t nonce);
  [[nodiscard]] static core::Result<KeyRegistration> weighted(DelegationDistribution delegation,
                                                              crypto::PublicKey stake_credential,
                                                              chain::Address payment_address,
                                                              std::uint64_t nonce);
  [[nodiscard]] static core::Result<KeyRegistration> from_cbor_value(
      const core::cbor::Value& value);
  [[nodiscard]] static core::Result<KeyRegistration> from_json(std::string_view json);

  [[nodiscard]] const DelegationDistribution& delegation() const noexcept;
  [[nodiscard]] const crypto::PublicKey& stake_credential() const noexcept;
  [[nodiscard]] const chain::Address& payment_address() const noexcept;
  [[nodiscard]] std::uint64_t nonce() const noexcept;
  [[nodiscard]] std::uint64_t voting_purpose() const noexcept;
  [[nodiscard]] bool has_explicit_voting_purpose() const noexcept;

  void set_voting_purpose(std::uint64_t purpose);
  [[nodiscard]] core::VoidResult verify() const;
  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] std::string to_json() const;

 private:
  KeyRegistration(DelegationDistribution delegation, crypto::PublicKey stake_credential,
                  chain::Address payment_address, std::uint64_t nonce, std::uint64_t voting_purpose,
                  bool purpose_explicit, bool legacy_locked,
                  std::shared_ptr<const core::cbor::Value> preserved = nullptr);

  DelegationDistribution delegation_;
  crypto::PublicKey stake_credential_;
  chain::Address payment_address_;
  std::uint64_t nonce_;
  std::uint64_t voting_purpose_;
  bool purpose_explicit_;
  bool legacy_locked_;
  bool mutated_{false};
  std::shared_ptr<const core::cbor::Value> preserved_;
};

class KeyDeregistration {
 public:
  KeyDeregistration(crypto::PublicKey stake_credential, std::uint64_t nonce);
  [[nodiscard]] static core::Result<KeyDeregistration> from_cbor_value(
      const core::cbor::Value& value);
  [[nodiscard]] static core::Result<KeyDeregistration> from_json(std::string_view json);

  [[nodiscard]] const crypto::PublicKey& stake_credential() const noexcept;
  [[nodiscard]] std::uint64_t nonce() const noexcept;
  [[nodiscard]] std::uint64_t voting_purpose() const noexcept;
  [[nodiscard]] bool has_explicit_voting_purpose() const noexcept;

  void set_voting_purpose(std::uint64_t purpose);
  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] std::string to_json() const;

 private:
  KeyDeregistration(crypto::PublicKey stake_credential, std::uint64_t nonce,
                    std::uint64_t voting_purpose, bool purpose_explicit,
                    std::shared_ptr<const core::cbor::Value> preserved);

  crypto::PublicKey stake_credential_;
  std::uint64_t nonce_;
  std::uint64_t voting_purpose_{};
  bool purpose_explicit_{false};
  bool mutated_{false};
  std::shared_ptr<const core::cbor::Value> preserved_;
};

class RegistrationCbor {
 public:
  RegistrationCbor(KeyRegistration registration, RegistrationWitness witness);
  [[nodiscard]] static core::Result<RegistrationCbor> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<RegistrationCbor> from_cbor_value(
      const core::cbor::Value& value);
  [[nodiscard]] static core::Result<RegistrationCbor> from_json(std::string_view json);

  [[nodiscard]] const KeyRegistration& registration() const noexcept;
  [[nodiscard]] KeyRegistration& registration() noexcept;
  [[nodiscard]] const RegistrationWitness& witness() const noexcept;
  [[nodiscard]] core::Bytes hash_to_sign(bool force_canonical = false) const;
  [[nodiscard]] core::Result<core::Bytes> to_bytes(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<core::cbor::Value> add_to_metadata(
      const core::cbor::Value& metadata) const;
  [[nodiscard]] std::string to_json() const;

 private:
  KeyRegistration registration_;
  RegistrationWitness witness_;
};

class DeregistrationCbor {
 public:
  DeregistrationCbor(KeyDeregistration deregistration, DeregistrationWitness witness);
  [[nodiscard]] static core::Result<DeregistrationCbor> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<DeregistrationCbor> from_cbor_value(
      const core::cbor::Value& value);
  [[nodiscard]] static core::Result<DeregistrationCbor> from_json(std::string_view json);

  [[nodiscard]] const KeyDeregistration& deregistration() const noexcept;
  [[nodiscard]] KeyDeregistration& deregistration() noexcept;
  [[nodiscard]] const DeregistrationWitness& witness() const noexcept;
  [[nodiscard]] core::Bytes hash_to_sign(bool force_canonical = false) const;
  [[nodiscard]] core::Result<core::Bytes> to_bytes(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<core::cbor::Value> add_to_metadata(
      const core::cbor::Value& metadata) const;
  [[nodiscard]] std::string to_json() const;

 private:
  KeyDeregistration deregistration_;
  DeregistrationWitness witness_;
};

using DelegationList = std::vector<Delegation>;
using NonEmptyDelegationList = std::vector<Delegation>;
using CIP36Delegation = Delegation;
using CIP36DelegationDistribution = DelegationDistribution;
using CIP36DelegationDistributionKind = DelegationDistributionKind;
using CIP36DelegationList = DelegationList;
using NonEmptyCIP36DelegationList = NonEmptyDelegationList;
using CIP36RegistrationWitness = RegistrationWitness;
using CIP36DeregistrationWitness = DeregistrationWitness;
using CIP36KeyRegistration = KeyRegistration;
using CIP36KeyDeregistration = KeyDeregistration;
using CIP36RegistrationCbor = RegistrationCbor;
using CIP36DeregistrationCbor = DeregistrationCbor;
using CIP36LegacyKeyRegistration = crypto::PublicKey;
using CIP36Nonce = std::uint64_t;
using CIP36StakeCredential = crypto::PublicKey;
using CIP36StakeWitness = crypto::Ed25519Signature;
using CIP36StakingPubKey = crypto::PublicKey;
using CIP36VotingPubKey = crypto::PublicKey;
using CIP36VotingPurpose = std::uint64_t;
using CIP36Weight = std::uint32_t;

}  // namespace cardano::cip::cip36
