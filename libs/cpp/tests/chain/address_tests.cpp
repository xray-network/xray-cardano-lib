#include <array>
#include <cardano/chain/address.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

using namespace cardano;

namespace {

[[nodiscard]] chain::Credential key_credential(std::uint8_t fill = 0) {
  const core::Bytes bytes(28, static_cast<core::Byte>(fill));
  return chain::Credential::key(*crypto::Ed25519KeyHash::from_bytes(bytes));
}

[[nodiscard]] chain::Credential script_credential(std::uint8_t fill = 0) {
  const core::Bytes bytes(28, static_cast<core::Byte>(fill));
  return chain::Credential::script(*crypto::ScriptHash::from_bytes(bytes));
}

}  // namespace

TEST_CASE("credential specialized JSON preserves its exact variant", "[chain][address][json]") {
  const auto key = key_credential(0x11);
  const auto script = script_credential(0x22);
  const auto parsed_key = chain::Credential::from_json(key.to_json());
  const auto parsed_script = chain::Credential::from_json(script.to_json());
  REQUIRE(parsed_key);
  REQUIRE(parsed_script);
  CHECK(*parsed_key == key);
  CHECK(*parsed_script == script);
  CHECK_FALSE(chain::Credential::from_json(R"({"PubKey":{"hash":"00"},"extra":1})"));
  CHECK_FALSE(chain::Credential::from_json(R"({"Other":{"hash":"00"}})"));
}

TEST_CASE("base address variants own credentials and truncate network on wire",
          "[chain][address]") {
  const chain::BaseAddress typed(0xff, key_credential(1), script_credential(2));
  const auto generic = typed.to_address();
  REQUIRE(generic.kind() == chain::AddressKind::base);
  REQUIRE(generic.network_id());
  CHECK(*generic.network_id() == 15);
  CHECK(generic.to_bytes().size() == 57);
  CHECK((std::to_integer<std::uint8_t>(generic.to_bytes()[0]) >> 4U) == 2);

  const auto restored = chain::BaseAddress::from_address(generic);
  REQUIRE(restored);
  CHECK(restored->network_id() == 15);
  CHECK(restored->payment_credential() == key_credential(1));
  CHECK(restored->stake_credential() == script_credential(2));
}

TEST_CASE("enterprise and reward address text and JSON contracts are strict", "[chain][address]") {
  const chain::EnterpriseAddress enterprise(0, script_credential(3));
  const auto enterprise_address = enterprise.to_address();
  const auto enterprise_text = enterprise_address.to_bech32();
  REQUIRE(enterprise_text);
  CHECK(enterprise_text->starts_with("addr_test1"));
  REQUIRE(chain::Address::from_bech32(*enterprise_text));

  const chain::RewardAddress reward(1, key_credential(4));
  const auto reward_address = reward.to_address();
  const auto reward_text = reward_address.to_bech32();
  REQUIRE(reward_text);
  CHECK(reward_text->starts_with("stake1"));
  const auto json = reward.to_json();
  CHECK(json == "\"" + *reward_text + "\"");
  REQUIRE(chain::RewardAddress::from_json(json));
  CHECK_FALSE(chain::RewardAddress::from_json("{}"));
  CHECK_FALSE(chain::RewardAddress::from_json("\"addr1v000000\""));

  const auto custom = reward_address.to_bech32("unconstrained_hrp");
  REQUIRE(custom);
  CHECK_FALSE(chain::Address::from_bech32(*custom));
  const auto custom_decoded = chain::Address::from_bech32_payload_compatible(*custom);
  REQUIRE(custom_decoded);
  CHECK(custom_decoded->to_bytes() == reward_address.to_bytes());
}

