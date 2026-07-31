#include <catch2/catch_test_macros.hpp>

#include "cardano/chain/builder.hpp"

using namespace cardano;

namespace {

core::Bytes repeated(std::size_t size, std::uint8_t value) {
  return core::Bytes(size, static_cast<core::Byte>(value));
}

core::Bytes bytes(std::initializer_list<std::uint8_t> values) {
  core::Bytes output;
  for (const auto value : values) output.push_back(static_cast<core::Byte>(value));
  return output;
}

chain::Address address(std::uint8_t marker = 1) {
  auto hash = crypto::Ed25519KeyHash::from_bytes(repeated(28, marker)).value();
  return chain::EnterpriseAddress(0, chain::Credential::key(std::move(hash))).to_address();
}

chain::Address script_address(std::uint8_t marker = 1) {
  auto hash = crypto::ScriptHash::from_bytes(repeated(28, marker)).value();
  return chain::EnterpriseAddress(0, chain::Credential::script(std::move(hash))).to_address();
}

crypto::TransactionHash tx_hash(std::uint8_t marker) {
  return crypto::TransactionHash::from_bytes(repeated(32, marker)).value();
}

chain::TransactionUnspentOutput utxo(std::uint8_t marker, std::uint64_t coin,
                                     chain::Value value = chain::Value{}) {
  if (value.coin() == 0 && !value.has_assets()) value.set_coin(coin);
  return {
      chain::TransactionInput(tx_hash(marker), marker),
      chain::TransactionOutput(address(marker), std::move(value)),
  };
}

chain::TransactionBuilderConfig config(bool pure_change = false) {
  return chain::TransactionBuilderConfigBuilder()
      .linear_fee({1, 10})
      .reference_script_cost_per_byte(10)
      .pool_deposit(500)
      .key_deposit(200)
      .max_value_size(5000)
      .max_transaction_size(16384)
      .coins_per_utxo_byte(1)
      .ex_unit_prices({1, 1, 1, 1})
      .collateral_percentage(150)
      .max_collateral_inputs(3)
      .prefer_pure_change(pure_change)
      .build()
      .value();
}

class XorShiftRandom final : public core::SecureRandomSource {
 public:
  core::Result<core::Bytes> random_bytes(std::size_t length) override {
    core::Bytes output;
    output.reserve(length);
    while (output.size() < length) {
      state_ ^= state_ << 13U;
      state_ ^= state_ >> 17U;
      state_ ^= state_ << 5U;
      for (unsigned offset = 0; offset < 4 && output.size() < length; ++offset) {
        output.push_back(static_cast<core::Byte>((state_ >> (8U * offset)) & 0xffU));
      }
    }
    return output;
  }

 private:
  std::uint32_t state_{0x5eedc1f2U};
};

class ZeroRandom final : public core::SecureRandomSource {
 public:
  core::Result<core::Bytes> random_bytes(std::size_t length) override {
    return core::Bytes(length, core::Byte{0});
  }
};

}  // namespace

TEST_CASE("transaction inputs and Values expose frozen specialized JSON", "[builder][json]") {
  const chain::TransactionInput input(tx_hash(0x11), 42);
  const auto parsed = chain::TransactionInput::from_json(input.to_json());
  REQUIRE(parsed);
  CHECK(*parsed == input);
  CHECK_FALSE(chain::TransactionInput::from_json(R"({"transaction_id":"00","index":0})"));
  CHECK_FALSE(chain::TransactionInput::from_json(
      R"({"transaction_id":"1111111111111111111111111111111111111111111111111111111111111111","index":9007199254740992})"));

  const auto policy = crypto::ScriptHash::from_bytes(repeated(28, 0x22)).value();
  const auto asset = chain::AssetName::from_hex("4142").value();
  const chain::Value value(9, {chain::PolicyAssets{policy, {chain::AssetQuantity{asset, 3}}}});
  const auto json = value.to_json();
  CHECK(json.find("\"coin\":9.0") != std::string::npos);
  CHECK(json.find("\"4142\":3.0") != std::string::npos);
}

TEST_CASE("transaction builder config requires every frozen field", "[builder]") {
  CHECK_FALSE(chain::TransactionBuilderConfigBuilder().build());
  const auto built = config();
  CHECK(built.collateral_percentage == 150);
  CHECK(built.max_collateral_inputs == 3);
}

