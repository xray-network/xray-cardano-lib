#include "cardano/crypto/crypto.hpp"

#include <blst.h>
#include <botan/aead.h>
#include <botan/exceptn.h>
#include <botan/hash.h>
#include <botan/mac.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>
#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "cardano/core/cbor.hpp"

namespace cardano::crypto {
namespace {

using U8Vector = std::vector<std::uint8_t>;

[[nodiscard]] U8Vector to_u8(core::ByteSpan bytes) {
  U8Vector output(bytes.size());
  std::transform(bytes.begin(), bytes.end(), output.begin(),
                 [](core::Byte value) { return std::to_integer<std::uint8_t>(value); });
  return output;
}

[[nodiscard]] core::Bytes core_bytes_from_u8(std::span<const std::uint8_t> bytes) {
  core::Bytes output(bytes.size());
  std::transform(bytes.begin(), bytes.end(), output.begin(),
                 [](std::uint8_t value) { return static_cast<core::Byte>(value); });
  return output;
}

template <std::size_t Size>
[[nodiscard]] std::array<core::Byte, Size> to_array(core::ByteSpan bytes) {
  std::array<core::Byte, Size> output{};
  std::ranges::copy(bytes, output.begin());
  return output;
}

[[nodiscard]] core::Bytes hash(std::string_view algorithm, core::ByteSpan input) {
  auto function = Botan::HashFunction::create_or_throw(algorithm);
  const auto native = to_u8(input);
  function->update(native);
  return core_bytes_from_u8(function->final_stdvec());
}

[[nodiscard]] core::Bytes hmac_sha512(core::ByteSpan key, core::ByteSpan input) {
  auto mac = Botan::MessageAuthenticationCode::create_or_throw("HMAC(SHA-512)");
  const auto native_key = to_u8(key);
  const auto native_input = to_u8(input);
  mac->set_key(native_key);
  mac->update(native_input);
  return core_bytes_from_u8(mac->final_stdvec());
}

[[nodiscard]] core::Result<core::Bytes> pbkdf2_sha512(core::ByteSpan password, core::ByteSpan salt,
                                                      std::uint32_t iterations,
                                                      std::size_t length) {
  if (iterations == 0) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_argument,
                                              "PBKDF2 iterations must be positive"));
  }
  core::Bytes output;
  output.reserve(length);
  std::uint32_t block = 1;
  while (output.size() < length) {
    core::Bytes initial(salt.begin(), salt.end());
    initial.push_back(static_cast<core::Byte>((block >> 24U) & 0xffU));
    initial.push_back(static_cast<core::Byte>((block >> 16U) & 0xffU));
    initial.push_back(static_cast<core::Byte>((block >> 8U) & 0xffU));
    initial.push_back(static_cast<core::Byte>(block & 0xffU));
    auto previous = hmac_sha512(password, initial);
    auto accumulated = previous;
    for (std::uint32_t round = 1; round < iterations; ++round) {
      previous = hmac_sha512(password, previous);
      for (std::size_t index = 0; index < accumulated.size(); ++index) {
        accumulated[index] ^= previous[index];
      }
    }
    const auto count = std::min(accumulated.size(), length - output.size());
    output.insert(output.end(), accumulated.begin(),
                  accumulated.begin() + static_cast<std::ptrdiff_t>(count));
    sodium_memzero(previous.data(), previous.size());
    sodium_memzero(accumulated.data(), accumulated.size());
    if (block == std::numeric_limits<std::uint32_t>::max() && output.size() < length) {
      sodium_memzero(output.data(), output.size());
      return std::unexpected(
          core::CardanoError(core::ErrorCode::out_of_range, "PBKDF2 output is too long"));
    }
    ++block;
  }
  return output;
}

[[nodiscard]] bool sodium_ready() noexcept { return sodium_init() >= 0; }

void add_little_endian_256(std::span<core::Byte, 32> destination, core::ByteSpan addend) {
  std::uint16_t carry = 0;
  for (std::size_t index = 0; index < destination.size(); ++index) {
    const auto right = index < addend.size() ? std::to_integer<std::uint8_t>(addend[index]) : 0U;
    const auto sum = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(destination[index]) +
                                                right + carry);
    destination[index] = static_cast<core::Byte>(sum & 0xffU);
    carry = sum >> 8U;
  }
}

[[nodiscard]] std::array<core::Byte, 32> eight_times_z28(core::ByteSpan z) {
  std::array<core::Byte, 32> result{};
  std::uint16_t carry = 0;
  for (std::size_t index = 0; index < 28; ++index) {
    const auto shifted =
        static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(z[index]) * 8U + carry);
    result[index] = static_cast<core::Byte>(shifted & 0xffU);
    carry = shifted >> 8U;
  }
  for (std::size_t index = 28; index < result.size() && carry != 0; ++index) {
    result[index] = static_cast<core::Byte>(carry & 0xffU);
    carry >>= 8U;
  }
  return result;
}

[[nodiscard]] std::array<core::Byte, 4> le32(std::uint32_t value) {
  return {static_cast<core::Byte>(value & 0xffU), static_cast<core::Byte>((value >> 8U) & 0xffU),
          static_cast<core::Byte>((value >> 16U) & 0xffU),
          static_cast<core::Byte>((value >> 24U) & 0xffU)};
}

[[nodiscard]] core::Result<std::array<core::Byte, 32>> extended_public(core::ByteSpan secret64) {
  if (secret64.size() != 64 || !sodium_ready()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::crypto_failure, "cannot derive extended public key"));
  }
  std::array<unsigned char, 32> public_key{};
  const auto secret = to_u8(secret64.first<32>());
  if (crypto_scalarmult_ed25519_base_noclamp(public_key.data(), secret.data()) != 0) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::crypto_failure, "extended secret scalar is invalid"));
  }
  std::array<core::Byte, 32> output{};
  std::transform(public_key.begin(), public_key.end(), output.begin(),
                 [](unsigned char value) { return static_cast<core::Byte>(value); });
  return output;
}

