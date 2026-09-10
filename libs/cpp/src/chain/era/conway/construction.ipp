#include "cardano/chain/era/conway.hpp"

namespace cardano::chain {
namespace {

using CborValue = core::cbor::Value;

[[nodiscard]] CborValue construction_uint(std::uint64_t value) {
  return CborValue::unsigned_integer(core::BigInteger(value));
}

[[nodiscard]] core::CardanoError construction_error(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::VoidResult insert_unique(
    std::vector<std::pair<CborValue, CborValue>>& entries, CborValue key, CborValue value,
    std::string_view owner) {
  if (std::ranges::any_of(entries, [&](const auto& entry) {
        return entry.first.semantic_equal(key);
      })) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::duplicate_key, std::string(owner) + ": duplicate map key"));
  }
  entries.emplace_back(std::move(key), std::move(value));
  return std::monostate{};
}

}  // namespace

core::VoidResult ConwayProtocolParamUpdateBuilder::integer(
    ConwayIntegerProtocolParameter key, std::uint64_t value) {
  return insert_unique(entries_, construction_uint(static_cast<std::uint8_t>(key)),
                       construction_uint(value), "Conway protocol parameter update");
}

core::VoidResult ConwayProtocolParamUpdateBuilder::rational(
    ConwayRationalProtocolParameter key, NonnegativeRational value) {
  if (value.denominator == 0U) {
    return std::unexpected(construction_error("protocol parameter denominator must be nonzero"));
  }
  auto encoded = CborValue::tag(
      core::BigInteger(std::uint64_t{30}),
      CborValue::array(
          {construction_uint(value.numerator), construction_uint(value.denominator)}));
  return insert_unique(entries_, construction_uint(static_cast<std::uint8_t>(key)),
                       std::move(encoded), "Conway protocol parameter update");
}

core::VoidResult ConwayProtocolParamUpdateBuilder::validated_extension(std::uint8_t key,
                                                                        CborValue value) {
  auto candidate = entries_;
  auto inserted = insert_unique(candidate, construction_uint(key), std::move(value),
                                "Conway protocol parameter update");
  if (!inserted) return inserted;
  auto validated = ProtocolParamUpdate::from_value(CborValue::map(candidate));
  if (!validated) return std::unexpected(validated.error());
  entries_ = std::move(candidate);
  return std::monostate{};
}

core::Result<ProtocolParamUpdate> ConwayProtocolParamUpdateBuilder::build() const {
  if (entries_.empty()) {
    return std::unexpected(construction_error("protocol parameter update must be nonempty"));
  }
  return ProtocolParamUpdate::from_value(CborValue::map(entries_));
}

ConwayProposalProcedureBuilder& ConwayProposalProcedureBuilder::deposit(std::uint64_t value) {
  deposit_ = value;
  return *this;
}

ConwayProposalProcedureBuilder& ConwayProposalProcedureBuilder::reward_account(
    RewardAddress value) {
  reward_account_ = std::move(value);
  return *this;
}

ConwayProposalProcedureBuilder& ConwayProposalProcedureBuilder::governance_action(GovAction value) {
  governance_action_ = std::move(value);
  return *this;
}

ConwayProposalProcedureBuilder& ConwayProposalProcedureBuilder::anchor(Anchor value) {
  anchor_ = std::move(value);
  return *this;
}

core::Result<ProposalProcedure> ConwayProposalProcedureBuilder::build() const {
  if (!deposit_ || !reward_account_ || !governance_action_ || !anchor_) {
    return std::unexpected(construction_error("proposal procedure is missing a required field"));
  }
  return ProposalProcedure::from_value(CborValue::array(
      {construction_uint(*deposit_),
       CborValue::byte_string(reward_account_->to_address().to_bytes()),
       governance_action_->cbor(), anchor_->cbor()}));
}

core::VoidResult ConwayVotingProceduresBuilder::add(const Voter& voter,
                                                     const GovActionId& action,
                                                     const VotingProcedure& procedure) {
  for (const auto& entry : entries_) {
    if (entry.voter.cbor().semantic_equal(voter.cbor()) &&
        entry.action.cbor().semantic_equal(action.cbor())) {
      return std::unexpected(core::CardanoError(core::ErrorCode::duplicate_key,
                                                "duplicate voter/governance-action pair"));
    }
  }
  entries_.push_back({voter, action, procedure});
  return std::monostate{};
}

