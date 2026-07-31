#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <vector>

#include "../../src/plutus/phase_two_context.hpp"
#include "cardano/crypto/primitives.hpp"
#include "cardano/plutus/uplc.hpp"

using namespace cardano;

TEST_CASE("UPLC exposes every stable builtin tag", "[plutus][uplc]") {
  for (std::uint8_t tag = 0; tag <= 100U; ++tag) {
    const auto builtin = plutus::builtin_tag(tag);
    REQUIRE(builtin);
    CHECK(plutus::builtin_tag(*builtin) == tag);
    CHECK_FALSE(plutus::builtin_name(*builtin).empty());
    const auto named = plutus::builtin_from_name(plutus::builtin_name(*builtin));
    REQUIRE(named);
    CHECK(*named == *builtin);
  }
  CHECK_FALSE(plutus::builtin_tag(101U));
  CHECK(plutus::builtin_tag(plutus::Builtin::append_string) == 22U);
  CHECK(plutus::builtin_tag(plutus::Builtin::verify_ecdsa_secp256k1_signature) == 52U);
  CHECK(plutus::builtin_tag(plutus::Builtin::verify_schnorr_secp256k1_signature) == 53U);
  CHECK(plutus::builtin_tag(plutus::Builtin::mk_pair_data) == 48U);
  CHECK(plutus::builtin_tag(plutus::Builtin::serialise_data) == 51U);
  CHECK(plutus::builtin_tag(plutus::Builtin::bls12_381_g1_add) == 54U);
}

TEST_CASE("UPLC constants enforce homogeneous collections", "[plutus][uplc]") {
  const auto integers = plutus::UplcType::primitive(plutus::UplcTypeTag::integer);
  const auto valid = plutus::UplcConstant::list(
      integers, {plutus::UplcConstant::integer(core::BigInteger(std::int64_t{1}))});
  REQUIRE(valid);
  CHECK(valid->type() == plutus::UplcType::list(integers));

  const auto invalid = plutus::UplcConstant::list(integers, {plutus::UplcConstant::boolean(true)});
  CHECK_FALSE(invalid);
}

TEST_CASE("UPLC terms preserve immutable recursive structure", "[plutus][uplc]") {
  const auto program = plutus::UplcProgram{
      plutus::UplcVersion::v1_0_0(),
      plutus::UplcTerm::apply(plutus::UplcTerm::lambda(plutus::UplcTerm::variable(1)),
                              plutus::UplcTerm::constant(plutus::UplcConstant::integer(
                                  core::BigInteger(std::int64_t{42}))))};
  CHECK(program.version == plutus::UplcVersion::v1_0_0());
  CHECK(std::holds_alternative<plutus::UplcApply>(program.term.node()));
}

TEST_CASE("raw parameter application matches TypeScript vectors and is strict",
          "[plutus][uplc][parameters]") {
  const auto identity = core::hex_to_bytes("4d01000033222220051200120011");
  REQUIRE(identity);

  const auto no_parameters = core::hex_to_bytes("80");
  REQUIRE(no_parameters);
  const auto unchanged = plutus::apply_params_to_script(*no_parameters, *identity);
  REQUIRE(unchanged);
  CHECK(*unchanged == *identity);

  const auto one_parameter = core::hex_to_bytes("81182a");
  const auto one_expected = core::hex_to_bytes("54010000333222220051200120014c0102182a0001");
  REQUIRE(one_parameter);
  REQUIRE(one_expected);
  const auto one = plutus::apply_params_to_script(*one_parameter, *identity);
  REQUIRE(one);
  CHECK(*one == *one_expected);

  const auto two_parameters = core::hex_to_bytes("820102");
  const auto two_expected =
      core::hex_to_bytes("58180100003333222220051200120014c10101004c0101020001");
  REQUIRE(two_parameters);
  REQUIRE(two_expected);
  const auto two = plutus::apply_params_to_script(*two_parameters, *identity);
  REQUIRE(two);
  CHECK(*two == *two_expected);

  const auto non_array = core::hex_to_bytes("01");
  const auto trailing_parameters = core::hex_to_bytes("8000");
  REQUIRE(non_array);
  REQUIRE(trailing_parameters);
  CHECK_FALSE(plutus::apply_params_to_script(*non_array, *identity));
  CHECK_FALSE(plutus::apply_params_to_script(*trailing_parameters, *identity));

  const auto free_program = plutus::encode_program_envelope(
      {plutus::UplcVersion::v1_0_0(), plutus::UplcTerm::variable(1)});
  REQUIRE(free_program);
  CHECK_FALSE(plutus::apply_params_to_script(*no_parameters, *free_program));

  const auto wrapped = core::cbor::encode_cbor(core::cbor::Value::byte_string(*identity));
  REQUIRE(wrapped);
  CHECK_FALSE(plutus::apply_params_to_script(*no_parameters, *wrapped));

  const auto oversized_data = core::cbor::encode_cbor(
      core::cbor::Value::array({core::cbor::Value::byte_string(core::Bytes(65U))}));
  REQUIRE(oversized_data);
  CHECK_FALSE(plutus::apply_params_to_script(*oversized_data, *identity));
}

