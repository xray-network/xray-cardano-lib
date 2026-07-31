#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

#include "cardano/core/big_integer.hpp"
#include "cardano/core/bytes.hpp"
#include "cardano/core/error.hpp"

namespace cardano::crypto {

[[nodiscard]] core::Result<core::Bytes> secure_random_bytes(std::size_t length);

[[nodiscard]] core::Bytes blake2b224(core::ByteSpan input);
[[nodiscard]] core::Bytes blake2b256(core::ByteSpan input);
[[nodiscard]] core::Bytes sha2_256(core::ByteSpan input);
[[nodiscard]] core::Bytes sha3_256(core::ByteSpan input);
[[nodiscard]] core::Bytes keccak_256(core::ByteSpan input);
[[nodiscard]] core::Bytes ripemd_160(core::ByteSpan input);

[[nodiscard]] bool verify_ed25519(core::ByteSpan public_key, core::ByteSpan message,
                                  core::ByteSpan signature) noexcept;
[[nodiscard]] core::Result<bool> verify_ed25519_strict(core::ByteSpan public_key,
                                                       core::ByteSpan message,
                                                       core::ByteSpan signature);
[[nodiscard]] bool verify_secp256k1_ecdsa(core::ByteSpan public_key, core::ByteSpan message_hash,
                                          core::ByteSpan compact_signature) noexcept;
[[nodiscard]] core::Result<bool> verify_secp256k1_ecdsa_strict(core::ByteSpan public_key,
                                                               core::ByteSpan message_hash,
                                                               core::ByteSpan compact_signature);
[[nodiscard]] bool verify_secp256k1_schnorr(core::ByteSpan x_only_public_key,
                                            core::ByteSpan message,
                                            core::ByteSpan signature) noexcept;
[[nodiscard]] core::Result<bool> verify_secp256k1_schnorr_strict(core::ByteSpan x_only_public_key,
                                                                 core::ByteSpan message,
                                                                 core::ByteSpan signature);

enum class BlsGroup : std::uint8_t { g1, g2 };

class BlsMlResult;

class BlsPoint {
 public:
  [[nodiscard]] BlsGroup group() const noexcept;

 private:
  struct Impl;
  BlsPoint(BlsGroup group, std::shared_ptr<const Impl> impl);
  BlsGroup group_{};
  std::shared_ptr<const Impl> impl_;

  friend core::Result<BlsPoint> bls12_381_uncompress(core::ByteSpan);
  friend core::Bytes bls12_381_compress(const BlsPoint&);
  friend core::Result<BlsPoint> bls12_381_hash_to_group(BlsGroup, core::ByteSpan, core::ByteSpan);
  friend core::Result<BlsPoint> bls12_381_add(const BlsPoint&, const BlsPoint&);
  friend BlsPoint bls12_381_neg(const BlsPoint&);
  friend core::Result<BlsPoint> bls12_381_scalar_mul(const core::BigInteger&, const BlsPoint&);
  friend bool bls12_381_equal(const BlsPoint&, const BlsPoint&) noexcept;
  friend class BlsMlResult;
  friend core::Result<BlsMlResult> bls12_381_miller_loop(const BlsPoint&, const BlsPoint&);
};

class BlsMlResult {
 public:
  BlsMlResult(const BlsMlResult&) = default;
  BlsMlResult& operator=(const BlsMlResult&) = default;

 private:
  struct Impl;
  explicit BlsMlResult(std::shared_ptr<const Impl> impl);
  std::shared_ptr<const Impl> impl_;

  friend core::Result<BlsMlResult> bls12_381_miller_loop(const BlsPoint&, const BlsPoint&);
  friend BlsMlResult bls12_381_mul_ml_result(const BlsMlResult&, const BlsMlResult&);
  friend bool bls12_381_final_verify(const BlsMlResult&, const BlsMlResult&) noexcept;
};

[[nodiscard]] core::Result<BlsPoint> bls12_381_uncompress(core::ByteSpan compressed);
[[nodiscard]] core::Bytes bls12_381_compress(const BlsPoint& point);
[[nodiscard]] core::Result<BlsPoint> bls12_381_hash_to_group(BlsGroup group, core::ByteSpan message,
                                                             core::ByteSpan domain_separation_tag);
[[nodiscard]] core::Result<BlsPoint> bls12_381_add(const BlsPoint& left, const BlsPoint& right);
[[nodiscard]] BlsPoint bls12_381_neg(const BlsPoint& point);
[[nodiscard]] core::Result<BlsPoint> bls12_381_scalar_mul(const core::BigInteger& scalar,
                                                          const BlsPoint& point);
[[nodiscard]] bool bls12_381_equal(const BlsPoint& left, const BlsPoint& right) noexcept;
[[nodiscard]] core::Result<BlsMlResult> bls12_381_miller_loop(const BlsPoint& g1,
                                                              const BlsPoint& g2);
[[nodiscard]] BlsMlResult bls12_381_mul_ml_result(const BlsMlResult& left,
                                                  const BlsMlResult& right);
[[nodiscard]] bool bls12_381_final_verify(const BlsMlResult& left,
                                          const BlsMlResult& right) noexcept;

[[nodiscard]] core::Result<std::string> emip3_encrypt_with_password(std::string_view password_hex,
                                                                    std::string_view salt_hex,
                                                                    std::string_view nonce_hex,
                                                                    std::string_view plaintext_hex);
[[nodiscard]] core::Result<std::string> emip3_decrypt_with_password(std::string_view password_hex,
                                                                    std::string_view envelope_hex);

}  // namespace cardano::crypto
