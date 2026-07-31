#include <cardano/core/core.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

using namespace cardano::core;

namespace {

[[nodiscard]] cbor::Value decode(std::string_view hex) {
  const auto bytes = hex_to_bytes(hex);
  REQUIRE(bytes);
  const auto value = cbor::decode_cbor(*bytes);
  REQUIRE(value);
  return *value;
}

[[nodiscard]] std::string encode_hex(const cbor::Value& value, cbor::Mode mode) {
  const auto bytes = cbor::encode_cbor(value, cbor::EncodeOptions{.mode = mode});
  REQUIRE(bytes);
  return bytes_to_hex(*bytes);
}

}  // namespace

TEST_CASE("CBOR preservation and canonical encoding are distinct", "[core][cbor]") {
  const auto value = decode("9f18011800ff");
  CHECK(encode_hex(value, cbor::Mode::preserve) == "9f18011800ff");
  CHECK(encode_hex(value, cbor::Mode::canonical) == "820100");
}

TEST_CASE("CBOR retains nonminimal heads and indefinite chunks", "[core][cbor]") {
  const auto integer = decode("1817");
  CHECK(encode_hex(integer, cbor::Mode::preserve) == "1817");
  CHECK(encode_hex(integer, cbor::Mode::canonical) == "17");

  const auto bytes = decode("5f42010243030405ff");
  REQUIRE(bytes.as_byte_string());
  CHECK(bytes.as_byte_string()->value == *hex_to_bytes("0102030405"));
  CHECK(encode_hex(bytes, cbor::Mode::preserve) == "5f42010243030405ff");
  CHECK(encode_hex(bytes, cbor::Mode::canonical) == "450102030405");

  const auto text = decode("7f61616162ff");
  REQUIRE(text.as_text_string());
  CHECK(text.as_text_string()->value == "ab");
  CHECK(encode_hex(text, cbor::Mode::preserve) == "7f61616162ff");
  CHECK(encode_hex(text, cbor::Mode::canonical) == "626162");
}

TEST_CASE("CBOR maps retain duplicates and canonical key ordering", "[core][cbor]") {
  const auto value = decode("bf616202616101616103ff");
  REQUIRE(value.as_map());
  CHECK(value.as_map()->entries.size() == 3);
  CHECK(encode_hex(value, cbor::Mode::preserve) == "bf616202616101616103ff");
  CHECK(encode_hex(value, cbor::Mode::canonical) == "a3616101616103616202");

  const auto length_first = decode("a2181800616101");
  CHECK(encode_hex(length_first, cbor::Mode::canonical) == "a2181800616101");
}

TEST_CASE("CBOR covers every simple primitive and float width", "[core][cbor]") {
  CHECK(std::get<cbor::BooleanValue>(decode("f4").node()).value == false);
  CHECK(std::get<cbor::BooleanValue>(decode("f5").node()).value == true);
  CHECK(std::holds_alternative<cbor::NullValue>(decode("f6").node()));
  CHECK(std::holds_alternative<cbor::UndefinedValue>(decode("f7").node()));
  CHECK(std::get<cbor::SimpleValue>(decode("f820").node()).value == 32);

  const auto half = std::get<cbor::FloatingValue>(decode("f93e00").node());
  CHECK(half.width == cbor::FloatWidth::half);
  CHECK(half.value == 1.5);
  CHECK(encode_hex(decode("fa3fc00000"), cbor::Mode::canonical) == "f93e00");
  CHECK(encode_hex(decode("fb3ff8000000000000"), cbor::Mode::canonical) == "f93e00");
  CHECK(encode_hex(decode("fb8000000000000000"), cbor::Mode::canonical) == "f98000");
}