TEST_CASE("builder rejects duplicate explicit inputs and ignores duplicate candidates",
          "[builder]") {
  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(utxo(1, 300)));
  CHECK_FALSE(builder.add_input(utxo(1, 2000)));
  builder.add_utxo(utxo(1, 3000));
  builder.add_utxo(utxo(2, 3000));
  builder.add_utxo(utxo(2, 4000));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(500))));
  REQUIRE(builder.select_utxos(chain::CoinSelectionStrategyCIP2::largest_first));
  CHECK(builder.inputs().size() == 2);
}

TEST_CASE("largest-first selection is quantity-descending with canonical tie break", "[builder]") {
  chain::TransactionBuilder builder(config());
  builder.add_utxo(utxo(3, 2000));
  builder.add_utxo(utxo(1, 3000));
  builder.add_utxo(utxo(2, 3000));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(2500))));
  REQUIRE(builder.select_utxos(chain::CoinSelectionStrategyCIP2::largest_first));
  REQUIRE(builder.inputs().size() == 1);
  CHECK(builder.inputs().front().input.transaction_id() == tx_hash(1));
}

TEST_CASE("random-improve requires explicit randomness and is deterministic", "[builder]") {
  auto make = [] {
    chain::TransactionBuilder builder(config());
    for (std::uint8_t marker = 1; marker <= 5; ++marker) {
      builder.add_utxo(utxo(marker, 1000));
    }
    REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1500))));
    return builder;
  };
  auto missing = make();
  CHECK_FALSE(missing.select_utxos(chain::CoinSelectionStrategyCIP2::random_improve));
  auto first = make();
  auto second = make();
  XorShiftRandom random1;
  XorShiftRandom random2;
  REQUIRE(first.select_utxos(chain::CoinSelectionStrategyCIP2::random_improve, &random1));
  REQUIRE(second.select_utxos(chain::CoinSelectionStrategyCIP2::random_improve, &random2));
  REQUIRE(first.inputs().size() == second.inputs().size());
  for (std::size_t index = 0; index < first.inputs().size(); ++index) {
    CHECK(first.inputs()[index].input == second.inputs()[index].input);
  }
}

TEST_CASE("random-improve performs the frozen ideal-value replacement", "[builder]") {
  chain::TransactionBuilder builder(config());
  builder.add_utxo(utxo(1, 1000));
  builder.add_utxo(utxo(2, 2000));
  builder.add_utxo(utxo(3, 2500));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));

  ZeroRandom random;
  REQUIRE(builder.select_utxos(chain::CoinSelectionStrategyCIP2::random_improve, &random));
  REQUIRE(builder.inputs().size() == 1);
  CHECK(builder.inputs().front().input.transaction_id() == tx_hash(3));
}

TEST_CASE("multi-asset largest-first covers assets before coin", "[builder]") {
  auto policy = crypto::ScriptHash::from_bytes(repeated(28, 8)).value();
  auto asset = chain::AssetName::from_hex("01").value();
  chain::Value target(500, {{policy, {{asset, 5}}}});
  chain::Value asset_input(1000, {{policy, {{asset, 5}}}});

  chain::TransactionBuilder builder(config());
  builder.add_utxo(utxo(1, 4000));
  builder.add_utxo(utxo(2, 1000, asset_input));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), target)));
  REQUIRE(builder.select_utxos(chain::CoinSelectionStrategyCIP2::largest_first_multi_asset));
  CHECK(builder.total_input()->asset_quantity(policy, asset) == 5);
}

TEST_CASE("mint merging rejects zero and overflow semantics", "[builder]") {
  auto policy = crypto::ScriptHash::from_bytes(repeated(28, 9)).value();
  auto asset = chain::AssetName::from_hex("aa").value();
  chain::TransactionBuilder builder(config());
  CHECK_FALSE(builder.set_mint({{policy, {{asset, 0}}}}));
  CHECK_FALSE(builder.set_mint({
      {policy, {{asset, 2}}},
      {policy, {{asset, -2}}},
  }));
  REQUIRE(builder.set_mint({
      {policy, {{asset, 2}}},
      {policy, {{asset, 3}}},
  }));
}