TEST_CASE("the frozen long Shelley address and suffix whitelist round trip", "[chain][address]") {
  constexpr std::string_view long_address =
      "addr1q9d66zzs27kppmx8qc8h43q7m4hkxp5d39377lvxefvxd8j7eukjsdqc5c97t"
      "2zg5guqadepqqx6rc9m7wtnxy6tajjvk4a0kze4ljyuvvrpexg5up2sqxj33363v35"
      "gtew";
  const auto decoded = chain::Address::from_bech32(long_address);
  REQUIRE(decoded);
  const auto encoded = decoded->to_bech32();
  REQUIRE(encoded);
  CHECK(*encoded == long_address);

  auto canonical = chain::BaseAddress::from_address(*decoded);
  REQUIRE(canonical);
  CHECK(canonical->to_address().to_bytes().size() == 57);

  constexpr std::array<std::string_view, 8> suffixes{
      "cb57afb0b35fc89c63061c9914e055001a518c7516",
      "13d5f4a3fe0478b2241e0168e3cba5001a22c15a11",
      "00",
      "6a33306635616d6b776877716134777666796a64657a7961656c6d6e6e676436643465",
      "35616379327230656b7270717a71646b386c7a716e357234356e",
      "061d070c0d041b07020f0b0d0b0f020912051d1c100911040e1f0713110301000b101600",
      "126e7735333567367673703778376668787071327074736839676b72",
      "2c"};
  const auto base = chain::BaseAddress(1, key_credential(), key_credential()).to_address();
  for (const auto suffix : suffixes) {
    auto bytes = base.to_bytes();
    const auto suffix_bytes = *core::hex_to_bytes(suffix);
    bytes.insert(bytes.end(), suffix_bytes.begin(), suffix_bytes.end());
    const auto accepted = chain::Address::from_bytes(bytes);
    REQUIRE(accepted);
    CHECK(accepted->to_bytes() == bytes);
  }

  auto rejected = base.to_bytes();
  const auto bad_suffix = *core::hex_to_bytes("00040206030086cc");
  rejected.insert(rejected.end(), bad_suffix.begin(), bad_suffix.end());
  CHECK_FALSE(chain::Address::from_bytes(rejected));
}

TEST_CASE("pointer naturals accept uint64 maxima and canonicalize nonminimal forms",
          "[chain][address]") {
  constexpr std::string_view pointer_fixture =
      "addr_test1grqe6lg9ay8wkcu5k5e38lne63c80h3nq6xxhqfmhewf645pllllllllllll7"
      "lupllllllllllll7lupllllllllllll7lc9wayvj";
  const auto address = chain::Address::from_bech32(pointer_fixture);
  REQUIRE(address);
  REQUIRE(address->kind() == chain::AddressKind::pointer);
  const auto pointer = chain::PointerAddress::from_address(*address);
  REQUIRE(pointer);
  CHECK(pointer->pointer().slot.to_decimal() == "18446744073709551615");
  CHECK(pointer->pointer().transaction_index.to_decimal() == "18446744073709551615");
  CHECK(pointer->pointer().certificate_index.to_decimal() == "18446744073709551615");
  const auto canonical = pointer->to_address();
  REQUIRE(canonical);
  const auto text = canonical->to_bech32();
  REQUIRE(text);
  CHECK(*text == pointer_fixture);

  const auto nonminimal = *core::hex_to_bytes("8000");
  const auto decoded = chain::decode_variable_natural(nonminimal);
  REQUIRE(decoded);
  CHECK(decoded->first.is_zero());
  CHECK(decoded->second == 2);
  CHECK(core::bytes_to_hex(chain::encode_variable_natural(decoded->first)) == "00");
  CHECK_FALSE(chain::decode_variable_natural(*core::hex_to_bytes("80")));
}

TEST_CASE("reserved headers and arbitrary Byron envelopes have distinct validation",
          "[chain][address]") {
  for (std::uint8_t variant = 9; variant <= 13; ++variant) {
    const core::Bytes raw{static_cast<core::Byte>(variant << 4U)};
    CHECK_FALSE(chain::Address::from_bytes(raw));
  }

  const auto generic = chain::Address::from_bytes(*core::hex_to_bytes("8200"));
  REQUIRE(generic);
  CHECK(generic->kind() == chain::AddressKind::byron);
  CHECK_FALSE(generic->network_id());
  CHECK_FALSE(generic->to_bech32());
  CHECK_FALSE(chain::ByronAddress::from_address(*generic));
}

TEST_CASE("Base58 and CRC32 match standard boundary vectors", "[chain][byron]") {
  const auto check = *core::hex_to_bytes("313233343536373839");
  CHECK(chain::crc32(check) == 0xcbf43926U);

  for (const auto hex : {"00", "00000102", "ffffffff", "8200"}) {
    const auto bytes = *core::hex_to_bytes(hex);
    const auto encoded = chain::encode_base58(bytes);
    const auto decoded = chain::decode_base58(encoded);
    REQUIRE(decoded);
    CHECK(*decoded == bytes);
  }
  CHECK(chain::encode_base58(*core::hex_to_bytes("00")) == "1");
  CHECK_FALSE(chain::decode_base58(""));
  CHECK_FALSE(chain::decode_base58("0OIl"));
}

TEST_CASE("Address is_valid accepts only parseable Bech32 or CRC-valid Byron Base58",
          "[chain][address]") {
  const auto address = chain::EnterpriseAddress(1, key_credential(7)).to_address().to_bech32();
  REQUIRE(address);
  CHECK(chain::Address::is_valid(*address));
  CHECK_FALSE(chain::Address::is_valid("addr1invalid"));
  CHECK_FALSE(chain::Address::is_valid("not base58"));
}
