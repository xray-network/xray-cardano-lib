#include "cardano/chain/ledger.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

#include "cardano/chain/plutus_data.hpp"
#include "cardano/crypto/primitives.hpp"

namespace cardano::chain {
namespace {

[[nodiscard]] core::CardanoError structure_error(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::Result<std::uint64_t> to_uint(const core::cbor::Value& value,
                                                  std::string_view description) {
  const auto* integer = value.as_unsigned();
  if (integer == nullptr) {
    return std::unexpected(structure_error(std::string(description) + " must be unsigned"));
  }
  return integer->value.to_uint64();
}

[[nodiscard]] core::Result<std::int64_t> to_nonnegative_int64(const core::cbor::Value& value,
                                                              std::string_view description) {
  auto converted = to_uint(value, description);
  if (!converted ||
      *converted > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::unexpected(converted
                               ? core::CardanoError(core::ErrorCode::out_of_range,
                                                    std::string(description) + " exceeds int64")
                               : converted.error());
  }
  return static_cast<std::int64_t>(*converted);
}

[[nodiscard]] core::Result<ExUnits> decode_ex_units(const core::cbor::Value& value) {
  const auto* pair = value.as_array();
  if (pair == nullptr || pair->values.size() != 2U) {
    return std::unexpected(structure_error("ExUnits must be [memory, steps]"));
  }
  auto memory = to_nonnegative_int64(pair->values[0], "ExUnits memory");
  auto steps = to_nonnegative_int64(pair->values[1], "ExUnits steps");
  if (!memory || !steps) {
    return std::unexpected(!memory ? memory.error() : steps.error());
  }
  return ExUnits{*memory, *steps};
}

[[nodiscard]] core::Result<std::uint64_t> checked_u64(const core::BigInteger& value,
                                                      std::string_view description) {
  auto converted = value.to_uint64();
  return converted
             ? converted
             : std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                                  std::string(description) + " exceeds uint64"));
}

[[nodiscard]] std::size_t uint_head_size(std::uint64_t value) noexcept {
  if (value < 24U) {
    return 1U;
  }
  if (value <= std::numeric_limits<std::uint8_t>::max()) {
    return 2U;
  }
  if (value <= std::numeric_limits<std::uint16_t>::max()) {
    return 3U;
  }
  if (value <= std::numeric_limits<std::uint32_t>::max()) {
    return 5U;
  }
  return 9U;
}

[[nodiscard]] const core::cbor::Value* map_unsigned_key(const core::cbor::Value& value,
                                                        std::uint64_t wanted) {
  const auto* map = value.as_map();
  if (map == nullptr) {
    return nullptr;
  }
  for (const auto& [key, item] : map->entries) {
    const auto* integer = key.as_unsigned();
    if (integer == nullptr) {
      continue;
    }
    const auto converted = integer->value.to_uint64();
    if (converted && *converted == wanted) {
      return &item;
    }
  }
  return nullptr;
}

[[nodiscard]] core::Result<std::uint64_t> output_coin(const core::cbor::Value& output) {
  const core::cbor::Value* amount = nullptr;
  if (const auto* array = output.as_array(); array != nullptr && array->values.size() >= 2U) {
    amount = &array->values[1];
  } else {
    amount = map_unsigned_key(output, 1U);
  }
  if (amount == nullptr) {
    return std::unexpected(structure_error("transaction output has no amount"));
  }
  if (const auto* value = amount->as_array(); value != nullptr && !value->values.empty()) {
    amount = &value->values[0];
  }
  return to_uint(*amount, "transaction output coin");
}

}  // namespace

core::Result<Blake2b256> hash_cbor_value(const core::cbor::Value& value) {
  auto encoded =
      core::cbor::encode_cbor(value, core::cbor::EncodeOptions{.mode = core::cbor::Mode::preserve});
  if (!encoded) {
    return std::unexpected(encoded.error());
  }
  return Blake2b256::from_bytes(crypto::blake2b256(*encoded));
}

core::Result<crypto::TransactionHash> hash_transaction(const core::cbor::Value& body) {
  auto hash = hash_cbor_value(body);
  return hash ? crypto::TransactionHash::from_bytes(hash->span()) : std::unexpected(hash.error());
}

core::Result<crypto::AuxiliaryDataHash> hash_auxiliary_data(
    const core::cbor::Value& auxiliary_data) {
  auto hash = hash_cbor_value(auxiliary_data);
  return hash ? crypto::AuxiliaryDataHash::from_bytes(hash->span()) : std::unexpected(hash.error());
}

core::Result<crypto::DatumHash> hash_plutus_data(const core::cbor::Value& data) {
  auto hash = hash_cbor_value(data);
  return hash ? crypto::DatumHash::from_bytes(hash->span()) : std::unexpected(hash.error());
}

crypto::ScriptHash hash_script(std::uint8_t namespace_byte, core::ByteSpan script) {
  core::Bytes domain{static_cast<core::Byte>(namespace_byte)};
  domain.insert(domain.end(), script.begin(), script.end());
  return *crypto::ScriptHash::from_bytes(crypto::blake2b224(domain));
}

core::Result<std::uint64_t> min_no_script_fee(const core::cbor::Value& transaction, LinearFee fee) {
  auto encoded = core::cbor::encode_cbor(
      transaction, core::cbor::EncodeOptions{.mode = core::cbor::Mode::preserve});
  if (!encoded) {
    return std::unexpected(encoded.error());
  }
  const auto result = core::BigInteger(static_cast<std::uint64_t>(encoded->size())) *
                          core::BigInteger(fee.coefficient) +
                      core::BigInteger(fee.constant);
  return checked_u64(result, "minimum transaction fee");
}