TEST_CASE("change converges fees and final body balances", "[builder]") {
  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(utxo(1, 10'000)));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  const auto changed = builder.add_change_if_needed(address(2));
  REQUIRE(changed);
  CHECK(*changed);
  REQUIRE(builder.fee());
  CHECK(*builder.fee() >= 10);
  const auto input = builder.total_input();
  const auto output = builder.total_output();
  REQUIRE(input);
  REQUIRE(output);
  CHECK(input->coin() == output->coin() + *builder.fee());
  REQUIRE(builder.build_body());
  REQUIRE(builder.build_transaction());
}

TEST_CASE("builder emits ordered body fields and rejects zero donation", "[builder]") {
  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(utxo(1, 5000)));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  builder.set_ttl(100);
  builder.set_validity_start(5);
  builder.set_network_id(0);
  builder.add_reference_input(chain::TransactionInput(tx_hash(9), 0));
  builder.add_reference_input(chain::TransactionInput(tx_hash(9), 0));
  builder.set_donation(0);
  CHECK_FALSE(builder.add_change_if_needed(address(2)));

  chain::TransactionBuilder valid(config());
  REQUIRE(valid.add_input(utxo(1, 5000)));
  REQUIRE(valid.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  valid.set_ttl(100);
  valid.set_validity_start(5);
  valid.set_network_id(0);
  valid.set_donation(1);
  REQUIRE(valid.add_change_if_needed(address(2)));
  const auto body = valid.build_body();
  REQUIRE(body);
  REQUIRE(body->as_map());
  std::uint64_t previous = 0;
  for (const auto& [key, value] : body->as_map()->entries) {
    static_cast<void>(value);
    const auto current = key.as_unsigned()->value.to_uint64().value();
    CHECK(current >= previous);
    previous = current;
  }
}

TEST_CASE("communication datum queues witness and exact datum hash option", "[builder]") {
  using core::BigInteger;
  using core::cbor::Value;
  chain::TransactionOutput output(address(), chain::Value(1000));
  const auto datum = Value::unsigned_integer(BigInteger(std::uint64_t{42}));
  REQUIRE(output.set_communication_datum(datum));
  REQUIRE(output.datum_option());
  CHECK(output.datum_option()->as_array()->values[0].as_unsigned()->value ==
        BigInteger(std::uint64_t{0}));

  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(utxo(1, 5000)));
  REQUIRE(builder.add_output(std::move(output)));
  REQUIRE(builder.add_change_if_needed(address(2)));
  const auto transaction = builder.build_transaction();
  REQUIRE(transaction);
  const auto* witness = transaction->as_array()->values[1].as_map();
  REQUIRE(witness);
  CHECK(witness->entries.size() == 1);
}

TEST_CASE("redeemers sort by purpose and identity and index within each purpose", "[builder]") {
  using core::BigInteger;
  using core::cbor::Value;
  chain::RedeemerSetBuilder redeemers;
  redeemers.add({chain::RedeemerPurpose::mint, "ff"},
                {Value::unsigned_integer(BigInteger(std::uint64_t{1})), chain::ExUnits{3, 4}});
  redeemers.add({chain::RedeemerPurpose::spend, "bb"},
                {Value::unsigned_integer(BigInteger(std::uint64_t{2})), chain::ExUnits{5, 6}});
  redeemers.add({chain::RedeemerPurpose::spend, "aa"},
                {Value::unsigned_integer(BigInteger(std::uint64_t{3})), chain::ExUnits{7, 8}});
  const auto built = redeemers.build();
  REQUIRE(built);
  REQUIRE(built->as_array());
  const auto& values = built->as_array()->values;
  REQUIRE(values.size() == 3);
  CHECK(values[0].as_array()->values[0].as_unsigned()->value == BigInteger(std::uint64_t{0}));
  CHECK(values[0].as_array()->values[1].as_unsigned()->value == BigInteger(std::uint64_t{0}));
  CHECK(values[1].as_array()->values[1].as_unsigned()->value == BigInteger(std::uint64_t{1}));
  CHECK(values[2].as_array()->values[0].as_unsigned()->value == BigInteger(std::uint64_t{1}));
  CHECK(values[2].as_array()->values[1].as_unsigned()->value == BigInteger(std::uint64_t{0}));

  chain::RedeemerSetBuilder missing;
  missing.add({chain::RedeemerPurpose::spend, "x"}, {Value::null(), std::nullopt});
  CHECK_FALSE(missing.build());
  REQUIRE(missing.build(true));
  missing.set_ex_units(chain::RedeemerPurpose::spend, 0, {9, 10});
  REQUIRE(missing.build());
}

TEST_CASE("checked witness assembly rejects missing identities and signing satisfies vkey",
          "[builder]") {
  using core::BigInteger;
  using core::cbor::Value;
  auto private_key = crypto::PrivateKey::from_bytes(repeated(32, 12));
  REQUIRE(private_key);
  auto public_key = private_key->public_key();
  REQUIRE(public_key);

  chain::TransactionWitnessSetBuilder witnesses;
  witnesses.require_vkey(public_key->hash());
  CHECK_FALSE(witnesses.build());
  const auto fake = witnesses.build_unchecked(true);
  REQUIRE(fake);
  REQUIRE(fake->as_map());
  CHECK(fake->as_map()->entries.size() == 1);

  const auto body = Value::map({
      {Value::unsigned_integer(BigInteger(std::uint64_t{0})), Value::array({})},
      {Value::unsigned_integer(BigInteger(std::uint64_t{1})), Value::array({})},
      {Value::unsigned_integer(BigInteger(std::uint64_t{2})),
       Value::unsigned_integer(BigInteger(std::uint64_t{0}))},
  });
  chain::SignedTxBuilder signed_builder(body, std::move(witnesses));
  CHECK_FALSE(signed_builder.build());
  REQUIRE(signed_builder.sign(*private_key));
  const auto transaction = signed_builder.build();
  REQUIRE(transaction);
  REQUIRE(transaction->as_array());
  CHECK(transaction->as_array()->values.size() == 4);
}

TEST_CASE("witness builder verifies script hashes and emits tagged witness arrays", "[builder]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto script = bytes({1, 2, 3});
  const auto hash = chain::hash_script(2, script);
  chain::TransactionWitnessSetBuilder witnesses;
  witnesses.require_plutus_script(2, hash);
  REQUIRE(witnesses.add_plutus_script(2, hash, script));
  CHECK_FALSE(witnesses.add_plutus_script(2, chain::hash_script(2, bytes({9})), script));

  const auto datum = Value::unsigned_integer(BigInteger(std::uint64_t{5}));
  const auto datum_hash = chain::hash_plutus_data(datum).value();
  witnesses.require_datum(datum_hash);
  witnesses.add_datum(datum_hash, datum);
  const auto built = witnesses.build();
  REQUIRE(built);
  REQUIRE(built->as_map());
  CHECK(built->as_map()->entries.size() == 2);
  for (const auto& [key, value] : built->as_map()->entries) {
    static_cast<void>(key);
    CHECK(value.as_tag());
    CHECK(value.as_tag()->tag == BigInteger(std::uint64_t{258}));
  }
}

TEST_CASE("reference scripts dominate inline witnesses and charge their bytes",
          "[builder][reference]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto raw_script = bytes({1, 2, 3, 4});
  const auto script_hash = chain::hash_script(2, raw_script);
  const auto script = Value::array({
      Value::unsigned_integer(BigInteger(std::uint64_t{2})),
      Value::byte_string(raw_script),
  });
  const auto encoded_script = core::cbor::encode_cbor(script);
  REQUIRE(encoded_script);

  chain::TransactionWitnessSetBuilder witnesses;
  REQUIRE(witnesses.satisfy_script_reference(2, script_hash));
  witnesses.require_plutus_script(2, script_hash);
  REQUIRE(witnesses.add_plutus_script(2, script_hash, raw_script));
  const auto built = witnesses.build();
  REQUIRE(built);
  REQUIRE(built->as_map());
  CHECK(built->as_map()->entries.empty());

  chain::TransactionOutput reference_output(address(8), chain::Value(2000));
  reference_output.set_script_reference(
      Value::tag(BigInteger(std::uint64_t{24}), Value::byte_string(*encoded_script)));
  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_reference_input(chain::TransactionUnspentOutput{
      chain::TransactionInput(tx_hash(8), 0),
      std::move(reference_output),
  }));
  REQUIRE(builder.add_input(utxo(1, 5000)));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  builder.set_fee(100);
  REQUIRE(builder.min_fee());
}