[[nodiscard]] core::Result<Ed25519Signature> extended_sign(core::ByteSpan secret64,
                                                           core::ByteSpan message) {
  auto public_key = extended_public(secret64);
  if (!public_key) {
    return std::unexpected(public_key.error());
  }

  core::Bytes nonce_input(secret64.begin() + 32, secret64.end());
  nonce_input.insert(nonce_input.end(), message.begin(), message.end());
  auto nonce_wide = hash("SHA-512", nonce_input);
  std::array<unsigned char, 32> nonce_scalar{};
  const auto nonce_native = to_u8(nonce_wide);
  crypto_core_ed25519_scalar_reduce(nonce_scalar.data(), nonce_native.data());

  std::array<unsigned char, 32> encoded_r{};
  if (crypto_scalarmult_ed25519_base_noclamp(encoded_r.data(), nonce_scalar.data()) != 0) {
    sodium_memzero(nonce_wide.data(), nonce_wide.size());
    return std::unexpected(
        core::CardanoError(core::ErrorCode::crypto_failure, "extended signing nonce is invalid"));
  }

  core::Bytes challenge_input;
  challenge_input.reserve(64 + message.size());
  for (const auto value : encoded_r) {
    challenge_input.push_back(static_cast<core::Byte>(value));
  }
  challenge_input.insert(challenge_input.end(), public_key->begin(), public_key->end());
  challenge_input.insert(challenge_input.end(), message.begin(), message.end());
  auto challenge_wide = hash("SHA-512", challenge_input);
  const auto challenge_native = to_u8(challenge_wide);
  std::array<unsigned char, 32> challenge_scalar{};
  crypto_core_ed25519_scalar_reduce(challenge_scalar.data(), challenge_native.data());

  std::array<unsigned char, 64> secret_wide{};
  const auto secret_left = to_u8(secret64.first<32>());
  std::ranges::copy(secret_left, secret_wide.begin());
  std::array<unsigned char, 32> secret_scalar{};
  crypto_core_ed25519_scalar_reduce(secret_scalar.data(), secret_wide.data());

  std::array<unsigned char, 32> product{};
  std::array<unsigned char, 32> signature_scalar{};
  crypto_core_ed25519_scalar_mul(product.data(), challenge_scalar.data(), secret_scalar.data());
  crypto_core_ed25519_scalar_add(signature_scalar.data(), nonce_scalar.data(), product.data());

  core::Bytes signature;
  signature.reserve(64);
  for (const auto value : encoded_r) {
    signature.push_back(static_cast<core::Byte>(value));
  }
  for (const auto value : signature_scalar) {
    signature.push_back(static_cast<core::Byte>(value));
  }
  sodium_memzero(nonce_wide.data(), nonce_wide.size());
  sodium_memzero(challenge_wide.data(), challenge_wide.size());
  sodium_memzero(secret_wide.data(), secret_wide.size());
  sodium_memzero(secret_scalar.data(), secret_scalar.size());
  sodium_memzero(product.data(), product.size());
  sodium_memzero(signature_scalar.data(), signature_scalar.size());
  return Ed25519Signature::from_bytes(signature);
}

struct SecpContextDeleter {
  void operator()(secp256k1_context* context) const noexcept { secp256k1_context_destroy(context); }
};

[[nodiscard]] std::unique_ptr<secp256k1_context, SecpContextDeleter> secp_context() {
  return std::unique_ptr<secp256k1_context, SecpContextDeleter>(
      secp256k1_context_create(SECP256K1_CONTEXT_VERIFY));
}

[[nodiscard]] core::Result<core::Bytes> parse_hex(std::string_view input) {
  auto bytes = core::hex_to_bytes(input);
  if (!bytes) {
    return std::unexpected(bytes.error());
  }
  return bytes;
}

}  // namespace

void enforce_linkage() noexcept {}

core::Result<core::Bytes> secure_random_bytes(std::size_t length) {
  if (!sodium_ready()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::random_unavailable,
                           "operating-system secure random source is unavailable"));
  }
  core::Bytes output(length);
  randombytes_buf(output.data(), output.size());
  return output;
}

core::Bytes blake2b224(core::ByteSpan input) { return hash("Blake2b(224)", input); }
core::Bytes blake2b160(core::ByteSpan input) { return hash("Blake2b(160)", input); }
core::Bytes blake2b256(core::ByteSpan input) { return hash("Blake2b(256)", input); }
core::Bytes sha2_256(core::ByteSpan input) { return hash("SHA-256", input); }
core::Bytes sha3_256(core::ByteSpan input) { return hash("SHA-3(256)", input); }
core::Bytes keccak_256(core::ByteSpan input) { return hash("Keccak-1600(256)", input); }
core::Bytes ripemd_160(core::ByteSpan input) { return hash("RIPEMD-160", input); }

bool verify_ed25519(core::ByteSpan public_key, core::ByteSpan message,
                    core::ByteSpan signature) noexcept {
  if (public_key.size() != crypto_sign_PUBLICKEYBYTES || signature.size() != crypto_sign_BYTES ||
      !sodium_ready()) {
    return false;
  }
  const auto key = to_u8(public_key);
  const auto native_message = to_u8(message);
  const auto native_signature = to_u8(signature);
  return crypto_sign_verify_detached(native_signature.data(), native_message.data(),
                                     native_message.size(), key.data()) == 0;
}

core::Result<bool> verify_ed25519_strict(core::ByteSpan public_key, core::ByteSpan message,
                                         core::ByteSpan signature) {
  if (public_key.size() != crypto_sign_PUBLICKEYBYTES || signature.size() != crypto_sign_BYTES) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_length,
                           "Ed25519 verification requires a 32-byte key and 64-byte signature"));
  }
  if (!sodium_ready()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::crypto_failure, "Ed25519 backend is unavailable"));
  }
  return verify_ed25519(public_key, message, signature);
}

bool verify_secp256k1_ecdsa(core::ByteSpan public_key, core::ByteSpan message_hash,
                            core::ByteSpan compact_signature) noexcept {
  if ((public_key.size() != 33 && public_key.size() != 65) || message_hash.size() != 32 ||
      compact_signature.size() != 64) {
    return false;
  }
  auto context = secp_context();
  if (!context) {
    return false;
  }
  const auto key_bytes = to_u8(public_key);
  const auto hash_bytes = to_u8(message_hash);
  const auto signature_bytes = to_u8(compact_signature);
  secp256k1_pubkey key{};
  secp256k1_ecdsa_signature signature{};
  if (secp256k1_ec_pubkey_parse(context.get(), &key, key_bytes.data(), key_bytes.size()) != 1 ||
      secp256k1_ecdsa_signature_parse_compact(context.get(), &signature, signature_bytes.data()) !=
          1) {
    return false;
  }
  secp256k1_ecdsa_signature normalized{};
  if (secp256k1_ecdsa_signature_normalize(context.get(), &normalized, &signature) != 0) {
    return false;
  }
  return secp256k1_ecdsa_verify(context.get(), &signature, hash_bytes.data(), &key) == 1;
}

core::Result<bool> verify_secp256k1_ecdsa_strict(core::ByteSpan public_key,
                                                 core::ByteSpan message_hash,
                                                 core::ByteSpan compact_signature) {
  if (public_key.size() != 33U || message_hash.size() != 32U || compact_signature.size() != 64U) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::invalid_length,
        "ECDSA verification requires a 33-byte key, 32-byte message hash, and 64-byte signature"));
  }
  auto context = secp_context();
  if (!context) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::crypto_failure, "secp256k1 backend is unavailable"));
  }
  const auto key_bytes = to_u8(public_key);
  const auto hash_bytes = to_u8(message_hash);
  const auto signature_bytes = to_u8(compact_signature);
  secp256k1_pubkey key{};
  secp256k1_ecdsa_signature signature{};
  if (secp256k1_ec_pubkey_parse(context.get(), &key, key_bytes.data(), key_bytes.size()) != 1) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding,
                           "ECDSA public key is not a valid compressed secp256k1 point"));
  }
  if (secp256k1_ecdsa_signature_parse_compact(context.get(), &signature, signature_bytes.data()) !=
      1) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "ECDSA compact signature scalar is out of range"));
  }
  secp256k1_ecdsa_signature normalized{};
  if (secp256k1_ecdsa_signature_normalize(context.get(), &normalized, &signature) != 0) {
    return false;
  }
  return secp256k1_ecdsa_verify(context.get(), &signature, hash_bytes.data(), &key) == 1;
}

