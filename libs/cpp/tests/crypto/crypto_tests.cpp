#include <algorithm>
#include <array>
#include <cardano/crypto/crypto.hpp>
#include <cardano/crypto/derivation.hpp>
#include <cardano/crypto/identity.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

using namespace cardano;

namespace {

class RecordedRandom final : public core::SecureRandomSource {
 public:
  core::Result<core::Bytes> random_bytes(std::size_t length) override {
    core::Bytes bytes(length, core::Byte{0});
    if (!bytes.empty()) {
      bytes[0] = core::Byte{0x28};
      if (bytes.size() > 31) {
        bytes[31] = core::Byte{0x40};
      }
    }
    return bytes;
  }
};

class FailingRandom final : public core::SecureRandomSource {
 public:
  core::Result<core::Bytes> random_bytes(std::size_t) override {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::random_unavailable, "recorded failure"));
  }
};

}  // namespace

static_assert(!std::is_copy_constructible_v<crypto::PrivateKey>);
static_assert(!std::is_copy_assignable_v<crypto::PrivateKey>);
static_assert(std::is_move_constructible_v<crypto::PrivateKey>);
static_assert(!std::is_copy_constructible_v<crypto::Bip32PrivateKey>);
static_assert(std::is_move_constructible_v<crypto::Bip32PrivateKey>);