TEST_CASE("auxiliary data merges by canonical key without moving replacements",
          "[builder][metadata]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(utxo(1, 5000)));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  builder.set_auxiliary_data(Value::map({{u(1), u(10)}, {u(2), u(20)}}));
  builder.merge_auxiliary_data(Value::map({{u(1), u(11)}, {u(3), u(30)}}));
  builder.set_fee(100);

  const auto body = builder.build_body();
  REQUIRE(body);
  const auto expected = Value::map({{u(1), u(11)}, {u(2), u(20)}, {u(3), u(30)}});
  const auto expected_hash = chain::hash_auxiliary_data(expected);
  REQUIRE(expected_hash);
  const auto hash_field =
      std::ranges::find(body->as_map()->entries, BigInteger(std::uint64_t{7}),
                        [](const auto& entry) { return entry.first.as_unsigned()->value; });
  REQUIRE(hash_field != body->as_map()->entries.end());
  CHECK(hash_field->second.as_byte_string()->value == expected_hash->to_bytes());
}

TEST_CASE("evaluation drafts use dummy ExUnits and signing remains checked", "[builder][draft]") {
  auto private_key = crypto::PrivateKey::from_bytes(repeated(32, 22));
  REQUIRE(private_key);
  auto public_key = private_key->public_key();
  REQUIRE(public_key);
  const auto input_address =
      chain::EnterpriseAddress(0, chain::Credential::key(public_key->hash())).to_address();
  chain::TransactionUnspentOutput input{
      chain::TransactionInput(tx_hash(1), 0),
      chain::TransactionOutput(input_address, chain::Value(5000)),
  };
  auto staged = chain::SingleInputBuilder(std::move(input)).payment_key();
  REQUIRE(staged);

  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(std::move(*staged)));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  builder.set_fee(100);
  REQUIRE(builder.build_for_evaluation());
  auto signed_builder = builder.build_signed();
  REQUIRE(signed_builder);
  CHECK_FALSE(signed_builder->build());
  REQUIRE(signed_builder->sign(*private_key));
  REQUIRE(signed_builder->build());
}