bool verify_secp256k1_schnorr(core::ByteSpan x_only_public_key, core::ByteSpan message,
                              core::ByteSpan signature) noexcept {
  if (x_only_public_key.size() != 32 || signature.size() != 64) {
    return false;
  }
  auto context = secp_context();
  if (!context) {
    return false;
  }
  const auto key_bytes = to_u8(x_only_public_key);
  const auto message_bytes = to_u8(message);
  const auto signature_bytes = to_u8(signature);
  secp256k1_xonly_pubkey key{};
  if (secp256k1_xonly_pubkey_parse(context.get(), &key, key_bytes.data()) != 1) {
    return false;
  }
  return secp256k1_schnorrsig_verify(context.get(), signature_bytes.data(), message_bytes.data(),
                                     message_bytes.size(), &key) == 1;
}

core::Result<bool> verify_secp256k1_schnorr_strict(core::ByteSpan x_only_public_key,
                                                   core::ByteSpan message,
                                                   core::ByteSpan signature) {
  if (x_only_public_key.size() != 32U || signature.size() != 64U) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::invalid_length,
        "Schnorr verification requires a 32-byte x-only key and 64-byte signature"));
  }
  auto context = secp_context();
  if (!context) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::crypto_failure, "secp256k1 backend is unavailable"));
  }
  const auto key_bytes = to_u8(x_only_public_key);
  secp256k1_xonly_pubkey key{};
  if (secp256k1_xonly_pubkey_parse(context.get(), &key, key_bytes.data()) != 1) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding,
                           "Schnorr public key is not a valid secp256k1 x coordinate"));
  }
  return verify_secp256k1_schnorr(x_only_public_key, message, signature);
}

struct BlsPoint::Impl {
  blst_p1 g1{};
  blst_p2 g2{};
};

struct BlsMlResult::Impl {
  blst_fp12 value{};
};

BlsPoint::BlsPoint(BlsGroup group, std::shared_ptr<const Impl> impl)
    : group_(group), impl_(std::move(impl)) {}

BlsGroup BlsPoint::group() const noexcept { return group_; }

BlsMlResult::BlsMlResult(std::shared_ptr<const Impl> impl) : impl_(std::move(impl)) {}

core::Result<BlsPoint> bls12_381_uncompress(core::ByteSpan compressed) {
  auto impl = std::make_shared<BlsPoint::Impl>();
  const auto native = to_u8(compressed);
  if (compressed.size() == 48U) {
    blst_p1_affine affine{};
    if (blst_p1_uncompress(&affine, native.data()) != BLST_SUCCESS ||
        !blst_p1_affine_in_g1(&affine)) {
      return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                                "invalid compressed BLS12-381 G1 point"));
    }
    blst_p1_from_affine(&impl->g1, &affine);
    return BlsPoint(BlsGroup::g1, std::move(impl));
  }
  if (compressed.size() == 96U) {
    blst_p2_affine affine{};
    if (blst_p2_uncompress(&affine, native.data()) != BLST_SUCCESS ||
        !blst_p2_affine_in_g2(&affine)) {
      return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                                "invalid compressed BLS12-381 G2 point"));
    }
    blst_p2_from_affine(&impl->g2, &affine);
    return BlsPoint(BlsGroup::g2, std::move(impl));
  }
  return std::unexpected(core::CardanoError(
      core::ErrorCode::invalid_length, "compressed BLS12-381 point must contain 48 or 96 bytes"));
}

core::Bytes bls12_381_compress(const BlsPoint& point) {
  if (point.group_ == BlsGroup::g1) {
    std::array<std::uint8_t, 48> encoded{};
    blst_p1_compress(encoded.data(), &point.impl_->g1);
    return core_bytes_from_u8(encoded);
  }
  std::array<std::uint8_t, 96> encoded{};
  blst_p2_compress(encoded.data(), &point.impl_->g2);
  return core_bytes_from_u8(encoded);
}

core::Result<BlsPoint> bls12_381_hash_to_group(BlsGroup group, core::ByteSpan message,
                                               core::ByteSpan domain_separation_tag) {
  if (domain_separation_tag.size() > 255U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "BLS12-381 domain-separation tag exceeds 255 bytes"));
  }
  const auto native_message = to_u8(message);
  const auto native_domain = to_u8(domain_separation_tag);
  auto impl = std::make_shared<BlsPoint::Impl>();
  if (group == BlsGroup::g1) {
    blst_hash_to_g1(&impl->g1, native_message.data(), native_message.size(), native_domain.data(),
                    native_domain.size(), nullptr, 0U);
  } else {
    blst_hash_to_g2(&impl->g2, native_message.data(), native_message.size(), native_domain.data(),
                    native_domain.size(), nullptr, 0U);
  }
  return BlsPoint(group, std::move(impl));
}

core::Result<BlsPoint> bls12_381_add(const BlsPoint& left, const BlsPoint& right) {
  if (left.group_ != right.group_) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::invalid_argument, "BLS12-381 addition requires points in the same group"));
  }
  auto impl = std::make_shared<BlsPoint::Impl>();
  if (left.group_ == BlsGroup::g1) {
    blst_p1_add_or_double(&impl->g1, &left.impl_->g1, &right.impl_->g1);
  } else {
    blst_p2_add_or_double(&impl->g2, &left.impl_->g2, &right.impl_->g2);
  }
  return BlsPoint(left.group_, std::move(impl));
}

BlsPoint bls12_381_neg(const BlsPoint& point) {
  auto impl = std::make_shared<BlsPoint::Impl>(*point.impl_);
  if (point.group_ == BlsGroup::g1) {
    blst_p1_cneg(&impl->g1, true);
  } else {
    blst_p2_cneg(&impl->g2, true);
  }
  return BlsPoint(point.group_, std::move(impl));
}

core::Result<BlsPoint> bls12_381_scalar_mul(const core::BigInteger& scalar, const BlsPoint& point) {
  static const auto scalar_order = *core::BigInteger::from_decimal(
      "52435875175126190479447740508185965837690552500527637822603658699938581184513");
  auto reduced = scalar % scalar_order;
  if (reduced.is_negative()) {
    reduced += scalar_order;
  }
  auto bytes = reduced.to_unsigned_bytes_be();
  if (bytes.empty()) {
    bytes.push_back(core::Byte{0});
  }
  std::reverse(bytes.begin(), bytes.end());
  const auto native = to_u8(bytes);
  auto impl = std::make_shared<BlsPoint::Impl>();
  if (point.group_ == BlsGroup::g1) {
    blst_p1_mult(&impl->g1, &point.impl_->g1, native.data(), native.size() * 8U);
  } else {
    blst_p2_mult(&impl->g2, &point.impl_->g2, native.data(), native.size() * 8U);
  }
  return BlsPoint(point.group_, std::move(impl));
}