TEST_CASE("UPLC default cost models preserve semantics A through E", "[plutus][uplc][cost]") {
  const std::array arguments{plutus::UplcConstant::integer(core::BigInteger(std::int64_t{1})),
                             plutus::UplcConstant::integer(core::BigInteger(std::int64_t{2}))};
  CHECK(plutus::builtin_cost(plutus::Builtin::add_integer, arguments,
                             plutus::default_builtin_cost_model(plutus::SemanticsVariant::a)) ==
        plutus::MachineBudget{206'477, 2});
  for (const auto semantics : {plutus::SemanticsVariant::b, plutus::SemanticsVariant::c,
                               plutus::SemanticsVariant::d, plutus::SemanticsVariant::e}) {
    CHECK(plutus::builtin_cost(plutus::Builtin::add_integer, arguments,
                               plutus::default_builtin_cost_model(semantics)) ==
          plutus::MachineBudget{101'208, 2});
  }

  const std::array division_arguments{
      plutus::UplcConstant::integer(
          *core::BigInteger::from_decimal("340282366920938463463374607431768211456")),
      plutus::UplcConstant::integer(core::BigInteger(std::int64_t{2}))};
  CHECK(plutus::builtin_cost(plutus::Builtin::divide_integer, division_arguments,
                             plutus::default_builtin_cost_model(plutus::SemanticsVariant::b))
            .cpu == 228'831);
  CHECK(plutus::builtin_cost(plutus::Builtin::divide_integer, division_arguments,
                             plutus::default_builtin_cost_model(plutus::SemanticsVariant::c))
            .cpu == 136'916);
  CHECK(plutus::builtin_cost(plutus::Builtin::divide_integer, division_arguments,
                             plutus::default_builtin_cost_model(plutus::SemanticsVariant::d))
            .cpu == 228'831);
}

TEST_CASE("UPLC ledger cost arrays update named formula parameters", "[plutus][uplc][cost]") {
  std::vector<std::int64_t> parameters(332U, 1);
  parameters[0] = 10;
  parameters[1] = 20;
  parameters[2] = 30;
  parameters[3] = 40;
  const auto model = plutus::make_builtin_cost_model(plutus::PlutusLanguage::v1, parameters,
                                                     plutus::SemanticsVariant::a);
  const std::array arguments{plutus::UplcConstant::integer(core::BigInteger(std::int64_t{1})),
                             plutus::UplcConstant::integer(core::BigInteger(std::int64_t{2}))};
  CHECK(plutus::builtin_cost(plutus::Builtin::add_integer, arguments, model) ==
        plutus::MachineBudget{30, 70});

  std::vector<std::int64_t> v3_parameters(350U, 1);
  for (std::size_t index = 49U; index < 60U; ++index) {
    v3_parameters[index] = static_cast<std::int64_t>(index - 48U);
  }
  const auto v3_model = plutus::make_builtin_cost_model(plutus::PlutusLanguage::v3, v3_parameters,
                                                        plutus::SemanticsVariant::e);
  CHECK(plutus::builtin_cost(plutus::Builtin::divide_integer, arguments, v3_model) ==
        plutus::MachineBudget{27, 119});
}

TEST_CASE("UPLC Flat codec is stable and strictly bounded", "[plutus][uplc]") {
  const auto program = plutus::UplcProgram{plutus::UplcVersion::v1_0_0(),
                                           plutus::UplcTerm::builtin(plutus::Builtin::add_integer)};
  const auto encoded = plutus::encode_flat_program(program);
  REQUIRE(encoded);
  CHECK(core::bytes_to_hex(*encoded) == "0100007001");
  CHECK(plutus::decode_flat_program(*encoded) == program);

  auto trailing = *encoded;
  trailing.push_back(std::byte{0});
  CHECK_FALSE(plutus::decode_flat_program(trailing));

  const auto old_constr =
      plutus::encode_flat_program({plutus::UplcVersion::v1_0_0(), plutus::UplcTerm::constr(0, {})});
  CHECK_FALSE(old_constr);

  const auto constr_program = plutus::UplcProgram{
      plutus::UplcVersion::v1_1_0(),
      plutus::UplcTerm::constr(0U, {
                                       plutus::UplcTerm::constant(plutus::UplcConstant::unit()),
                                       plutus::UplcTerm::constant(plutus::UplcConstant::unit()),
                                   })};
  const auto constr_flat = plutus::encode_flat_program(constr_program);
  REQUIRE(constr_flat);
  plutus::ProgramDecodeOptions one_field;
  one_field.max_constr_fields = 1U;
  CHECK_FALSE(plutus::decode_flat_program(*constr_flat, one_field));

  const auto data_program = plutus::UplcProgram{
      plutus::UplcVersion::v1_0_0(), plutus::UplcTerm::constant(plutus::UplcConstant::data(
                                         plutus::Data::bytes(core::Bytes(65U))))};
  const auto data_flat = plutus::encode_flat_program(data_program);
  REQUIRE(data_flat);
  CHECK(plutus::decode_flat_program(*data_flat));
  const auto legacy_data_cbor =
      core::cbor::encode_cbor(core::cbor::Value::byte_string(core::Bytes(65U)));
  REQUIRE(legacy_data_cbor);
  const auto legacy_data = core::cbor::decode_cbor(*legacy_data_cbor);
  REQUIRE(legacy_data);
  CHECK_FALSE(plutus::validate_plutus_data_node(*legacy_data, 128U));
  CHECK(plutus::validate_plutus_data_node(*legacy_data, 128U, false));
}

TEST_CASE("UPLC serialized envelopes preserve compatibility boundaries", "[plutus][uplc]") {
  const auto program = plutus::UplcProgram{plutus::UplcVersion::v1_0_0(),
                                           plutus::UplcTerm::constant(plutus::UplcConstant::integer(
                                               core::BigInteger(std::int64_t{42})))};
  const auto envelope = plutus::encode_program_envelope(program);
  REQUIRE(envelope);
  CHECK(plutus::decode_program_envelope(*envelope) == program);

  auto trailing = *envelope;
  trailing.push_back(std::byte{0xff});
  CHECK_FALSE(plutus::decode_program_envelope(trailing));
  CHECK(plutus::decode_program_envelope_compatible(trailing, 1) == program);
  CHECK(plutus::decode_program_envelope_compatible(trailing, 2) == program);
  CHECK_FALSE(plutus::decode_program_envelope_compatible(trailing, 3));
}

TEST_CASE("UPLC text parser scopes variables and normalizes application", "[plutus][uplc]") {
  const auto identity = plutus::parse_uplc_text("(program 1.0.0 (lam x x))");
  REQUIRE(identity);
  const auto* lambda = std::get_if<plutus::UplcLambda>(&identity->term.node());
  REQUIRE(lambda != nullptr);
  const auto* variable = std::get_if<plutus::UplcVariable>(&lambda->body->node());
  REQUIRE(variable != nullptr);
  CHECK(variable->index == 1);

  const auto addition = plutus::parse_uplc_text(
      "(program 1.0.0 "
      "[(builtin addInteger) (con integer 3) (con integer 4)])");
  REQUIRE(addition);
  CHECK(std::holds_alternative<plutus::UplcApply>(addition->term.node()));
  const auto free = plutus::parse_uplc_text("(program 1.0.0 free)");
  REQUIRE(free);
  CHECK_FALSE(plutus::evaluate_program(*free, {1'000'000, 1'000'000}));
  CHECK_FALSE(plutus::parse_uplc_text("(program 1.0.0 (constr 0))"));
}

TEST_CASE("UPLC CEK evaluates closures, builtins, traces, and budgets", "[plutus][uplc]") {
  const auto constant = plutus::parse_uplc_text("(program 1.0.0 (con integer 23))");
  REQUIRE(constant);
  const auto constant_result = plutus::evaluate_program(*constant, {16'100, 200});
  REQUIRE(constant_result);
  CHECK(constant_result->spent == plutus::MachineBudget{16'100, 200});
  CHECK_FALSE(plutus::evaluate_program(*constant, {16'099, 200}));

  const auto identity = plutus::parse_uplc_text("(program 1.0.0 [(lam x x) (con integer 42)])");
  REQUIRE(identity);
  const auto identity_result = plutus::evaluate_program(*identity, {1'000'000, 10'000});
  REQUIRE(identity_result);
  const auto* identity_constant =
      std::get_if<plutus::UplcConstant>(&identity_result->result.node());
  REQUIRE(identity_constant != nullptr);
  CHECK(std::get<core::BigInteger>(identity_constant->value()) ==
        core::BigInteger(std::int64_t{42}));

  const auto addition = plutus::parse_uplc_text(
      "(program 1.0.0 "
      "[(builtin addInteger) (con integer 3) (con integer 4)])");
  REQUIRE(addition);
  const auto addition_result = plutus::evaluate_program(*addition, {2'000'000, 20'000});
  REQUIRE(addition_result);
  const auto* sum = std::get_if<plutus::UplcConstant>(&addition_result->result.node());
  REQUIRE(sum != nullptr);
  CHECK(std::get<core::BigInteger>(sum->value()) == core::BigInteger(std::int64_t{7}));

  const auto trace = plutus::parse_uplc_text(
      "(program 1.0.0 "
      "[(force (builtin trace)) (con string \"hello\") "
      "(con unit ())])");
  REQUIRE(trace);
  const auto trace_result = plutus::evaluate_program(*trace, {2'000'000, 20'000});
  REQUIRE(trace_result);
  CHECK(trace_result->logs == std::vector<std::string>{"hello"});
}

TEST_CASE("raw phase-two valuation validates protocols and handles empty redeemers",
          "[plutus][phase-two]") {
  const auto transaction = core::hex_to_bytes("84a0a0f5f6");
  const auto cost_models = core::hex_to_bytes("a0");
  REQUIRE(transaction);
  REQUIRE(cost_models);
  const auto empty = plutus::eval_phase_two_raw(*transaction, {}, *cost_models,
                                                {10'000'000, 10'000'000}, {0, 0, 1'000}, 9, true);
  REQUIRE(empty);
  CHECK(empty->empty());
  CHECK_FALSE(plutus::eval_phase_two_raw(*transaction, {}, *cost_models, {10'000'000, 10'000'000},
                                         {0, 0, 1'000}, 12, false));
}

TEST_CASE("raw phase-two valuation resolves V1 spending scripts and rewrites ExUnits",
          "[plutus][phase-two]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto script = plutus::encode_program_envelope(
      {plutus::UplcVersion::v1_0_0(),
       plutus::UplcTerm::lambda(plutus::UplcTerm::lambda(
           plutus::UplcTerm::lambda(plutus::UplcTerm::constant(plutus::UplcConstant::unit()))))});
  REQUIRE(script);
  core::Bytes hash_input{std::byte{1}};
  hash_input.insert(hash_input.end(), script->begin(), script->end());
  const auto script_hash = crypto::blake2b224(hash_input);

  const auto input = Value::array({Value::byte_string(core::Bytes(32U, std::byte{0})), u(0)});
  const auto input_bytes = core::cbor::encode_cbor(input);
  REQUIRE(input_bytes);
  core::Bytes address{std::byte{0x70}};
  address.insert(address.end(), script_hash.begin(), script_hash.end());
  const auto datum = u(42);
  const auto datum_bytes = core::cbor::encode_cbor(datum);
  REQUIRE(datum_bytes);
  const auto output = Value::array({Value::byte_string(std::move(address)), u(1'000'000),
                                    Value::byte_string(crypto::blake2b256(*datum_bytes))});
  const auto output_bytes = core::cbor::encode_cbor(output);
  REQUIRE(output_bytes);
  const auto body = Value::map({
      {u(0), Value::array({input})},
      {u(1), Value::array({})},
      {u(2), u(0)},
  });
  const auto redeemer = Value::array({
      u(0),
      u(0),
      u(7),
      Value::array({u(0), u(0)}),
  });
  const auto witnesses = Value::map({
      {u(3), Value::array({Value::byte_string(*script)})},
      {u(4), Value::array({datum})},
      {u(5), Value::array({redeemer})},
  });
  const auto transaction = core::cbor::encode_cbor(Value::array({
      body,
      witnesses,
      Value::boolean(true),
      Value::null(),
  }));
  REQUIRE(transaction);

  std::vector<Value> parameters(332U, u(1));
  for (const auto [index, value] :
       std::array<std::pair<std::size_t, std::uint64_t>, 16>{{{17U, 16'000U},
                                                              {18U, 100U},
                                                              {19U, 23'000U},
                                                              {20U, 100U},
                                                              {21U, 23'000U},
                                                              {22U, 100U},
                                                              {23U, 23'000U},
                                                              {24U, 100U},
                                                              {25U, 23'000U},
                                                              {26U, 100U},
                                                              {27U, 23'000U},
                                                              {28U, 100U},
                                                              {29U, 100U},
                                                              {30U, 100U},
                                                              {31U, 23'000U},
                                                              {32U, 100U}}}) {
    parameters[index] = u(value);
  }
  const auto cost_models = core::cbor::encode_cbor(Value::map({
      {u(0), Value::array(std::move(parameters))},
  }));
  REQUIRE(cost_models);

  const std::array utxos{plutus::PhaseTwoUtxo{*input_bytes, *output_bytes}};
  const auto result = plutus::eval_phase_two_raw(*transaction, utxos, *cost_models,
                                                 {10'000'000, 10'000'000}, {0, 0, 1'000}, 5, true);
  REQUIRE(result);
  REQUIRE(result->size() == 1U);
  CHECK(result->front().evaluation.cost == plutus::MachineBudget{209'100, 1'100});
  const auto rewritten = core::cbor::decode_cbor(result->front().redeemer);
  REQUIRE(rewritten);
  REQUIRE(rewritten->as_array());
  const auto* ex_units = rewritten->as_array()->values[3].as_array();
  REQUIRE(ex_units);
  CHECK(ex_units->values[0].as_unsigned()->value == BigInteger(std::uint64_t{1'100}));
  CHECK(ex_units->values[1].as_unsigned()->value == BigInteger(std::uint64_t{209'100}));
}

TEST_CASE("raw phase-two valuation constructs Babbage and Conway contexts", "[plutus][phase-two]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto evaluate_mint = [&](std::uint8_t language, bool unit_result) {
    auto body_term = unit_result ? plutus::UplcTerm::constant(plutus::UplcConstant::unit())
                                 : plutus::UplcTerm::constant(
                                       plutus::UplcConstant::integer(BigInteger(std::int64_t{1})));
    auto term = plutus::UplcTerm::lambda(std::move(body_term));
    if (language != 2U) {
      term = plutus::UplcTerm::lambda(std::move(term));
    }
    const auto script =
        plutus::encode_program_envelope({plutus::UplcVersion::v1_0_0(), std::move(term)});
    REQUIRE(script);
    core::Bytes hash_input{static_cast<std::byte>(language + 1U)};
    hash_input.insert(hash_input.end(), script->begin(), script->end());
    const auto policy = crypto::blake2b224(hash_input);
    const auto body = Value::map({
        {u(1), Value::array({})},
        {u(2), u(100)},
        {u(9), Value::map({
                   {Value::byte_string(policy), Value::map({})},
               })},
    });
    const auto redeemer = Value::array({u(1), u(0), u(7), Value::array({u(0), u(0)})});
    const auto witnesses = Value::map({
        {u(language == 1U ? 6U : 7U), Value::array({Value::byte_string(*script)})},
        {u(5), Value::array({redeemer})},
    });
    const auto transaction = core::cbor::encode_cbor(
        Value::array({body, witnesses, Value::boolean(true), Value::null()}));
    REQUIRE(transaction);
    std::vector<Value> parameters(400U, u(1));
    const auto cost_models = core::cbor::encode_cbor(Value::map({
        {u(language), Value::array(std::move(parameters))},
    }));
    REQUIRE(cost_models);
    return plutus::eval_phase_two_raw(*transaction, {}, *cost_models, {10'000'000, 10'000'000},
                                      {0, 0, 1'000}, language == 1U ? 7U : 11U, true);
  };

  const auto babbage = evaluate_mint(1U, true);
  REQUIRE(babbage);
  REQUIRE(babbage->size() == 1U);

  const auto conway = evaluate_mint(2U, true);
  REQUIRE(conway);
  REQUIRE(conway->size() == 1U);

  CHECK_FALSE(evaluate_mint(2U, false));
}

TEST_CASE("ledger contexts use the exact V1 V2 and V3 Data layouts",
          "[plutus][phase-two][context]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto fields = [](const Value& value) -> const std::vector<Value>& {
    const auto* tag = value.as_tag();
    REQUIRE(tag != nullptr);
    const auto* values = tag->value->as_array();
    REQUIRE(values != nullptr);
    return values->values;
  };
  const core::Bytes transaction_id(32U, std::byte{1});
  const auto input = Value::array({Value::byte_string(transaction_id), u(2)});
  core::Bytes address{std::byte{0x60}};
  address.insert(address.end(), 28U, std::byte{3});
  const auto alonzo_output = Value::array({Value::byte_string(address), u(4'000'000)});
  const auto babbage_output = Value::map({
      {u(0), Value::byte_string(address)},
      {u(1), u(4'000'000)},
  });
  const auto body_value = Value::map({
      {u(0), Value::array({input})},
      {u(1), Value::array({babbage_output})},
      {u(2), u(170'000)},
      {u(3), u(20)},
      {u(8), u(10)},
  });
  const auto witnesses_value = Value::map({});
  const auto* body = body_value.as_map();
  const auto* witnesses = witnesses_value.as_map();
  REQUIRE(body != nullptr);
  REQUIRE(witnesses != nullptr);
  const plutus::detail::ContextRedeemer redeemer{0U, 0U, u(7)};

  const auto make = [&](std::uint8_t language, const Value& output) {
    return plutus::detail::make_script_context(
        redeemer, {redeemer}, *body, *witnesses, {{input, output}}, std::nullopt, {1'000, 0, 100},
        language == 0U ? 5U : (language == 1U ? 7U : 9U), language);
  };
  const auto decode = [](const plutus::Data& data) {
    const auto encoded = data.to_cbor();
    REQUIRE(encoded);
    const auto decoded = core::cbor::decode_cbor(*encoded);
    REQUIRE(decoded);
    return *decoded;
  };

  const auto v1_data = make(0U, alonzo_output);
  REQUIRE(v1_data);
  const auto v1 = decode(*v1_data);
  const auto& v1_context = fields(v1);
  REQUIRE(v1_context.size() == 2U);
  const auto& v1_info = fields(v1_context[0]);
  CHECK(v1_info.size() == 10U);
  CHECK(v1_context[1].as_tag()->tag == BigInteger(std::uint64_t{122}));
  const auto& input_info = v1_info[0].as_array()->values.front();
  const auto& output_reference = fields(input_info)[0];
  const auto& wrapped_transaction_id = fields(output_reference)[0];
  CHECK(fields(wrapped_transaction_id)[0].as_byte_string()->value == transaction_id);
  const auto body_bytes = core::cbor::encode_cbor(body_value, {.mode = core::cbor::Mode::preserve});
  REQUIRE(body_bytes);
  CHECK(fields(v1_info[9])[0].as_byte_string()->value == crypto::blake2b256(*body_bytes));

  const auto v2_data = make(1U, babbage_output);
  REQUIRE(v2_data);
  const auto v2 = decode(*v2_data);
  CHECK(fields(fields(v2)[0]).size() == 12U);
  CHECK(fields(v2)[1].as_tag()->tag == BigInteger(std::uint64_t{122}));

  const auto v3_data = make(2U, babbage_output);
  REQUIRE(v3_data);
  const auto v3 = decode(*v3_data);
  const auto& v3_context = fields(v3);
  REQUIRE(v3_context.size() == 3U);
  CHECK(fields(v3_context[0]).size() == 16U);
  CHECK(v3_context[1].as_unsigned()->value == BigInteger(std::uint64_t{7}));
  CHECK(v3_context[2].as_tag()->tag == BigInteger(std::uint64_t{122}));
  CHECK(fields(v3_context[2]).size() == 2U);

  const auto& range = fields(v1_info[6]);
  const auto& lower = fields(fields(range[0])[0]);
  const auto& upper = fields(fields(range[1])[0]);
  CHECK(lower[0].as_unsigned()->value == BigInteger(std::uint64_t{2'000}));
  CHECK(upper[0].as_unsigned()->value == BigInteger(std::uint64_t{3'000}));
}
