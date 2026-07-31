#include <cardano/core/core.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace cardano::core;

TEST_CASE("hex conversion owns and validates bytes", "[core]") {
  const auto bytes = hex_to_bytes("00aBff");
  REQUIRE(bytes);
  CHECK(bytes_to_hex(*bytes) == "00abff");
  CHECK_FALSE(hex_to_bytes("0"));
  CHECK_FALSE(hex_to_bytes("zz"));

  Bytes mutable_input{Byte{1}, Byte{2}};
  const auto owned = copy_bytes(mutable_input);
  mutable_input[0] = Byte{9};
  CHECK(owned == Bytes{Byte{1}, Byte{2}});
  CHECK(bytes_equal(owned, Bytes{Byte{1}, Byte{2}}));
  CHECK_FALSE(bytes_equal(owned, Bytes{Byte{1}}));
}

TEST_CASE("byte validation covers exact lengths and UTF-8 boundaries", "[core]") {
  CHECK(assert_byte_length(*hex_to_bytes("0001"), 2));
  const auto failure = assert_byte_length(*hex_to_bytes("00"), 2, "hash");
  REQUIRE_FALSE(failure);
  CHECK(failure.error().code() == ErrorCode::out_of_range);

  CHECK(is_valid_utf8(*hex_to_bytes("68656c6c6f")));
  CHECK(is_valid_utf8(*hex_to_bytes("c3a9")));
  CHECK_FALSE(is_valid_utf8(*hex_to_bytes("c0af")));
  CHECK_FALSE(is_valid_utf8(*hex_to_bytes("eda080")));
  CHECK_FALSE(is_valid_utf8(*hex_to_bytes("f4908080")));
}

TEST_CASE("arbitrary integers retain values beyond machine bounds", "[core]") {
  const auto huge = BigInteger::from_decimal("340282366920938463463374607431768211455");
  REQUIRE(huge);
  CHECK(huge->to_decimal() == "340282366920938463463374607431768211455");
  CHECK_FALSE(huge->fits_uint64());
  CHECK_FALSE(huge->to_uint64());

  const auto negative = BigInteger::from_decimal("-9223372036854775809");
  REQUIRE(negative);
  CHECK(negative->is_negative());
  CHECK_FALSE(negative->fits_int64());
  CHECK((*huge + BigInteger(std::uint64_t{1})).to_decimal() ==
        "340282366920938463463374607431768211456");

  CHECK_FALSE(BigInteger::from_decimal(""));
  CHECK_FALSE(BigInteger::from_decimal("-"));
  CHECK_FALSE(BigInteger::from_decimal("12x"));
}

TEST_CASE("ordered and nonempty collections preserve ownership and order", "[core]") {
  OrderedMap<std::string, int> values;
  values.insert_or_assign("first", 1);
  values.insert_or_assign("second", 2);
  values.insert_or_assign("first", 3);
  REQUIRE(values.size() == 2);
  CHECK(values.entries()[0] == std::pair(std::string("first"), 3));
  CHECK(values.entries()[1] == std::pair(std::string("second"), 2));
  CHECK(*values.find("first") == 3);
  CHECK(values.find("missing") == nullptr);
  CHECK(values.erase("second"));
  CHECK_FALSE(values.erase("second"));

  CHECK_FALSE(NonEmptyVector<int>::from({}));
  auto nonempty = NonEmptyVector<int>::from({1, 2});
  REQUIRE(nonempty);
  CHECK(nonempty->values() == std::vector{1, 2});

  OrderedMap<int, int> empty_map;
  CHECK_FALSE(NonEmptyMap<int, int>::from(std::move(empty_map)));
}

TEST_CASE("bech32 round trips without the legacy 90-character limit", "[core]") {
  const Bytes payload(128, Byte{0x5a});
  const auto encoded = encode_bech32("addr_test", payload);
  REQUIRE(encoded);
  CHECK(encoded->size() > 90);
  const auto decoded = decode_bech32(*encoded);
  REQUIRE(decoded);
  CHECK(decoded->prefix == "addr_test");
  CHECK(decoded->bytes == payload);
}

TEST_CASE("bech32 follows frozen case checksum padding and HRP rules", "[core][bech32]") {
  const auto empty = encode_bech32("a", {});
  REQUIRE(empty);
  CHECK(*empty == "a12uel5l");
  const auto upper = decode_bech32("A12UEL5L");
  REQUIRE(upper);
  CHECK(upper->prefix == "a");
  CHECK(upper->bytes.empty());

  CHECK_FALSE(encode_bech32("", {}));
  CHECK_FALSE(encode_bech32("UPPER", {}));
  CHECK_FALSE(encode_bech32(std::string("\x20", 1), {}));
  CHECK_FALSE(decode_bech32("a12uEL5l"));
  CHECK_FALSE(decode_bech32("a12uel5q"));
  CHECK_FALSE(decode_bech32("no-separator"));
}

TEST_CASE("protocol and integer bounds are frozen", "[core]") {
  CHECK(BYRON_MAINNET_NETWORK_MAGIC == 764824073U);
  CHECK(BYRON_TESTNET_NETWORK_MAGIC == 1097911063U);
  CHECK(PREPROD_NETWORK_MAGIC == 1U);
  CHECK(PREVIEW_NETWORK_MAGIC == 2U);
  CHECK(SANCHO_TESTNET_NETWORK_MAGIC == 4U);
  CHECK(INT64_MIN_VALUE == std::numeric_limits<std::int64_t>::min());
  CHECK(INT64_MAX_VALUE == std::numeric_limits<std::int64_t>::max());
  CHECK(UINT64_MAX_VALUE == std::numeric_limits<std::uint64_t>::max());
}