bool bls12_381_equal(const BlsPoint& left, const BlsPoint& right) noexcept {
  if (left.group_ != right.group_) {
    return false;
  }
  return left.group_ == BlsGroup::g1 ? blst_p1_is_equal(&left.impl_->g1, &right.impl_->g1)
                                     : blst_p2_is_equal(&left.impl_->g2, &right.impl_->g2);
}

core::Result<BlsMlResult> bls12_381_miller_loop(const BlsPoint& g1, const BlsPoint& g2) {
  if (g1.group_ != BlsGroup::g1 || g2.group_ != BlsGroup::g2) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_argument,
                           "BLS12-381 Miller loop requires a G1 point followed by G2"));
  }
  blst_p1_affine affine_g1{};
  blst_p2_affine affine_g2{};
  blst_p1_to_affine(&affine_g1, &g1.impl_->g1);
  blst_p2_to_affine(&affine_g2, &g2.impl_->g2);
  auto impl = std::make_shared<BlsMlResult::Impl>();
  blst_miller_loop(&impl->value, &affine_g2, &affine_g1);
  return BlsMlResult(std::move(impl));
}

BlsMlResult bls12_381_mul_ml_result(const BlsMlResult& left, const BlsMlResult& right) {
  auto impl = std::make_shared<BlsMlResult::Impl>();
  blst_fp12_mul(&impl->value, &left.impl_->value, &right.impl_->value);
  return BlsMlResult(std::move(impl));
}

bool bls12_381_final_verify(const BlsMlResult& left, const BlsMlResult& right) noexcept {
  return blst_fp12_finalverify(&left.impl_->value, &right.impl_->value);
}

PublicKey::PublicKey(std::array<core::Byte, 32> bytes) : bytes_(std::move(bytes)) {}

core::Result<PublicKey> PublicKey::from_bytes(core::ByteSpan bytes) {
  if (bytes.size() != 32) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_length, "Ed25519 public key must be 32 bytes"));
  }
  return PublicKey(to_array<32>(bytes));
}

core::Result<PublicKey> PublicKey::from_hex(std::string_view hex) {
  auto bytes = parse_hex(hex);
  return bytes ? from_bytes(*bytes) : std::unexpected(bytes.error());
}

core::Result<PublicKey> PublicKey::from_bech32(std::string_view encoded) {
  auto decoded = core::decode_bech32(encoded);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  if (decoded->prefix != "ed25519_pk") {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "Ed25519 public key Bech32 HRP must be ed25519_pk"));
  }
  return from_bytes(decoded->bytes);
}

core::Bytes PublicKey::to_bytes() const { return {bytes_.begin(), bytes_.end()}; }
std::string PublicKey::to_hex() const { return core::bytes_to_hex(bytes_); }
core::Result<std::string> PublicKey::to_bech32() const {
  return core::encode_bech32("ed25519_pk", bytes_);
}
Ed25519KeyHash PublicKey::hash() const { return *Ed25519KeyHash::from_bytes(blake2b224(bytes_)); }
bool PublicKey::verify(core::ByteSpan message, const Ed25519Signature& signature) const noexcept {
  return verify_ed25519(bytes_, message, signature.span());
}

PrivateKey::PrivateKey(std::array<core::Byte, 64> bytes, PrivateKeyForm form)
    : bytes_(std::move(bytes)), form_(form) {}
PrivateKey::PrivateKey(PrivateKey&& other) noexcept
    : bytes_(other.bytes_), form_(other.form_), cleared_(other.cleared_) {
  other.clear();
}
PrivateKey& PrivateKey::operator=(PrivateKey&& other) noexcept {
  if (this != &other) {
    clear();
    bytes_ = other.bytes_;
    form_ = other.form_;
    cleared_ = other.cleared_;
    other.clear();
  }
  return *this;
}
PrivateKey::~PrivateKey() { clear(); }