core::Result<ExUnits> compute_total_ex_units(const core::cbor::Value& redeemers) {
  ExUnits total{};
  const auto append = [&](const core::cbor::Value& value) -> core::VoidResult {
    auto decoded = decode_ex_units(value);
    if (!decoded) {
      return std::unexpected(decoded.error());
    }
    if (decoded->memory > std::numeric_limits<std::int64_t>::max() - total.memory ||
        decoded->steps > std::numeric_limits<std::int64_t>::max() - total.steps) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::out_of_range, "total ExUnits overflow int64"));
    }
    total.memory += decoded->memory;
    total.steps += decoded->steps;
    return std::monostate{};
  };

  if (const auto* array = redeemers.as_array()) {
    for (std::size_t index = 0; index < array->values.size(); ++index) {
      const auto* redeemer = array->values[index].as_array();
      if (redeemer == nullptr || redeemer->values.size() != 4U) {
        return std::unexpected(structure_error("legacy redeemer must contain four fields"));
      }
      auto status = append(redeemer->values[3]);
      if (!status) {
        return std::unexpected(status.error().at(index));
      }
    }
    return total;
  }
  if (const auto* map = redeemers.as_map()) {
    for (std::size_t index = 0; index < map->entries.size(); ++index) {
      const auto* value = map->entries[index].second.as_array();
      if (value == nullptr || value->values.size() != 2U) {
        return std::unexpected(structure_error("map redeemer value must be [data, ExUnits]"));
      }
      auto status = append(value->values[1]);
      if (!status) {
        return std::unexpected(status.error().at(index));
      }
    }
    return total;
  }
  return std::unexpected(structure_error("redeemers must be an array or map"));
}

core::Result<std::uint64_t> min_script_fee(ExUnits total, ExUnitPrices prices) {
  if (total.memory < 0 || total.steps < 0 || prices.memory_denominator == 0U ||
      prices.steps_denominator == 0U) {
    return std::unexpected(core::CardanoError(
        core::ErrorCode::invalid_argument,
        "script fee inputs require nonnegative units and positive denominators"));
  }
  const auto common_denominator =
      core::BigInteger(prices.memory_denominator) * core::BigInteger(prices.steps_denominator);
  const auto numerator =
      core::BigInteger(static_cast<std::uint64_t>(total.memory)) *
          core::BigInteger(prices.memory_numerator) * core::BigInteger(prices.steps_denominator) +
      core::BigInteger(static_cast<std::uint64_t>(total.steps)) *
          core::BigInteger(prices.steps_numerator) * core::BigInteger(prices.memory_denominator);
  const auto rounded =
      (numerator + common_denominator - core::BigInteger(std::uint64_t{1})) / common_denominator;
  return checked_u64(rounded, "minimum script fee");
}

core::Result<std::uint64_t> min_reference_script_fee(std::uint64_t script_size,
                                                     std::uint64_t cost_per_byte) {
  constexpr std::uint64_t tier_size = 25'600U;
  auto remaining = script_size;
  auto price_numerator = core::BigInteger(cost_per_byte);
  auto price_denominator = core::BigInteger(std::uint64_t{1});
  core::BigInteger total_numerator(std::uint64_t{0});
  core::BigInteger total_denominator(std::uint64_t{1});
  while (remaining != 0U) {
    const auto bytes = std::min(remaining, tier_size);
    total_numerator = total_numerator * price_denominator +
                      core::BigInteger(bytes) * price_numerator * total_denominator;
    total_denominator *= price_denominator;
    const auto floor = total_numerator / total_denominator;
    if (!floor.fits_uint64()) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::out_of_range, "reference-script fee exceeds uint64"));
    }
    remaining -= bytes;
    price_numerator *= core::BigInteger(std::uint64_t{6});
    price_denominator *= core::BigInteger(std::uint64_t{5});
  }
  return checked_u64(total_numerator / total_denominator, "reference-script fee");
}

core::Result<std::uint64_t> min_fee(const core::cbor::Value& transaction, LinearFee fee,
                                    std::uint64_t script_fee, std::uint64_t reference_script_fee) {
  auto base = min_no_script_fee(transaction, fee);
  if (!base) {
    return std::unexpected(base.error());
  }
  const auto total = core::BigInteger(*base) + core::BigInteger(script_fee) +
                     core::BigInteger(reference_script_fee);
  return checked_u64(total, "total transaction fee");
}

core::Result<std::uint64_t> min_ada_required(const core::cbor::Value& output,
                                             std::uint64_t coins_per_utxo_byte) {
  auto current_coin = output_coin(output);
  if (!current_coin) {
    return std::unexpected(current_coin.error());
  }
  auto encoded = core::cbor::encode_cbor(
      output, core::cbor::EncodeOptions{.mode = core::cbor::Mode::preserve});
  if (!encoded) {
    return std::unexpected(encoded.error());
  }
  const auto old_size = uint_head_size(*current_coin);
  auto latest_size = old_size;
  for (;;) {
    const auto adjusted_size = core::BigInteger(static_cast<std::uint64_t>(encoded->size())) +
                               core::BigInteger(std::uint64_t{160}) +
                               core::BigInteger(static_cast<std::uint64_t>(latest_size)) -
                               core::BigInteger(static_cast<std::uint64_t>(old_size));
    auto tentative =
        checked_u64(adjusted_size * core::BigInteger(coins_per_utxo_byte), "minimum ADA");
    if (!tentative) {
      return std::unexpected(tentative.error());
    }
    const auto next_size = uint_head_size(*tentative);
    if (next_size == latest_size) {
      return *tentative;
    }
    latest_size = next_size;
  }
}

core::cbor::Value encode_arbitrary_bytes_as_metadatum(core::ByteSpan bytes) {
  std::vector<core::cbor::Value> chunks;
  chunks.reserve((bytes.size() + 63U) / 64U);
  for (std::size_t offset = 0; offset < bytes.size(); offset += 64U) {
    const auto length = std::min<std::size_t>(64U, bytes.size() - offset);
    chunks.push_back(core::cbor::Value::byte_string(
        core::Bytes(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                    bytes.begin() + static_cast<std::ptrdiff_t>(offset + length))));
  }
  return core::cbor::Value::array(std::move(chunks));
}

std::optional<core::Bytes> decode_arbitrary_bytes_from_metadatum(
    const core::cbor::Value& metadatum) {
  const auto* chunks = metadatum.as_array();
  if (chunks == nullptr) {
    return std::nullopt;
  }
  core::Bytes output;
  for (const auto& chunk : chunks->values) {
    const auto* bytes = chunk.as_byte_string();
    if (bytes == nullptr) {
      return std::nullopt;
    }
    output.insert(output.end(), bytes->value.begin(), bytes->value.end());
  }
  return output;
}