TEST_CASE("CBOR normalizes NaNs under the frozen exception", "[core][cbor]") {
  const auto positive = decode("f97e01");
  const auto negative = decode("f9fe01");
  CHECK(std::isnan(std::get<cbor::FloatingValue>(positive.node()).value));
  CHECK(std::isnan(std::get<cbor::FloatingValue>(negative.node()).value));
  CHECK(encode_hex(positive, cbor::Mode::preserve) == "f97e00");
  CHECK(encode_hex(negative, cbor::Mode::preserve) == "f97e00");
  CHECK(encode_hex(positive, cbor::Mode::canonical) == "f97e00");
}

TEST_CASE("CBOR represents arbitrary tags and validates Cardano semantic tags", "[core][cbor]") {
  const auto arbitrary = decode("d903e86178");
  REQUIRE(arbitrary.as_tag());
  CHECK(arbitrary.as_tag()->tag == BigInteger(std::uint64_t{1000}));
  CHECK(encode_hex(arbitrary, cbor::Mode::preserve) == "d903e86178");

  const auto embedded = decode("d81843820102");
  const auto nested = cbor::decode_embedded_cbor(embedded);
  REQUIRE(nested);
  REQUIRE(nested->as_array());
  CHECK(nested->as_array()->values.size() == 2);

  CHECK_FALSE(cbor::decode_embedded_cbor(decode("d8174100")));
  CHECK_FALSE(cbor::decode_embedded_cbor(decode("d81800")));

  const auto zero_bignum = decode("c240");
  CHECK(encode_hex(zero_bignum, cbor::Mode::preserve) == "c240");
  CHECK(encode_hex(zero_bignum, cbor::Mode::canonical) == "00");
  const auto padded_bignum = decode("c2420001");
  CHECK(encode_hex(padded_bignum, cbor::Mode::preserve) == "c2420001");
  CHECK(encode_hex(padded_bignum, cbor::Mode::canonical) == "01");
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("c201")));
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("d81e8101")));
}

TEST_CASE("CBOR rejects malformed complete inputs", "[core][cbor]") {
  CHECK_FALSE(cbor::decode_cbor({}));
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("0000")));
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("18")));
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("1c")));
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("ff")));
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("81ff")));
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("5f6100ff")));
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("63ff0000")));
}

TEST_CASE("CBOR decoder limits are enforced independently", "[core][cbor][limits]") {
  CHECK(cbor::DEFAULT_CBOR_LIMITS.max_depth == 512);
  CHECK(cbor::DEFAULT_CBOR_LIMITS.max_collection_length == 1'000'000);
  CHECK(cbor::DEFAULT_CBOR_LIMITS.max_string_bytes == 67'108'864);
  CHECK(cbor::DEFAULT_CBOR_LIMITS.max_tokens == 2'000'000);

  auto limits = cbor::DEFAULT_CBOR_LIMITS;
  limits.max_depth = 0;
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("8100"), cbor::DecodeOptions{.limits = limits}));

  limits = cbor::DEFAULT_CBOR_LIMITS;
  limits.max_collection_length = 1;
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("820001"), cbor::DecodeOptions{.limits = limits}));

  limits = cbor::DEFAULT_CBOR_LIMITS;
  limits.max_string_bytes = 2;
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("43010203"), cbor::DecodeOptions{.limits = limits}));

  limits = cbor::DEFAULT_CBOR_LIMITS;
  limits.max_tokens = 2;
  CHECK_FALSE(cbor::decode_cbor(*hex_to_bytes("820001"), cbor::DecodeOptions{.limits = limits}));
}

TEST_CASE("CBOR source spans cover each decoded node", "[core][cbor]") {
  const auto value = decode("8201a10203");
  REQUIRE(value.span());
  CHECK(*value.span() == cbor::Span{0, 5});
  REQUIRE(value.as_array());
  REQUIRE(value.as_array()->values[0].span());
  CHECK(*value.as_array()->values[0].span() == cbor::Span{1, 2});
  REQUIRE(value.as_array()->values[1].span());
  CHECK(*value.as_array()->values[1].span() == cbor::Span{2, 5});
}