core::Result<PrivateKey> PrivateKey::from_bytes(core::ByteSpan bytes) {
  return from_normal_bytes(bytes);
}
core::Result<PrivateKey> PrivateKey::from_normal_bytes(core::ByteSpan bytes) {
  if (bytes.size() != 32) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "Ed25519 private key must be 32 bytes"));
  }
  std::array<core::Byte, 64> owned{};
  std::ranges::copy(bytes, owned.begin());
  return PrivateKey(owned, PrivateKeyForm::normal);
}
core::Result<PrivateKey> PrivateKey::from_extended_bytes(core::ByteSpan bytes) {
  if (bytes.size() != 64) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "extended Ed25519 private key must be 64 bytes"));
  }
  return PrivateKey(to_array<64>(bytes), PrivateKeyForm::extended);
}
core::Result<PrivateKey> PrivateKey::from_hex(std::string_view hex) {
  auto bytes = parse_hex(hex);
  return bytes ? from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Result<PrivateKey> PrivateKey::from_bech32(std::string_view encoded) {
  auto decoded = core::decode_bech32(encoded);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  if (decoded->prefix != "ed25519_sk") {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "Ed25519 private key Bech32 HRP must be ed25519_sk"));
  }
  return from_bytes(decoded->bytes);
}
core::Result<PrivateKey> PrivateKey::generate(core::SecureRandomSource& random) {
  auto bytes = random.random_bytes(32);
  return bytes ? from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Result<PrivateKey> PrivateKey::generate() {
  auto bytes = secure_random_bytes(32);
  return bytes ? from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Bytes PrivateKey::to_bytes() const {
  return {bytes_.begin(), bytes_.begin() + (form_ == PrivateKeyForm::normal ? 32 : 64)};
}
std::string PrivateKey::to_hex() const { return core::bytes_to_hex(to_bytes()); }
core::Result<std::string> PrivateKey::to_bech32() const {
  return cleared_ ? std::unexpected(
                        core::CardanoError(core::ErrorCode::disposed, "private key was cleared"))
                  : core::encode_bech32("ed25519_sk", to_bytes());
}
core::Result<PublicKey> PrivateKey::public_key() const {
  if (cleared_ || !sodium_ready()) {
    return std::unexpected(core::CardanoError(
        cleared_ ? core::ErrorCode::disposed : core::ErrorCode::crypto_failure,
        cleared_ ? "private key was cleared" : "libsodium initialization failed"));
  }
  if (form_ == PrivateKeyForm::extended) {
    auto value = extended_public(bytes_);
    return value ? PublicKey::from_bytes(*value) : std::unexpected(value.error());
  }
  const auto seed = to_u8(core::ByteSpan(bytes_).first<32>());
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> public_key{};
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> secret_key{};
  crypto_sign_seed_keypair(public_key.data(), secret_key.data(), seed.data());
  sodium_memzero(secret_key.data(), secret_key.size());
  return PublicKey::from_bytes(core_bytes_from_u8(public_key));
}
core::Result<Ed25519Signature> PrivateKey::sign(core::ByteSpan message) const {
  if (cleared_ || !sodium_ready()) {
    return std::unexpected(core::CardanoError(
        cleared_ ? core::ErrorCode::disposed : core::ErrorCode::crypto_failure,
        cleared_ ? "private key was cleared" : "libsodium initialization failed"));
  }
  if (form_ == PrivateKeyForm::extended) return extended_sign(bytes_, message);
  const auto seed = to_u8(core::ByteSpan(bytes_).first<32>());
  const auto native_message = to_u8(message);
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> public_key{};
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> secret_key{};
  std::array<unsigned char, crypto_sign_BYTES> signature{};
  crypto_sign_seed_keypair(public_key.data(), secret_key.data(), seed.data());
  crypto_sign_detached(signature.data(), nullptr, native_message.data(), native_message.size(),
                       secret_key.data());
  sodium_memzero(secret_key.data(), secret_key.size());
  return Ed25519Signature::from_bytes(core_bytes_from_u8(signature));
}
void PrivateKey::clear() noexcept {
  sodium_memzero(bytes_.data(), bytes_.size());
  cleared_ = true;
}
PrivateKeyForm PrivateKey::form() const noexcept { return form_; }

Bip32PublicKey::Bip32PublicKey(std::array<core::Byte, 64> bytes) : bytes_(std::move(bytes)) {}
core::Result<Bip32PublicKey> Bip32PublicKey::from_bytes(core::ByteSpan bytes) {
  if (bytes.size() != 64 || !sodium_ready()) {
    return std::unexpected(core::CardanoError(
        bytes.size() != 64 ? core::ErrorCode::invalid_length : core::ErrorCode::crypto_failure,
        bytes.size() != 64 ? "BIP32 public key must be 64 bytes"
                           : "libsodium initialization failed"));
  }
  const auto point = to_u8(bytes.first<32>());
  if (crypto_core_ed25519_is_valid_point(point.data()) != 1) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::invalid_encoding, "BIP32 public key contains an invalid Ed25519 point"));
  }
  return Bip32PublicKey(to_array<64>(bytes));
}
core::Result<Bip32PublicKey> Bip32PublicKey::from_hex(std::string_view hex) {
  auto bytes = parse_hex(hex);
  return bytes ? from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Result<Bip32PublicKey> Bip32PublicKey::from_bech32(std::string_view encoded) {
  auto decoded = core::decode_bech32(encoded);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  if (decoded->prefix != "xpub") {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "BIP32 public key Bech32 HRP must be xpub"));
  }
  return from_bytes(decoded->bytes);
}
core::Bytes Bip32PublicKey::to_bytes() const { return {bytes_.begin(), bytes_.end()}; }
std::string Bip32PublicKey::to_hex() const { return core::bytes_to_hex(bytes_); }
core::Result<std::string> Bip32PublicKey::to_bech32() const {
  return core::encode_bech32("xpub", bytes_);
}
core::Bytes Bip32PublicKey::chain_code() const { return {bytes_.begin() + 32, bytes_.end()}; }
PublicKey Bip32PublicKey::public_key() const {
  return *PublicKey::from_bytes(core::ByteSpan(bytes_).first<32>());
}
core::Result<Bip32PublicKey> Bip32PublicKey::derive(std::uint32_t index) const {
  if (index >= 0x80000000U) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_argument,
                           "cannot derive a hardened child from a BIP32 public key"));
  }
  const auto index_bytes = le32(index);
  core::Bytes z_input{core::Byte{0x02}};
  z_input.insert(z_input.end(), bytes_.begin(), bytes_.begin() + 32);
  z_input.insert(z_input.end(), index_bytes.begin(), index_bytes.end());
  core::Bytes i_input{core::Byte{0x03}};
  i_input.insert(i_input.end(), bytes_.begin(), bytes_.begin() + 32);
  i_input.insert(i_input.end(), index_bytes.begin(), index_bytes.end());
  const core::ByteSpan chain(bytes_.data() + 32, 32);
  const auto z = hmac_sha512(chain, z_input);
  const auto i = hmac_sha512(chain, i_input);
  const auto scalar_bytes = eight_times_z28(z);
  const auto scalar = to_u8(scalar_bytes);
  const auto parent = to_u8(core::ByteSpan(bytes_).first<32>());
  std::array<unsigned char, 32> delta{};
  std::array<unsigned char, 32> child{};
  if (crypto_scalarmult_ed25519_base_noclamp(delta.data(), scalar.data()) != 0 ||
      crypto_core_ed25519_add(child.data(), parent.data(), delta.data()) != 0) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::crypto_failure,
                           "BIP32 public child derivation produced an invalid point"));
  }
  core::Bytes result = core_bytes_from_u8(child);
  result.insert(result.end(), i.begin() + 32, i.end());
  return from_bytes(result);
}

