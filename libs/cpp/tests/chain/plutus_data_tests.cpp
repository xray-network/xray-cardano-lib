#include <cardano/chain/plutus_data.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace cardano;

namespace {

[[nodiscard]] std::string encode(const chain::PlutusData& data,
                                 core::cbor::Mode mode = core::cbor::Mode::canonical) {
  const auto bytes = data.to_cbor(mode);
  REQUIRE(bytes);
  return core::bytes_to_hex(*bytes);
}

}  // namespace

TEST_CASE("Plutus Data constructor alternatives use all three wire forms", "[chain][plutus-data]") {
  CHECK(encode(chain::PlutusData::constr(core::BigInteger(std::uint64_t{0}), {})) == "d87980");
  CHECK(encode(chain::PlutusData::constr(core::BigInteger(std::uint64_t{6}), {})) == "d87f80");
  CHECK(encode(chain::PlutusData::constr(core::BigInteger(std::uint64_t{7}), {})) == "d9050080");
  CHECK(encode(chain::PlutusData::constr(core::BigInteger(std::uint64_t{127}), {})) == "d9057880");
  CHECK(encode(chain::PlutusData::constr(core::BigInteger(std::uint64_t{128}), {})) ==
        "d86682188080");
  CHECK_FALSE(chain::PlutusData::constr(core::BigInteger(std::int64_t{-1}), {}).to_cbor());
}

TEST_CASE("Plutus Data recursively round trips lists maps bytes and large integers",
          "[chain][plutus-data]") {
  auto large = core::BigInteger::from_decimal("184467440737095516160000");
  REQUIRE(large);
  const auto value = chain::PlutusData::constr(
      core::BigInteger(std::uint64_t{3}),
      {
          chain::PlutusData::list({
              chain::PlutusData::integer(*large),
              chain::PlutusData::integer(core::BigInteger(std::int64_t{-9})),
              chain::PlutusData::bytes(*core::hex_to_bytes("00ff")),
          }),
          chain::PlutusData::map({
              {
                  chain::PlutusData::bytes(*core::hex_to_bytes("01")),
                  chain::PlutusData::integer(core::BigInteger(std::uint64_t{2})),
              },
          }),
      });
  const auto encoded = value.to_cbor();
  REQUIRE(encoded);
  const auto decoded = chain::PlutusData::from_cbor(*encoded);
  REQUIRE(decoded);
  CHECK(*decoded == value);
}

TEST_CASE("Plutus maps preserve duplicate entries and original CBOR separately",
          "[chain][plutus-data]") {
  const auto bytes = *core::hex_to_bytes("bf01020103ff");
  const auto data = chain::PlutusData::from_cbor(bytes);
  REQUIRE(data);
  CHECK(encode(*data, core::cbor::Mode::preserve) == "bf01020103ff");
  CHECK(encode(*data, core::cbor::Mode::canonical) == "a201020103");
}

TEST_CASE("Plutus lists preserve indefinite wire form but canonicalize deterministically",
          "[chain][plutus-data]") {
  const auto data = chain::PlutusData::from_cbor(*core::hex_to_bytes("9f18011800ff"));
  REQUIRE(data);
  CHECK(encode(*data, core::cbor::Mode::preserve) == "9f18011800ff");
  CHECK(encode(*data, core::cbor::Mode::canonical) == "820100");
}

TEST_CASE("Plutus Data rejects non-Data nodes and malformed constructors", "[chain][plutus-data]") {
  CHECK_FALSE(chain::PlutusData::from_cbor(*core::hex_to_bytes("6161")));
  CHECK_FALSE(chain::PlutusData::from_cbor(*core::hex_to_bytes("f6")));
  CHECK_FALSE(chain::PlutusData::from_cbor(*core::hex_to_bytes("d86480")));
  CHECK_FALSE(chain::PlutusData::from_cbor(*core::hex_to_bytes("d8668100")));
  CHECK_FALSE(chain::PlutusData::from_cbor(*core::hex_to_bytes("d87900")));
}

TEST_CASE("Plutus Data conversion enforces the recursive depth bound",
          "[chain][plutus-data][limits]") {
  const auto node = core::cbor::decode_cbor(*core::hex_to_bytes("81818100"));
  REQUIRE(node);
  CHECK(chain::PlutusData::from_cbor_value(*node, 3));
  CHECK_FALSE(chain::PlutusData::from_cbor_value(*node, 2));
}

TEST_CASE("Plutus Data uses 64-byte wire chunks and rejects oversized leaves",
          "[chain][plutus-data][limits]") {
  const auto generated = chain::PlutusData::bytes(core::Bytes(65U));
  const auto encoded = generated.to_cbor();
  REQUIRE(encoded);
  CHECK(encoded->front() == std::byte{0x5f});
  const auto decoded = chain::PlutusData::from_cbor(*encoded);
  REQUIRE(decoded);
  CHECK(*decoded == generated);

  core::Bytes definite{std::byte{0x58}, std::byte{0x41}};
  definite.insert(definite.end(), 65U, std::byte{0});
  CHECK_FALSE(chain::PlutusData::from_cbor(definite));
  const auto raw = core::cbor::decode_cbor(definite);
  REQUIRE(raw);
  CHECK(chain::PlutusData::from_cbor_value(*raw, 512U, false));
}

TEST_CASE("ledger PlutusData is the single nominal owner", "[chain][plutus-data]") {
  const auto value = chain::PlutusData::integer(core::BigInteger(std::uint64_t{42}));
  const chain::PlutusData& same_binding = value;
  CHECK(encode(same_binding) == "182a");
}
