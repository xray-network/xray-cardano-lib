#include <catch2/catch_test_macros.hpp>
#include <type_traits>

#include "cardano/chain/era/conway.hpp"

TEST_CASE("typed Conway builders return the existing validated model owners") {
  using namespace cardano;
  using core::BigInteger;
  using core::cbor::Value;
  const auto uint = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  static_assert(!std::is_constructible_v<chain::TransactionBody, Value>);

  chain::ConwayProtocolParamUpdateBuilder parameters;
  REQUIRE(parameters.integer(chain::ConwayIntegerProtocolParameter::min_fee_coefficient, 44));
  REQUIRE(parameters.rational(chain::ConwayRationalProtocolParameter::min_fee_reference_scripts,
                              {15, 10}));
  CHECK(parameters.build());

  const auto action = *chain::GovAction::from_value(Value::array({uint(6)}));
  const auto anchor = *chain::Anchor::from_value(Value::array(
      {Value::text_string("https://example.test"), Value::byte_string(core::Bytes(32))}));
  const auto stake_hash = *crypto::Ed25519KeyHash::from_bytes(core::Bytes(28));
  chain::ConwayProposalProcedureBuilder proposal_builder;
  const auto proposal =
      proposal_builder.deposit(1)
          .reward_account(chain::RewardAddress(0, chain::Credential::key(stake_hash)))
          .governance_action(action)
          .anchor(anchor)
          .build();
  REQUIRE(proposal);

  const auto voter =
      *chain::Voter::from_value(Value::array({uint(2), Value::byte_string(core::Bytes(28))}));
  const auto action_id =
      *chain::GovActionId::from_value(Value::array({Value::byte_string(core::Bytes(32)), uint(0)}));
  const auto voting_procedure =
      *chain::VotingProcedure::from_value(Value::array({uint(1), Value::null()}));
  chain::ConwayVotingProceduresBuilder votes_builder;
  REQUIRE(votes_builder.add(voter, action_id, voting_procedure));
  CHECK_FALSE(votes_builder.add(voter, action_id, voting_procedure));
  const auto votes = votes_builder.build();
  REQUIRE(votes);

  const auto transaction_hash = *crypto::TransactionHash::from_bytes(core::Bytes(32));
  const auto payment_hash = *crypto::Ed25519KeyHash::from_bytes(core::Bytes(28));
  const auto address =
      chain::EnterpriseAddress(0, chain::Credential::key(payment_hash)).to_address();
  chain::ConwayTransactionBodyBuilder body;
  REQUIRE(body.add_input(chain::TransactionInput(transaction_hash, 0)));
  CHECK_FALSE(body.add_input(chain::TransactionInput(transaction_hash, 0)));
  body.add_output(chain::TransactionOutput(address, chain::Value(2))).fee(1);
  REQUIRE(body.voting_procedures(*votes));
  REQUIRE(body.add_proposal(*proposal));
  CHECK(body.build());
}
