#include <catch2/catch_test_macros.hpp>

#include "cardano/cip/cip36.hpp"
#include "cardano/crypto/primitives.hpp"

using namespace cardano;

namespace {

core::Bytes repeated(std::size_t size, std::uint8_t value) {
  return core::Bytes(size, static_cast<core::Byte>(value));
}

crypto::PublicKey key(std::uint8_t value) {
  return crypto::PublicKey::from_bytes(repeated(32, value)).value();
}

crypto::Ed25519Signature signature(std::uint8_t value = 0) {
  return crypto::Ed25519Signature::from_bytes(repeated(64, value)).value();
}

chain::Address payment_address() {
  auto hash = crypto::Ed25519KeyHash::from_bytes(repeated(28, 7)).value();
  return chain::EnterpriseAddress(0, chain::Credential::key(std::move(hash))).to_address();
}

cip::cip36::RegistrationCbor registration(std::uint32_t weight = 0) {
  auto distribution = cip::cip36::DelegationDistribution::weighted({{key(1), weight}}).value();
  auto proposal =
      cip::cip36::KeyRegistration::weighted(std::move(distribution), key(2), payment_address(), 42)
          .value();
  return cip::cip36::RegistrationCbor(std::move(proposal),
                                      cip::cip36::RegistrationWitness(signature()));
}

}  // namespace

TEST_CASE("CIP-36 weighted and legacy purpose presence follows frozen rules", "[cip36]") {
  auto weighted = registration();
  CHECK(weighted.registration().has_explicit_voting_purpose());
  auto weighted_value = weighted.registration().to_cbor_value();
  REQUIRE(weighted_value.as_map());
  CHECK(weighted_value.as_map()->entries.size() == 5);

  weighted.registration().set_voting_purpose(0);
  CHECK(weighted.registration().has_explicit_voting_purpose());

  auto legacy_distribution = cip::cip36::DelegationDistribution::legacy(key(3));
  auto legacy = cip::cip36::KeyRegistration::legacy(std::move(legacy_distribution), key(4),
                                                    payment_address(), 9);
  CHECK_FALSE(legacy.has_explicit_voting_purpose());
  legacy.set_voting_purpose(8);
  CHECK_FALSE(legacy.has_explicit_voting_purpose());
  REQUIRE(legacy.to_cbor_value().as_map());
  CHECK(legacy.to_cbor_value().as_map()->entries.size() == 4);

  cip::cip36::KeyDeregistration deregistration(key(5), 11);
  CHECK_FALSE(deregistration.has_explicit_voting_purpose());
  deregistration.set_voting_purpose(0);
  CHECK_FALSE(deregistration.has_explicit_voting_purpose());
  deregistration.set_voting_purpose(1);
  CHECK(deregistration.has_explicit_voting_purpose());
}

TEST_CASE("CIP-36 registration compatibility check is narrow and does not verify signatures",
          "[cip36]") {
  auto accepted = registration(0);
  REQUIRE(accepted.to_bytes());

  auto rejected = registration(1);
  const auto bytes = rejected.to_bytes();
  REQUIRE_FALSE(bytes);
  CHECK(bytes.error().code() == core::ErrorCode::invalid_structure);
}

TEST_CASE("CIP-36 hashes exactly the one-label proposal preimage", "[cip36]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto view = registration();
  const auto inner = view.registration().to_cbor_value();
  const auto preimage = core::cbor::encode_cbor(
                            Value::map({
                                {Value::unsigned_integer(BigInteger(std::uint64_t{61284})), inner},
                            }))
                            .value();
  CHECK(view.hash_to_sign() == crypto::blake2b256(preimage));

  const auto other_witness = cip::cip36::RegistrationCbor(
      view.registration(), cip::cip36::RegistrationWitness(signature(9)));
  CHECK(other_witness.hash_to_sign() == view.hash_to_sign());
}

