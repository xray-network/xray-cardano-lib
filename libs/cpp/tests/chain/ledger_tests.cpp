#include <cardano/chain/ledger.hpp>
#include <cardano/chain/plutus_data.hpp>
#include <cardano/core/bytes.hpp>
#include <cardano/crypto/primitives.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>

using namespace cardano;

namespace {

[[nodiscard]] core::cbor::Value decode(std::string_view hex) {
  const auto bytes = core::hex_to_bytes(hex);
  REQUIRE(bytes);
  const auto value = core::cbor::decode_cbor(*bytes);
  REQUIRE(value);
  return *value;
}

}  // namespace

TEST_CASE("network information constants match the frozen ledger contract", "[chain][ledger]") {
  CHECK(chain::TESTNET_NETWORK_INFO == chain::NetworkInfo{0U, 1'097'911'063U});
  CHECK(chain::MAINNET_NETWORK_INFO == chain::NetworkInfo{1U, 764'824'073U});
  CHECK(chain::PREVIEW_NETWORK_INFO == chain::NetworkInfo{0U, 2U});
  CHECK(chain::PREPROD_NETWORK_INFO == chain::NetworkInfo{0U, 1U});
  CHECK(chain::SANCHO_NETWORK_INFO == chain::NetworkInfo{0U, 4U});
}

TEST_CASE("ledger hashes consume preserved CBOR and script namespaces", "[chain][ledger]") {
  const auto value = decode("9f01ff");
  const auto hash = chain::hash_cbor_value(value);
  REQUIRE(hash);
  CHECK(hash->to_bytes() == crypto::blake2b256(*core::hex_to_bytes("9f01ff")));

  const auto script = *core::hex_to_bytes("0102");
  core::Bytes namespaced{core::Byte{3}, core::Byte{1}, core::Byte{2}};
  CHECK(chain::hash_script(3U, script).to_bytes() == crypto::blake2b224(namespaced));
}

TEST_CASE("ledger fee operations use exact checked arithmetic", "[chain][ledger]") {
  const auto no_script = chain::min_no_script_fee(decode("80"), chain::LinearFee{44U, 155'381U});
  REQUIRE(no_script);
  CHECK(*no_script == 155'425U);

  const auto legacy = chain::compute_total_ex_units(decode("8184000000820102"));
  REQUIRE(legacy);
  CHECK(*legacy == chain::ExUnits{1, 2});

  const auto mapped = chain::compute_total_ex_units(decode("a18200008200820304"));
  REQUIRE(mapped);
  CHECK(*mapped == chain::ExUnits{3, 4});

  const auto script_fee =
      chain::min_script_fee(chain::ExUnits{4, 6}, chain::ExUnitPrices{1U, 2U, 1U, 3U});
  REQUIRE(script_fee);
  CHECK(*script_fee == 4U);
  CHECK_FALSE(chain::min_script_fee(chain::ExUnits{1, 1}, chain::ExUnitPrices{1U, 0U, 1U, 1U}));

  const auto first_tier = chain::min_reference_script_fee(25'600U, 10U);
  const auto next_byte = chain::min_reference_script_fee(25'601U, 10U);
  REQUIRE(first_tier);
  REQUIRE(next_byte);
  CHECK(*first_tier == 256'000U);
  CHECK(*next_byte == 256'012U);

  const auto total = chain::min_fee(decode("80"), chain::LinearFee{44U, 155'381U}, 4U, 12U);
  REQUIRE(total);
  CHECK(*total == 155'441U);
}

TEST_CASE("ExUnits and prices own their specialized JSON contracts", "[chain][ledger][json]") {
  const auto units =
      chain::ExUnits::from_json(R"({"mem":9007199254740991,"steps":"9223372036854775807"})");
  REQUIRE(units);
  CHECK(*units == chain::ExUnits{9'007'199'254'740'991LL, INT64_MAX});
  CHECK(units->to_json() == R"({"mem":9007199254740991,"steps":"9223372036854775807"})");
  CHECK_FALSE(chain::ExUnits::from_json(R"({"mem":-1,"steps":1})"));
  CHECK_FALSE(chain::ExUnits::from_json(R"({"mem":1,"steps":1,"extra":0})"));

  const auto prices = chain::ExUnitPrices::from_json(
      R"({"mem_price":{"numerator":1,"denominator":2},"step_price":{"numerator":"9007199254740992","denominator":3}})");
  REQUIRE(prices);
  CHECK(prices->memory_numerator == 1U);
  CHECK(prices->memory_denominator == 2U);
  CHECK(prices->steps_numerator == 9'007'199'254'740'992ULL);
  CHECK(prices->steps_denominator == 3U);
  CHECK(
      prices->to_json() ==
      R"({"mem_price":{"denominator":2,"numerator":1},"step_price":{"denominator":3,"numerator":"9007199254740992"}})");
  CHECK_FALSE(chain::ExUnitPrices::from_json(
      R"({"mem_price":{"numerator":1,"denominator":0},"step_price":{"numerator":1,"denominator":1}})"));
}

TEST_CASE("minimum ADA converges across changed coin head widths", "[chain][ledger]") {
  const auto required = chain::min_ada_required(decode("82410000"), 10U);
  REQUIRE(required);
  CHECK(*required == 1'660U);
  CHECK_FALSE(chain::min_ada_required(decode("80"), 10U));
}

TEST_CASE("arbitrary metadata bytes use consecutive 64-byte chunks", "[chain][ledger]") {
  core::Bytes bytes(130U);
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    bytes[index] = static_cast<core::Byte>(index);
  }
  const auto metadata = chain::encode_arbitrary_bytes_as_metadatum(bytes);
  REQUIRE(metadata.as_array());
  REQUIRE(metadata.as_array()->values.size() == 3U);
  CHECK(metadata.as_array()->values[0].as_byte_string()->value.size() == 64U);
  CHECK(metadata.as_array()->values[1].as_byte_string()->value.size() == 64U);
  CHECK(metadata.as_array()->values[2].as_byte_string()->value.size() == 2U);
  CHECK(chain::decode_arbitrary_bytes_from_metadatum(metadata) == bytes);
  CHECK_FALSE(chain::decode_arbitrary_bytes_from_metadatum(decode("01")));

  const auto empty = chain::encode_arbitrary_bytes_as_metadatum({});
  REQUIRE(empty.as_array());
  CHECK(empty.as_array()->values.empty());
}

TEST_CASE("script-data hashing follows normal and datums-only domains", "[chain][ledger]") {
  const auto datums = decode("8101");
  const auto language_views = decode("a0");
  const auto datums_only = chain::calc_script_data_hash(std::nullopt, datums, language_views);
  REQUIRE(datums_only);
  REQUIRE(*datums_only);
  CHECK((*datums_only)->to_bytes() == crypto::blake2b256(*core::hex_to_bytes("a0d901028101a0")));

  const auto normal = chain::calc_script_data_hash(decode("80"), datums, language_views);
  REQUIRE(normal);
  REQUIRE(*normal);
  CHECK((*normal)->to_bytes() == crypto::blake2b256(*core::hex_to_bytes("80d901028101a0")));

  const auto absent = chain::calc_script_data_hash(std::nullopt, std::nullopt, language_views);
  REQUIRE(absent);
  CHECK_FALSE(*absent);

  const auto witness =
      chain::calc_script_data_hash_from_witness(decode("a20481010580"), language_views);
  REQUIRE(witness);
  REQUIRE(*witness);
  CHECK((*witness)->to_bytes() == (*normal)->to_bytes());
  const auto incomplete =
      chain::calc_script_data_hash_from_witness(decode("a1048101"), language_views);
  REQUIRE(incomplete);
  CHECK_FALSE(*incomplete);
}

TEST_CASE("vkey witnesses sign the exact body hash", "[chain][ledger]") {
  auto private_key =
      crypto::PrivateKey::from_bytes(*core::hex_to_bytes("000102030405060708090a0b0c0d0e0f"
                                                         "101112131415161718191a1b1c1d1e1f"));
  auto body_hash =
      crypto::TransactionHash::from_bytes(crypto::blake2b256(*core::hex_to_bytes("a0")));
  REQUIRE(private_key);
  REQUIRE(body_hash);
  const auto witness = chain::make_vkey_witness(*body_hash, *private_key);
  REQUIRE(witness);
  REQUIRE(witness->as_array());
  REQUIRE(witness->as_array()->values.size() == 2U);
  const auto* public_key = witness->as_array()->values[0].as_byte_string();
  const auto* signature = witness->as_array()->values[1].as_byte_string();
  REQUIRE(public_key);
  REQUIRE(signature);
  CHECK(crypto::verify_ed25519(public_key->value, body_hash->span(), signature->value));

  CHECK(chain::genesis_txid_shelley(*core::hex_to_bytes("0102")).to_bytes() ==
        crypto::blake2b256(*core::hex_to_bytes("0102")));
}

TEST_CASE("ledger deposit and implicit-input accounting covers Conway tags", "[chain][ledger]") {
  using core::BigInteger;
  using core::cbor::Value;
  auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto certificates =
      Value::tag(BigInteger(std::uint64_t{258}),
                 Value::array({
                     Value::array({u(0), Value::null()}),
                     Value::array({u(3), Value::array({
                                             Value::byte_string(core::Bytes(28)),
                                             Value::null(),
                                             Value::null(),
                                             Value::null(),
                                             Value::null(),
                                             Value::null(),
                                             Value::array({}),
                                         })}),
                     Value::array({u(7), Value::null(), u(33)}),
                     Value::array({u(1), Value::null()}),
                     Value::array({u(8), Value::null(), u(44)}),
                     Value::array({u(15), Value::null()}),
                     Value::array({u(17), Value::null(), u(55)}),
                 }),
                 core::cbor::HeadWidth::two);
  const auto proposals = Value::tag(BigInteger(std::uint64_t{258}),
                                    Value::array({
                                        Value::array({u(66), Value::null(), Value::null()}),
                                    }),
                                    core::cbor::HeadWidth::two);
  const auto body = Value::map({
      {u(4), certificates},
      {u(5), Value::map({
                 {Value::byte_string(core::Bytes(29)), u(77)},
             })},
      {u(20), proposals},
  });
  CHECK(chain::get_deposit(body, 10, 20).value() == 10 + 20 + 33 + 66);
  CHECK(chain::get_implicit_input(body, 10, 20).value() == 10 + 44 + 10 + 55 + 77);
}

TEST_CASE("generic CBOR JSON preserves the frozen large-integer distinction", "[chain][ledger]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto large = BigInteger::from_decimal("9007199254740992").value();
  const auto value = Value::map({
      {Value::text_string("large"), Value::unsigned_integer(large)},
      {Value::text_string("bytes"), Value::byte_string(*core::hex_to_bytes("dead"))},
  });
  const auto json = chain::cbor_value_to_json(value);
  REQUIRE(json);
  CHECK(json->find("\"9007199254740992\"") != std::string::npos);
  const auto roundtrip = chain::cbor_value_from_json(*json);
  REQUIRE(roundtrip);
  REQUIRE(roundtrip->as_array());
  CHECK(roundtrip->as_array()->values.size() == 2);

  const auto conway = chain::cbor_value_to_json(value, true);
  REQUIRE(conway);
  CHECK(conway->find("\"k\"") != std::string::npos);
  CHECK(conway->find("\"v\"") != std::string::npos);
}

TEST_CASE("metadata JSON implements detailed and deterministic basic schemas", "[chain][ledger]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto metadata = Value::map({
      {Value::text_string("z"), Value::byte_string(*core::hex_to_bytes("01"))},
      {Value::unsigned_integer(BigInteger(std::uint64_t{2})),
       Value::array({Value::text_string("x")})},
  });
  const auto detailed =
      chain::decode_metadatum_to_json_str(metadata, chain::MetadataJsonSchema::detailed);
  REQUIRE(detailed);
  const auto detailed_back =
      chain::encode_json_str_to_metadatum(*detailed, chain::MetadataJsonSchema::detailed);
  REQUIRE(detailed_back);
  CHECK(detailed_back->semantic_equal(metadata));

  const auto basic =
      chain::decode_metadatum_to_json_str(metadata, chain::MetadataJsonSchema::basic_conversions);
  REQUIRE(basic);
  CHECK(basic->find("\"0x01\"") != std::string::npos);
  const auto basic_back = chain::encode_json_str_to_metadatum(
      R"({"z":"0x01","2":["x"]})", chain::MetadataJsonSchema::basic_conversions);
  REQUIRE(basic_back);
  CHECK_FALSE(chain::decode_metadatum_to_json_str(Value::byte_string({}),
                                                  chain::MetadataJsonSchema::no_conversions));
  CHECK_FALSE(
      chain::encode_json_str_to_metadatum("null", chain::MetadataJsonSchema::basic_conversions));
}

TEST_CASE("Plutus JSON basic and detailed schemas enforce their variant boundaries",
          "[chain][ledger]") {
  using core::BigInteger;
  const auto data = chain::PlutusData::constr(
      BigInteger(std::uint64_t{2}), {
                                        chain::PlutusData::integer(BigInteger(std::int64_t{-7})),
                                        chain::PlutusData::bytes(*core::hex_to_bytes("abcd")),
                                    });
  const auto cbor = data.to_cbor_value();
  REQUIRE(cbor);
  const auto detailed =
      chain::decode_plutus_datum_to_json_str(*cbor, chain::CardanoNodePlutusDatumSchema::detailed);
  REQUIRE(detailed);
  CHECK(detailed->find("\"constructor\":2") != std::string::npos);
  const auto roundtrip = chain::encode_json_str_to_plutus_datum(
      *detailed, chain::CardanoNodePlutusDatumSchema::detailed);
  REQUIRE(roundtrip);
  CHECK(roundtrip->semantic_equal(*cbor));
  CHECK_FALSE(
      chain::decode_plutus_datum_to_json_str(*cbor, chain::CardanoNodePlutusDatumSchema::basic));

  const auto basic = chain::encode_json_str_to_plutus_datum(
      R"([1,"0xab","text"])", chain::CardanoNodePlutusDatumSchema::basic);
  REQUIRE(basic);
  const auto basic_json =
      chain::decode_plutus_datum_to_json_str(*basic, chain::CardanoNodePlutusDatumSchema::basic);
  REQUIRE(basic_json);
  CHECK(basic_json->find("\"0xab\"") != std::string::npos);
}

TEST_CASE("all ledger JSON entry points enforce depth 128", "[chain][ledger]") {
  std::string json = "0";
  for (std::size_t depth = 0; depth < 129; ++depth) {
    json = "[" + json + "]";
  }
  CHECK_FALSE(chain::cbor_value_from_json(json));
  CHECK_FALSE(
      chain::encode_json_str_to_metadatum(json, chain::MetadataJsonSchema::basic_conversions));
  CHECK_FALSE(
      chain::encode_json_str_to_plutus_datum(json, chain::CardanoNodePlutusDatumSchema::basic));
}
