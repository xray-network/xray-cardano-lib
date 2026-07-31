#include "cardano/chain/era_models.hpp"

#include <arpa/inet.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <string_view>
#include <variant>

namespace cardano::chain::detail {
namespace {

using CborValue = core::cbor::Value;

[[nodiscard]] core::CardanoError model_error(std::string_view name, std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure,
                            std::string(name) + ": " + std::move(message));
}

[[nodiscard]] core::Result<std::uint64_t> unsigned_value(std::string_view name,
                                                         const CborValue& value,
                                                         std::string_view field) {
  const auto* integer = value.as_unsigned();
  if (integer == nullptr) {
    return std::unexpected(model_error(name, std::string(field) + " must be unsigned"));
  }
  return integer->value.to_uint64();
}

[[nodiscard]] core::VoidResult require_bytes(std::string_view name, const CborValue& value,
                                             std::size_t size, std::string_view field) {
  const auto* bytes = value.as_byte_string();
  if (bytes == nullptr || bytes->value.size() != size) {
    return std::unexpected(
        model_error(name, std::string(field) + " must be bytes" + std::to_string(size)));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult require_bytes_range(std::string_view name, const CborValue& value,
                                                   std::size_t minimum, std::size_t maximum,
                                                   std::string_view field) {
  const auto* bytes = value.as_byte_string();
  if (bytes == nullptr || bytes->value.size() < minimum || bytes->value.size() > maximum) {
    return std::unexpected(model_error(name, std::string(field) + " byte length must be in " +
                                                 std::to_string(minimum) + ".." +
                                                 std::to_string(maximum)));
  }
  return std::monostate{};
}

[[nodiscard]] bool is_null(const CborValue& value) {
  return std::holds_alternative<core::cbor::NullValue>(value.node());
}

[[nodiscard]] core::VoidResult require_unsigned_bound(std::string_view name, const CborValue& value,
                                                      std::uint64_t maximum,
                                                      std::string_view field) {
  auto number = unsigned_value(name, value, field);
  if (!number) return std::unexpected(number.error());
  if (*number > maximum) {
    return std::unexpected(model_error(name, std::string(field) + " is out of range"));
  }
  return std::monostate{};
}

[[nodiscard]] core::Result<const core::cbor::ArrayValue*> set_array(std::string_view name,
                                                                    const CborValue& value,
                                                                    std::string_view field) {
  const CborValue* candidate = &value;
  if (const auto* tag = value.as_tag()) {
    auto tag_number = tag->tag.to_uint64();
    if (!tag_number || *tag_number != 258U || tag->value == nullptr) {
      return std::unexpected(model_error(name, std::string(field) + " uses an invalid set tag"));
    }
    candidate = tag->value.get();
  }
  const auto* array = candidate->as_array();
  if (array == nullptr) {
    return std::unexpected(model_error(name, std::string(field) + " must be an array or tag 258"));
  }
  return array;
}

[[nodiscard]] core::Result<const CborValue*> numeric_field(std::string_view name,
                                                           const core::cbor::MapValue& map,
                                                           std::uint64_t key) {
  for (const auto& [candidate, value] : map.entries) {
    auto number = unsigned_value(name, candidate, "map key");
    if (!number) return std::unexpected(number.error());
    if (*number == key) return &value;
  }
  return static_cast<const CborValue*>(nullptr);
}

[[nodiscard]] core::VoidResult validate_numeric_keys(
    std::string_view name, const core::cbor::MapValue& map,
    std::initializer_list<std::uint64_t> allowed,
    std::initializer_list<std::uint64_t> required = {}) {
  std::set<std::uint64_t> seen;
  for (const auto& [key, unused] : map.entries) {
    static_cast<void>(unused);
    auto number = unsigned_value(name, key, "map key");
    if (!number) return std::unexpected(number.error());
    if (std::ranges::find(allowed, *number) == allowed.end()) {
      return std::unexpected(model_error(name, "unknown map key " + std::to_string(*number)));
    }
    if (!seen.insert(*number).second) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::duplicate_key,
                             std::string(name) + ": duplicate map key " + std::to_string(*number)));
    }
  }
  for (const auto key : required) {
    if (!seen.contains(key)) {
      return std::unexpected(model_error(name, "missing required map key " + std::to_string(key)));
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult require_array_size(std::string_view name, const CborValue& value,
                                                  std::size_t minimum, std::size_t maximum) {
  const auto* array = value.as_array();
  if (array == nullptr || array->values.size() < minimum || array->values.size() > maximum) {
    return std::unexpected(model_error(name, "array length must be in " + std::to_string(minimum) +
                                                 ".." + std::to_string(maximum)));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_transaction_input(std::string_view name,
                                                          const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto hash = require_bytes(name, fields[0], 32, "transaction hash");
  if (!hash) return std::unexpected(hash.error().at(0U));
  auto index = require_unsigned_bound(name, fields[1], 65'535U, "transaction index");
  return index ? core::VoidResult(std::monostate{}) : std::unexpected(index.error().at(1U));
}

[[nodiscard]] core::VoidResult validate_transaction_input_set(std::string_view name,
                                                              const CborValue& value,
                                                              std::string_view field,
                                                              bool nonempty) {
  auto values = set_array(name, value, field);
  if (!values) return std::unexpected(values.error());
  if (nonempty && (*values)->values.empty()) {
    return std::unexpected(model_error(name, std::string(field) + " must be nonempty"));
  }
  for (std::size_t index = 0; index < (*values)->values.size(); ++index) {
    auto valid = validate_transaction_input(name, (*values)->values[index]);
    if (!valid) return std::unexpected(valid.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_credential(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto kind = unsigned_value(name, fields[0], "credential kind");
  if (!kind || *kind > 1) {
    return std::unexpected(kind ? model_error(name, "credential kind must be 0 or 1")
                                : kind.error());
  }
  return require_bytes(name, fields[1], 28, "credential hash");
}

[[nodiscard]] core::VoidResult validate_native_script(std::string_view name, const CborValue& value,
                                                      std::size_t depth = 0) {
  if (depth > 128) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                           std::string(name) + ": native script exceeds depth 128"));
  }
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(model_error(name, "native script must be a nonempty array"));
  }
  auto kind = unsigned_value(name, fields->values[0], "script kind");
  if (!kind || *kind > 5) {
    return std::unexpected(kind ? model_error(name, "script kind must be in 0..5") : kind.error());
  }
  if (*kind == 0) {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return shape;
    return require_bytes(name, fields->values[1], 28, "signer hash");
  }
  if (*kind == 4 || *kind == 5) {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return shape;
    auto slot = unsigned_value(name, fields->values[1], "timelock slot");
    return slot ? core::VoidResult(std::monostate{}) : std::unexpected(slot.error());
  }
  const auto expected = *kind == 3 ? std::size_t{3} : std::size_t{2};
  auto shape = require_array_size(name, value, expected, expected);
  if (!shape) return shape;
  if (*kind == 3) {
    auto required = unsigned_value(name, fields->values[1], "required signer count");
    if (!required) return std::unexpected(required.error());
  }
  const auto* children = fields->values[*kind == 3 ? 2 : 1].as_array();
  if (children == nullptr) {
    return std::unexpected(model_error(name, "script children must be an array"));
  }
  for (std::size_t index = 0; index < children->values.size(); ++index) {
    auto valid = validate_native_script(name, children->values[index], depth + 1);
    if (!valid) return std::unexpected(valid.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_anchor(std::string_view name, const CborValue& value);
[[nodiscard]] core::VoidResult validate_drep(std::string_view name, const CborValue& value);
[[nodiscard]] core::VoidResult validate_proposal(std::string_view name, const CborValue& value);
[[nodiscard]] core::VoidResult validate_relay(std::string_view name, const CborValue& value);
[[nodiscard]] core::VoidResult validate_metadata_map(std::string_view name, const CborValue& value);
[[nodiscard]] core::VoidResult validate_auxiliary_data(std::string_view name,
                                                       const CborValue& value,
                                                       std::uint64_t maximum_plutus_key);
[[nodiscard]] core::VoidResult validate_voting_procedures(std::string_view name,
                                                          const CborValue& value);
[[nodiscard]] core::VoidResult validate_ex_units(std::string_view name, const CborValue& value);
[[nodiscard]] core::VoidResult validate_rational(std::string_view name, const CborValue& value,
                                                 bool unit_interval);
enum class BodyEra : std::uint8_t;
[[nodiscard]] core::VoidResult validate_witness_set(std::string_view name, const CborValue& value,
                                                    BodyEra era);
[[nodiscard]] core::VoidResult validate_era_update(std::string_view name, const CborValue& value);
[[nodiscard]] core::VoidResult validate_protocol_param_update(std::string_view name,
                                                              const CborValue& value, bool conway);
[[nodiscard]] core::VoidResult validate_map(std::string_view name, const CborValue& value);
[[nodiscard]] core::VoidResult validate_ssc_signed_commitment(std::string_view name,
                                                              const CborValue& value);
[[nodiscard]] core::VoidResult validate_ssc_shares(std::string_view name, const CborValue& value);

[[nodiscard]] core::VoidResult validate_optional_hash(std::string_view name, const CborValue& value,
                                                      std::size_t size, std::string_view field) {
  return is_null(value) ? core::VoidResult(std::monostate{})
                        : require_bytes(name, value, size, field);
}

[[nodiscard]] core::VoidResult validate_mir(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto pot = require_unsigned_bound(name, fields[0], 1U, "MIR pot");
  if (!pot) return std::unexpected(pot.error().at(0U));
  if (const auto* rewards = fields[1].as_map()) {
    for (std::size_t index = 0; index < rewards->entries.size(); ++index) {
      auto credential = validate_credential(name, rewards->entries[index].first);
      if (!credential) return std::unexpected(credential.error().at(1U).at(index));
      const auto& amount = rewards->entries[index].second;
      bool valid_amount = false;
      if (const auto* positive = amount.as_unsigned()) {
        valid_amount = positive->value.to_int64().has_value();
      } else if (const auto* negative = amount.as_negative()) {
        valid_amount = negative->value.to_int64().has_value();
      }
      if (!valid_amount) {
        return std::unexpected(
            model_error(name, "MIR reward must be a signed int64").at(1U).at(index));
      }
    }
    return std::monostate{};
  }
  auto coin = unsigned_value(name, fields[1], "MIR transfer");
  return coin ? core::VoidResult(std::monostate{}) : std::unexpected(coin.error().at(1U));
}

[[nodiscard]] core::VoidResult validate_nonce(std::string_view name, const CborValue& value) {
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(model_error(name, "nonce must be a nonempty array"));
  }
  auto kind = require_unsigned_bound(name, fields->values[0], 1U, "nonce kind");
  if (!kind) return kind;
  const auto kind_value = unsigned_value(name, fields->values[0], "nonce kind").value();
  auto shape =
      require_array_size(name, value, kind_value == 0U ? 1U : 2U, kind_value == 0U ? 1U : 2U);
  if (!shape) return shape;
  return kind_value == 0U ? core::VoidResult(std::monostate{})
                          : require_bytes(name, fields->values[1], 32U, "nonce hash");
}

[[nodiscard]] core::VoidResult validate_pool_metadata(std::string_view name,
                                                      const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  const auto* url = fields[0].as_text_string();
  const auto maximum = name.find("Shelley") == std::string_view::npos ? 128U : 64U;
  if (url == nullptr || url->value.size() > maximum) {
    return std::unexpected(model_error(name, "pool metadata URL is out of range").at(0U));
  }
  return require_bytes(name, fields[1], 32U, "pool metadata hash");
}

[[nodiscard]] core::VoidResult validate_thresholds(std::string_view name, const CborValue& value,
                                                   std::size_t expected) {
  auto shape = require_array_size(name, value, expected, expected);
  if (!shape) return shape;
  for (std::size_t index = 0; index < expected; ++index) {
    auto threshold = validate_rational(name, value.as_array()->values[index], true);
    if (!threshold) return std::unexpected(threshold.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_redeemer_key(std::string_view name, const CborValue& value,
                                                     std::uint64_t maximum_tag) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  auto tag = require_unsigned_bound(name, value.as_array()->values[0], maximum_tag, "redeemer tag");
  auto index =
      require_unsigned_bound(name, value.as_array()->values[1], UINT32_MAX, "redeemer index");
  if (!tag || !index) {
    return std::unexpected(!tag ? tag.error().at(0U) : index.error().at(1U));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_redeemer_value(std::string_view name,
                                                       const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  auto data = validate_plutus_data_node(value.as_array()->values[0]);
  auto units = validate_ex_units(name, value.as_array()->values[1]);
  if (!data || !units) {
    return std::unexpected(!data ? data.error().at(0U) : units.error().at(1U));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_legacy_redeemer(std::string_view name,
                                                        const CborValue& value,
                                                        std::uint64_t maximum_tag) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto key = validate_redeemer_key(name, CborValue::array({fields[0], fields[1]}), maximum_tag);
  auto data = validate_plutus_data_node(fields[2]);
  auto units = validate_ex_units(name, fields[3]);
  if (!key || !data || !units) {
    return std::unexpected(!key ? key.error()
                                : (!data ? data.error().at(2U) : units.error().at(3U)));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_redeemers(std::string_view name, const CborValue& value,
                                                  std::uint64_t maximum_tag) {
  if (const auto* legacy = value.as_array()) {
    if (legacy->values.empty()) {
      return std::unexpected(model_error(name, "redeemer array must be nonempty"));
    }
    for (std::size_t index = 0; index < legacy->values.size(); ++index) {
      auto valid = validate_legacy_redeemer(name, legacy->values[index], maximum_tag);
      if (!valid) return std::unexpected(valid.error().at(index));
    }
    return std::monostate{};
  }
  const auto* map = value.as_map();
  if (map == nullptr || map->entries.empty()) {
    return std::unexpected(model_error(name, "redeemers must be a nonempty array or map"));
  }
  for (std::size_t index = 0; index < map->entries.size(); ++index) {
    auto key = validate_redeemer_key(name, map->entries[index].first, maximum_tag);
    auto redeemer = validate_redeemer_value(name, map->entries[index].second);
    if (!key || !redeemer) {
      return std::unexpected(!key ? key.error().at(index) : redeemer.error().at(index));
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_withdrawals(std::string_view name, const CborValue& value,
                                                    bool nonempty) {
  const auto* map = value.as_map();
  if (map == nullptr || (nonempty && map->entries.empty())) {
    return std::unexpected(model_error(name, "withdrawals must be a nonempty map"));
  }
  for (std::size_t index = 0; index < map->entries.size(); ++index) {
    if (map->entries[index].first.as_byte_string() == nullptr) {
      return std::unexpected(model_error(name, "reward account must be bytes").at(index));
    }
    auto coin = unsigned_value(name, map->entries[index].second, "withdrawal coin");
    if (!coin) return std::unexpected(coin.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_rational(std::string_view name, const CborValue& value,
                                                 bool unit_interval) {
  const CborValue* candidate = &value;
  if (const auto* tag = value.as_tag()) {
    auto number = tag->tag.to_uint64();
    if (!number || *number != 30U || tag->value == nullptr) {
      return std::unexpected(model_error(name, "rational must use semantic tag 30"));
    }
    candidate = tag->value.get();
  }
  auto shape = require_array_size(name, *candidate, 2, 2);
  if (!shape) return shape;
  auto numerator = unsigned_value(name, candidate->as_array()->values[0], "numerator");
  auto denominator = unsigned_value(name, candidate->as_array()->values[1], "denominator");
  if (!numerator || !denominator) {
    return std::unexpected(!numerator ? numerator.error() : denominator.error());
  }
  if (*denominator == 0U || (unit_interval && *numerator > *denominator)) {
    return std::unexpected(model_error(name, "invalid rational interval"));
  }
  return std::monostate{};
}
