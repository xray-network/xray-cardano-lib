#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "cardano/core/cbor.hpp"
#include "cardano/crypto/fixed_bytes.hpp"
#include "cardano/crypto/keys.hpp"

namespace cardano::chain {

struct NetworkInfo {
  std::uint8_t network_id{};
  std::uint32_t protocol_magic{};

  friend bool operator==(const NetworkInfo&, const NetworkInfo&) = default;
};

inline constexpr NetworkInfo TESTNET_NETWORK_INFO{0U, 1'097'911'063U};
inline constexpr NetworkInfo MAINNET_NETWORK_INFO{1U, 764'824'073U};
inline constexpr NetworkInfo PREVIEW_NETWORK_INFO{0U, 2U};
inline constexpr NetworkInfo PREPROD_NETWORK_INFO{0U, 1U};
inline constexpr NetworkInfo SANCHO_NETWORK_INFO{0U, 4U};

struct Blake2b224Domain;
struct Blake2b256Domain;
using Blake2b224 = crypto::FixedBytes<28, Blake2b224Domain>;
using Blake2b256 = crypto::FixedBytes<32, Blake2b256Domain>;

struct LinearFee {
  std::uint64_t coefficient{};
  std::uint64_t constant{};
};

struct ExUnits {
  std::int64_t memory{};
  std::int64_t steps{};

  [[nodiscard]] static core::Result<ExUnits> from_json(std::string_view json);
  [[nodiscard]] std::string to_json() const;
  friend bool operator==(const ExUnits&, const ExUnits&) = default;
};

struct ExUnitPrices {
  std::uint64_t memory_numerator{};
  std::uint64_t memory_denominator{};
  std::uint64_t steps_numerator{};
  std::uint64_t steps_denominator{};

  [[nodiscard]] static core::Result<ExUnitPrices> from_json(std::string_view json);
  [[nodiscard]] std::string to_json() const;
};

enum class MetadataJsonSchema : std::uint8_t {
  no_conversions = 0,
  basic_conversions = 1,
  detailed = 2
};

enum class CardanoNodePlutusDatumSchema : std::uint8_t { basic = 0, detailed = 1 };

[[nodiscard]] core::Result<Blake2b256> hash_cbor_value(const core::cbor::Value& value);
[[nodiscard]] core::Result<crypto::TransactionHash> hash_transaction(const core::cbor::Value& body);
[[nodiscard]] core::Result<crypto::AuxiliaryDataHash> hash_auxiliary_data(
    const core::cbor::Value& auxiliary_data);
[[nodiscard]] core::Result<crypto::DatumHash> hash_plutus_data(const core::cbor::Value& data);
[[nodiscard]] crypto::ScriptHash hash_script(std::uint8_t namespace_byte, core::ByteSpan script);

[[nodiscard]] core::Result<std::uint64_t> min_no_script_fee(const core::cbor::Value& transaction,
                                                            LinearFee fee);
[[nodiscard]] core::Result<ExUnits> compute_total_ex_units(const core::cbor::Value& redeemers);
[[nodiscard]] core::Result<std::uint64_t> min_script_fee(ExUnits total, ExUnitPrices prices);
[[nodiscard]] core::Result<std::uint64_t> min_reference_script_fee(std::uint64_t script_size,
                                                                   std::uint64_t cost_per_byte);
[[nodiscard]] core::Result<std::uint64_t> min_fee(const core::cbor::Value& transaction,
                                                  LinearFee fee, std::uint64_t script_fee,
                                                  std::uint64_t reference_script_fee);
[[nodiscard]] core::Result<std::uint64_t> min_ada_required(const core::cbor::Value& output,
                                                           std::uint64_t coins_per_utxo_byte);

[[nodiscard]] core::cbor::Value encode_arbitrary_bytes_as_metadatum(core::ByteSpan bytes);
[[nodiscard]] std::optional<core::Bytes> decode_arbitrary_bytes_from_metadatum(
    const core::cbor::Value& metadatum);
[[nodiscard]] core::Result<std::optional<crypto::ScriptDataHash>> calc_script_data_hash(
    const std::optional<core::cbor::Value>& redeemers,
    const std::optional<core::cbor::Value>& datums, const core::cbor::Value& language_views);
[[nodiscard]] core::Result<crypto::ScriptDataHash> hash_script_data(
    const std::optional<core::cbor::Value>& redeemers,
    const std::optional<core::cbor::Value>& datums, const core::cbor::Value& language_views);
[[nodiscard]] core::Result<std::optional<crypto::ScriptDataHash>>
calc_script_data_hash_from_witness(const core::cbor::Value& witness_set,
                                   const core::cbor::Value& language_views);
[[nodiscard]] core::Result<core::cbor::Value> make_vkey_witness(
    const crypto::TransactionHash& body_hash, const crypto::PrivateKey& private_key);
[[nodiscard]] crypto::TransactionHash genesis_txid_shelley(core::ByteSpan address_bytes);
[[nodiscard]] core::Result<std::uint64_t> get_deposit(const core::cbor::Value& transaction_body,
                                                      std::uint64_t key_deposit,
                                                      std::uint64_t pool_deposit);
[[nodiscard]] core::Result<std::uint64_t> get_implicit_input(
    const core::cbor::Value& transaction_body, std::uint64_t key_deposit,
    std::uint64_t pool_deposit);

[[nodiscard]] core::Result<std::string> cbor_value_to_json(const core::cbor::Value& value,
                                                           bool conway_map_shape = false);
[[nodiscard]] core::Result<core::cbor::Value> cbor_value_from_json(std::string_view json);
[[nodiscard]] core::Result<std::string> decode_metadatum_to_json_str(
    const core::cbor::Value& metadatum, MetadataJsonSchema schema);
[[nodiscard]] core::Result<core::cbor::Value> encode_json_str_to_metadatum(
    std::string_view json, MetadataJsonSchema schema);
[[nodiscard]] core::Result<std::string> decode_plutus_datum_to_json_str(
    const core::cbor::Value& datum, CardanoNodePlutusDatumSchema schema);
[[nodiscard]] core::Result<core::cbor::Value> encode_json_str_to_plutus_datum(
    std::string_view json, CardanoNodePlutusDatumSchema schema);

}  // namespace cardano::chain
