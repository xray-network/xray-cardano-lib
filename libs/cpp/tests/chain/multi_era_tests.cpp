#include <cardano/chain/era_models.hpp>
#include <cardano/chain/multi_era.hpp>
#include <cardano/core/bytes.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

using namespace cardano;

TEST_CASE("era-owned models keep nominal type and enforce declared wire shape",
          "[chain][era-model]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto certificate_node = Value::array({
      Value::unsigned_integer(BigInteger(std::uint64_t{0})),
      Value::array({
          Value::unsigned_integer(BigInteger(std::uint64_t{0})),
          Value::byte_string(core::Bytes(28)),
      }),
  });
  const auto encoded = core::cbor::encode_cbor(certificate_node);
  REQUIRE(encoded);
  const auto certificate = chain::Certificate::from_cbor(*encoded);
  REQUIRE(certificate);
  CHECK(certificate->discriminator() == 0);
  CHECK_FALSE(chain::Certificate::from_cbor(*core::cbor::encode_cbor(Value::map({}))));

  const auto u = [](std::uint64_t number) { return Value::unsigned_integer(BigInteger(number)); };
  const auto body = chain::TransactionBody::from_cbor(*core::cbor::encode_cbor(Value::map({
      {u(0), Value::tag(BigInteger(std::uint64_t{258}),
                        Value::array({Value::array({Value::byte_string(core::Bytes(32)), u(0)})}))},
      {u(1), Value::array({Value::map({
                 {u(0), Value::byte_string(core::Bytes{std::byte{0x60}})},
                 {u(1), u(1)},
             })})},
      {u(2), u(10)},
  })));
  REQUIRE(body);
  REQUIRE(body->field(2));
  CHECK(body->to_json());
  static_assert(!std::is_same_v<chain::ShelleyTransactionBody, chain::AllegraTransactionBody>);
}

TEST_CASE("era-owned models enforce discriminators lengths and duplicate-map rules",
          "[chain][era-model]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto encode = [](const Value& value) { return core::cbor::encode_cbor(value).value(); };

  CHECK_FALSE(chain::Certificate::from_cbor(
      encode(Value::array({u(0), Value::array({u(0), Value::byte_string(core::Bytes(27))})}))));
  CHECK_FALSE(chain::Certificate::from_cbor(
      encode(Value::array({u(5), Value::array({u(0), Value::byte_string(core::Bytes(28))})}))));
  CHECK(chain::Certificate::from_cbor(encode(Value::array(
      {u(18), Value::array({u(1), Value::byte_string(core::Bytes(28))}), Value::null()}))));
  CHECK_FALSE(chain::Certificate::from_cbor(encode(Value::array({u(17), Value::null()}))));
  CHECK_FALSE(chain::NativeScript::from_cbor(
      encode(Value::array({u(3), u(1), Value::array({Value::array({u(9)})})}))));
  CHECK_FALSE(chain::DatumOption::from_cbor(
      encode(Value::array({u(0), Value::byte_string(core::Bytes(31))}))));
  CHECK_FALSE(chain::UnitInterval::from_cbor(encode(Value::array({u(2), u(1)}))));
  CHECK_FALSE(chain::TransactionWitnessSet::from_cbor(
      encode(Value::map({{u(0), Value::array({})}, {u(0), Value::array({})}}))));
  CHECK_FALSE(chain::TransactionBody::from_cbor(
      encode(Value::map({{u(0), Value::array({})}, {u(1), Value::array({})}, {u(2), u(1)}}))));
  CHECK_FALSE(chain::TransactionBody::from_cbor(encode(Value::map({
      {u(0), Value::array({Value::array({Value::byte_string(core::Bytes(32)), u(0)})})},
      {u(1), Value::array({})},
      {u(2), u(1)},
      {u(6), u(1)},
  }))));
}

