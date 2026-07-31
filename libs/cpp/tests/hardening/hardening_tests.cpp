#include <algorithm>
#include <cardano/cardano.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <random>

using namespace cardano;

TEST_CASE("deterministic malformed CBOR campaign fails closed", "[hardening][cbor]") {
  constexpr std::size_t configured = CARDANO_HARDENING_CASES;
  const std::size_t cases = configured == 0U ? 1'000U : configured;
  std::mt19937_64 random(0xc0b012f0U);
  for (std::size_t index = 0; index < cases; ++index) {
    const auto length = static_cast<std::size_t>(random() % 257U);
    core::Bytes bytes(length);
    for (auto& byte : bytes) {
      byte = static_cast<core::Byte>(random() & 0xffU);
    }
    const auto decoded = core::cbor::decode_cbor(bytes);
    if (!decoded) {
      continue;
    }
    const auto preserved = core::cbor::encode_cbor(
        *decoded, core::cbor::EncodeOptions{.mode = core::cbor::Mode::preserve});
    const auto canonical = core::cbor::encode_cbor(
        *decoded, core::cbor::EncodeOptions{.mode = core::cbor::Mode::canonical});
    REQUIRE(preserved);
    REQUIRE(canonical);
    CHECK(core::cbor::decode_cbor(*preserved));
    const auto canonical_value = core::cbor::decode_cbor(*canonical);
    REQUIRE(canonical_value);
    const auto converged = core::cbor::encode_cbor(
        *canonical_value, core::cbor::EncodeOptions{.mode = core::cbor::Mode::canonical});
    REQUIRE(converged);
    CHECK(*converged == *canonical);
  }
}

TEST_CASE("signature mutation campaign never authenticates", "[hardening][crypto]") {
  auto key =
      crypto::PrivateKey::from_bytes(*core::hex_to_bytes("000102030405060708090a0b0c0d0e0f"
                                                         "101112131415161718191a1b1c1d1e1f"));
  REQUIRE(key);
  const auto public_key = key->public_key();
  const auto message = *core::hex_to_bytes("010203040506");
  const auto signature = key->sign(message);
  REQUIRE(public_key);
  REQUIRE(signature);
  CHECK(public_key->verify(message, *signature));
  for (std::size_t bit = 0U; bit < 512U; ++bit) {
    auto mutated = signature->to_bytes();
    mutated[bit / 8U] ^= static_cast<core::Byte>(std::uint8_t{1} << (bit % 8U));
    const auto candidate = crypto::Ed25519Signature::from_bytes(mutated);
    REQUIRE(candidate);
    CHECK_FALSE(public_key->verify(message, *candidate));
  }
}

TEST_CASE("typed Data depth and count campaigns retain configured bounds", "[hardening][data]") {
  auto value = plutus::Data::integer(core::BigInteger(std::uint64_t{0}));
  for (std::size_t depth = 0U; depth < 130U; ++depth) {
    value = plutus::Data::list({std::move(value)});
  }
  CHECK_FALSE(value.to_json(128U, 1'000U));

  std::vector<plutus::Data> many;
  many.reserve(1'001U);
  for (std::size_t index = 0U; index < 1'001U; ++index) {
    many.push_back(plutus::Data::integer(core::BigInteger(static_cast<std::uint64_t>(index))));
  }
  CHECK_FALSE(plutus::Data::list(std::move(many)).to_json(10U, 1'000U));
}
