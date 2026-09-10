#include <algorithm>
#include <catch2/catch_test_macros.hpp>

#include "cardano/cip/cip21.hpp"

TEST_CASE("CIP-21 diagnoses validated preserved Conway transactions without mutation") {
  using cardano::core::BigInteger;
  using cardano::core::cbor::Value;
  const auto uint = [](std::uint64_t value) { return Value::unsigned_integer(BigInteger(value)); };
  cardano::core::Bytes address(29);
  address[0] = static_cast<cardano::core::Byte>(0x61);
  const auto input = Value::array({Value::byte_string(cardano::core::Bytes(32)), uint(0)});
  const auto output = Value::array({Value::byte_string(address), uint(1)});
  const auto body = Value::map({
      {uint(0), Value::tag(BigInteger(std::uint64_t{258}), Value::array({input}))},
      {uint(1), Value::array({output})},
      {uint(2), uint(1)},
  });
  const auto transaction =
      Value::array({body, Value::map({}), Value::boolean(true), Value::null()});
  const auto bytes = *cardano::core::cbor::encode_cbor(transaction);
  const auto report = cardano::cip::diagnose_cip21(cardano::cip::Cip21Era::conway, bytes);
  REQUIRE(report);
  CHECK(report->compatible());
  CHECK(std::ranges::any_of(report->diagnostics, [](const auto& item) {
    return item.code == cardano::cip::Cip21Code::legacy_output_format &&
           item.severity == cardano::cip::Cip21Severity::advisory;
  }));

  const auto catalyst =
      cardano::cip::diagnose_cip21(cardano::cip::Cip21Era::conway, bytes,
                                   {cardano::cip::Cip21AuxiliaryMode::catalyst_registration});
  REQUIRE(catalyst);
  CHECK_FALSE(catalyst->compatible());
  CHECK(std::ranges::any_of(catalyst->diagnostics, [](const auto& item) {
    return item.code == cardano::cip::Cip21Code::catalyst_auxiliary_shape;
  }));
}
