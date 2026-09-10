#include <catch2/catch_test_macros.hpp>

#include "cardano/plutus/blueprint.hpp"

TEST_CASE("CIP-57 blueprints retain definitions and validate recursive Data schemas") {
  const auto blueprint = cardano::plutus::ContractBlueprint::parse(R"json({
    "preamble":{"title":"Example","version":"1.0.0","plutusVersion":"v2"},
    "validators":[{"title":"validator","redeemer":{"schema":{"$ref":"#/definitions/count"}}}],
    "definitions":{"count":{"dataType":"integer","minimum":"0","multipleOf":"2"}}
  })json");
  REQUIRE(blueprint);
  REQUIRE(blueprint->definitions().size() == 1);
  const auto valid = cardano::plutus::Data::integer(cardano::core::BigInteger(std::int64_t{4}));
  const auto invalid = cardano::plutus::Data::integer(cardano::core::BigInteger(std::int64_t{3}));
  CHECK(blueprint->validate_data(blueprint->validators()[0].redeemer.schema, valid).empty());
  CHECK_FALSE(
      blueprint->validate_data(blueprint->validators()[0].redeemer.schema, invalid).empty());
}

TEST_CASE("CIP-57 rejects duplicate JSON keys and closed preamble extensions") {
  CHECK_FALSE(cardano::plutus::ContractBlueprint::parse(
      R"json({"preamble":{"title":"a","title":"b","version":"1","plutusVersion":"v1"},"validators":[]})json"));
  CHECK_FALSE(cardano::plutus::ContractBlueprint::parse(
      R"json({"preamble":{"title":"a","version":"1","plutusVersion":"v1","unknown":true},"validators":[]})json"));
}