TEST_CASE("required hash primitives match published empty-message vectors", "[crypto][hash]") {
  CHECK(core::bytes_to_hex(crypto::sha2_256({})) ==
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  CHECK(core::bytes_to_hex(crypto::sha3_256({})) ==
        "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");
  CHECK(core::bytes_to_hex(crypto::keccak_256({})) ==
        "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
  CHECK(core::bytes_to_hex(crypto::ripemd_160({})) == "9c1185a5c5e9fc54612808977ee8f548b2258d31");
  CHECK(core::bytes_to_hex(crypto::blake2b256({})) ==
        "0e5751c026e543b2e8ab2eb06099daa1d1e5df47778f7787faab45cdf12fe3a8");
  CHECK(crypto::blake2b224({}).size() == 28);
}

TEST_CASE("role-aware key text and CIP-1852 paths are checked") {
  auto extended = crypto::PrivateKey::from_extended_bytes(core::Bytes(64));
  REQUIRE(extended);
  const auto encoded = crypto::encode_cardano_private_key(crypto::KeyTextRole::drep, *extended);
  REQUIRE(encoded);
  const auto decoded = crypto::decode_cardano_private_key(
      crypto::KeyTextRole::drep, crypto::PrivateKeyForm::extended, *encoded);
  REQUIRE(decoded);
  CHECK(decoded->form() == crypto::PrivateKeyForm::extended);
  CHECK_FALSE(crypto::encode_cardano_private_key(crypto::KeyTextRole::payment, *extended));
  CHECK_FALSE(crypto::decode_cardano_private_key(crypto::KeyTextRole::stake,
                                                 crypto::PrivateKeyForm::extended, *encoded));

  CHECK(crypto::Cip1852Path::make(0, crypto::Cip1852Role::external, 0));
  CHECK(crypto::Cip1852Path::make(0x7fffffffU, crypto::Cip1852Role::cc_hot, 0x7fffffffU));
  CHECK_FALSE(crypto::Cip1852Path::make(0x80000000U, crypto::Cip1852Role::external, 0));
  CHECK_FALSE(crypto::Cip1852Path::make(0, crypto::Cip1852Role::external, 0x80000000U));
}

TEST_CASE("fixed hash wrappers enforce lengths and defensive ownership", "[crypto][hash]") {
  auto source = *core::hex_to_bytes(std::string(56, 'a'));
  auto hash = crypto::Ed25519KeyHash::from_bytes(source);
  REQUIRE(hash);
  source[0] = core::Byte{0};
  CHECK(hash->to_hex() == std::string(56, 'a'));
  CHECK_FALSE(crypto::Ed25519KeyHash::from_bytes({}));
  CHECK_FALSE(crypto::TransactionHash::from_hex(std::string(62, '0')));

  const auto bech32 = hash->to_bech32("caller_hrp");
  REQUIRE(bech32);
  const auto decoded = crypto::Ed25519KeyHash::from_bech32(*bech32);
  REQUIRE(decoded);
  CHECK(*decoded == *hash);
}

TEST_CASE("normal Ed25519 keys sign strictly and clear explicitly", "[crypto][ed25519]") {
  auto key = crypto::PrivateKey::from_hex(std::string(64, '0'));
  REQUIRE(key);
  const auto public_key = key->public_key();
  REQUIRE(public_key);
  const auto message = *core::hex_to_bytes("0102030405");
  const auto signature = key->sign(message);
  REQUIRE(signature);
  CHECK(public_key->verify(message, *signature));

  auto mutated = signature->to_bytes();
  mutated[0] ^= core::Byte{1};
  const auto bad_signature = crypto::Ed25519Signature::from_bytes(mutated);
  REQUIRE(bad_signature);
  CHECK_FALSE(public_key->verify(message, *bad_signature));
  CHECK_FALSE(crypto::verify_ed25519({}, message, signature->span()));

  const auto encoded_secret = key->to_bech32();
  REQUIRE(encoded_secret);
  CHECK(encoded_secret->starts_with("ed25519_sk1"));
  key->clear();
  CHECK_FALSE(key->sign(message));
  CHECK_FALSE(key->public_key());
  CHECK_FALSE(key->to_bech32());
}

TEST_CASE("BIP32 root and soft-child vectors match the frozen contract", "[crypto][bip32]") {
  const auto entropy =
      *core::hex_to_bytes("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
  const std::string password_text = "cardano";
  const core::ByteSpan password(reinterpret_cast<const core::Byte*>(password_text.data()),
                                password_text.size());
  auto root = crypto::Bip32PrivateKey::from_entropy(entropy, password);
  REQUIRE(root);
  CHECK(root->to_hex() ==
        "28bcf7f6439e62f304f589619f6b1612f9a984978b445e4ec6f59e595c051150"
        "fa47864365423db7ed7a117cd33c89aef1295322d385cad08eff39ede419e913a"
        "f586f2ce665c8bdddc4f470cdcea8b7a3a33e35730235f2c50fa08abbef2e48");

  auto child = root->derive(17);
  REQUIRE(child);
  auto child_public = child->public_key();
  REQUIRE(child_public);
  CHECK(child_public->to_hex() ==
        "e184c106be8885cfcaa5834fe6ff9f9e34531cd31729cef0517a0996cec2eae6"
        "83bb05dd2f53b203ecab6bb857c0979e0d4c810cf166412ab0564e91520ba22e");

  auto public_root = root->public_key();
  REQUIRE(public_root);
  const auto public_child = public_root->derive(17);
  REQUIRE(public_child);
  CHECK(*public_child == *child_public);
  CHECK_FALSE(public_root->derive(0x80000000U));
}

TEST_CASE("BIP32 constructors enforce scalar bits and 128-byte compatibility", "[crypto][bip32]") {
  auto invalid = *core::hex_to_bytes(std::string(192, '0'));
  CHECK_FALSE(crypto::Bip32PrivateKey::from_bytes(invalid));
  invalid[0] = core::Byte{0x28};
  invalid[31] = core::Byte{0x40};
  auto key = crypto::Bip32PrivateKey::from_bytes(invalid);
  REQUIRE(key);
  const auto expanded = key->to_128_xprv();
  REQUIRE(expanded);
  REQUIRE(expanded->size() == 128);

  auto altered_public = *expanded;
  std::fill(altered_public.begin() + 64, altered_public.begin() + 96, core::Byte{0xff});
  const auto restored = crypto::Bip32PrivateKey::from_128_xprv(altered_public);
  REQUIRE(restored);
  CHECK(restored->to_hex() == key->to_hex());
  CHECK_FALSE(crypto::Bip32PrivateKey::from_128_xprv({}));
}

TEST_CASE("injectable randomness propagates success and failure", "[crypto][random]") {
  RecordedRandom recorded;
  auto normal = crypto::PrivateKey::generate(recorded);
  REQUIRE(normal);
  CHECK(normal->to_hex().starts_with("28"));

  auto extended = crypto::Bip32PrivateKey::generate(recorded);
  REQUIRE(extended);
  CHECK((std::to_integer<std::uint8_t>(extended->to_bytes()[0]) & 0x07U) == 0);
  CHECK((std::to_integer<std::uint8_t>(extended->to_bytes()[31]) & 0xe0U) == 0x40U);

  FailingRandom failing;
  CHECK_FALSE(crypto::PrivateKey::generate(failing));
  CHECK_FALSE(crypto::Bip32PrivateKey::generate(failing));

  const auto system = crypto::secure_random_bytes(64);
  REQUIRE(system);
  CHECK(system->size() == 64);
}

TEST_CASE("EMIP-3 matches its frozen envelope and rejects mutations", "[crypto][emip3]") {
  constexpr std::string_view password = "70617373776f7264";
  constexpr std::string_view salt =
      "50515253c0c1c2c3c4c5c6c750515253c0c1c2c3c4c5c6c750515253c0c1c2c3";
  constexpr std::string_view nonce = "50515253c0c1c2c3c4c5c6c7";
  constexpr std::string_view plaintext = "736f6d65206461746120746f20656e6372797074";
  constexpr std::string_view envelope =
      "50515253c0c1c2c3c4c5c6c750515253c0c1c2c3c4c5c6c750515253c0c1c2c3"
      "50515253c0c1c2c3c4c5c6c7c266630887d216bf88cc4990f73bad7f35bc7c022"
      "5b38fe24a7c28b5f9bda6283e3c5768";

  const auto encrypted = crypto::emip3_encrypt_with_password(password, salt, nonce, plaintext);
  REQUIRE(encrypted);
  CHECK(*encrypted == envelope);
  const auto decrypted = crypto::emip3_decrypt_with_password(password, envelope);
  REQUIRE(decrypted);
  CHECK(*decrypted == plaintext);

  auto mutated = *core::hex_to_bytes(envelope);
  mutated[44] ^= core::Byte{1};
  CHECK_FALSE(crypto::emip3_decrypt_with_password(password, core::bytes_to_hex(mutated)));
  CHECK_FALSE(crypto::emip3_encrypt_with_password("", salt, nonce, plaintext));
  CHECK_FALSE(crypto::emip3_encrypt_with_password(password, "00", nonce, plaintext));
  CHECK_FALSE(crypto::emip3_decrypt_with_password(password, std::string(120, '0')));
}

TEST_CASE("secp256k1 adapters fail closed on malformed encodings", "[crypto][secp256k1]") {
  const auto zeros32 = *core::hex_to_bytes(std::string(64, '0'));
  const auto zeros64 = *core::hex_to_bytes(std::string(128, '0'));
  CHECK_FALSE(crypto::verify_secp256k1_ecdsa({}, zeros32, zeros64));
  CHECK_FALSE(crypto::verify_secp256k1_ecdsa(*core::hex_to_bytes("02" + std::string(64, '0')),
                                             zeros32, zeros64));
  CHECK_FALSE(crypto::verify_secp256k1_schnorr(zeros32, {}, zeros64));
  CHECK_FALSE(crypto::verify_secp256k1_schnorr(zeros32, zeros32, zeros64));
}

TEST_CASE("BLS12-381 adapters preserve group algebra and pairing results", "[crypto][bls]") {
  const auto message = *core::hex_to_bytes("010203");
  const auto domain = *core::hex_to_bytes("424c532d54455354");
  const auto g1 = crypto::bls12_381_hash_to_group(crypto::BlsGroup::g1, message, domain);
  const auto g2 = crypto::bls12_381_hash_to_group(crypto::BlsGroup::g2, message, domain);
  REQUIRE(g1);
  REQUIRE(g2);
  CHECK(crypto::bls12_381_compress(*g1).size() == 48U);
  CHECK(crypto::bls12_381_compress(*g2).size() == 96U);

  const auto decoded_g1 = crypto::bls12_381_uncompress(crypto::bls12_381_compress(*g1));
  const auto decoded_g2 = crypto::bls12_381_uncompress(crypto::bls12_381_compress(*g2));
  REQUIRE(decoded_g1);
  REQUIRE(decoded_g2);
  CHECK(crypto::bls12_381_equal(*decoded_g1, *g1));
  CHECK(crypto::bls12_381_equal(*decoded_g2, *g2));
  CHECK_FALSE(crypto::bls12_381_equal(*g1, *g2));

  const auto doubled = crypto::bls12_381_add(*g1, *g1);
  const auto scaled = crypto::bls12_381_scalar_mul(core::BigInteger(std::uint64_t{2}), *g1);
  REQUIRE(doubled);
  REQUIRE(scaled);
  CHECK(crypto::bls12_381_equal(*doubled, *scaled));

  const auto zero = crypto::bls12_381_add(*g1, crypto::bls12_381_neg(*g1));
  REQUIRE(zero);
  const auto zero_bytes = crypto::bls12_381_compress(*zero);
  REQUIRE(zero_bytes.size() == 48U);
  CHECK(zero_bytes.front() == core::Byte{0xc0});
  CHECK(std::all_of(zero_bytes.begin() + 1, zero_bytes.end(),
                    [](core::Byte value) { return value == core::Byte{0}; }));

  const auto pairing = crypto::bls12_381_miller_loop(*g1, *g2);
  const auto doubled_pairing = crypto::bls12_381_miller_loop(*doubled, *g2);
  REQUIRE(pairing);
  REQUIRE(doubled_pairing);
  CHECK(crypto::bls12_381_final_verify(*pairing, *pairing));
  const auto squared = crypto::bls12_381_mul_ml_result(*pairing, *pairing);
  CHECK(crypto::bls12_381_final_verify(*doubled_pairing, squared));
}

TEST_CASE("BLS12-381 adapters reject malformed encodings and domains", "[crypto][bls]") {
  CHECK_FALSE(crypto::bls12_381_uncompress(core::Bytes(47U)));
  CHECK_FALSE(crypto::bls12_381_uncompress(core::Bytes(48U)));
  CHECK_FALSE(crypto::bls12_381_hash_to_group(crypto::BlsGroup::g1, {}, core::Bytes(256U)));
}

TEST_CASE("legacy Byron signing and proxy certificates retain historical bytes",
          "[crypto][byron]") {
  const auto legacy_raw = core::hex_to_bytes(
      "28bcf7f6439e62f304f589619f6b1612f9a984978b445e4ec6f59e595c051150"
      "fa47864365423db7ed7a117cd33c89aef1295322d385cad08eff39ede419e913"
      "af586f2ce665c8bdddc4f470cdcea8b7a3a33e35730235f2c50fa08abbef2e48");
  REQUIRE(legacy_raw);
  auto legacy = crypto::LegacyDaedalusPrivateKey::from_bytes(*legacy_raw);
  REQUIRE(legacy);
  const core::Bytes message{std::byte{'c'}, std::byte{'a'}, std::byte{'r'}, std::byte{'d'},
                            std::byte{'a'}, std::byte{'n'}, std::byte{'o'}};
  const auto public_bytes = crypto::legacy_public_key(*legacy);
  const auto signature = crypto::legacy_sign(*legacy, message);
  REQUIRE(public_bytes);
  REQUIRE(signature);
  const auto public_key = crypto::PublicKey::from_bytes(core::ByteSpan(*public_bytes).first<32>());
  REQUIRE(public_key);
  CHECK(public_key->verify(message, *signature));
  const auto chain_code = legacy->chain_code();
  REQUIRE(chain_code);
  CHECK(*chain_code == core::Bytes(legacy_raw->begin() + 64, legacy_raw->end()));

  const auto issuer_bytes = core::hex_to_bytes(
      "b8b054ec1b92dd4542db35e2f813f013a8d7ee9f53255b26f3ef3dafb74e1146"
      "2545bd9c85aa0a6f6719a933eba16909c1a2fa0bbb58e9cd98bf9ddbb79f7d50"
      "fcfc22db8155f8d6ca0e3a975cb1b6aa5d6e7609b30c99877e469db06b5d5016");
  const auto delegate_bytes = core::hex_to_bytes(
      "695b380fc72ae7d830d46f902a7c9d4057a4b9a7a0be235b87fdf51e698619e0"
      "33aac8d93fd4cb82785973bb943f2047ddd1e664d4e185e7be634722e108389a");
  REQUIRE(issuer_bytes);
  REQUIRE(delegate_bytes);
  auto issuer = crypto::Bip32PrivateKey::from_bytes(*issuer_bytes);
  const auto delegate = crypto::Bip32PublicKey::from_bytes(*delegate_bytes);
  REQUIRE(issuer);
  REQUIRE(delegate);
  const auto proxy_data = crypto::byron_proxy_signing_data(*delegate, 0U, 328'429'219U);
  CHECK(core::bytes_to_hex(proxy_data) ==
        "0a1a13936ea358433030695b380fc72ae7d830d46f902a7c9d4057a4b9a7a0be"
        "235b87fdf51e698619e033aac8d93fd4cb82785973bb943f2047ddd1e664d4e18"
        "5e7be634722e108389a00");
  const auto certificate =
      crypto::sign_byron_proxy_certificate(*issuer, *delegate, 0U, 328'429'219U);
  REQUIRE(certificate);
  CHECK(certificate->to_hex() ==
        "a72bf0119afd1ba5bed56b6521544105b6077c884609666296dbc59275477149a"
        "1b8230ce5b6c0fa81e1ec61c717164be57422e86a8f2f5773cdc66da99fcc0e");
  const auto issuer_public = issuer->public_key();
  REQUIRE(issuer_public);
  CHECK(crypto::verify_byron_proxy_certificate(*issuer_public, *delegate, 0U, 328'429'219U,
                                               *certificate));
  CHECK_FALSE(crypto::verify_byron_proxy_certificate(*issuer_public, *delegate, 1U, 328'429'219U,
                                                     *certificate));

  legacy->clear();
  CHECK_FALSE(legacy->chain_code());
  CHECK_FALSE(crypto::legacy_sign(*legacy, message));
}

TEST_CASE("ABOR preserves fixed little-endian widths and nested token counts",
          "[crypto][byron][abor]") {
  crypto::AborEncoder encoder;
  REQUIRE(encoder.u16(10U));
  REQUIRE(encoder.u32(0x12345U));
  REQUIRE(encoder.u64(0xffeeddcc00112233ULL));
  const auto wide = core::BigInteger::from_decimal("1328880484970988709912991736447633682");
  REQUIRE(wide);
  REQUIRE(encoder.u128(*wide));
  const auto payload = core::hex_to_bytes("010203040506070809");
  REQUIRE(payload);
  REQUIRE(encoder.bytes(*payload));
  const auto encoded = encoder.finalize();
  REQUIRE(encoded);
  CHECK(core::bytes_to_hex(*encoded) ==
        "020a0003452301000433221100ccddeeff"
        "051209481902492133221100ccddeeff00"
        "0609010203040506070809");

  crypto::AborDecoder decoder(*encoded);
  CHECK(decoder.u16() == 10U);
  CHECK(decoder.u32() == 0x12345U);
  CHECK(decoder.u64() == 0xffeeddcc00112233ULL);
  CHECK(decoder.u128() == *wide);
  CHECK(decoder.bytes() == *payload);
  CHECK(decoder.end());

  crypto::AborEncoder nested;
  REQUIRE(nested.struct_start());
  REQUIRE(nested.u8(1U));
  REQUIRE(nested.struct_start());
  REQUIRE(nested.u8(2U));
  REQUIRE(nested.struct_end());
  REQUIRE(nested.struct_end());
  const auto nested_bytes = nested.finalize();
  REQUIRE(nested_bytes);
  CHECK(core::bytes_to_hex(*nested_bytes) == "0703010107010102");
  CHECK_FALSE(nested.struct_end());
}