Bip32PrivateKey::Bip32PrivateKey(std::array<core::Byte, 96> bytes) : bytes_(std::move(bytes)) {}
Bip32PrivateKey::Bip32PrivateKey(Bip32PrivateKey&& other) noexcept
    : bytes_(other.bytes_), cleared_(other.cleared_) {
  other.clear();
}
Bip32PrivateKey& Bip32PrivateKey::operator=(Bip32PrivateKey&& other) noexcept {
  if (this != &other) {
    clear();
    bytes_ = other.bytes_;
    cleared_ = other.cleared_;
    other.clear();
  }
  return *this;
}
Bip32PrivateKey::~Bip32PrivateKey() { clear(); }
core::Result<Bip32PrivateKey> Bip32PrivateKey::from_bytes(core::ByteSpan bytes) {
  if (bytes.size() != 96) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_length, "BIP32 private key must be 96 bytes"));
  }
  if ((std::to_integer<std::uint8_t>(bytes[0]) & 0x07U) != 0 ||
      (std::to_integer<std::uint8_t>(bytes[31]) & 0xe0U) != 0x40U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "BIP32 private key scalar bits are not normalized"));
  }
  return Bip32PrivateKey(to_array<96>(bytes));
}
core::Result<Bip32PrivateKey> Bip32PrivateKey::from_hex(std::string_view hex) {
  auto bytes = parse_hex(hex);
  return bytes ? from_bytes(*bytes) : std::unexpected(bytes.error());
}
core::Result<Bip32PrivateKey> Bip32PrivateKey::from_bech32(std::string_view encoded) {
  auto decoded = core::decode_bech32(encoded);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  if (decoded->prefix != "xprv") {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "BIP32 private key Bech32 HRP must be xprv"));
  }
  return from_bytes(decoded->bytes);
}
core::Result<Bip32PrivateKey> Bip32PrivateKey::from_128_xprv(core::ByteSpan bytes) {
  if (bytes.size() != 128) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::invalid_length, "128-byte extended private key has an invalid length"));
  }
  core::Bytes compact(bytes.begin(), bytes.begin() + 64);
  compact.insert(compact.end(), bytes.begin() + 96, bytes.end());
  return from_bytes(compact);
}
core::Result<Bip32PrivateKey> Bip32PrivateKey::from_entropy(core::ByteSpan entropy,
                                                            core::ByteSpan password) {
  auto derived = pbkdf2_sha512(password, entropy, 4096, 96);
  if (!derived) {
    return std::unexpected(derived.error());
  }
  (*derived)[0] &= core::Byte{0xf8};
  (*derived)[31] =
      static_cast<core::Byte>((std::to_integer<std::uint8_t>((*derived)[31]) & 0x1fU) | 0x40U);
  auto result = from_bytes(*derived);
  sodium_memzero(derived->data(), derived->size());
  return result;
}
core::Result<Bip32PrivateKey> Bip32PrivateKey::generate(core::SecureRandomSource& random) {
  auto bytes = random.random_bytes(96);
  if (!bytes) {
    return std::unexpected(bytes.error());
  }
  (*bytes)[0] &= core::Byte{0xf8};
  (*bytes)[31] =
      static_cast<core::Byte>((std::to_integer<std::uint8_t>((*bytes)[31]) & 0x1fU) | 0x40U);
  auto result = from_bytes(*bytes);
  sodium_memzero(bytes->data(), bytes->size());
  return result;
}
core::Result<Bip32PrivateKey> Bip32PrivateKey::generate() {
  auto bytes = secure_random_bytes(96);
  if (!bytes) {
    return std::unexpected(bytes.error());
  }
  (*bytes)[0] &= core::Byte{0xf8};
  (*bytes)[31] =
      static_cast<core::Byte>((std::to_integer<std::uint8_t>((*bytes)[31]) & 0x1fU) | 0x40U);
  auto result = from_bytes(*bytes);
  sodium_memzero(bytes->data(), bytes->size());
  return result;
}
core::Bytes Bip32PrivateKey::to_bytes() const { return {bytes_.begin(), bytes_.end()}; }
core::Result<core::Bytes> Bip32PrivateKey::to_128_xprv() const {
  auto public_value = public_key();
  if (!public_value) {
    return std::unexpected(public_value.error());
  }
  core::Bytes output(bytes_.begin(), bytes_.begin() + 64);
  const auto public_bytes = public_value->to_bytes();
  output.insert(output.end(), public_bytes.begin(), public_bytes.begin() + 32);
  output.insert(output.end(), bytes_.begin() + 64, bytes_.end());
  return output;
}
std::string Bip32PrivateKey::to_hex() const { return core::bytes_to_hex(bytes_); }
core::Result<std::string> Bip32PrivateKey::to_bech32() const {
  return cleared_ ? std::unexpected(core::CardanoError(core::ErrorCode::disposed,
                                                       "BIP32 private key was cleared"))
                  : core::encode_bech32("xprv", bytes_);
}
core::Bytes Bip32PrivateKey::chain_code() const { return {bytes_.begin() + 64, bytes_.end()}; }
core::Result<Bip32PublicKey> Bip32PrivateKey::public_key() const {
  if (cleared_) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::disposed, "BIP32 private key was cleared"));
  }
  auto public_bytes = extended_public(core::ByteSpan(bytes_).first<64>());
  if (!public_bytes) {
    return std::unexpected(public_bytes.error());
  }
  core::Bytes output(public_bytes->begin(), public_bytes->end());
  output.insert(output.end(), bytes_.begin() + 64, bytes_.end());
  return Bip32PublicKey::from_bytes(output);
}
core::Result<Bip32PrivateKey> Bip32PrivateKey::derive(std::uint32_t index) const {
  if (cleared_) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::disposed, "BIP32 private key was cleared"));
  }
  const bool hardened = index >= 0x80000000U;
  core::Bytes material;
  if (hardened) {
    material.insert(material.end(), bytes_.begin(), bytes_.begin() + 64);
  } else {
    auto public_value = extended_public(core::ByteSpan(bytes_).first<64>());
    if (!public_value) {
      return std::unexpected(public_value.error());
    }
    material.insert(material.end(), public_value->begin(), public_value->end());
  }
  const auto index_bytes = le32(index);
  core::Bytes z_input{static_cast<core::Byte>(hardened ? 0x00 : 0x02)};
  z_input.insert(z_input.end(), material.begin(), material.end());
  z_input.insert(z_input.end(), index_bytes.begin(), index_bytes.end());
  core::Bytes i_input{static_cast<core::Byte>(hardened ? 0x01 : 0x03)};
  i_input.insert(i_input.end(), material.begin(), material.end());
  i_input.insert(i_input.end(), index_bytes.begin(), index_bytes.end());
  const core::ByteSpan chain(bytes_.data() + 64, 32);
  const auto z = hmac_sha512(chain, z_input);
  const auto i = hmac_sha512(chain, i_input);
  auto child = bytes_;
  const auto left_delta = eight_times_z28(z);
  add_little_endian_256(std::span<core::Byte, 32>(child.data(), 32), left_delta);
  add_little_endian_256(std::span<core::Byte, 32>(child.data() + 32, 32),
                        core::ByteSpan(z).subspan(32, 32));
  std::ranges::copy(i.begin() + 32, i.end(), child.begin() + 64);
  return from_bytes(child);
}
core::Result<Ed25519Signature> Bip32PrivateKey::sign(core::ByteSpan message) const {
  return cleared_ ? std::unexpected(core::CardanoError(core::ErrorCode::disposed,
                                                       "BIP32 private key was cleared"))
                  : extended_sign(core::ByteSpan(bytes_).first<64>(), message);
}
core::Result<PrivateKey> Bip32PrivateKey::to_raw_key() const {
  return cleared_ ? std::unexpected(core::CardanoError(core::ErrorCode::disposed,
                                                       "BIP32 private key was cleared"))
                  : PrivateKey::from_extended_bytes(core::ByteSpan(bytes_).first<64>());
}
void Bip32PrivateKey::clear() noexcept {
  sodium_memzero(bytes_.data(), bytes_.size());
  cleared_ = true;
}

core::Result<std::string> emip3_encrypt_with_password(std::string_view password_hex,
                                                      std::string_view salt_hex,
                                                      std::string_view nonce_hex,
                                                      std::string_view plaintext_hex) {
  auto password = parse_hex(password_hex);
  auto salt = parse_hex(salt_hex);
  auto nonce = parse_hex(nonce_hex);
  auto plaintext = parse_hex(plaintext_hex);
  if (!password || !salt || !nonce || !plaintext) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "EMIP-3 inputs must be strict hexadecimal"));
  }
  if (password->empty() || salt->size() != 32 || nonce->size() != 12) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_length,
                           "EMIP-3 requires nonempty password, 32-byte salt, and 12-byte nonce"));
  }
  auto key = pbkdf2_sha512(*password, *salt, 19162, 32);
  if (!key) {
    return std::unexpected(key.error());
  }
  try {
    auto cipher =
        Botan::AEAD_Mode::create_or_throw("ChaCha20Poly1305", Botan::Cipher_Dir::Encryption);
    const auto native_key = to_u8(*key);
    const auto native_nonce = to_u8(*nonce);
    cipher->set_key(native_key);
    cipher->start(native_nonce);
    const auto native_plaintext = to_u8(*plaintext);
    Botan::secure_vector<std::uint8_t> buffer(native_plaintext.begin(), native_plaintext.end());
    cipher->finish(buffer);
    const auto tag_length = cipher->tag_size();
    core::Bytes envelope(*salt);
    envelope.insert(envelope.end(), nonce->begin(), nonce->end());
    const auto native_ciphertext = core_bytes_from_u8(buffer);
    envelope.insert(envelope.end(),
                    native_ciphertext.end() - static_cast<std::ptrdiff_t>(tag_length),
                    native_ciphertext.end());
    envelope.insert(envelope.end(), native_ciphertext.begin(),
                    native_ciphertext.end() - static_cast<std::ptrdiff_t>(tag_length));
    sodium_memzero(key->data(), key->size());
    sodium_memzero(password->data(), password->size());
    return core::bytes_to_hex(envelope);
  } catch (const Botan::Exception&) {
    sodium_memzero(key->data(), key->size());
    sodium_memzero(password->data(), password->size());
    return std::unexpected(
        core::CardanoError(core::ErrorCode::crypto_failure, "EMIP-3 encryption failed"));
  }
}