core::Result<VotingProcedures> ConwayVotingProceduresBuilder::build() const {
  if (entries_.empty()) {
    return std::unexpected(construction_error("voting procedures must be nonempty"));
  }
  std::vector<std::pair<CborValue, CborValue>> voters;
  for (const auto& entry : entries_) {
    auto existing = std::ranges::find_if(voters, [&](const auto& item) {
      return item.first.semantic_equal(entry.voter.cbor());
    });
    if (existing == voters.end()) {
      voters.emplace_back(entry.voter.cbor(),
                          CborValue::map({{entry.action.cbor(), entry.procedure.cbor()}}));
    } else {
      auto actions = existing->second.as_map()->entries;
      actions.emplace_back(entry.action.cbor(), entry.procedure.cbor());
      existing->second = CborValue::map(std::move(actions));
    }
  }
  return VotingProcedures::from_value(CborValue::map(std::move(voters)));
}

core::VoidResult ConwayTransactionBodyBuilder::add_input(TransactionInput value) {
  if (std::ranges::any_of(inputs_, [&](const auto& candidate) { return candidate == value; })) {
    return std::unexpected(core::CardanoError(core::ErrorCode::duplicate_key,
                                              "duplicate Conway transaction input"));
  }
  inputs_.push_back(std::move(value));
  return std::monostate{};
}

ConwayTransactionBodyBuilder& ConwayTransactionBodyBuilder::add_output(TransactionOutput value) {
  outputs_.push_back(std::move(value));
  return *this;
}

ConwayTransactionBodyBuilder& ConwayTransactionBodyBuilder::fee(std::uint64_t value) {
  fee_ = value;
  return *this;
}

core::VoidResult ConwayTransactionBodyBuilder::voting_procedures(VotingProcedures value) {
  if (voting_procedures_) {
    return std::unexpected(core::CardanoError(core::ErrorCode::duplicate_key,
                                              "voting procedures were already set"));
  }
  voting_procedures_ = std::move(value);
  return std::monostate{};
}

core::VoidResult ConwayTransactionBodyBuilder::add_proposal(ProposalProcedure value) {
  proposals_.push_back(std::move(value));
  return std::monostate{};
}

core::VoidResult ConwayTransactionBodyBuilder::validated_extension(std::uint8_t key,
                                                                    CborValue value) {
  if (key == 0U || key == 1U || key == 2U || key == 19U || key == 20U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::duplicate_key,
                                              "extension key is owned by a typed setter"));
  }
  return insert_unique(extensions_, construction_uint(key), std::move(value),
                       "Conway transaction body");
}

core::Result<TransactionBody> ConwayTransactionBodyBuilder::build() const {
  if (inputs_.empty() || outputs_.empty() || !fee_) {
    return std::unexpected(construction_error("transaction body requires inputs, outputs, and fee"));
  }
  std::vector<CborValue> inputs;
  for (const auto& input : inputs_) inputs.push_back(input.to_cbor_value());
  std::vector<CborValue> outputs;
  for (const auto& output : outputs_) outputs.push_back(output.to_cbor_value());
  std::vector<std::pair<CborValue, CborValue>> fields{
      {construction_uint(0U), CborValue::array(std::move(inputs))},
      {construction_uint(1U), CborValue::array(std::move(outputs))},
      {construction_uint(2U), construction_uint(*fee_)},
  };
  if (voting_procedures_) {
    fields.emplace_back(construction_uint(19U), voting_procedures_->cbor());
  }
  if (!proposals_.empty()) {
    std::vector<CborValue> proposals;
    for (const auto& proposal : proposals_) proposals.push_back(proposal.cbor());
    fields.emplace_back(construction_uint(20U), CborValue::array(std::move(proposals)));
  }
  for (const auto& extension : extensions_) {
    auto inserted = insert_unique(fields, extension.first, extension.second,
                                  "Conway transaction body");
    if (!inserted) return std::unexpected(inserted.error());
  }
  return TransactionBody::from_value(CborValue::map(std::move(fields)));
}

}  // namespace cardano::chain
