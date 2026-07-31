#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cardano/core/bytes.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/core/error.hpp"

namespace cardano::chain {

enum class MultiEraBlockKind : std::uint8_t {
  byron,
  shelley,
  allegra,
  mary,
  alonzo,
  babbage,
  conway
};

enum class MultiEraTransactionBodyKind : std::uint8_t {
  byron,
  shelley,
  allegra,
  mary,
  alonzo,
  babbage,
  conway
};

enum class MultiEraCertificateKind : std::uint8_t {
  stake_registration = 0,
  stake_deregistration = 1,
  stake_delegation = 2,
  pool_registration = 3,
  pool_retirement = 4,
  genesis_key_delegation = 5,
  move_instantaneous_rewards = 6,
  registration = 7,
  unregistration = 8,
  vote_delegation = 9,
  stake_vote_delegation = 10,
  stake_registration_delegation = 11,
  vote_registration_delegation = 12,
  stake_vote_registration_delegation = 13,
  authorize_committee_hot = 14,
  resign_committee_cold = 15,
  register_drep = 16,
  unregister_drep = 17,
  update_drep = 18
};

class MultiEraCertificate {
 public:
  [[nodiscard]] static core::Result<MultiEraCertificate> from_cbor(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<MultiEraCertificate> from_json(std::string_view json);
  [[nodiscard]] MultiEraCertificateKind kind() const noexcept;
  [[nodiscard]] const core::cbor::Value& cbor() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<std::string> to_json() const;

 private:
  MultiEraCertificate(MultiEraCertificateKind kind, core::cbor::Value value);
  MultiEraCertificateKind kind_;
  core::cbor::Value value_;
};

class MultiEraProtocolParamUpdate {
 public:
  [[nodiscard]] static core::Result<MultiEraProtocolParamUpdate> from_cbor(core::ByteSpan bytes);
  [[nodiscard]] const core::cbor::Value& cbor() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<std::optional<std::uint64_t>> numeric(std::uint64_t key) const;
  [[nodiscard]] core::Result<std::optional<std::uint64_t>> minfee_a() const;
  [[nodiscard]] core::Result<std::optional<std::uint64_t>> minfee_b() const;
  [[nodiscard]] core::Result<std::optional<std::uint64_t>> max_transaction_size() const;
  [[nodiscard]] core::Result<std::optional<std::uint64_t>> collateral_percentage() const;
  [[nodiscard]] core::Result<std::optional<std::uint64_t>> max_collateral_inputs() const;

 private:
  explicit MultiEraProtocolParamUpdate(core::cbor::Value value);
  core::cbor::Value value_;
};

class MultiEraUpdate {
 public:
  [[nodiscard]] static core::Result<MultiEraUpdate> from_cbor(core::ByteSpan bytes);
  [[nodiscard]] std::uint64_t epoch() const noexcept;
  [[nodiscard]] const core::cbor::Value& proposed_updates() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;

 private:
  MultiEraUpdate(core::cbor::Value value, core::cbor::Value proposed_updates, std::uint64_t epoch);
  core::cbor::Value value_;
  core::cbor::Value proposed_updates_;
  std::uint64_t epoch_;
};

class MultiEraTransactionInput {
 public:
  [[nodiscard]] const core::cbor::Value& cbor() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> transaction_hash() const;
  [[nodiscard]] core::Result<std::uint64_t> index() const;

 private:
  MultiEraTransactionInput(bool byron, core::cbor::Value value);
  bool byron_{};
  core::cbor::Value value_;
  friend class MultiEraTransactionBody;
};

class MultiEraTransactionOutput {
 public:
  [[nodiscard]] const core::cbor::Value& cbor() const noexcept;
  [[nodiscard]] core::Result<core::cbor::Value> address() const;
  [[nodiscard]] core::Result<core::cbor::Value> value() const;

 private:
  MultiEraTransactionOutput(bool byron, core::cbor::Value value);
  bool byron_{};
  core::cbor::Value value_;
  friend class MultiEraTransactionBody;
};

class MultiEraBlockHeader {
 public:
  [[nodiscard]] const core::cbor::Value& cbor() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<std::uint64_t> block_number() const;
  [[nodiscard]] core::Result<std::uint64_t> slot() const;
  [[nodiscard]] core::Result<std::optional<core::Bytes>> previous_hash() const;
  [[nodiscard]] core::Result<core::Bytes> hash() const;

 private:
  MultiEraBlockHeader(std::uint8_t network_tag, core::cbor::Value header);

  std::uint8_t network_tag_{};
  core::cbor::Value header_;

  friend class MultiEraBlock;
};

class MultiEraTransactionBody {
 public:
  [[nodiscard]] const core::cbor::Value& cbor() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<core::Bytes> hash() const;
  [[nodiscard]] MultiEraTransactionBodyKind kind() const noexcept;
  [[nodiscard]] core::Result<std::vector<MultiEraTransactionInput>> inputs() const;
  [[nodiscard]] core::Result<std::vector<MultiEraTransactionOutput>> outputs() const;
  [[nodiscard]] core::Result<std::optional<std::uint64_t>> fee() const;
  [[nodiscard]] core::Result<std::vector<MultiEraCertificate>> certificates() const;
  [[nodiscard]] core::Result<std::optional<MultiEraUpdate>> update() const;
  [[nodiscard]] std::optional<core::cbor::Value> field(std::uint64_t key) const;

 private:
  MultiEraTransactionBody(std::uint8_t network_tag, core::cbor::Value body);
  std::uint8_t network_tag_{};
  core::cbor::Value body_;

  friend class MultiEraBlock;
};

class MultiEraBlock {
 public:
  [[nodiscard]] static core::Result<MultiEraBlock> from_cbor(
      core::ByteSpan bytes, core::cbor::DecodeOptions options = {});
  [[nodiscard]] static core::Result<MultiEraBlock> from_cbor_hex(
      std::string_view hex, core::cbor::DecodeOptions options = {});

  [[nodiscard]] MultiEraBlockKind kind() const noexcept;
  [[nodiscard]] std::uint8_t network_tag() const noexcept;
  [[nodiscard]] const core::cbor::Value& cbor() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] MultiEraBlockHeader header() const;
  [[nodiscard]] core::Result<std::vector<MultiEraTransactionBody>> transaction_bodies() const;

 private:
  MultiEraBlock(std::uint8_t network_tag, core::cbor::Value envelope, core::cbor::Value block,
                core::cbor::Value header);

  std::uint8_t network_tag_{};
  core::cbor::Value envelope_;
  core::cbor::Value block_;
  core::cbor::Value header_;
};

using MultiEraTransactionInputList = std::vector<MultiEraTransactionInput>;
using MultiEraTransactionOutputList = std::vector<MultiEraTransactionOutput>;
using MultiEraTransactionBodyList = std::vector<MultiEraTransactionBody>;
using MultiEraCertificateList = std::vector<MultiEraCertificate>;
using MapGenesisHashToMultiEraProtocolParamUpdate =
    std::vector<std::pair<core::Bytes, MultiEraProtocolParamUpdate>>;

}  // namespace cardano::chain