TEST_CASE("CIP-36 preserves decoded proposal and witness CBOR", "[cip36]") {
  using core::BigInteger;
  using core::cbor::HeadWidth;
  using core::cbor::LengthEncoding;
  using core::cbor::Value;
  const auto proposal = Value::map(
      {
          {Value::unsigned_integer(BigInteger(std::uint64_t{1}), HeadWidth::one),
           Value::array(
               {
                   Value::array(
                       {
                           Value::byte_string(key(1).to_bytes()),
                           Value::unsigned_integer(BigInteger(std::uint64_t{0}), HeadWidth::one),
                       },
                       LengthEncoding{.indefinite = true}),
               },
               LengthEncoding{.indefinite = true})},
          {Value::unsigned_integer(BigInteger(std::uint64_t{2})),
           Value::byte_string(key(2).to_bytes())},
          {Value::unsigned_integer(BigInteger(std::uint64_t{3})),
           Value::byte_string(payment_address().to_bytes())},
          {Value::unsigned_integer(BigInteger(std::uint64_t{4})),
           Value::unsigned_integer(BigInteger(std::uint64_t{42}), HeadWidth::two)},
      },
      LengthEncoding{.indefinite = true});
  const auto witness = Value::map(
      {
          {Value::unsigned_integer(BigInteger(std::uint64_t{1}), HeadWidth::one),
           Value::byte_string(signature().to_bytes())},
      },
      LengthEncoding{.indefinite = true});
  const auto source = Value::map({
      {Value::unsigned_integer(BigInteger(std::uint64_t{61284})), proposal},
      {Value::unsigned_integer(BigInteger(std::uint64_t{61285})), witness},
  });
  const auto bytes = core::cbor::encode_cbor(source).value();
  const auto parsed = cip::cip36::RegistrationCbor::from_bytes(bytes);
  REQUIRE(parsed);
  CHECK(parsed->to_bytes().value() == bytes);
  CHECK(parsed->to_bytes(core::cbor::Mode::canonical).value() != bytes);
}

TEST_CASE("CIP-36 metadata views ignore unrelated labels but reject duplicates", "[cip36]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto view = registration();
  const auto encoded = view.to_bytes().value();
  const auto value = core::cbor::decode_cbor(encoded).value();
  auto entries = value.as_map()->entries;
  entries.insert(entries.begin(),
                 {Value::unsigned_integer(BigInteger(std::uint64_t{1})), Value::null()});
  REQUIRE(cip::cip36::RegistrationCbor::from_cbor_value(Value::map(entries)));
  entries.push_back(entries.back());
  CHECK_FALSE(cip::cip36::RegistrationCbor::from_cbor_value(Value::map(entries)));
}

TEST_CASE("CIP-36 JSON accepts decimal uint64 and emits large values as strings", "[cip36]") {
  const auto payment = payment_address().to_bech32().value();
  const std::string json = R"({"delegation":{"Weighted":[{"voting_pub_key":")" + key(1).to_hex() +
                           R"(","weight":0}]},"stake_credential":")" + key(2).to_hex() +
                           R"(","payment_address":")" + payment +
                           R"(","nonce":"18446744073709551615","voting_purpose":0})";
  const auto proposal = cip::cip36::KeyRegistration::from_json(json);
  REQUIRE(proposal);
  CHECK(proposal->nonce() == std::numeric_limits<std::uint64_t>::max());
  CHECK(proposal->to_json().find("\"18446744073709551615\"") != std::string::npos);
}

TEST_CASE("CIP-36 deregistration emits exact label order and retains unrelated metadata",
          "[cip36]") {
  using core::BigInteger;
  using core::cbor::Value;
  cip::cip36::DeregistrationCbor view(cip::cip36::KeyDeregistration(key(1), 5),
                                      cip::cip36::DeregistrationWitness(signature()));
  const auto bytes = view.to_bytes();
  REQUIRE(bytes);
  const auto decoded = core::cbor::decode_cbor(*bytes);
  REQUIRE(decoded);
  REQUIRE(decoded->as_map());
  CHECK(decoded->as_map()->entries[0].first.as_unsigned()->value ==
        BigInteger(std::uint64_t{61285}));
  CHECK(decoded->as_map()->entries[1].first.as_unsigned()->value ==
        BigInteger(std::uint64_t{61286}));

  const auto merged = view.add_to_metadata(Value::map({
      {Value::unsigned_integer(BigInteger(std::uint64_t{7})), Value::null()},
  }));
  REQUIRE(merged);
  CHECK(merged->as_map()->entries.size() == 3);
  CHECK(merged->as_map()->entries[1].first.as_unsigned()->value ==
        BigInteger(std::uint64_t{61286}));
  CHECK(merged->as_map()->entries[2].first.as_unsigned()->value ==
        BigInteger(std::uint64_t{61285}));
}
