#pragma once

// Conway-era model entry point.
#include "cardano/chain/era/shared/models.hpp"

namespace cardano::chain {

enum class ConwayIntegerProtocolParameter : std::uint8_t {
  min_fee_coefficient = 0,
  min_fee_constant = 1,
  max_block_body_size = 2,
  max_transaction_size = 3,
  max_block_header_size = 4,
  key_deposit = 5,
  pool_deposit = 6,
  maximum_epoch = 7,
  desired_pool_count = 8,
  min_pool_cost = 16,
  coins_per_utxo_byte = 17,
  max_value_size = 22,
  collateral_percentage = 23,
  max_collateral_inputs = 24,
  min_committee_size = 28,
  committee_term_limit = 29,
  governance_action_validity = 30,
  governance_action_deposit = 31,
  drep_deposit = 32,
};

enum class ConwayRationalProtocolParameter : std::uint8_t {
  pool_pledge_influence = 9,
  monetary_expansion = 10,
  treasury_expansion = 11,
  min_fee_reference_scripts = 33,
};

class ConwayProtocolParamUpdateBuilder {
 public:
  [[nodiscard]] core::VoidResult integer(ConwayIntegerProtocolParameter key, std::uint64_t value);
  [[nodiscard]] core::VoidResult rational(ConwayRationalProtocolParameter key,
                                          NonnegativeRational value);
  [[nodiscard]] core::VoidResult validated_extension(std::uint8_t key, core::cbor::Value value);
  [[nodiscard]] core::Result<ProtocolParamUpdate> build() const;

 private:
  std::vector<std::pair<core::cbor::Value, core::cbor::Value>> entries_;
};

class ConwayProposalProcedureBuilder {
 public:
  ConwayProposalProcedureBuilder& deposit(std::uint64_t value);
  ConwayProposalProcedureBuilder& reward_account(RewardAddress value);
  ConwayProposalProcedureBuilder& governance_action(GovAction value);
  ConwayProposalProcedureBuilder& anchor(Anchor value);
  [[nodiscard]] core::Result<ProposalProcedure> build() const;

 private:
  std::optional<std::uint64_t> deposit_;
  std::optional<RewardAddress> reward_account_;
  std::optional<GovAction> governance_action_;
  std::optional<Anchor> anchor_;
};

class ConwayVotingProceduresBuilder {
 public:
  [[nodiscard]] core::VoidResult add(const Voter& voter, const GovActionId& action,
                                     const VotingProcedure& procedure);
  [[nodiscard]] core::Result<VotingProcedures> build() const;

 private:
  struct Entry {
    Voter voter;
    GovActionId action;
    VotingProcedure procedure;
  };
  std::vector<Entry> entries_;
};

class ConwayTransactionBodyBuilder {
 public:
  [[nodiscard]] core::VoidResult add_input(TransactionInput value);
  ConwayTransactionBodyBuilder& add_output(TransactionOutput value);
  ConwayTransactionBodyBuilder& fee(std::uint64_t value);
  [[nodiscard]] core::VoidResult voting_procedures(VotingProcedures value);
  [[nodiscard]] core::VoidResult add_proposal(ProposalProcedure value);
  [[nodiscard]] core::VoidResult validated_extension(std::uint8_t key, core::cbor::Value value);
  [[nodiscard]] core::Result<TransactionBody> build() const;

 private:
  std::vector<TransactionInput> inputs_;
  std::vector<TransactionOutput> outputs_;
  std::optional<std::uint64_t> fee_;
  std::optional<VotingProcedures> voting_procedures_;
  std::vector<ProposalProcedure> proposals_;
  std::vector<std::pair<core::cbor::Value, core::cbor::Value>> extensions_;
};

}  // namespace cardano::chain