core::Result<std::string> emip3_decrypt_with_password(std::string_view password_hex,
                                                      std::string_view envelope_hex) {
  auto password = parse_hex(password_hex);
  auto envelope = parse_hex(envelope_hex);
  if (!password || !envelope) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              "EMIP-3 inputs must be strict hexadecimal"));
  }
  if (password->empty() || envelope->size() <= 60) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::invalid_length, "EMIP-3 envelope must contain a nonempty ciphertext"));
  }
  const core::ByteSpan salt(envelope->data(), 32);
  const core::ByteSpan nonce(envelope->data() + 32, 12);
  const core::ByteSpan tag(envelope->data() + 44, 16);
  const core::ByteSpan ciphertext(envelope->data() + 60, envelope->size() - 60);
  auto key = pbkdf2_sha512(*password, salt, 19162, 32);
  if (!key) {
    return std::unexpected(key.error());
  }
  try {
    auto cipher =
        Botan::AEAD_Mode::create_or_throw("ChaCha20Poly1305", Botan::Cipher_Dir::Decryption);
    const auto native_key = to_u8(*key);
    const auto native_nonce = to_u8(nonce);
    cipher->set_key(native_key);
    cipher->start(native_nonce);
    core::Bytes combined(ciphertext.begin(), ciphertext.end());
    combined.insert(combined.end(), tag.begin(), tag.end());
    const auto native_combined = to_u8(combined);
    Botan::secure_vector<std::uint8_t> buffer(native_combined.begin(), native_combined.end());
    cipher->finish(buffer);
    const auto plaintext = core_bytes_from_u8(buffer);
    sodium_memzero(key->data(), key->size());
    sodium_memzero(password->data(), password->size());
    return core::bytes_to_hex(plaintext);
  } catch (const Botan::Exception&) {
    sodium_memzero(key->data(), key->size());
    sodium_memzero(password->data(), password->size());
    return std::unexpected(
        core::CardanoError(core::ErrorCode::authentication_failed, "EMIP-3 authentication failed"));
  }
}

LegacyDaedalusPrivateKey::LegacyDaedalusPrivateKey(std::array<core::Byte, 96> bytes)
    : bytes_(std::move(bytes)) {}
LegacyDaedalusPrivateKey::LegacyDaedalusPrivateKey(LegacyDaedalusPrivateKey&& other) noexcept
    : bytes_(other.bytes_), cleared_(other.cleared_) {
  other.clear();
}
LegacyDaedalusPrivateKey& LegacyDaedalusPrivateKey::operator=(
    LegacyDaedalusPrivateKey&& other) noexcept {
  if (this != &other) {
    clear();
    bytes_ = other.bytes_;
    cleared_ = other.cleared_;
    other.clear();
  }
  return *this;
}
LegacyDaedalusPrivateKey::~LegacyDaedalusPrivateKey() { clear(); }
core::Result<LegacyDaedalusPrivateKey> LegacyDaedalusPrivateKey::from_bytes(core::ByteSpan bytes) {
  if (bytes.size() != 96U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "legacy Daedalus private key must be 96 bytes"));
  }
  return LegacyDaedalusPrivateKey(to_array<96>(bytes));
}
core::Result<core::Bytes> LegacyDaedalusPrivateKey::to_bytes() const {
  return cleared_ ? core::Result<core::Bytes>(std::unexpected(core::CardanoError(
                        core::ErrorCode::disposed, "legacy Daedalus private key was cleared")))
                  : core::Result<core::Bytes>(core::Bytes(bytes_.begin(), bytes_.end()));
}
core::Result<core::Bytes> LegacyDaedalusPrivateKey::chain_code() const {
  return cleared_ ? core::Result<core::Bytes>(std::unexpected(core::CardanoError(
                        core::ErrorCode::disposed, "legacy Daedalus private key was cleared")))
                  : core::Result<core::Bytes>(core::Bytes(bytes_.begin() + 64, bytes_.end()));
}
void LegacyDaedalusPrivateKey::clear() noexcept {
  sodium_memzero(bytes_.data(), bytes_.size());
  cleared_ = true;
}

core::Result<core::Bytes> legacy_public_key(const LegacyDaedalusPrivateKey& key) {
  if (key.cleared_) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::disposed, "legacy Daedalus private key was cleared"));
  }
  auto public_key = extended_public(core::ByteSpan(key.bytes_).first<64>());
  if (!public_key) {
    return std::unexpected(public_key.error());
  }
  core::Bytes output(public_key->begin(), public_key->end());
  output.insert(output.end(), key.bytes_.begin() + 64, key.bytes_.end());
  return output;
}

core::Result<Ed25519Signature> legacy_sign(const LegacyDaedalusPrivateKey& key,
                                           core::ByteSpan message) {
  return key.cleared_ ? core::Result<Ed25519Signature>(std::unexpected(core::CardanoError(
                            core::ErrorCode::disposed, "legacy Daedalus private key was cleared")))
                      : extended_sign(core::ByteSpan(key.bytes_).first<64>(), message);
}

core::Bytes byron_proxy_signing_data(const Bip32PublicKey& delegate, std::uint64_t omega,
                                     std::uint32_t protocol_magic) {
  const auto magic = *core::cbor::encode_cbor(core::cbor::Value::unsigned_integer(core::BigInteger(
                                                  static_cast<std::uint64_t>(protocol_magic))),
                                              {.mode = core::cbor::Mode::canonical});
  core::Bytes inner{core::Byte{0x30}, core::Byte{0x30}};
  const auto delegate_bytes = delegate.to_bytes();
  inner.insert(inner.end(), delegate_bytes.begin(), delegate_bytes.end());
  const auto encoded_omega =
      *core::cbor::encode_cbor(core::cbor::Value::unsigned_integer(core::BigInteger(omega)),
                               {.mode = core::cbor::Mode::canonical});
  inner.insert(inner.end(), encoded_omega.begin(), encoded_omega.end());
  const auto encoded_inner = *core::cbor::encode_cbor(
      core::cbor::Value::byte_string(std::move(inner)), {.mode = core::cbor::Mode::canonical});
  core::Bytes output{static_cast<core::Byte>(ByronSigningTag::proxy_secret_key)};
  output.insert(output.end(), magic.begin(), magic.end());
  output.insert(output.end(), encoded_inner.begin(), encoded_inner.end());
  return output;
}

core::Result<Ed25519Signature> sign_byron_proxy_certificate(const Bip32PrivateKey& issuer,
                                                            const Bip32PublicKey& delegate,
                                                            std::uint64_t omega,
                                                            std::uint32_t protocol_magic) {
  const auto data = byron_proxy_signing_data(delegate, omega, protocol_magic);
  return issuer.sign(data);
}