TEST_CASE("native witness sizing discovers signers in script order", "[builder][native]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto native = Value::array({
      u(1),
      Value::array({
          Value::array({u(0), Value::byte_string(repeated(28, 31))}),
          Value::array({u(0), Value::byte_string(repeated(28, 32))}),
      }),
  });
  const auto native_bytes = core::cbor::encode_cbor(native);
  REQUIRE(native_bytes);
  const auto native_hash = chain::hash_script(0, *native_bytes);
  const auto asset = chain::AssetName::from_hex("01").value();
  auto mint = chain::SingleMintBuilder::create({{asset, 1}});
  REQUIRE(mint);
  auto mint_result =
      mint->native_script(native_hash, native, chain::NativeScriptWitnessInfo::signature_count(1));
  REQUIRE(mint_result);

  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(utxo(1, 5000)));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  REQUIRE(builder.add_mint(std::move(*mint_result)));
  builder.set_fee(100);
  const auto draft = builder.build_for_evaluation();
  REQUIRE(draft);
  const auto* witness = draft->as_array()->values[1].as_map();
  REQUIRE(witness);
  const auto vkeys =
      std::ranges::find(witness->entries, BigInteger(std::uint64_t{0}),
                        [](const auto& entry) { return entry.first.as_unsigned()->value; });
  REQUIRE(vkeys != witness->entries.end());
  REQUIRE(vkeys->second.as_tag());
  REQUIRE(vkeys->second.as_tag()->value->as_array());
  CHECK(vkeys->second.as_tag()->value->as_array()->values.size() == 1);
}

TEST_CASE("single input, mint, withdrawal, and output builders validate witnesses",
          "[builder][staged]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto native_hash = crypto::ScriptHash::from_bytes(repeated(28, 4)).value();
  const auto native = Value::array({
      Value::unsigned_integer(BigInteger(std::uint64_t{0})),
      Value::byte_string(repeated(28, 4)),
  });
  chain::SingleInputBuilder input(chain::TransactionUnspentOutput{
      chain::TransactionInput(tx_hash(4), 0),
      chain::TransactionOutput(script_address(4), chain::Value(1000)),
  });
  CHECK_FALSE(input.payment_key());
  REQUIRE(input.native_script(native_hash, native,
                              chain::NativeScriptWitnessInfo::assume_signature_count()));

  const auto asset = chain::AssetName::from_hex("aa").value();
  const auto mint = chain::SingleMintBuilder::create({{asset, 7}});
  REQUIRE(mint);
  REQUIRE(
      mint->native_script(native_hash, native, chain::NativeScriptWitnessInfo::signature_count(1)));
  CHECK_FALSE(chain::SingleMintBuilder::create({{asset, 0}}));

  auto staged = chain::TransactionOutputBuilder()
                    .with_address(address())
                    .with_data(Value::array({
                        Value::unsigned_integer(BigInteger(std::uint64_t{1})),
                        Value::unsigned_integer(BigInteger(std::uint64_t{9})),
                    }))
                    .next();
  REQUIRE(staged);
  staged->with_value(chain::Value(1000));
  const auto output = staged->build();
  REQUIRE(output);
  CHECK(output->output.datum_option().has_value());
}

