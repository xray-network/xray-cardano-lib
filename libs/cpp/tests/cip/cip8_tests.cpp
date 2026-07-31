#include <catch2/catch_test_macros.hpp>

#include "cardano/cip/cip8.hpp"
#include "cardano/crypto/keys.hpp"
#include "cardano/crypto/primitives.hpp"

using namespace cardano;

namespace {

core::Bytes bytes(std::initializer_list<std::uint8_t> values) {
  core::Bytes output;
  for (const auto value : values) output.push_back(static_cast<core::Byte>(value));
  return output;
}

cip::cip8::Headers empty_headers() {
  return {cip::cip8::ProtectedHeaderMap(), cip::cip8::HeaderMap()};
}

}  // namespace

TEST_CASE("CIP-8 signature structures preserve exact context spelling and field order", "[cip8]") {
  using core::cbor::Value;
  cip::cip8::SigStructure structure(cip::cip8::SigContext::signature1,
                                    cip::cip8::ProtectedHeaderMap(), bytes({8, 9, 100}),
                                    core::Bytes(23, static_cast<core::Byte>(73)));
  const auto encoded = structure.to_bytes();
  REQUIRE(encoded);
  const auto expected =
      core::cbor::encode_cbor(Value::array({
                                  Value::text_string("Signature1"),
                                  Value::byte_string({}),
                                  Value::byte_string(bytes({8, 9, 100})),
                                  Value::byte_string(core::Bytes(23, static_cast<core::Byte>(73))),
                              }),
                              {.mode = core::cbor::Mode::canonical});
  REQUIRE(expected);
  CHECK(*encoded == *expected);
  const auto decoded = cip::cip8::SigStructure::from_bytes(*encoded);
  REQUIRE(decoded);
  CHECK(decoded->context() == cip::cip8::SigContext::signature1);
}

TEST_CASE("CIP-8 Sign1 builder hashes once and marks only its unprotected header", "[cip8]") {
  const auto payload = bytes({1, 2, 3, 4});
  cip::cip8::COSESign1Builder builder(empty_headers(), payload, false);
  builder.hash_payload();
  const auto first = builder.make_data_to_sign().payload();
  builder.hash_payload();
  const auto second = builder.make_data_to_sign().payload();
  CHECK(first == crypto::blake2b224(payload));
  CHECK(second == first);

  const auto signed_message = builder.build(core::Bytes(64, core::Byte{}));
  const auto hashed =
      signed_message.headers().unprotected.header(cip::cip8::Label(std::string("hashed")));
  REQUIRE(hashed);
  REQUIRE(std::holds_alternative<core::cbor::BooleanValue>(hashed->node()));
  CHECK(std::get<core::cbor::BooleanValue>(hashed->node()).value);
}

TEST_CASE("CIP-8 signs and verifies Sign1 with external AAD", "[cip8]") {
  const auto secret = crypto::PrivateKey::from_bytes(core::Bytes(32, static_cast<core::Byte>(7)));
  REQUIRE(secret);
  const auto public_key = secret->public_key();
  REQUIRE(public_key);

  cip::cip8::HeaderMap protected_map;
  protected_map.set_algorithm_id(cip::cip8::Label(cip::cip8::AlgorithmId::ed_dsa));
  cip::cip8::Headers headers{
      cip::cip8::ProtectedHeaderMap(protected_map),
      cip::cip8::HeaderMap(),
  };
  cip::cip8::COSESign1Builder builder(headers, bytes({4, 5, 6}), false);
  builder.set_external_aad(bytes({9, 8, 7}));
  const auto to_sign = builder.make_data_to_sign().to_bytes();
  REQUIRE(to_sign);
  const auto signature = secret->sign(*to_sign);
  REQUIRE(signature);
  const auto message = builder.build(signature->to_bytes());
  const auto reconstructed = message.signed_data(bytes({9, 8, 7}));
  REQUIRE(reconstructed);
  const auto reconstructed_bytes = reconstructed->to_bytes();
  REQUIRE(reconstructed_bytes);
  CHECK(public_key->verify(*reconstructed_bytes, *signature));
}

TEST_CASE("CIP-8 detached payloads require the external payload", "[cip8]") {
  cip::cip8::COSESign1Builder builder(empty_headers(), bytes({1, 2}), true);
  const auto message = builder.build(bytes({3}));
  CHECK_FALSE(message.signed_data());
  const auto signed_data = message.signed_data({}, bytes({1, 2}));
  REQUIRE(signed_data);
  CHECK(signed_data->payload() == bytes({1, 2}));
}