bool verify_byron_proxy_certificate(const Bip32PublicKey& issuer, const Bip32PublicKey& delegate,
                                    std::uint64_t omega, std::uint32_t protocol_magic,
                                    const Ed25519Signature& certificate) noexcept {
  const auto data = byron_proxy_signing_data(delegate, omega, protocol_magic);
  return issuer.public_key().verify(data, certificate);
}

core::VoidResult AborEncoder::integer(std::uint8_t tag, const core::BigInteger& value,
                                      std::size_t width) {
  if (value.is_negative()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::out_of_range, "ABOR integer cannot be negative"));
  }
  const auto encoded = value.to_unsigned_bytes_be();
  if (encoded.size() > width) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::out_of_range, "ABOR integer is out of range"));
  }
  bytes_.push_back(static_cast<core::Byte>(tag));
  for (std::size_t index = 0U; index < width; ++index) {
    const auto from_end = index + 1U;
    bytes_.push_back(from_end <= encoded.size() ? encoded[encoded.size() - from_end]
                                                : core::Byte{0});
  }
  ++tokens_;
  return std::monostate{};
}
core::VoidResult AborEncoder::u8(std::uint8_t value) {
  return integer(1U, core::BigInteger(static_cast<std::uint64_t>(value)), 1U);
}
core::VoidResult AborEncoder::u16(std::uint16_t value) {
  return integer(2U, core::BigInteger(static_cast<std::uint64_t>(value)), 2U);
}
core::VoidResult AborEncoder::u32(std::uint32_t value) {
  return integer(3U, core::BigInteger(static_cast<std::uint64_t>(value)), 4U);
}
core::VoidResult AborEncoder::u64(std::uint64_t value) {
  return integer(4U, core::BigInteger(value), 8U);
}
core::VoidResult AborEncoder::u128(const core::BigInteger& value) {
  return integer(5U, value, 16U);
}
core::VoidResult AborEncoder::bytes(core::ByteSpan value) {
  if (value.size() >= 256U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                              "ABOR byte strings must be shorter than 256 bytes"));
  }
  bytes_.push_back(core::Byte{6});
  bytes_.push_back(static_cast<core::Byte>(value.size()));
  bytes_.insert(bytes_.end(), value.begin(), value.end());
  ++tokens_;
  return std::monostate{};
}
core::VoidResult AborEncoder::struct_start() {
  const auto offset = bytes_.size();
  bytes_.push_back(core::Byte{7});
  bytes_.push_back(core::Byte{0xfe});
  arrays_.push_back({offset, tokens_ + 1U});
  ++tokens_;
  return std::monostate{};
}
core::VoidResult AborEncoder::struct_end() {
  if (arrays_.empty()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure, "unmatched ABOR array end"));
  }
  const auto marker = arrays_.back();
  arrays_.pop_back();
  const auto length = tokens_ - marker.token_start;
  if (length >= 256U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                              "ABOR arrays must contain fewer than 256 values"));
  }
  bytes_[marker.byte_offset + 1U] = static_cast<core::Byte>(length);
  return std::monostate{};
}
core::Result<core::Bytes> AborEncoder::finalize() const {
  if (!arrays_.empty()) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_structure, "unclosed ABOR array"));
  }
  return bytes_;
}

AborDecoder::AborDecoder(core::ByteSpan bytes) : bytes_(bytes.begin(), bytes.end()) {}
core::Result<core::ByteSpan> AborDecoder::take(std::size_t length) {
  if (length > bytes_.size() - offset_) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::truncated_input, "truncated ABOR input"));
  }
  const core::ByteSpan output(bytes_.data() + offset_, length);
  offset_ += length;
  return output;
}
core::Result<core::BigInteger> AborDecoder::integer(std::uint8_t tag, std::size_t width) {
  auto actual = take(1U);
  if (!actual) {
    return std::unexpected(actual.error());
  }
  if (std::to_integer<std::uint8_t>((*actual)[0]) != tag) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "unexpected ABOR tag"));
  }
  auto encoded = take(width);
  if (!encoded) {
    return std::unexpected(encoded.error());
  }
  core::Bytes big_endian(encoded->rbegin(), encoded->rend());
  return core::BigInteger::from_unsigned_bytes_be(big_endian);
}
core::Result<std::uint8_t> AborDecoder::u8() {
  auto value = integer(1U, 1U);
  auto converted =
      value ? value->to_uint64() : core::Result<std::uint64_t>(std::unexpected(value.error()));
  return converted ? core::Result<std::uint8_t>(static_cast<std::uint8_t>(*converted))
                   : std::unexpected(converted.error());
}
core::Result<std::uint16_t> AborDecoder::u16() {
  auto value = integer(2U, 2U);
  auto converted =
      value ? value->to_uint64() : core::Result<std::uint64_t>(std::unexpected(value.error()));
  return converted ? core::Result<std::uint16_t>(static_cast<std::uint16_t>(*converted))
                   : std::unexpected(converted.error());
}
core::Result<std::uint32_t> AborDecoder::u32() {
  auto value = integer(3U, 4U);
  auto converted =
      value ? value->to_uint64() : core::Result<std::uint64_t>(std::unexpected(value.error()));
  return converted ? core::Result<std::uint32_t>(static_cast<std::uint32_t>(*converted))
                   : std::unexpected(converted.error());
}
core::Result<std::uint64_t> AborDecoder::u64() {
  auto value = integer(4U, 8U);
  return value ? value->to_uint64() : core::Result<std::uint64_t>(std::unexpected(value.error()));
}
core::Result<core::BigInteger> AborDecoder::u128() { return integer(5U, 16U); }
core::Result<core::Bytes> AborDecoder::bytes() {
  auto tag = take(1U);
  auto length = take(1U);
  if (!tag || !length) {
    return std::unexpected(!tag ? tag.error() : length.error());
  }
  if (std::to_integer<std::uint8_t>((*tag)[0]) != 6U) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding, "unexpected ABOR tag"));
  }
  auto value = take(std::to_integer<std::uint8_t>((*length)[0]));
  return value ? core::Result<core::Bytes>(core::Bytes(value->begin(), value->end()))
               : std::unexpected(value.error());
}
core::Result<std::uint8_t> AborDecoder::array() {
  auto tag = take(1U);
  auto length = take(1U);
  if (!tag || !length) {
    return std::unexpected(!tag ? tag.error() : length.error());
  }
  return std::to_integer<std::uint8_t>((*tag)[0]) == 7U
             ? core::Result<std::uint8_t>(std::to_integer<std::uint8_t>((*length)[0]))
             : std::unexpected(
                   core::CardanoError(core::ErrorCode::invalid_encoding, "unexpected ABOR tag"));
}
core::VoidResult AborDecoder::end() const {
  return offset_ == bytes_.size() ? core::VoidResult(std::monostate{})
                                  : std::unexpected(core::CardanoError(
                                        core::ErrorCode::trailing_data, "pending ABOR input"));
}

}  // namespace cardano::crypto
