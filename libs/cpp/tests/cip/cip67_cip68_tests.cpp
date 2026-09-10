#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "cardano/cip/cip67.hpp"
#include "cardano/cip/cip68.hpp"

namespace {
using namespace std::string_view_literals;
cardano::core::Bytes text(std::string_view value) {
  cardano::core::Bytes output;
  for (const auto character : value) output.push_back(static_cast<cardano::core::Byte>(character));
  return output;
}
}  // namespace

TEST_CASE("CIP-67 matches every captured label vector") {
  using cardano::cip::experimental::cip67::AssetNameLabel;
  const std::array vectors{
      std::pair{0U, "00000000"sv},     std::pair{1U, "00001070"sv},
      std::pair{23U, "00017650"sv},    std::pair{99U, "000632e0"sv},
      std::pair{533U, "00215410"sv},   std::pair{2000U, "007d0550"sv},
      std::pair{4567U, "011d7690"sv},  std::pair{11111U, "02b670b0"sv},
      std::pair{49328U, "0c0b0f40"sv}, std::pair{65535U, "0ffff240"sv},
  };
  for (const auto& [label, expected] : vectors) {
    const auto encoded = cardano::cip::experimental::cip67::encode(
        AssetNameLabel(static_cast<std::uint16_t>(label)));
    CHECK(cardano::core::bytes_to_hex(encoded) == expected);
    CHECK(cardano::cip::experimental::cip67::decode(encoded)->value() == label);
  }
}

TEST_CASE("CIP-68 validates relationships and three-field datum metadata") {
  using namespace cardano;
  const auto policy = *crypto::ScriptHash::from_bytes(core::Bytes(28));
  const auto content = text("token");
  const auto user_name = *cip::experimental::cip67::make_asset_name(
      cip::experimental::cip67::AssetNameLabel(222), content);
  const cip::cip68::TokenIdentity user{policy, user_name};
  const auto reference = *cip::cip68::reference_identity(user);
  CHECK(cip::cip68::validate_relationship(user, reference, 1, 1));
  CHECK_FALSE(cip::cip68::validate_relationship(user, reference, 0, 1));

  auto metadata = chain::PlutusData::map({
      {chain::PlutusData::bytes(text("name")), chain::PlutusData::bytes(text("Example"))},
      {chain::PlutusData::bytes(text("image")), chain::PlutusData::bytes(text("ipfs://example"))},
  });
  const auto datum = cip::cip68::Datum::make(
      std::move(metadata), 3, chain::PlutusData::constr(core::BigInteger(std::uint64_t{0}), {}));
  const auto parsed = cip::cip68::Datum::parse(datum.to_data(), cip::cip68::TokenClass::nft);
  REQUIRE(parsed);
  CHECK(parsed->version() == 3);
  CHECK(parsed->metadata_format() == cip::cip68::MetadataFormat::direct);
  CHECK(parsed->validate(cip::cip68::TokenClass::nft, user));

  const auto two_fields = chain::PlutusData::constr(
      core::BigInteger(std::uint64_t{0}),
      {datum.metadata(), chain::PlutusData::integer(core::BigInteger(std::uint64_t{1}))});
  CHECK_FALSE(cip::cip68::Datum::parse(two_fields, cip::cip68::TokenClass::nft));
}
