#include <catch2/catch_test_macros.hpp>
#include <string>

#include "cardano/cip/cip25.hpp"

using namespace cardano;

namespace {

cip::cip25::Metadata sample_metadata(cip::cip25::Version version) {
  auto policy = cip::cip25::PolicyId::from_hex(std::string(56, '1')).value();
  auto asset = cip::cip25::AssetName::from_hex("4e4654").value();
  auto name = cip::cip25::String64::from_string("Cardano NFT").value();
  auto image = cip::cip25::ChunkableString::from_string("ipfs://example").value();
  cip::cip25::MetadataDetails details{std::move(name), std::move(image), std::nullopt, std::nullopt,
                                      std::nullopt};
  cip::cip25::Nfts nfts;
  nfts.emplace_back(std::move(policy), cip::cip25::Assets{{std::move(asset), std::move(details)}});
  return cip::cip25::Metadata(cip::cip25::LabelMetadata(version, std::move(nfts)));
}

}  // namespace

TEST_CASE("CIP-25 enforces UTF-8 byte bounds and raw chunk boundaries", "[cip25]") {
  REQUIRE(cip::cip25::String64::from_string(std::string(64, 'x')));
  REQUIRE_FALSE(cip::cip25::String64::from_string(std::string(65, 'x')));
  REQUIRE(cip::cip25::String64::from_string(""));

  REQUIRE(cip::cip25::ChunkableString::from_string(std::string(128, 'x')));
  REQUIRE_FALSE(cip::cip25::ChunkableString::from_string(std::string(63, 'x') + "\xc3\xa9"));
}

TEST_CASE("CIP-25 V1 and V2 encode and parse label 721", "[cip25]") {
  for (const auto version : {cip::cip25::Version::v1, cip::cip25::Version::v2}) {
    const auto metadata = sample_metadata(version);
    const auto bytes = metadata.to_bytes();
    REQUIRE(bytes);
    const auto parsed = cip::cip25::Metadata::from_bytes(*bytes);
    REQUIRE(parsed);
    CHECK(parsed->label().version() == version);
    CHECK(parsed->label().nfts().size() == 1);
    CHECK(parsed->label().nfts().front().second.front().first.to_hex() == "4e4654");
    CHECK(parsed->label().nfts().front().second.front().second.name.value() == "Cardano NFT");
  }
}

TEST_CASE("CIP-25 top-level parsing selects first 721 and metadata merge retains labels",
          "[cip25]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto metadata = sample_metadata(cip::cip25::Version::v2);
  const auto empty_label =
      cip::cip25::LabelMetadata(cip::cip25::Version::v1, {}).to_cbor_value().value();
  const auto top = Value::map({
      {Value::unsigned_integer(BigInteger(std::uint64_t{99})), Value::text_string("keep")},
      {Value::unsigned_integer(BigInteger(std::uint64_t{721})), empty_label},
  });
  const auto merged = metadata.add_to_metadata(top);
  REQUIRE(merged);
  REQUIRE(merged->as_map());
  CHECK(merged->as_map()->entries.size() == 2);
  const auto bytes = core::cbor::encode_cbor(*merged);
  REQUIRE(bytes);
  const auto parsed = cip::cip25::Metadata::from_bytes(*bytes);
  REQUIRE(parsed);
  CHECK(parsed->label().version() == cip::cip25::Version::v2);
}

TEST_CASE("CIP-25 retains loose mini parsing behavior", "[cip25]") {
  using core::cbor::Value;
  const auto mini = cip::cip25::parse_mini_metadata(Value::map({
      {Value::text_string("Name"), Value::text_string("fallback")},
      {Value::text_string("image"), Value::text_string("ipfs://image")},
      {Value::text_string("unknown"), Value::unsigned_integer(core::BigInteger(std::uint64_t{1}))},
  }));
  REQUIRE(mini);
  REQUIRE(mini->name);
  CHECK(*mini->name == "fallback");
  REQUIRE(mini->image);
  CHECK(mini->image->joined() == "ipfs://image");

  const auto oversized_first = cip::cip25::parse_mini_metadata(Value::map({
      {Value::text_string("name"), Value::text_string(std::string(65, 'x'))},
      {Value::text_string("title"), Value::text_string("ignored")},
  }));
  REQUIRE(oversized_first);
  CHECK_FALSE(oversized_first->name);
}

TEST_CASE("CIP-25 JSON accepts historical version and file spellings", "[cip25]") {
  const std::string json =
      R"({"nfts":{")" + std::string(56, '2') +
      R"(" :{"00":{"name":42,"image":"ipfs://x","files":[{"name":7,"mediaType":"image/png","src":{"Single":"ipfs://f"}}]}}},"version":1})";
  const auto label = cip::cip25::LabelMetadata::from_json(json);
  REQUIRE(label);
  CHECK(label->version() == cip::cip25::Version::v2);
  CHECK(label->nfts().front().second.front().second.name.value() == "42");
  const auto output = label->to_json();
  CHECK(output.find("\"media_type\"") != std::string::npos);
}

TEST_CASE("CIP-25 rejects duplicate named keys and missing label", "[cip25]") {
  using core::cbor::Value;
  auto duplicate = Value::map({
      {Value::text_string("name"), Value::text_string("a")},
      {Value::text_string("name"), Value::text_string("b")},
      {Value::text_string("image"), Value::text_string("x")},
  });
  auto policy = Value::text_string(std::string(56, '0'));
  auto asset_map = Value::map({
      {Value::text_string("asset"), duplicate},
  });
  CHECK_FALSE(cip::cip25::LabelMetadata::from_cbor_value(Value::map({{policy, asset_map}})));
  CHECK_FALSE(cip::cip25::Metadata::from_cbor_value(Value::map({})));
}
