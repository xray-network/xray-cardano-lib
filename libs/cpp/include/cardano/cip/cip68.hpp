#pragma once

#include <cstddef>
#include <cstdint>

#include "cardano/chain/plutus_data.hpp"
#include "cardano/cip/cip67.hpp"

namespace cardano::cip::cip68 {

enum class TokenClass : std::uint16_t { nft = 222, ft = 333, rft = 444 };
inline constexpr std::uint16_t reference_label = 100;

struct TokenIdentity {
  crypto::ScriptHash policy;
  chain::AssetName name;
};
struct Relationship {
  TokenIdentity user;
  TokenIdentity reference;
  TokenClass token_class;
};
enum class MetadataFormat : std::uint8_t { direct, nested_721 };
struct Limits {
  std::size_t max_depth{128};
  std::size_t max_nodes{100'000};
  std::size_t max_map_entries{10'000};
  std::size_t max_byte_string_bytes{16U * 1024U * 1024U};
};

[[nodiscard]] core::Result<TokenIdentity> reference_identity(const TokenIdentity& user);
[[nodiscard]] core::Result<Relationship> validate_relationship(const TokenIdentity& user,
                                                               const TokenIdentity& reference,
                                                               std::uint64_t reference_quantity,
                                                               std::size_t candidate_count);

class Datum {
 public:
  [[nodiscard]] static core::Result<Datum> parse(chain::PlutusData data, TokenClass token_class);
  [[nodiscard]] static Datum make(
      chain::PlutusData metadata, std::uint64_t version,
      chain::PlutusData extra = chain::PlutusData::constr(core::BigInteger(std::uint64_t{0}), {}));
  [[nodiscard]] std::uint64_t version() const noexcept;
  [[nodiscard]] MetadataFormat metadata_format() const noexcept;
  [[nodiscard]] core::VoidResult validate(TokenClass token_class, const TokenIdentity& user,
                                          Limits limits = {}) const;
  [[nodiscard]] const chain::PlutusData& metadata() const noexcept;
  [[nodiscard]] const chain::PlutusData& extra() const noexcept;
  [[nodiscard]] const chain::PlutusData& to_data() const noexcept;

 private:
  Datum(chain::PlutusData data, chain::PlutusData metadata, std::uint64_t version,
        chain::PlutusData extra);
  chain::PlutusData data_;
  chain::PlutusData metadata_;
  std::uint64_t version_{};
  chain::PlutusData extra_;
};

}  // namespace cardano::cip::cip68
