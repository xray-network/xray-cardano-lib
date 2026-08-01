#include <cardano/chain/byron.hpp>
#include <cardano/chain/era_models.hpp>
#include <cardano/chain/genesis.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace cardano;

namespace {

[[nodiscard]] std::string read(const std::filesystem::path& path) {
  std::ifstream stream(path);
  REQUIRE(stream.good());
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST_CASE("Byron semantic models preserve historical address vectors", "[chain][byron]") {
  const std::vector<std::string> historical{
      "DdzFFzCqrhsrcTVhLygT24QwTnNqQqQ8mZrq5jykUzMveU26sxaH529kMpo7VhPrt5pwW3dXeB2k3EEvKcNBRmzCfcQ7"
      "dTkyGzTs658C",
      "DdzFFzCqrht4it4GYgBp4J39FNnKBsPFejSppARXHCf2gGiTJcwXzpRvgDmxPvKQ8aZZmVqcLUz5L66a8Ja46pfKVtFR"
      "aKyn9eKdvpaC",
      "Ae2tdPwUPEZ4YjgvykNpoFeYUxoyhNj2kg8KfKWN2FizsSpLUPv68MpTVDo",
      "2cWKMJemoBaipzQe9BArYdo2iPUfJQdZAjm4iCzDA1AfNxJSTgm9FZQTmFCYhKkeYrede",
  };
  for (const auto& encoded : historical) {
    const auto address = chain::ByronAddress::from_base58(encoded);
    REQUIRE(address);
    const auto content = address->content();
    REQUIRE(content);
    const auto rebuilt = chain::ByronAddress::from_content(*content);
    REQUIRE(rebuilt);
    CHECK(rebuilt->to_base58() == encoded);
  }

  const auto first = chain::ByronAddress::from_base58(historical.front());
  REQUIRE(first);
  const auto content = first->content();
  REQUIRE(content);
  const auto public_key = crypto::Bip32PublicKey::from_hex(
      "6a509689c653175865985ad1e0eb5ff9ada6997aa403e648614b3b78fcba9c273"
      "08228d9872af8b65b987ff23e1a20cd90d8346c31f0edb8998952dc67665580");
  REQUIRE(public_key);
  const auto identical = content->identical_with(*public_key);
  REQUIRE(identical);
  CHECK(*identical);

  const auto mainnet = chain::ByronAddress::from_base58(historical[2]);
  const auto testnet = chain::ByronAddress::from_base58(historical[3]);
  REQUIRE(mainnet);
  REQUIRE(testnet);
  CHECK(mainnet->protocol_magic() == core::BYRON_MAINNET_NETWORK_MAGIC);
  CHECK(testnet->protocol_magic() == core::BYRON_TESTNET_NETWORK_MAGIC);
}

TEST_CASE("incremental CRC and Byron genesis redeem match TypeScript vectors", "[chain][byron]") {
  chain::Crc32 crc;
  const std::string left = "The quick brown ";
  const std::string right = "fox jumps over the lazy dog";
  crc.update(std::as_bytes(std::span(left)));
  crc.update(std::as_bytes(std::span(right)));
  CHECK(crc.finalize() == 0x414fa339U);

  const auto public_key = crypto::PublicKey::from_hex(
      "0001b7bc9c13cc270bd33a76fb5c9f23e9a7ffb85a62f4982659f6c51fda052f");
  REQUIRE(public_key);
  const auto redeem = chain::genesis_txid_byron(*public_key);
  REQUIRE(redeem);
  CHECK(redeem->transaction_id().to_hex() ==
        "927edb96f3386ab91b5f5d85d84cb4253c65b1c2f65fa7df25f81fab1d62987a");
  CHECK(redeem->address().to_base58() ==
        "Ae2tdPwUPEZ9vtyppa1FdJzvqJZkEcXgdHxVYAzTWcPaoNycVq5rc36LC1S");
}

TEST_CASE("bootstrap witness CBOR and JSON own all fields", "[chain][byron][witness]") {
  const auto extended_public_key = crypto::Bip32PublicKey::from_hex(
      "6a509689c653175865985ad1e0eb5ff9ada6997aa403e648614b3b78fcba9c273"
      "08228d9872af8b65b987ff23e1a20cd90d8346c31f0edb8998952dc67665580");
  REQUIRE(extended_public_key);
  const auto public_key = extended_public_key->public_key();
  const auto signature = crypto::Ed25519Signature::from_hex(std::string(128, '2'));
  REQUIRE(signature);
  const auto witness = chain::BootstrapWitness::create(
      public_key, *signature, extended_public_key->chain_code(),
      chain::AddrAttributes::bootstrap_era(chain::HDAddressPayload(*core::hex_to_bytes("010203")),
                                           core::ProtocolMagic(core::BYRON_TESTNET_NETWORK_MAGIC)));
  REQUIRE(witness);
  const auto encoded = witness->to_cbor();
  REQUIRE(encoded);
  const auto decoded = chain::BootstrapWitness::from_cbor(*encoded);
  REQUIRE(decoded);
  CHECK(*decoded == *witness);
  const auto from_json = chain::BootstrapWitness::from_json(witness->to_json());
  REQUIRE(from_json);
  CHECK(*from_json == *witness);
  REQUIRE(witness->to_address_content());
  CHECK_FALSE(chain::BootstrapWitness::create(public_key, *signature, *core::hex_to_bytes("00"),
                                              chain::AddrAttributes{}));
}

TEST_CASE("Byron era models enforce embedded transaction productions", "[chain][byron][cddl]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto embedded = [&](Value value) {
    return Value::tag(BigInteger(std::uint64_t{24}),
                      Value::byte_string(core::cbor::encode_cbor(value).value()));
  };
  const auto encode = [](const Value& value) { return core::cbor::encode_cbor(value).value(); };

  const auto address = Value::array({
      embedded(Value::array({Value::byte_string(core::Bytes(28)), Value::map({}), u(0)})),
      u(0),
  });
  const auto input =
      Value::array({u(0), embedded(Value::array({Value::byte_string(core::Bytes(32)), u(0)}))});
  const auto transaction = Value::array(
      {Value::array({input}), Value::array({Value::array({address, u(1)})}), Value::map({})});

  CHECK(chain::ByronTx::from_cbor(encode(transaction)));
  CHECK(chain::ByronTxIn::from_cbor(encode(input)));
  CHECK_FALSE(chain::ByronTx::from_cbor(
      encode(Value::array({Value::array({}), Value::array({}), Value::map({})}))));
  CHECK_FALSE(
      chain::ByronTxIn::from_cbor(encode(Value::array({u(0), Value::byte_string(core::Bytes{})}))));
}

TEST_CASE("genesis parsers validate all captured TypeScript fixtures", "[chain][genesis]") {
  const auto root = std::filesystem::path(CARDANO_REPOSITORY_ROOT) /
                    ".xray/updates/providers/cardano-multiplatform-lib/"
                    "0001-cardano-multiplatform-lib/artifacts/test-vectors/genesis";
  std::size_t byron_count = 0;
  bool federal_seen = false;
  for (const auto& entry : std::filesystem::directory_iterator(root / "byron")) {
    const auto genesis = chain::parse_byron_genesis(read(entry.path()));
    REQUIRE(genesis);
    ++byron_count;
    if (genesis->protocol_magic.value() == 633343913U) {
      federal_seen = true;
      CHECK(genesis->epoch_stability_depth == 2160);
      CHECK(genesis->start_time == 1506450213);
      CHECK(genesis->slot_duration_milliseconds == 20000);
      CHECK(genesis->fee_policy.coefficient == 43946000000ULL);
      CHECK(genesis->fee_policy.constant == 155381000000000ULL);
      CHECK(genesis->avvm_distribution.at("-0BJDi-gauylk4LptQTgjMeo7kY9lTCbZv12vwOSTZk=")
                .to_decimal() == "9999300000000");
    }
  }
  CHECK(byron_count == 4);
  CHECK(federal_seen);

  const auto test = chain::parse_shelley_genesis(read(root / "shelley/test.json"));
  const auto yaci = chain::parse_shelley_genesis(read(root / "shelley/test-yaci.json"));
  REQUIRE(test);
  REQUIRE(yaci);
  CHECK(test->epoch_length.to_decimal() == "432000");
  CHECK(test->network_id == 0);
  CHECK(test->network_magic == 764824073U);
  CHECK(test->initial_funds.at("605276322ac7882434173dcc6441905f6737689bd309b68ad8b3614fd8")
            .to_decimal() == "3000000000000000");
  CHECK(yaci->epoch_length.to_decimal() == "600");
  CHECK(yaci->network_magic == 42U);
}