core::Result<crypto::ScriptDataHash> hash_script_data(
    const std::optional<core::cbor::Value>& redeemers,
    const std::optional<core::cbor::Value>& datums, const core::cbor::Value& language_views) {
  core::Bytes payload;
  const auto append = [&](const core::cbor::Value& value) -> core::VoidResult {
    auto bytes = core::cbor::encode_cbor(
        value, core::cbor::EncodeOptions{.mode = core::cbor::Mode::preserve});
    if (!bytes) {
      return std::unexpected(bytes.error());
    }
    payload.insert(payload.end(), bytes->begin(), bytes->end());
    return std::monostate{};
  };
  const auto tagged_datums = [&]() -> core::Result<std::optional<core::cbor::Value>> {
    if (!datums) {
      return std::optional<core::cbor::Value>{};
    }
    const core::cbor::ArrayValue* array = datums->as_array();
    if (array == nullptr) {
      const auto* tag = datums->as_tag();
      if (tag == nullptr || tag->value == nullptr ||
          tag->tag != core::BigInteger(std::uint64_t{258}) || tag->value->as_array() == nullptr) {
        return std::unexpected(
            structure_error("script-data datums must be an array or tag-258 array"));
      }
      array = tag->value->as_array();
    }
    return std::optional<core::cbor::Value>(core::cbor::Value::tag(
        core::BigInteger(std::uint64_t{258}), core::cbor::Value::array(array->values),
        core::cbor::HeadWidth::two));
  }();
  if (!tagged_datums) {
    return std::unexpected(tagged_datums.error());
  }
  if (!redeemers && *tagged_datums) {
    payload.push_back(core::Byte{0xa0});
    auto status = append(**tagged_datums);
    if (!status) {
      return std::unexpected(status.error());
    }
    payload.push_back(core::Byte{0xa0});
  } else {
    if (redeemers) {
      auto status = append(*redeemers);
      if (!status) {
        return std::unexpected(status.error());
      }
    } else {
      payload.push_back(core::Byte{0x80});
    }
    if (*tagged_datums) {
      auto status = append(**tagged_datums);
      if (!status) {
        return std::unexpected(status.error());
      }
    }
    auto status = append(language_views);
    if (!status) {
      return std::unexpected(status.error());
    }
  }
  auto hash = crypto::ScriptDataHash::from_bytes(crypto::blake2b256(payload));
  return hash ? core::Result<crypto::ScriptDataHash>(std::move(*hash))
              : std::unexpected(hash.error());
}

core::Result<std::optional<crypto::ScriptDataHash>> calc_script_data_hash(
    const std::optional<core::cbor::Value>& redeemers,
    const std::optional<core::cbor::Value>& datums, const core::cbor::Value& language_views) {
  if (!redeemers && !datums) {
    return std::optional<crypto::ScriptDataHash>{};
  }
  auto hash = hash_script_data(redeemers, datums, language_views);
  return hash ? core::Result<std::optional<crypto::ScriptDataHash>>(
                    std::optional<crypto::ScriptDataHash>(std::move(*hash)))
              : std::unexpected(hash.error());
}

core::Result<std::optional<crypto::ScriptDataHash>> calc_script_data_hash_from_witness(
    const core::cbor::Value& witness_set, const core::cbor::Value& language_views) {
  if (witness_set.as_map() == nullptr) {
    return std::unexpected(structure_error("transaction witness set must be a map"));
  }
  const auto* redeemers = map_unsigned_key(witness_set, 5U);
  const auto* datums = map_unsigned_key(witness_set, 4U);
  if (redeemers == nullptr || datums == nullptr) {
    return std::optional<crypto::ScriptDataHash>{};
  }
  return calc_script_data_hash(*redeemers, *datums, language_views);
}

core::Result<core::cbor::Value> make_vkey_witness(const crypto::TransactionHash& body_hash,
                                                  const crypto::PrivateKey& private_key) {
  auto public_key = private_key.public_key();
  auto signature = private_key.sign(body_hash.span());
  if (!public_key || !signature) {
    return std::unexpected(!public_key ? public_key.error() : signature.error());
  }
  std::vector<core::cbor::Value> fields;
  fields.push_back(core::cbor::Value::byte_string(public_key->to_bytes()));
  fields.push_back(core::cbor::Value::byte_string(signature->to_bytes()));
  return core::cbor::Value::array(std::move(fields));
}

crypto::TransactionHash genesis_txid_shelley(core::ByteSpan address_bytes) {
  return *crypto::TransactionHash::from_bytes(crypto::blake2b256(address_bytes));
}

namespace {

using Json = nlohmann::json;

[[nodiscard]] const core::cbor::ArrayValue* set_array(const core::cbor::Value* value) {
  if (value == nullptr) return nullptr;
  if (const auto* tag = value->as_tag();
      tag != nullptr && tag->tag == core::BigInteger(std::uint64_t{258})) {
    value = tag->value.get();
  }
  return value->as_array();
}

[[nodiscard]] core::Result<std::uint64_t> checked_add_coin(std::uint64_t current,
                                                           std::uint64_t added,
                                                           std::string_view name) {
  if (added > std::numeric_limits<std::uint64_t>::max() - current) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::out_of_range, std::string(name) + " overflow"));
  }
  return current + added;
}

[[nodiscard]] core::Result<std::uint64_t> sum_certificate_coins(const core::cbor::Value& body,
                                                                std::uint64_t key_deposit,
                                                                std::uint64_t pool_deposit,
                                                                bool refunds) {
  const auto* certificates = set_array(map_unsigned_key(body, 4));
  std::uint64_t total = 0;
  if (certificates == nullptr) return total;
  for (std::size_t index = 0; index < certificates->values.size(); ++index) {
    const auto* certificate = certificates->values[index].as_array();
    if (certificate == nullptr || certificate->values.empty()) {
      return std::unexpected(structure_error("certificate must be a nonempty array").at(index));
    }
    auto tag = to_uint(certificate->values[0], "certificate tag");
    if (!tag) return std::unexpected(tag.error().at(index));
    std::optional<std::uint64_t> coin;
    if (!refunds) {
      if (*tag == 0) coin = key_deposit;
      if (*tag == 3) coin = pool_deposit;
      const std::map<std::uint64_t, std::size_t> positions{
          {7, 2}, {11, 3}, {12, 3}, {13, 4}, {16, 2}};
      if (const auto position = positions.find(*tag); position != positions.end()) {
        if (certificate->values.size() <= position->second) {
          return std::unexpected(
              structure_error("certificate explicit deposit field is missing").at(index));
        }
        auto parsed =
            to_uint(certificate->values[position->second], "certificate explicit deposit");
        if (!parsed) return std::unexpected(parsed.error().at(index));
        coin = *parsed;
      }
    } else {
      if (*tag == 1 || *tag == 15) coin = key_deposit;
      if (*tag == 4) coin = pool_deposit;
      const std::map<std::uint64_t, std::size_t> positions{{8, 2}, {17, 2}};
      if (const auto position = positions.find(*tag); position != positions.end()) {
        if (certificate->values.size() <= position->second) {
          return std::unexpected(
              structure_error("certificate explicit refund field is missing").at(index));
        }
        auto parsed = to_uint(certificate->values[position->second], "certificate explicit refund");
        if (!parsed) return std::unexpected(parsed.error().at(index));
        coin = *parsed;
      }
    }
    if (coin) {
      auto next = checked_add_coin(total, *coin, "certificate coin total");
      if (!next) return std::unexpected(next.error().at(index));
      total = *next;
    }
  }
  return total;
}