TEST_CASE("collateral limits and return coin produce body keys 13 16 and 17",
          "[builder][collateral]") {
  using core::BigInteger;
  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(utxo(1, 5000)));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  const auto collateral = chain::SingleInputBuilder(utxo(2, 900)).payment_key();
  REQUIRE(collateral);
  REQUIRE(builder.add_collateral(*collateral));
  builder.set_collateral_return(chain::TransactionOutput(address(3), chain::Value(300)));
  builder.set_fee(100);
  const auto body = builder.build_body();
  REQUIRE(body);
  std::set<std::uint64_t> keys;
  for (const auto& [key, value] : body->as_map()->entries) {
    static_cast<void>(value);
    keys.insert(key.as_unsigned()->value.to_uint64().value());
  }
  CHECK(keys.contains(13));
  CHECK(keys.contains(16));
  CHECK(keys.contains(17));
  const auto total = std::find_if(
      body->as_map()->entries.begin(), body->as_map()->entries.end(), [](const auto& field) {
        return field.first.as_unsigned()->value == BigInteger(std::uint64_t{17});
      });
  REQUIRE(total != body->as_map()->entries.end());
  CHECK(total->second.as_unsigned()->value == BigInteger(std::uint64_t{600}));

  chain::TransactionBuilder invalid(config());
  REQUIRE(invalid.add_collateral(*collateral));
  invalid.set_collateral_return(chain::TransactionOutput(address(), chain::Value(901)));
  invalid.set_fee(0);
  CHECK_FALSE(invalid.build_body());
}

TEST_CASE("vote builder replaces in place and transaction body orders governance",
          "[builder][governance]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto voter = Value::array({
      Value::unsigned_integer(BigInteger(std::uint64_t{0})),
      Value::byte_string(repeated(28, 1)),
  });
  const auto action = Value::array({
      Value::byte_string(repeated(32, 2)),
      Value::unsigned_integer(BigInteger(std::uint64_t{3})),
  });
  chain::VoteBuilder votes;
  votes.add(voter, action, Value::array({Value::unsigned_integer(BigInteger(std::uint64_t{0}))}));
  votes.add(voter, action, Value::array({Value::unsigned_integer(BigInteger(std::uint64_t{1}))}));
  CHECK(votes.build().entries.size() == 1);

  chain::ProposalBuilder proposals;
  proposals.add(Value::array({
      Value::unsigned_integer(BigInteger(std::uint64_t{10})),
      Value::null(),
  }));

  chain::TransactionBuilder builder(config());
  REQUIRE(builder.add_input(utxo(1, 5000)));
  REQUIRE(builder.add_output(chain::TransactionOutput(address(), chain::Value(1000))));
  REQUIRE(builder.add_votes(votes.build()));
  REQUIRE(builder.add_proposals(proposals.build()));
  builder.set_current_treasury_value(55);
  builder.set_donation(1);
  builder.set_fee(100);
  const auto body = builder.build_body();
  REQUIRE(body);
  std::vector<std::uint64_t> keys;
  for (const auto& [key, value] : body->as_map()->entries) {
    static_cast<void>(value);
    keys.push_back(key.as_unsigned()->value.to_uint64().value());
  }
  CHECK(std::ranges::is_sorted(keys));
  CHECK(std::ranges::find(keys, 19) != keys.end());
  CHECK(std::ranges::find(keys, 20) != keys.end());
  CHECK(std::ranges::find(keys, 21) != keys.end());
  CHECK(std::ranges::find(keys, 22) != keys.end());
}