TEST_CASE("CIP-8 public Ed25519 COSE keys exclude private label minus four", "[cip8]") {
  cip::cip8::EdDSA25519Key builder(core::Bytes(32, static_cast<core::Byte>(5)));
  builder.is_for_signing();
  builder.is_for_verifying();
  const auto key = builder.build();
  REQUIRE(key);
  CHECK(key->header(cip::cip8::Label(cip::cip8::ECKey::crv)));
  CHECK(key->header(cip::cip8::Label(cip::cip8::ECKey::x)));
  CHECK_FALSE(key->header(cip::cip8::Label(cip::cip8::ECKey::d)));
  REQUIRE(key->header(cip::cip8::Label(std::int64_t{4})));
  CHECK(key->header(cip::cip8::Label(std::int64_t{4}))->as_array()->values.size() == 2);
  CHECK_FALSE(cip::cip8::EdDSA25519Key(bytes({1})).build());
}

TEST_CASE("CIP-8 cms encoding matches captured padding compatibility", "[cip8]") {
  const auto first = cip::cip8::SignedMessage::from_user_facing_encoding(
      "cms_hEChAzkD51gnQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQwECZACyaZmw==");
  const auto second = cip::cip8::SignedMessage::from_user_facing_encoding(
      "cms_hEChAzkD51gnQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQwECZA==CyaZmw");
  const auto third = cip::cip8::SignedMessage::from_user_facing_encoding(
      "cms_hEChAzkD51gnQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQwECZA==CyaZmw==");
  REQUIRE(first);
  REQUIRE(second);
  REQUIRE(third);
  CHECK(first->to_bytes().value() == second->to_bytes().value());
  CHECK(second->to_bytes().value() == third->to_bytes().value());
  const auto encoded = first->to_user_facing_encoding();
  REQUIRE(encoded);
  CHECK(encoded->starts_with("cms_"));

  auto corrupted = *encoded;
  corrupted.back() = corrupted.back() == 'A' ? 'B' : 'A';
  const auto invalid = cip::cip8::SignedMessage::from_user_facing_encoding(corrupted);
  REQUIRE_FALSE(invalid);
  CHECK(invalid.error().code() == core::ErrorCode::checksum_mismatch);
}

TEST_CASE("CIP-8 preserves protected bytes and noncanonical outer CBOR", "[cip8]") {
  using core::BigInteger;
  using core::cbor::HeadWidth;
  using core::cbor::LengthEncoding;
  using core::cbor::Value;
  const auto protected_map = Value::map(
      {
          {Value::unsigned_integer(BigInteger(std::uint64_t{1}), HeadWidth::one),
           Value::negative_integer(BigInteger(std::int64_t{-8}), HeadWidth::one)},
      },
      LengthEncoding{.indefinite = true});
  const auto protected_bytes = core::cbor::encode_cbor(protected_map).value();
  const auto sign1 = Value::array(
      {
          Value::byte_string(protected_bytes),
          Value::map({}, LengthEncoding{.indefinite = true}),
          Value::null(),
          Value::byte_string(bytes({1, 2, 3})),
      },
      LengthEncoding{.indefinite = true});
  const auto source = core::cbor::encode_cbor(sign1).value();
  const auto parsed = cip::cip8::COSESign1::from_bytes(source);
  REQUIRE(parsed);
  CHECK(parsed->to_bytes().value() == source);
  CHECK(parsed->headers().protected_headers.bytes() == protected_bytes);
  CHECK(parsed->to_bytes(core::cbor::Mode::canonical).value() != source);
}

TEST_CASE("CIP-8 rejects duplicate header labels and invalid protected maps", "[cip8]") {
  using core::BigInteger;
  using core::cbor::Value;
  const auto duplicate = Value::map({
      {Value::unsigned_integer(BigInteger(std::uint64_t{1})), Value::null()},
      {Value::unsigned_integer(BigInteger(std::uint64_t{1})), Value::null()},
  });
  const auto bytes = core::cbor::encode_cbor(duplicate).value();
  CHECK_FALSE(cip::cip8::HeaderMap::from_bytes(bytes));
  CHECK_FALSE(cip::cip8::ProtectedHeaderMap::from_bytes(
      core::cbor::encode_cbor(Value::byte_string({})).value()));
}