TEST_CASE("Conway models enforce official pool interval and protocol parameter rules",
          "[chain][era-model][cddl]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto encode = [](const Value& value) { return core::cbor::encode_cbor(value).value(); };
  const auto interval = [&] {
    return Value::tag(BigInteger(std::uint64_t{30}), Value::array({u(1), u(2)}));
  };

  const auto pool_registration = Value::array({
      u(3),
      Value::byte_string(core::Bytes(28)),
      Value::byte_string(core::Bytes(32)),
      u(1),
      u(2),
      interval(),
      Value::byte_string(core::Bytes{std::byte{0xe1}}),
      Value::tag(BigInteger(std::uint64_t{258}), Value::array({})),
      Value::array(
          {Value::array({u(0), u(3'000), Value::byte_string(core::Bytes(4)), Value::null()})}),
      Value::null(),
  });
  CHECK(chain::Certificate::from_cbor(encode(pool_registration)));
  CHECK(chain::UnitInterval::from_cbor(encode(interval())));
  CHECK_FALSE(chain::UnitInterval::from_cbor(
      encode(Value::tag(BigInteger(std::uint64_t{30}), Value::array({u(2), u(1)})))));

  CHECK_FALSE(chain::ProtocolParamUpdate::from_cbor(encode(Value::map({{u(12), interval()}}))));
  CHECK(chain::ProtocolParamUpdate::from_cbor(encode(Value::map({
      {u(10), interval()},
      {u(23), u(150)},
      {u(24), u(3)},
  }))));
  CHECK_FALSE(chain::ProtocolParamUpdate::from_cbor(encode(Value::map({{u(23), u(65'536)}}))));

  std::vector<Value> alonzo_costs(166U, u(0));
  CHECK(chain::AlonzoProtocolParamUpdate::from_value(
      Value::map({{u(18), Value::map({{u(0), Value::array(alonzo_costs)}})}})));
  alonzo_costs.pop_back();
  CHECK_FALSE(chain::AlonzoProtocolParamUpdate::from_value(
      Value::map({{u(18), Value::map({{u(0), Value::array(alonzo_costs)}})}})));
  CHECK_FALSE(chain::BabbageProtocolParamUpdate::from_value(
      Value::map({{u(14), Value::array({u(10), u(0)})}})));
  CHECK_FALSE(chain::ProtocolVersionStruct::from_value(Value::array({u(4), u(0)})));
}

TEST_CASE("standalone era productions validate nested CDDL instead of only their container shape",
          "[chain][era-model][cddl]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto interval = [&] {
    return Value::tag(BigInteger(std::uint64_t{30}), Value::array({u(1), u(2)}));
  };
  const auto credential =
      Value::array({u(0), Value::byte_string(core::Bytes(28, std::byte{0x01}))});

  CHECK(chain::MoveInstantaneousReward::from_value(
      Value::array({u(0), Value::map({{credential, u(1)}})})));
  CHECK_FALSE(chain::MoveInstantaneousReward::from_value(
      Value::array({u(2), Value::map({{credential, u(1)}})})));

  CHECK(chain::Nonce::from_value(Value::array({u(0)})));
  CHECK(chain::Nonce::from_value(Value::array({u(1), Value::byte_string(core::Bytes(32))})));
  CHECK_FALSE(chain::Nonce::from_value(Value::array({u(0), Value::byte_string(core::Bytes(32))})));

  CHECK(chain::PoolVotingThresholds::from_value(
      Value::array({interval(), interval(), interval(), interval(), interval()})));
  CHECK_FALSE(
      chain::DRepVotingThresholds::from_value(Value::array({interval(), interval(), interval()})));

  CHECK(chain::LegacyRedeemer::from_value(
      Value::array({u(5), u(UINT32_MAX), u(0), Value::array({u(1), u(2)})})));
  CHECK_FALSE(chain::LegacyRedeemer::from_value(
      Value::array({u(6), u(0), u(0), Value::array({u(1), u(2)})})));
  CHECK(chain::Redeemers::from_value(Value::map(
      {{Value::array({u(5), u(0)}), Value::array({u(0), Value::array({u(1), u(2)})})}})));
  CHECK_FALSE(chain::Redeemers::from_value(Value::map({})));

  const auto script = Value::array({u(1), Value::byte_string(core::Bytes{std::byte{0x01}})});
  const auto embedded = core::cbor::encode_cbor(script);
  REQUIRE(embedded);
  CHECK(chain::BabbageScriptRef::from_value(
      Value::tag(BigInteger(std::uint64_t{24}), Value::byte_string(*embedded))));
  CHECK_FALSE(chain::BabbageScriptRef::from_value(script));

  CHECK(chain::Constitution::from_value(
      Value::array({Value::array({Value::text_string("https://example.test"),
                                  Value::byte_string(core::Bytes(32))}),
                    Value::null()})));
  CHECK_FALSE(chain::Constitution::from_value(
      Value::array({Value::array({Value::text_string("https://example.test"),
                                  Value::byte_string(core::Bytes(31))}),
                    Value::null()})));
}

TEST_CASE("era auxiliary data and updates enforce their production-specific shapes",
          "[chain][era-model][cddl]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto u = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  const auto encode = [](const Value& value) { return core::cbor::encode_cbor(value).value(); };
  const auto format = [&](std::uint64_t script_key) {
    return Value::tag(
        BigInteger(std::uint64_t{259}),
        Value::map({
            {u(0), Value::map({{u(1), Value::text_string("metadata")}})},
            {u(script_key), Value::array({Value::byte_string(core::Bytes{std::byte{0x01}})})},
        }));
  };

  CHECK(chain::AlonzoAuxiliaryData::from_cbor(encode(format(2))));
  CHECK_FALSE(chain::AlonzoAuxiliaryData::from_cbor(encode(format(3))));
  CHECK(chain::BabbageAuxiliaryData::from_cbor(encode(format(3))));
  CHECK_FALSE(chain::ConwayFormatAuxData::from_value(Value::map({})));
  CHECK_FALSE(chain::ShelleyMAFormatAuxData::from_value(Value::map({})));

  const auto update = Value::array({
      Value::map({
          {Value::byte_string(core::Bytes(28)), Value::map({{u(0), u(44)}})},
      }),
      u(10),
  });
  CHECK(chain::ShelleyUpdate::from_cbor(encode(update)));
  CHECK_FALSE(chain::ShelleyUpdate::from_cbor(encode(Value::array({
      Value::map({
          {Value::byte_string(core::Bytes(27)), Value::map({{u(0), u(44)}})},
      }),
      u(10),
  }))));
}

TEST_CASE("specialized era JSON contracts use their declared field shapes",
          "[chain][era-model][json]") {
  const auto anchor = chain::Anchor::from_json(
      R"({"anchor_url":"https://example.com","anchor_doc_hash":"1111111111111111111111111111111111111111111111111111111111111111"})");
  REQUIRE(anchor);
  CHECK(anchor->to_json()->find("\"anchor_url\":\"https://example.com\"") != std::string::npos);
  CHECK_FALSE(
      chain::Anchor::from_json(R"({"anchor_url":"https://example.com","anchor_doc_hash":"11"})"));

  const auto interval = chain::UnitInterval::from_json(R"({"start":1,"end":2})");
  REQUIRE(interval);
  CHECK(interval->to_json() == R"({"end":2.0,"start":1.0})");
  CHECK_FALSE(chain::UnitInterval::from_json(R"({"start":3,"end":2})"));

  const auto version = chain::ProtocolVersion::from_json(R"({"major":11,"minor":0})");
  REQUIRE(version);
  CHECK(version->to_json() == R"({"major":11.0,"minor":0.0})");

  const auto ipv6 = chain::Ipv6::from_json(R"("2001:0db8:0:0:0:0:0:1")");
  REQUIRE(ipv6);
  CHECK(ipv6->to_json() == R"("2001:db8::1")");

  std::string vrf_json = R"({"output":[],"proof":[)";
  for (std::size_t index = 0; index < 80U; ++index) {
    if (index != 0U) vrf_json += ',';
    vrf_json += '0';
  }
  vrf_json += "]}";
  const auto vrf = chain::VRFCert::from_json(vrf_json);
  REQUIRE(vrf);
  CHECK(vrf->to_json());

  const auto script = chain::NativeScript::from_json(
      R"({"ScriptAll":{"native_scripts":[{"ScriptPubkey":{"ed25519_key_hash":"11111111111111111111111111111111111111111111111111111111"}}]}})");
  REQUIRE(script);
  const auto script_json = script->to_json();
  REQUIRE(script_json);
  CHECK(
      *script_json ==
      R"({"ScriptAll":{"native_scripts":[{"ScriptPubkey":{"ed25519_key_hash":"11111111111111111111111111111111111111111111111111111111"}}]}})");
  CHECK_FALSE(chain::NativeScript::from_json(
      R"({"ScriptPubkey":{"ed25519_key_hash":"11"},"ScriptAll":{"native_scripts":[]}})"));
  CHECK_FALSE(chain::MultisigScript::from_json(R"({"ScriptInvalidBefore":{"before":1}})"));
}

TEST_CASE("multi-era certificates updates and protocol parameters expose common views",
          "[chain][multi-era]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto certificate = Value::array({
      Value::unsigned_integer(BigInteger(std::uint64_t{7})),
      Value::array({
          Value::unsigned_integer(BigInteger(std::uint64_t{0})),
          Value::byte_string(core::Bytes(28)),
      }),
      Value::unsigned_integer(BigInteger(std::uint64_t{5})),
  });
  const auto decoded = chain::MultiEraCertificate::from_cbor(*core::cbor::encode_cbor(certificate));
  REQUIRE(decoded);
  CHECK(decoded->kind() == chain::MultiEraCertificateKind::registration);
  CHECK(decoded->to_json());

  const auto parameters =
      chain::MultiEraProtocolParamUpdate::from_cbor(*core::cbor::encode_cbor(Value::map({
          {Value::unsigned_integer(BigInteger(std::uint64_t{0})),
           Value::unsigned_integer(BigInteger(std::uint64_t{44}))},
          {Value::unsigned_integer(BigInteger(std::uint64_t{23})),
           Value::unsigned_integer(BigInteger(std::uint64_t{150}))},
      })));
  REQUIRE(parameters);
  REQUIRE(parameters->minfee_a());
  CHECK(*parameters->minfee_a() == 44);
  REQUIRE(parameters->collateral_percentage());
  CHECK(*parameters->collateral_percentage() == 150);

  const auto update = chain::MultiEraUpdate::from_cbor(*core::cbor::encode_cbor(Value::array({
      Value::map({}),
      Value::unsigned_integer(BigInteger(std::uint64_t{12})),
  })));
  REQUIRE(update);
  CHECK(update->epoch() == 12);
}

namespace {

[[nodiscard]] std::vector<std::byte> read_binary(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  REQUIRE(stream.good());
  const std::vector<char> raw{std::istreambuf_iterator<char>(stream),
                              std::istreambuf_iterator<char>()};
  std::vector<std::byte> bytes;
  bytes.reserve(raw.size());
  for (const char value : raw) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
  }
  return bytes;
}

[[nodiscard]] std::string read_text(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  REQUIRE(stream.good());
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST_CASE("multi-era decoder rejects malformed explicit envelopes", "[chain][multi-era]") {
  CHECK_FALSE(chain::MultiEraBlock::from_cbor_hex("80"));
  CHECK_FALSE(chain::MultiEraBlock::from_cbor_hex("820880"));
  CHECK_FALSE(chain::MultiEraBlock::from_cbor_hex("820280"));
}

TEST_CASE("captured historical blocks retain their declared decode outcomes",
          "[chain][multi-era][provider]") {
  const auto repository = std::filesystem::path(CARDANO_REPOSITORY_ROOT);
  const auto fixture_root = repository /
                            ".xray/updates/providers/cardano-multiplatform-lib/"
                            "0001-cardano-multiplatform-lib/artifacts/test-vectors";
  const auto manifest_path = fixture_root / "manifest.json";
  const auto manifest = nlohmann::json::parse(read_text(manifest_path));
  const auto& fixtures = manifest.at("fixtures").at("goldenBlocks");
  REQUIRE(fixtures.size() == 86U);

  std::size_t accepted = 0U;
  std::size_t rejected = 0U;
  for (const auto& fixture : fixtures) {
    CAPTURE(fixture.at("fixturePath").get<std::string>());
    const auto path = fixture_root / fixture.at("path").get<std::string>();
    core::Bytes wire;
    if (fixture.at("storageFormat") == "raw-cbor") {
      wire = read_binary(path);
    } else {
      auto decoded = core::hex_to_bytes(read_text(path));
      REQUIRE(decoded);
      wire = std::move(*decoded);
    }

    const auto block = chain::MultiEraBlock::from_cbor(wire);
    if (fixture.at("expected") == "byte-exact-round-trip") {
      const auto decode_error = block ? std::string{} : block.error().message();
      CAPTURE(decode_error);
      REQUIRE(block);
      CHECK(block->network_tag() == fixture.at("eraTag").get<std::uint8_t>());
      if (block->network_tag() <= 1U) {
        const auto byron = chain::ByronBlock::from_cbor(wire);
        const auto byron_error = byron ? std::string{} : byron.error().message();
        std::string byron_error_path;
        if (!byron) {
          for (const auto& component : byron.error().path()) {
            byron_error_path += std::visit(
                [](const auto& value) {
                  using Value = std::decay_t<decltype(value)>;
                  if constexpr (std::is_same_v<Value, std::size_t>) {
                    return "[" + std::to_string(value) + "]";
                  } else {
                    return "." + value;
                  }
                },
                component);
          }
        }
        std::string byron_ssc_json;
        if (!byron) {
          const auto decoded = core::cbor::decode_cbor(wire);
          if (decoded && decoded->as_array() != nullptr &&
              decoded->as_array()->values.size() == 2U) {
            const auto& main = decoded->as_array()->values[1];
            if (main.as_array() != nullptr && main.as_array()->values.size() >= 2U) {
              const auto& body = main.as_array()->values[1];
              if (body.as_array() != nullptr && body.as_array()->values.size() >= 2U) {
                auto diagnostic = chain::cbor_value_to_json(body.as_array()->values[1]);
                if (diagnostic) byron_ssc_json = std::move(*diagnostic);
              }
            }
          }
        }
        CAPTURE(byron_error);
        CAPTURE(byron_error_path);
        CAPTURE(byron_ssc_json);
        REQUIRE(byron);
      }
      const auto encoded = block->to_cbor(core::cbor::Mode::preserve);
      REQUIRE(encoded);
      CHECK(*encoded == wire);
      CHECK(block->header().block_number());
      CHECK(block->header().slot());
      CHECK(block->header().hash());
      CHECK(block->header().previous_hash());
      const auto bodies = block->transaction_bodies();
      REQUIRE(bodies);
      for (const auto& body : *bodies) {
        CHECK(body.hash());
        const auto inputs = body.inputs();
        const auto outputs = body.outputs();
        REQUIRE(inputs);
        REQUIRE(outputs);
        for (const auto& input : *inputs) {
          CHECK(input.transaction_hash());
          CHECK(input.index());
        }
        for (const auto& output : *outputs) {
          CHECK(output.address());
          CHECK(output.value());
        }
        CHECK(body.fee());
      }
      ++accepted;
    } else {
      CHECK_FALSE(block);
      ++rejected;
    }
  }

  CHECK(accepted == 85U);
  CHECK(rejected == 1U);
}