struct JsonBudget {
  std::size_t visited{};
  static constexpr std::size_t max_depth = 128;
  static constexpr std::size_t max_visited = 100'000;

  core::VoidResult enter(std::size_t depth) {
    if (depth > max_depth) {
      return std::unexpected(core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                                                "JSON conversion depth exceeds 128"));
    }
    if (++visited > max_visited) {
      return std::unexpected(core::CardanoError(core::ErrorCode::resource_limit_exceeded,
                                                "JSON conversion exceeds 100000 values"));
    }
    return std::monostate{};
  }
};

[[nodiscard]] Json integer_json(const core::BigInteger& value) {
  constexpr std::int64_t max_safe = 9'007'199'254'740'991LL;
  if (value.fits_int64()) {
    const auto integer = value.to_int64().value();
    if (integer >= -max_safe && integer <= max_safe) return Json(integer);
  }
  return Json(value.to_decimal());
}

[[nodiscard]] core::Result<Json> generic_to_json(const core::cbor::Value& value, bool conway,
                                                 JsonBudget& budget, std::size_t depth) {
  auto entered = budget.enter(depth);
  if (!entered) return std::unexpected(entered.error());
  if (const auto* integer = value.as_unsigned()) return integer_json(integer->value);
  if (const auto* integer = value.as_negative()) return integer_json(integer->value);
  if (const auto* bytes = value.as_byte_string()) {
    return Json(core::bytes_to_hex(bytes->value));
  }
  if (const auto* text = value.as_text_string()) return Json(text->value);
  if (const auto* array = value.as_array()) {
    Json output = Json::array();
    for (const auto& item : array->values) {
      auto converted = generic_to_json(item, conway, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      output.push_back(std::move(*converted));
    }
    return output;
  }
  if (const auto* map = value.as_map()) {
    Json output = Json::array();
    for (const auto& [key, item] : map->entries) {
      auto converted_key = generic_to_json(key, conway, budget, depth + 1);
      auto converted_value = generic_to_json(item, conway, budget, depth + 1);
      if (!converted_key) return std::unexpected(converted_key.error());
      if (!converted_value) return std::unexpected(converted_value.error());
      output.push_back(
          conway ? Json{{"k", std::move(*converted_key)}, {"v", std::move(*converted_value)}}
                 : Json::array({std::move(*converted_key), std::move(*converted_value)}));
    }
    return output;
  }
  if (const auto* tag = value.as_tag()) {
    auto converted = generic_to_json(*tag->value, conway, budget, depth + 1);
    if (!converted) return std::unexpected(converted.error());
    return Json{{"tag", integer_json(tag->tag)}, {"value", std::move(*converted)}};
  }
  if (const auto* boolean = std::get_if<core::cbor::BooleanValue>(&value.node())) {
    return Json(boolean->value);
  }
  if (std::holds_alternative<core::cbor::NullValue>(value.node())) return Json(nullptr);
  if (std::holds_alternative<core::cbor::UndefinedValue>(value.node())) return Json(nullptr);
  if (const auto* simple = std::get_if<core::cbor::SimpleValue>(&value.node())) {
    return Json(simple->value);
  }
  if (const auto* floating = std::get_if<core::cbor::FloatingValue>(&value.node())) {
    return Json(floating->value);
  }
  return std::unexpected(structure_error("unsupported CBOR JSON value"));
}

[[nodiscard]] core::Result<core::cbor::Value> generic_from_json(const Json& value,
                                                                JsonBudget& budget,
                                                                std::size_t depth) {
  auto entered = budget.enter(depth);
  if (!entered) return std::unexpected(entered.error());
  if (value.is_null()) return core::cbor::Value::null();
  if (value.is_boolean()) return core::cbor::Value::boolean(value.get<bool>());
  if (value.is_number_integer()) {
    const auto integer = value.get<std::int64_t>();
    constexpr std::int64_t max_safe = 9'007'199'254'740'991LL;
    if (integer < -max_safe || integer > max_safe) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::out_of_range, "JSON integer exceeds safe range"));
    }
    return integer >= 0 ? core::cbor::Value::unsigned_integer(core::BigInteger(integer))
                        : core::cbor::Value::negative_integer(core::BigInteger(integer));
  }
  if (value.is_number_unsigned()) {
    const auto integer = value.get<std::uint64_t>();
    if (integer > 9'007'199'254'740'991ULL) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::out_of_range, "JSON integer exceeds safe range"));
    }
    return core::cbor::Value::unsigned_integer(core::BigInteger(integer));
  }
  if (value.is_number_float()) {
    const auto number = value.get<double>();
    if (std::trunc(number) != number || number < -9'007'199'254'740'991.0 ||
        number > 9'007'199'254'740'991.0) {
      return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                                "generic JSON number must be a safe integer"));
    }
    const auto integer = static_cast<std::int64_t>(number);
    return integer >= 0 ? core::cbor::Value::unsigned_integer(core::BigInteger(integer))
                        : core::cbor::Value::negative_integer(core::BigInteger(integer));
  }
  if (value.is_string()) {
    return core::cbor::Value::text_string(value.get<std::string>());
  }
  if (value.is_array()) {
    std::vector<core::cbor::Value> values;
    values.reserve(value.size());
    for (const auto& item : value) {
      auto converted = generic_from_json(item, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      values.push_back(std::move(*converted));
    }
    return core::cbor::Value::array(std::move(values));
  }
  if (value.is_object()) {
    std::vector<std::pair<core::cbor::Value, core::cbor::Value>> entries;
    for (const auto& [key, item] : value.items()) {
      auto converted = generic_from_json(item, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      entries.emplace_back(core::cbor::Value::text_string(key), std::move(*converted));
    }
    return core::cbor::Value::map(std::move(entries));
  }
  return std::unexpected(structure_error("unsupported JSON value"));
}

[[nodiscard]] core::Result<Json> metadata_to_json(const core::cbor::Value& value,
                                                  MetadataJsonSchema schema, JsonBudget& budget,
                                                  std::size_t depth) {
  auto entered = budget.enter(depth);
  if (!entered) return std::unexpected(entered.error());
  const bool detailed = schema == MetadataJsonSchema::detailed;
  if (const auto* integer = value.as_unsigned()) {
    return detailed ? Json{{"int", integer_json(integer->value)}} : integer_json(integer->value);
  }
  if (const auto* integer = value.as_negative()) {
    return detailed ? Json{{"int", integer_json(integer->value)}} : integer_json(integer->value);
  }
  if (const auto* text = value.as_text_string()) {
    return detailed ? Json{{"string", text->value}} : Json(text->value);
  }
  if (const auto* bytes = value.as_byte_string()) {
    if (schema == MetadataJsonSchema::no_conversions) {
      return std::unexpected(structure_error("NoConversions metadata cannot represent bytes"));
    }
    const auto hex = core::bytes_to_hex(bytes->value);
    return detailed ? Json{{"bytes", hex}} : Json("0x" + hex);
  }
  if (const auto* array = value.as_array()) {
    Json values = Json::array();
    for (const auto& item : array->values) {
      auto converted = metadata_to_json(item, schema, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      values.push_back(std::move(*converted));
    }
    return detailed ? Json{{"list", std::move(values)}} : values;
  }
  if (const auto* map = value.as_map()) {
    if (detailed) {
      Json entries = Json::array();
      for (const auto& [key, item] : map->entries) {
        auto converted_key = metadata_to_json(key, schema, budget, depth + 1);
        auto converted_value = metadata_to_json(item, schema, budget, depth + 1);
        if (!converted_key) return std::unexpected(converted_key.error());
        if (!converted_value) return std::unexpected(converted_value.error());
        entries.push_back({
            {"k", std::move(*converted_key)},
            {"v", std::move(*converted_value)},
        });
      }
      return Json{{"map", std::move(entries)}};
    }
    Json output = Json::object();
    for (const auto& [key, item] : map->entries) {
      std::string json_key;
      if (const auto* text = key.as_text_string()) {
        json_key = text->value;
      } else if (schema == MetadataJsonSchema::basic_conversions) {
        if (const auto* integer = key.as_unsigned())
          json_key = integer->value.to_decimal();
        else if (const auto* integer = key.as_negative())
          json_key = integer->value.to_decimal();
        else if (const auto* bytes = key.as_byte_string())
          json_key = "0x" + core::bytes_to_hex(bytes->value);
        else
          return std::unexpected(structure_error("metadata map key is unsupported"));
      } else {
        return std::unexpected(structure_error("metadata map key must be text"));
      }
      auto converted = metadata_to_json(item, schema, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      output[json_key] = std::move(*converted);
    }
    return output;
  }
  return std::unexpected(structure_error("value is not transaction metadata"));
}

[[nodiscard]] core::Result<core::BigInteger> json_metadata_integer(const Json& value) {
  if (value.is_number_integer()) return core::BigInteger(value.get<std::int64_t>());
  if (value.is_number_unsigned()) return core::BigInteger(value.get<std::uint64_t>());
  if (value.is_string()) return core::BigInteger::from_decimal(value.get<std::string>());
  return std::unexpected(structure_error("metadata integer is invalid"));
}

[[nodiscard]] core::Result<core::cbor::Value> metadata_from_json(const Json& value,
                                                                 MetadataJsonSchema schema,
                                                                 JsonBudget& budget,
                                                                 std::size_t depth) {
  auto entered = budget.enter(depth);
  if (!entered) return std::unexpected(entered.error());
  if (schema == MetadataJsonSchema::detailed) {
    if (!value.is_object() || value.size() != 1) {
      return std::unexpected(structure_error("detailed metadata requires one tagged member"));
    }
    const auto iterator = value.begin();
    const std::string tag = iterator.key();
    const Json& inner = iterator.value();
    if (tag == "int") {
      auto integer = json_metadata_integer(inner);
      if (!integer) return std::unexpected(integer.error());
      return integer->is_negative() ? core::cbor::Value::negative_integer(std::move(*integer))
                                    : core::cbor::Value::unsigned_integer(std::move(*integer));
    }
    if (tag == "string" && inner.is_string()) {
      return core::cbor::Value::text_string(inner.get<std::string>());
    }
    if (tag == "bytes" && inner.is_string()) {
      auto bytes = core::hex_to_bytes(inner.get<std::string>());
      if (!bytes) return std::unexpected(bytes.error());
      return core::cbor::Value::byte_string(std::move(*bytes));
    }
    if (tag == "list" && inner.is_array()) {
      std::vector<core::cbor::Value> values;
      for (const auto& item : inner) {
        auto converted = metadata_from_json(item, schema, budget, depth + 1);
        if (!converted) return std::unexpected(converted.error());
        values.push_back(std::move(*converted));
      }
      return core::cbor::Value::array(std::move(values));
    }
    if (tag == "map" && inner.is_array()) {
      std::vector<std::pair<core::cbor::Value, core::cbor::Value>> entries;
      for (const auto& item : inner) {
        if (!item.is_object() || !item.contains("k") || !item.contains("v")) {
          return std::unexpected(structure_error("metadata map entry is invalid"));
        }
        auto key = metadata_from_json(item.at("k"), schema, budget, depth + 1);
        auto converted = metadata_from_json(item.at("v"), schema, budget, depth + 1);
        if (!key) return std::unexpected(key.error());
        if (!converted) return std::unexpected(converted.error());
        entries.emplace_back(std::move(*key), std::move(*converted));
      }
      return core::cbor::Value::map(std::move(entries));
    }
    return std::unexpected(structure_error("unknown detailed metadata tag"));
  }
  if (value.is_number_integer() || value.is_number_unsigned()) {
    auto integer = json_metadata_integer(value);
    if (!integer) return std::unexpected(integer.error());
    if (!integer->fits_int64() || integer->to_int64().value() < -9'007'199'254'740'991LL ||
        integer->to_int64().value() > 9'007'199'254'740'991LL) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::out_of_range, "metadata JSON integer is not safe"));
    }
    return integer->is_negative() ? core::cbor::Value::negative_integer(std::move(*integer))
                                  : core::cbor::Value::unsigned_integer(std::move(*integer));
  }
  if (value.is_string()) {
    const auto text = value.get<std::string>();
    if (schema == MetadataJsonSchema::basic_conversions && text.size() >= 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X')) {
      auto bytes = core::hex_to_bytes(std::string_view(text).substr(2));
      if (bytes) return core::cbor::Value::byte_string(std::move(*bytes));
    }
    return core::cbor::Value::text_string(text);
  }
  if (value.is_array()) {
    std::vector<core::cbor::Value> values;
    for (const auto& item : value) {
      auto converted = metadata_from_json(item, schema, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      values.push_back(std::move(*converted));
    }
    return core::cbor::Value::array(std::move(values));
  }
  if (value.is_object()) {
    std::vector<std::pair<core::cbor::Value, core::cbor::Value>> entries;
    std::vector<std::string> keys;
    for (const auto& [key, item] : value.items()) {
      static_cast<void>(item);
      keys.push_back(key);
    }
    std::ranges::sort(keys);
    for (const auto& key : keys) {
      core::cbor::Value converted_key = core::cbor::Value::text_string(key);
      if (schema == MetadataJsonSchema::basic_conversions) {
        if (key.size() >= 2 && key[0] == '0' && (key[1] == 'x' || key[1] == 'X')) {
          auto bytes = core::hex_to_bytes(std::string_view(key).substr(2));
          if (bytes) converted_key = core::cbor::Value::byte_string(std::move(*bytes));
        } else {
          bool decimal = !key.empty();
          std::size_t offset = key.starts_with('-') ? 1 : 0;
          decimal = decimal && offset < key.size();
          for (; offset < key.size(); ++offset) {
            decimal = decimal && key[offset] >= '0' && key[offset] <= '9';
          }
          if (decimal) {
            auto integer = core::BigInteger::from_decimal(key);
            if (!integer) return std::unexpected(integer.error());
            converted_key = integer->is_negative()
                                ? core::cbor::Value::negative_integer(std::move(*integer))
                                : core::cbor::Value::unsigned_integer(std::move(*integer));
          }
        }
      }
      auto converted = metadata_from_json(value.at(key), schema, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      entries.emplace_back(std::move(converted_key), std::move(*converted));
    }
    return core::cbor::Value::map(std::move(entries));
  }
  return std::unexpected(
      structure_error("metadata JSON cannot contain null, boolean, or non-integral number"));
}

[[nodiscard]] core::Result<Json> plutus_to_json(const PlutusData& data,
                                                CardanoNodePlutusDatumSchema schema,
                                                JsonBudget& budget, std::size_t depth) {
  auto entered = budget.enter(depth);
  if (!entered) return std::unexpected(entered.error());
  if (const auto* integer = std::get_if<core::BigInteger>(&data.node())) {
    if (schema == CardanoNodePlutusDatumSchema::basic) return integer_json(*integer);
    return Json{{"int", integer_json(*integer)}};
  }
  if (const auto* bytes = std::get_if<core::Bytes>(&data.node())) {
    const auto hex = core::bytes_to_hex(*bytes);
    return schema == CardanoNodePlutusDatumSchema::basic ? Json("0x" + hex) : Json{{"bytes", hex}};
  }
  if (const auto* list = std::get_if<std::shared_ptr<PlutusData::List>>(&data.node())) {
    Json values = Json::array();
    for (const auto& item : **list) {
      auto converted = plutus_to_json(item, schema, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      values.push_back(std::move(*converted));
    }
    return schema == CardanoNodePlutusDatumSchema::basic ? values
                                                         : Json{{"list", std::move(values)}};
  }
  if (schema == CardanoNodePlutusDatumSchema::basic) {
    return std::unexpected(
        structure_error("basic Plutus JSON cannot represent maps or constructors"));
  }
  if (const auto* map = std::get_if<std::shared_ptr<PlutusMap>>(&data.node())) {
    Json entries = Json::array();
    for (const auto& [key, value] : (**map).entries) {
      auto converted_key = plutus_to_json(key, schema, budget, depth + 1);
      auto converted_value = plutus_to_json(value, schema, budget, depth + 1);
      if (!converted_key) return std::unexpected(converted_key.error());
      if (!converted_value) return std::unexpected(converted_value.error());
      entries.push_back({
          {"k", std::move(*converted_key)},
          {"v", std::move(*converted_value)},
      });
    }
    return Json{{"map", std::move(entries)}};
  }
  const auto& constr = **std::get_if<std::shared_ptr<ConstrPlutusData>>(&data.node());
  Json fields = Json::array();
  for (const auto& field : constr.fields) {
    auto converted = plutus_to_json(field, schema, budget, depth + 1);
    if (!converted) return std::unexpected(converted.error());
    fields.push_back(std::move(*converted));
  }
  return Json{
      {"constructor", integer_json(constr.alternative)},
      {"fields", std::move(fields)},
  };
}

[[nodiscard]] core::Result<PlutusData> plutus_from_json(const Json& value,
                                                        CardanoNodePlutusDatumSchema schema,
                                                        JsonBudget& budget, std::size_t depth) {
  auto entered = budget.enter(depth);
  if (!entered) return std::unexpected(entered.error());
  if (schema == CardanoNodePlutusDatumSchema::basic) {
    if (value.is_number_integer() || value.is_number_unsigned()) {
      auto integer = json_metadata_integer(value);
      if (!integer || !integer->fits_int64()) {
        return std::unexpected(integer ? core::CardanoError(core::ErrorCode::out_of_range,
                                                            "basic Plutus integer is not safe")
                                       : integer.error());
      }
      return PlutusData::integer(std::move(*integer));
    }
    if (value.is_string()) {
      const auto text = value.get<std::string>();
      if (text.starts_with("0x")) {
        auto bytes = core::hex_to_bytes(std::string_view(text).substr(2));
        if (!bytes) return std::unexpected(bytes.error());
        return PlutusData::bytes(std::move(*bytes));
      }
      const auto raw =
          core::ByteSpan(reinterpret_cast<const core::Byte*>(text.data()), text.size());
      return PlutusData::bytes(core::copy_bytes(raw));
    }
    if (value.is_array()) {
      PlutusData::List values;
      for (const auto& item : value) {
        auto converted = plutus_from_json(item, schema, budget, depth + 1);
        if (!converted) return std::unexpected(converted.error());
        values.push_back(std::move(*converted));
      }
      return PlutusData::list(std::move(values));
    }
    return std::unexpected(structure_error("unsupported basic Plutus JSON"));
  }
  if (!value.is_object()) {
    return std::unexpected(structure_error("detailed Plutus JSON must be an object"));
  }
  if (value.contains("constructor")) {
    if (!value.contains("fields") || !value.at("fields").is_array()) {
      return std::unexpected(structure_error("constructor fields must be an array"));
    }
    auto alternative = json_metadata_integer(value.at("constructor"));
    if (!alternative || alternative->is_negative()) {
      return std::unexpected(alternative ? core::CardanoError(core::ErrorCode::out_of_range,
                                                              "constructor must be nonnegative")
                                         : alternative.error());
    }
    PlutusData::List fields;
    for (const auto& item : value.at("fields")) {
      auto converted = plutus_from_json(item, schema, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      fields.push_back(std::move(*converted));
    }
    return PlutusData::constr(std::move(*alternative), std::move(fields));
  }
  if (value.contains("map")) {
    if (!value.at("map").is_array()) {
      return std::unexpected(structure_error("Plutus map must be an array"));
    }
    std::vector<std::pair<PlutusData, PlutusData>> entries;
    for (const auto& item : value.at("map")) {
      if (!item.is_object() || !item.contains("k") || !item.contains("v")) {
        return std::unexpected(structure_error("Plutus map entry is invalid"));
      }
      auto key = plutus_from_json(item.at("k"), schema, budget, depth + 1);
      auto converted = plutus_from_json(item.at("v"), schema, budget, depth + 1);
      if (!key) return std::unexpected(key.error());
      if (!converted) return std::unexpected(converted.error());
      entries.emplace_back(std::move(*key), std::move(*converted));
    }
    return PlutusData::map(std::move(entries));
  }
  if (value.contains("list")) {
    if (!value.at("list").is_array()) {
      return std::unexpected(structure_error("Plutus list must be an array"));
    }
    PlutusData::List values;
    for (const auto& item : value.at("list")) {
      auto converted = plutus_from_json(item, schema, budget, depth + 1);
      if (!converted) return std::unexpected(converted.error());
      values.push_back(std::move(*converted));
    }
    return PlutusData::list(std::move(values));
  }
  if (value.contains("int")) {
    auto integer = json_metadata_integer(value.at("int"));
    if (!integer) return std::unexpected(integer.error());
    return PlutusData::integer(std::move(*integer));
  }
  if (value.contains("bytes") && value.at("bytes").is_string()) {
    auto bytes = core::hex_to_bytes(value.at("bytes").get<std::string>());
    if (!bytes) return std::unexpected(bytes.error());
    return PlutusData::bytes(std::move(*bytes));
  }
  return std::unexpected(structure_error("unknown detailed Plutus JSON shape"));
}

[[nodiscard]] core::Result<Json> parse_json(std::string_view json) {
  try {
    return Json::parse(json);
  } catch (const std::exception& error) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding, error.what()));
  }
}

[[nodiscard]] core::Result<std::uint64_t> json_unsigned(std::string_view owner, const Json& value) {
  constexpr auto safe_max = std::uint64_t{9'007'199'254'740'991ULL};
  if (value.is_number_unsigned()) {
    const auto number = value.get<std::uint64_t>();
    if (number <= safe_max) return number;
  } else if (value.is_number_integer()) {
    const auto number = value.get<std::int64_t>();
    if (number >= 0) return static_cast<std::uint64_t>(number);
  } else if (value.is_number_float()) {
    const auto number = value.get<double>();
    if (std::isfinite(number) && number >= 0.0 && number <= static_cast<double>(safe_max) &&
        std::trunc(number) == number) {
      return static_cast<std::uint64_t>(number);
    }
  } else if (value.is_string()) {
    const auto& text = value.get_ref<const std::string&>();
    if (!text.empty() && std::ranges::all_of(text, [](char character) {
          return character >= '0' && character <= '9';
        })) {
      auto parsed = core::BigInteger::from_decimal(text);
      if (parsed) {
        auto converted = parsed->to_uint64();
        if (converted) return *converted;
      }
    }
  }
  return std::unexpected(core::CardanoError(
      core::ErrorCode::out_of_range,
      std::string(owner) + " JSON integer must be a nonnegative safe number or uint64 string"));
}

[[nodiscard]] Json json_unsigned_output(std::uint64_t value) {
  constexpr auto safe_max = std::uint64_t{9'007'199'254'740'991ULL};
  return value <= safe_max ? Json(value) : Json(std::to_string(value));
}

[[nodiscard]] core::Result<std::pair<std::uint64_t, std::uint64_t>> json_rational(
    std::string_view owner, const Json& value) {
  if (!value.is_object() || value.size() != 2U || !value.contains("numerator") ||
      !value.contains("denominator")) {
    return std::unexpected(structure_error(
        std::string(owner) + " JSON rational must contain numerator and denominator"));
  }
  auto numerator = json_unsigned(owner, value.at("numerator"));
  auto denominator = json_unsigned(owner, value.at("denominator"));
  if (!numerator || !denominator || *denominator == 0U) {
    return std::unexpected(
        !numerator
            ? numerator.error()
            : (!denominator
                   ? denominator.error()
                   : structure_error(std::string(owner) + " JSON denominator must be positive")));
  }
  return std::pair{*numerator, *denominator};
}

}  // namespace

core::Result<ExUnits> ExUnits::from_json(std::string_view json) {
  auto parsed = parse_json(json);
  if (!parsed) return std::unexpected(parsed.error());
  if (!parsed->is_object() || parsed->size() != 2U || !parsed->contains("mem") ||
      !parsed->contains("steps")) {
    return std::unexpected(structure_error("ExUnits JSON must contain exactly mem and steps"));
  }
  auto memory = json_unsigned("ExUnits", parsed->at("mem"));
  auto step_count = json_unsigned("ExUnits", parsed->at("steps"));
  constexpr auto maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
  if (!memory || !step_count || *memory > maximum || *step_count > maximum) {
    return std::unexpected(!memory ? memory.error()
                                   : (!step_count ? step_count.error()
                                                  : structure_error("ExUnits JSON exceeds int64")));
  }
  return ExUnits{static_cast<std::int64_t>(*memory), static_cast<std::int64_t>(*step_count)};
}

std::string ExUnits::to_json() const {
  Json output{
      {"mem", memory < 0 ? Json(memory) : json_unsigned_output(static_cast<std::uint64_t>(memory))},
      {"steps", steps < 0 ? Json(steps) : json_unsigned_output(static_cast<std::uint64_t>(steps))}};
  return output.dump();
}

core::Result<ExUnitPrices> ExUnitPrices::from_json(std::string_view json) {
  auto parsed = parse_json(json);
  if (!parsed) return std::unexpected(parsed.error());
  if (!parsed->is_object() || parsed->size() != 2U || !parsed->contains("mem_price") ||
      !parsed->contains("step_price")) {
    return std::unexpected(
        structure_error("ExUnitPrices JSON must contain exactly mem_price and step_price"));
  }
  auto memory = json_rational("ExUnitPrices.mem_price", parsed->at("mem_price"));
  auto steps_value = json_rational("ExUnitPrices.step_price", parsed->at("step_price"));
  if (!memory || !steps_value) {
    return std::unexpected(!memory ? memory.error() : steps_value.error());
  }
  return ExUnitPrices{memory->first, memory->second, steps_value->first, steps_value->second};
}

std::string ExUnitPrices::to_json() const {
  return Json{{"mem_price", Json{{"numerator", json_unsigned_output(memory_numerator)},
                                 {"denominator", json_unsigned_output(memory_denominator)}}},
              {"step_price", Json{{"numerator", json_unsigned_output(steps_numerator)},
                                  {"denominator", json_unsigned_output(steps_denominator)}}}}
      .dump();
}

core::Result<std::uint64_t> get_deposit(const core::cbor::Value& transaction_body,
                                        std::uint64_t key_deposit, std::uint64_t pool_deposit) {
  auto total = sum_certificate_coins(transaction_body, key_deposit, pool_deposit, false);
  if (!total) return std::unexpected(total.error());
  const auto* proposals = set_array(map_unsigned_key(transaction_body, 20));
  if (proposals != nullptr) {
    for (std::size_t index = 0; index < proposals->values.size(); ++index) {
      const auto* proposal = proposals->values[index].as_array();
      if (proposal == nullptr || proposal->values.empty()) {
        return std::unexpected(
            structure_error("proposal procedure must be a nonempty array").at(index));
      }
      auto coin = to_uint(proposal->values[0], "proposal deposit");
      if (!coin) return std::unexpected(coin.error().at(index));
      auto next = checked_add_coin(*total, *coin, "deposit total");
      if (!next) return std::unexpected(next.error().at(index));
      total = *next;
    }
  }
  return total;
}

core::Result<std::uint64_t> get_implicit_input(const core::cbor::Value& transaction_body,
                                               std::uint64_t key_deposit,
                                               std::uint64_t pool_deposit) {
  auto total = sum_certificate_coins(transaction_body, key_deposit, pool_deposit, true);
  if (!total) return std::unexpected(total.error());
  if (const auto* withdrawals = map_unsigned_key(transaction_body, 5); withdrawals != nullptr) {
    const auto* map = withdrawals->as_map();
    if (map == nullptr) {
      return std::unexpected(structure_error("withdrawals must be a map"));
    }
    for (std::size_t index = 0; index < map->entries.size(); ++index) {
      auto coin = to_uint(map->entries[index].second, "withdrawal coin");
      if (!coin) return std::unexpected(coin.error().at(index));
      auto next = checked_add_coin(*total, *coin, "implicit input total");
      if (!next) return std::unexpected(next.error().at(index));
      total = *next;
    }
  }
  return total;
}

core::Result<std::string> cbor_value_to_json(const core::cbor::Value& value,
                                             bool conway_map_shape) {
  JsonBudget budget;
  auto converted = generic_to_json(value, conway_map_shape, budget, 0);
  return converted ? core::Result<std::string>(converted->dump())
                   : std::unexpected(converted.error());
}

core::Result<core::cbor::Value> cbor_value_from_json(std::string_view json) {
  auto parsed = parse_json(json);
  if (!parsed) return std::unexpected(parsed.error());
  JsonBudget budget;
  return generic_from_json(*parsed, budget, 0);
}

core::Result<std::string> decode_metadatum_to_json_str(const core::cbor::Value& metadatum,
                                                       MetadataJsonSchema schema) {
  JsonBudget budget;
  auto converted = metadata_to_json(metadatum, schema, budget, 0);
  return converted ? core::Result<std::string>(converted->dump())
                   : std::unexpected(converted.error());
}

core::Result<core::cbor::Value> encode_json_str_to_metadatum(std::string_view json,
                                                             MetadataJsonSchema schema) {
  auto parsed = parse_json(json);
  if (!parsed) return std::unexpected(parsed.error());
  JsonBudget budget;
  return metadata_from_json(*parsed, schema, budget, 0);
}

core::Result<std::string> decode_plutus_datum_to_json_str(const core::cbor::Value& datum,
                                                          CardanoNodePlutusDatumSchema schema) {
  auto data = PlutusData::from_cbor_value(datum);
  if (!data) return std::unexpected(data.error());
  JsonBudget budget;
  auto converted = plutus_to_json(*data, schema, budget, 0);
  return converted ? core::Result<std::string>(converted->dump())
                   : std::unexpected(converted.error());
}

core::Result<core::cbor::Value> encode_json_str_to_plutus_datum(
    std::string_view json, CardanoNodePlutusDatumSchema schema) {
  auto parsed = parse_json(json);
  if (!parsed) return std::unexpected(parsed.error());
  JsonBudget budget;
  auto data = plutus_from_json(*parsed, schema, budget, 0);
  if (!data) return std::unexpected(data.error());
  return data->to_cbor_value();
}

}  // namespace cardano::chain
