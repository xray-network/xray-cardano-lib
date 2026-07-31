#include <cardano/plutus/data.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>

using namespace cardano;

TEST_CASE("typed Data shares the chain-owned ledger wire model", "[plutus][data]") {
  auto large = core::BigInteger::from_decimal("184467440737095516160000");
  REQUIRE(large);
  const auto data = plutus::Data::constr(core::BigInteger(std::uint64_t{7}),
                                         {
                                             plutus::Data::integer(*large),
                                             plutus::Data::bytes(*core::hex_to_bytes("00ff")),
                                         });
  const auto encoded = data.to_cbor_hex();
  REQUIRE(encoded);
  const auto decoded = plutus::Data::from_cbor_hex(*encoded);
  REQUIRE(decoded);
  CHECK(*decoded == data);

  const auto ledger = data.to_plutus_data().to_cbor();
  REQUIRE(ledger);
  CHECK(core::bytes_to_hex(*ledger) == *encoded);
}

TEST_CASE("typed Data JSON preserves constructors maps and large integers", "[plutus][data]") {
  const std::string json =
      R"({"constructor":{"alternative":"1","fields":[{"int":"18446744073709551616"},{"map":[{"k":{"bytes":"01"},"v":{"list":[]}}]}]}})";
  const auto data = plutus::Data::from_json(json);
  REQUIRE(data);
  const auto round_trip = data->to_json();
  REQUIRE(round_trip);
  const auto reparsed = plutus::Data::from_json(*round_trip);
  REQUIRE(reparsed);
  CHECK(*reparsed == *data);
}

TEST_CASE("typed Data JSON rejects ambiguity and enforces resource bounds", "[plutus][data]") {
  CHECK_FALSE(plutus::Data::from_json(R"({"int":"1","bytes":"01"})"));
  CHECK_FALSE(plutus::Data::from_json(R"({"bytes":"0"})"));
  CHECK_FALSE(plutus::Data::from_json(R"({"list":[{"list":[{"int":"1"}]}]})", 1U, 100U));
  CHECK_FALSE(plutus::Data::from_json(R"({"list":[{"int":"1"},{"int":"2"}]})", 10U, 2U));
}

TEST_CASE("typed Data schemas enforce variants lengths and bounds", "[plutus][data]") {
  const auto integer = plutus::DataSchema::integer(core::BigInteger(std::int64_t{-2}),
                                                   core::BigInteger(std::int64_t{2}));
  CHECK(integer.validate(plutus::Data::integer(core::BigInteger(std::int64_t{2}))));
  CHECK_FALSE(integer.validate(plutus::Data::integer(core::BigInteger(std::int64_t{3}))));

  const auto tuple = plutus::DataSchema::tuple({
      plutus::DataSchema::bytes(1U, 2U),
      integer,
  });
  CHECK(tuple.validate(plutus::Data::list({
      plutus::Data::bytes(*core::hex_to_bytes("00")),
      plutus::Data::integer(core::BigInteger(std::uint64_t{1})),
  })));
  CHECK_FALSE(tuple.validate(plutus::Data::list({})));

  const auto nullable = plutus::DataSchema::nullable(integer);
  CHECK(nullable.validate(plutus::Data::constr(core::BigInteger(std::uint64_t{1}), {})));
  CHECK(nullable.validate(
      plutus::Data::constr(core::BigInteger(std::uint64_t{0}),
                           {plutus::Data::integer(core::BigInteger(std::uint64_t{1}))})));
  CHECK_FALSE(nullable.validate(plutus::Data::constr(core::BigInteger(std::uint64_t{0}), {})));
}
