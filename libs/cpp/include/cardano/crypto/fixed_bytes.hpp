#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "cardano/core/bech32.hpp"
#include "cardano/core/bytes.hpp"
#include "cardano/core/error.hpp"

namespace cardano::crypto {

template <std::size_t Size, typename Domain>
class FixedBytes {
 public:
  FixedBytes() = delete;

  [[nodiscard]] static core::Result<FixedBytes> from_bytes(core::ByteSpan bytes) {
    if (bytes.size() != Size) {
      return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                                "fixed byte value has an invalid length"));
    }
    std::array<core::Byte, Size> owned{};
    std::ranges::copy(bytes, owned.begin());
    return FixedBytes(std::move(owned));
  }

  [[nodiscard]] static core::Result<FixedBytes> from_hex(std::string_view hex) {
    auto bytes = core::hex_to_bytes(hex);
    if (!bytes) {
      return std::unexpected(bytes.error());
    }
    return from_bytes(*bytes);
  }

  [[nodiscard]] static core::Result<FixedBytes> from_bech32(std::string_view encoded) {
    auto decoded = core::decode_bech32(encoded);
    if (!decoded) {
      return std::unexpected(decoded.error());
    }
    return from_bytes(decoded->bytes);
  }

  [[nodiscard]] core::Bytes to_bytes() const { return core::Bytes(bytes_.begin(), bytes_.end()); }

  [[nodiscard]] core::ByteSpan span() const noexcept { return bytes_; }
  [[nodiscard]] std::string to_hex() const { return core::bytes_to_hex(bytes_); }

  [[nodiscard]] core::Result<std::string> to_bech32(std::string_view hrp) const {
    return core::encode_bech32(hrp, bytes_);
  }

  friend bool operator==(const FixedBytes&, const FixedBytes&) = default;
  friend auto operator<=>(const FixedBytes&, const FixedBytes&) = default;

 private:
  explicit FixedBytes(std::array<core::Byte, Size> bytes) : bytes_(std::move(bytes)) {}
  std::array<core::Byte, Size> bytes_;
};

struct Ed25519KeyHashDomain;
struct ScriptHashDomain;
struct GenesisDelegateHashDomain;
struct GenesisHashDomain;
struct TransactionHashDomain;
struct AuxiliaryDataHashDomain;
struct PoolMetadataHashDomain;
struct VRFKeyHashDomain;
struct BlockBodyHashDomain;
struct BlockHeaderHashDomain;
struct DatumHashDomain;
struct ScriptDataHashDomain;
struct VRFVkeyDomain;
struct KESVkeyDomain;
struct NonceHashDomain;
struct AnchorDocHashDomain;
struct Ed25519SignatureDomain;

using Ed25519KeyHash = FixedBytes<28, Ed25519KeyHashDomain>;
using ScriptHash = FixedBytes<28, ScriptHashDomain>;
using GenesisDelegateHash = FixedBytes<28, GenesisDelegateHashDomain>;
using GenesisHash = FixedBytes<28, GenesisHashDomain>;
using TransactionHash = FixedBytes<32, TransactionHashDomain>;
using AuxiliaryDataHash = FixedBytes<32, AuxiliaryDataHashDomain>;
using PoolMetadataHash = FixedBytes<32, PoolMetadataHashDomain>;
using VRFKeyHash = FixedBytes<32, VRFKeyHashDomain>;
using BlockBodyHash = FixedBytes<32, BlockBodyHashDomain>;
using BlockHeaderHash = FixedBytes<32, BlockHeaderHashDomain>;
using DatumHash = FixedBytes<32, DatumHashDomain>;
using ScriptDataHash = FixedBytes<32, ScriptDataHashDomain>;
using VRFVkey = FixedBytes<32, VRFVkeyDomain>;
using KESVkey = FixedBytes<32, KESVkeyDomain>;
using NonceHash = FixedBytes<32, NonceHashDomain>;
using AnchorDocHash = FixedBytes<32, AnchorDocHashDomain>;
using Ed25519Signature = FixedBytes<64, Ed25519SignatureDomain>;

}  // namespace cardano::crypto
