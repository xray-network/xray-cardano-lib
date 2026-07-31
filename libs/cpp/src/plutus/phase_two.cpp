#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "cardano/core/cbor.hpp"
#include "cardano/crypto/primitives.hpp"
#include "cardano/plutus/uplc.hpp"
#include "phase_two_context.hpp"

namespace cardano::plutus {
namespace {

using Cbor = core::cbor::Value;

[[nodiscard]] core::CardanoError malformed(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::CardanoError evaluation(std::string message) {
  return core::CardanoError(core::ErrorCode::evaluation, std::move(message));
}

[[nodiscard]] std::optional<std::uint64_t> unsigned_value(const Cbor& value) {
  const auto* number = value.as_unsigned();
  if (number == nullptr) {
    return std::nullopt;
  }
  const auto converted = number->value.to_uint64();
  return converted ? std::optional<std::uint64_t>(*converted) : std::nullopt;
}

[[nodiscard]] const Cbor* map_get(const core::cbor::MapValue& map, std::uint64_t key) {
  for (const auto& [candidate, value] : map.entries) {
    const auto number = unsigned_value(candidate);
    if (number && *number == key) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] const std::vector<Cbor>* set_values(const Cbor* value) {
  if (value == nullptr) {
    return nullptr;
  }
  if (const auto* array = value->as_array()) {
    return &array->values;
  }
  const auto* tag = value->as_tag();
  if (tag == nullptr || tag->tag != core::BigInteger(std::uint64_t{258})) {
    return nullptr;
  }
  const auto* array = tag->value->as_array();
  return array == nullptr ? nullptr : &array->values;
}

[[nodiscard]] core::Result<core::Bytes> canonical(const Cbor& value) {
  return core::cbor::encode_cbor(value, {.mode = core::cbor::Mode::canonical});
}

[[nodiscard]] bool cbor_less(const Cbor& left, const Cbor& right) {
  const auto a = canonical(left);
  const auto b = canonical(right);
  return a && b && std::lexicographical_compare(a->begin(), a->end(), b->begin(), b->end());
}

struct LedgerRedeemer {
  std::uint8_t tag{};
  std::uint64_t index{};
  Cbor data;
};

[[nodiscard]] std::string pointer_key(const LedgerRedeemer& redeemer) {
  return std::to_string(redeemer.tag) + ":" + std::to_string(redeemer.index);
}

[[nodiscard]] core::Result<LedgerRedeemer> read_redeemer(const Cbor& tag, const Cbor& index,
                                                         const Cbor& data) {
  const auto tag_value = unsigned_value(tag);
  const auto index_value = unsigned_value(index);
  if (!tag_value || *tag_value > 5U || !index_value) {
    return std::unexpected(malformed("invalid redeemer pointer"));
  }
  return LedgerRedeemer{static_cast<std::uint8_t>(*tag_value), *index_value, data};
}

[[nodiscard]] core::Result<std::vector<LedgerRedeemer>> parse_redeemers(const Cbor* value) {
  std::vector<LedgerRedeemer> output;
  if (value == nullptr) {
    return output;
  }
  if (const auto* values = set_values(value)) {
    for (const auto& item : *values) {
      const auto* fields = item.as_array();
      if (fields == nullptr || fields->values.size() != 4U) {
        return std::unexpected(malformed("redeemer must be [tag,index,data,ex_units]"));
      }
      auto redeemer = read_redeemer(fields->values[0], fields->values[1], fields->values[2]);
      if (!redeemer) {
        return std::unexpected(redeemer.error());
      }
      output.push_back(std::move(*redeemer));
    }
  } else if (const auto* map = value->as_map()) {
    for (const auto& [key, item] : map->entries) {
      const auto* pointer = key.as_array();
      const auto* fields = item.as_array();
      if (pointer == nullptr || pointer->values.size() != 2U || fields == nullptr ||
          fields->values.size() != 2U) {
        return std::unexpected(malformed("map redeemer must be [tag,index] => [data,ex_units]"));
      }
      auto redeemer = read_redeemer(pointer->values[0], pointer->values[1], fields->values[0]);
      if (!redeemer) {
        return std::unexpected(redeemer.error());
      }
      output.push_back(std::move(*redeemer));
    }
  } else {
    return std::unexpected(malformed("redeemers must be an array or map"));
  }
  std::ranges::sort(output, {}, [](const LedgerRedeemer& redeemer) {
    return std::pair{redeemer.tag, redeemer.index};
  });
  for (std::size_t index = 1U; index < output.size(); ++index) {
    if (pointer_key(output[index - 1U]) == pointer_key(output[index])) {
      return std::unexpected(malformed("duplicate redeemer " + pointer_key(output[index])));
    }
  }
  return output;
}

struct Utxo {
  Cbor input;
  Cbor output;
};

[[nodiscard]] core::Result<std::vector<Utxo>> parse_utxos(std::span<const PhaseTwoUtxo> values) {
  std::vector<Utxo> output;
  std::set<core::Bytes> identities;
  for (const auto& [input_bytes, output_bytes] : values) {
    auto input = core::cbor::decode_cbor(input_bytes);
    auto transaction_output = core::cbor::decode_cbor(output_bytes);
    if (!input) {
      return std::unexpected(input.error());
    }
    if (!transaction_output) {
      return std::unexpected(transaction_output.error());
    }
    const auto* input_fields = input->as_array();
    if (input_fields == nullptr || input_fields->values.size() != 2U) {
      return std::unexpected(malformed("UTxO input must be [transaction_id,index]"));
    }
    if (transaction_output->as_array() == nullptr && transaction_output->as_map() == nullptr) {
      return std::unexpected(malformed("UTxO output must be an array or map"));
    }
    if (!identities.insert(input_bytes).second) {
      return std::unexpected(malformed("duplicate UTxO input"));
    }
    output.push_back({std::move(*input), std::move(*transaction_output)});
  }
  return output;
}

struct Script {
  std::uint8_t language{};
  core::Bytes bytes;
  core::Bytes hash;
};

[[nodiscard]] core::Bytes script_hash(std::uint8_t language, core::ByteSpan script) {
  core::Bytes input;
  input.reserve(script.size() + 1U);
  input.push_back(static_cast<std::byte>(language + 1U));
  input.insert(input.end(), script.begin(), script.end());
  return crypto::blake2b224(input);
}

[[nodiscard]] core::Result<std::vector<Script>> parse_scripts(const core::cbor::MapValue& witnesses,
                                                              const std::vector<Utxo>& utxos) {
  std::vector<Script> output;
  for (const auto [key, language] :
       std::array<std::pair<std::uint64_t, std::uint8_t>, 3>{{{3U, 0U}, {6U, 1U}, {7U, 2U}}}) {
    const auto* scripts = set_values(map_get(witnesses, key));
    if (scripts == nullptr && map_get(witnesses, key) != nullptr) {
      return std::unexpected(malformed("Plutus script witnesses must be sets"));
    }
    if (scripts == nullptr) {
      continue;
    }
    for (const auto& script : *scripts) {
      const auto* bytes = script.as_byte_string();
      if (bytes == nullptr) {
        return std::unexpected(malformed("Plutus script witness must be bytes"));
      }
      output.push_back({language, bytes->value, script_hash(language, bytes->value)});
    }
  }
  for (const auto& utxo : utxos) {
    const auto* map = utxo.output.as_map();
    if (map == nullptr) {
      continue;
    }
    const auto* reference = map_get(*map, 3U);
    const auto* embedded = reference == nullptr ? nullptr : reference->as_tag();
    if (embedded == nullptr || embedded->tag != core::BigInteger(std::uint64_t{24})) {
      continue;
    }
    const auto* bytes = embedded->value->as_byte_string();
    if (bytes == nullptr) {
      continue;
    }
    auto decoded = core::cbor::decode_cbor(bytes->value);
    if (!decoded) {
      return std::unexpected(decoded.error());
    }
    if (decoded->as_array() == nullptr || decoded->as_array()->values.size() != 2U) {
      continue;
    }
    const auto language_number = unsigned_value(decoded->as_array()->values[0]);
    const auto* script = decoded->as_array()->values[1].as_byte_string();
    if (!language_number || *language_number < 1U || *language_number > 3U || script == nullptr) {
      continue;
    }
    const auto language = static_cast<std::uint8_t>(*language_number - 1U);
    const auto hash = script_hash(language, script->value);
    if (std::ranges::none_of(output, [&](const Script& candidate) {
          return candidate.language == language && candidate.hash == hash;
        })) {
      output.push_back({language, script->value, hash});
    }
  }
  return output;
}

[[nodiscard]] const core::Bytes* payment_script_hash(const Cbor& output) {
  const Cbor* address = nullptr;
  if (const auto* fields = output.as_array(); fields != nullptr && !fields->values.empty()) {
    address = &fields->values[0];
  } else if (const auto* fields = output.as_map()) {
    address = map_get(*fields, 0U);
  }
  const auto* bytes = address == nullptr ? nullptr : address->as_byte_string();
  if (bytes == nullptr || bytes->value.size() < 29U) {
    return nullptr;
  }
  const auto kind = std::to_integer<std::uint8_t>(bytes->value[0]) >> 4U;
  if (kind != 1U && kind != 3U && kind != 5U && kind != 7U) {
    return nullptr;
  }
  return &bytes->value;
}

[[nodiscard]] const Utxo* find_utxo(const Cbor& input, const std::vector<Utxo>& utxos) {
  const auto identity = canonical(input);
  if (!identity) {
    return nullptr;
  }
  for (const auto& utxo : utxos) {
    const auto candidate = canonical(utxo.input);
    if (candidate && *candidate == *identity) {
      return &utxo;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<core::Bytes> first_script_credential(const Cbor& value) {
  if (const auto* array = value.as_array()) {
    if (array->values.size() == 2U) {
      const auto kind = unsigned_value(array->values[0]);
      const auto* bytes = array->values[1].as_byte_string();
      if (kind && *kind == 1U && bytes != nullptr && bytes->value.size() == 28U) {
        return bytes->value;
      }
    }
    for (const auto& item : array->values) {
      auto found = first_script_credential(item);
      if (found) {
        return found;
      }
    }
  }
  if (const auto* map = value.as_map()) {
    for (const auto& [key, item] : map->entries) {
      auto found = first_script_credential(key);
      if (!found) {
        found = first_script_credential(item);
      }
      if (found) {
        return found;
      }
    }
  }
  if (const auto* tag = value.as_tag()) {
    return first_script_credential(*tag->value);
  }
  return std::nullopt;
}

[[nodiscard]] core::Result<std::optional<core::Bytes>> purpose_script_hash(
    const LedgerRedeemer& redeemer, const core::cbor::MapValue& body,
    const std::vector<Utxo>& utxos) {
  switch (redeemer.tag) {
    case 0U: {
      const auto* inputs = set_values(map_get(body, 0U));
      if (inputs == nullptr) {
        return std::optional<core::Bytes>{};
      }
      auto sorted = *inputs;
      std::ranges::sort(sorted, cbor_less);
      if (redeemer.index >= sorted.size()) {
        return std::optional<core::Bytes>{};
      }
      const auto* utxo = find_utxo(sorted[static_cast<std::size_t>(redeemer.index)], utxos);
      if (utxo == nullptr) {
        return std::unexpected(evaluation("missing spending UTxO"));
      }
      const auto* address = payment_script_hash(utxo->output);
      if (address == nullptr) {
        return std::optional<core::Bytes>{};
      }
      return std::optional<core::Bytes>(core::Bytes(address->begin() + 1, address->begin() + 29));
    }
    case 1U: {
      const auto* mint = map_get(body, 9U);
      const auto* map = mint == nullptr ? nullptr : mint->as_map();
      if (map == nullptr) {
        return std::optional<core::Bytes>{};
      }
      std::vector<Cbor> policies;
      for (const auto& [policy, assets] : map->entries) {
        static_cast<void>(assets);
        if (policy.as_byte_string() != nullptr) {
          policies.push_back(policy);
        }
      }
      std::ranges::sort(policies, cbor_less);
      if (redeemer.index >= policies.size()) {
        return std::optional<core::Bytes>{};
      }
      const auto& bytes =
          policies[static_cast<std::size_t>(redeemer.index)].as_byte_string()->value;
      return bytes.size() == 28U ? std::optional<core::Bytes>(bytes) : std::optional<core::Bytes>{};
    }
    case 2U:
    case 5U: {
      const auto* value = map_get(body, redeemer.tag == 2U ? 4U : 20U);
      const auto* values = value == nullptr ? nullptr : value->as_array();
      if (values == nullptr || redeemer.index >= values->values.size()) {
        return std::optional<core::Bytes>{};
      }
      return first_script_credential(values->values[static_cast<std::size_t>(redeemer.index)]);
    }
    case 3U: {
      const auto* withdrawals = map_get(body, 5U);
      const auto* map = withdrawals == nullptr ? nullptr : withdrawals->as_map();
      if (map == nullptr) {
        return std::optional<core::Bytes>{};
      }
      std::vector<Cbor> accounts;
      for (const auto& [account, coin] : map->entries) {
        static_cast<void>(coin);
        if (account.as_byte_string() != nullptr) {
          accounts.push_back(account);
        }
      }
      std::ranges::sort(accounts, cbor_less);
      if (redeemer.index >= accounts.size()) {
        return std::optional<core::Bytes>{};
      }
      const auto& bytes =
          accounts[static_cast<std::size_t>(redeemer.index)].as_byte_string()->value;
      if (bytes.size() < 29U || (std::to_integer<std::uint8_t>(bytes[0]) >> 4U) != 0x0fU) {
        return std::optional<core::Bytes>{};
      }
      return std::optional<core::Bytes>(core::Bytes(bytes.end() - 28, bytes.end()));
    }
    case 4U: {
      const auto* voting = map_get(body, 19U);
      const auto* map = voting == nullptr ? nullptr : voting->as_map();
      if (map == nullptr) {
        return std::optional<core::Bytes>{};
      }
      std::vector<Cbor> voters;
      for (const auto& [voter, procedures] : map->entries) {
        static_cast<void>(procedures);
        voters.push_back(voter);
      }
      std::ranges::sort(voters, cbor_less);
      if (redeemer.index >= voters.size()) {
        return std::optional<core::Bytes>{};
      }
      return first_script_credential(voters[static_cast<std::size_t>(redeemer.index)]);
    }
    default:
      return std::optional<core::Bytes>{};
  }
}

[[nodiscard]] core::Result<const Script*> resolve_script(const LedgerRedeemer& redeemer,
                                                         const core::cbor::MapValue& body,
                                                         const std::vector<Script>& scripts,
                                                         const std::vector<Utxo>& utxos) {
  auto hash = purpose_script_hash(redeemer, body, utxos);
  if (!hash) {
    return std::unexpected(hash.error());
  }
  if (!*hash) {
    return std::unexpected(evaluation("cannot resolve script purpose " + pointer_key(redeemer)));
  }
  const Script* match = nullptr;
  for (const auto& script : scripts) {
    if (script.hash == **hash) {
      match = &script;
      break;
    }
  }
  if (match == nullptr) {
    return std::unexpected(evaluation("missing Plutus script"));
  }
  return match;
}

[[nodiscard]] core::Result<std::optional<Cbor>> find_datum(core::ByteSpan hash,
                                                           const core::cbor::MapValue& witnesses) {
  const auto* values = set_values(map_get(witnesses, 4U));
  if (values == nullptr) {
    return std::optional<Cbor>{};
  }
  for (const auto& datum : *values) {
    auto validated = validate_plutus_data_node(datum);
    if (!validated) {
      return std::unexpected(validated.error());
    }
    auto encoded = encode_plutus_data(*validated);
    if (!encoded) {
      return std::unexpected(encoded.error());
    }
    if (crypto::blake2b256(*encoded) == core::Bytes(hash.begin(), hash.end())) {
      return std::optional<Cbor>(datum);
    }
  }
  return std::optional<Cbor>{};
}

[[nodiscard]] core::Result<std::optional<Cbor>> spending_datum(
    const LedgerRedeemer& redeemer, const core::cbor::MapValue& body,
    const core::cbor::MapValue& witnesses, const std::vector<Utxo>& utxos) {
  if (redeemer.tag != 0U) {
    return std::optional<Cbor>{};
  }
  const auto* inputs = set_values(map_get(body, 0U));
  if (inputs == nullptr) {
    return std::optional<Cbor>{};
  }
  auto sorted = *inputs;
  std::ranges::sort(sorted, cbor_less);
  if (redeemer.index >= sorted.size()) {
    return std::optional<Cbor>{};
  }
  const auto* utxo = find_utxo(sorted[static_cast<std::size_t>(redeemer.index)], utxos);
  if (utxo == nullptr) {
    return std::unexpected(evaluation("missing spending UTxO"));
  }
  if (const auto* map = utxo->output.as_map()) {
    const auto* option = map_get(*map, 2U);
    const auto* fields = option == nullptr ? nullptr : option->as_array();
    if (fields != nullptr && fields->values.size() == 2U) {
      const auto kind = unsigned_value(fields->values[0]);
      if (kind && *kind == 1U) {
        return std::optional<Cbor>(fields->values[1]);
      }
      const auto* hash = fields->values[1].as_byte_string();
      if (kind && *kind == 0U && hash != nullptr) {
        return find_datum(hash->value, witnesses);
      }
    }
  }
  if (const auto* array = utxo->output.as_array(); array != nullptr && array->values.size() > 2U) {
    const auto* hash = array->values[2].as_byte_string();
    if (hash != nullptr) {
      return find_datum(hash->value, witnesses);
    }
  }
  return std::optional<Cbor>{};
}

[[nodiscard]] core::Result<std::map<std::uint8_t, std::vector<std::int64_t>>> parse_cost_models(
    core::ByteSpan bytes) {
  auto decoded = core::cbor::decode_cbor(bytes);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  const auto* map = decoded->as_map();
  if (map == nullptr) {
    return std::unexpected(malformed("CostModels must be a CBOR map"));
  }
  std::map<std::uint8_t, std::vector<std::int64_t>> output;
  for (const auto& [key, value] : map->entries) {
    const auto language = unsigned_value(key);
    const auto* parameters = value.as_array();
    if (!language || *language > 2U || parameters == nullptr) {
      return std::unexpected(malformed("invalid CostModels entry"));
    }
    std::vector<std::int64_t> parsed;
    parsed.reserve(parameters->values.size());
    for (const auto& parameter : parameters->values) {
      if (const auto* positive = parameter.as_unsigned()) {
        const auto converted = positive->value.to_int64();
        if (!converted) {
          return std::unexpected(converted.error());
        }
        parsed.push_back(*converted);
      } else if (const auto* negative = parameter.as_negative()) {
        const auto converted = negative->value.to_int64();
        if (!converted) {
          return std::unexpected(converted.error());
        }
        parsed.push_back(*converted);
      } else {
        return std::unexpected(malformed("cost parameter must be an integer"));
      }
    }
    output.insert_or_assign(static_cast<std::uint8_t>(*language), std::move(parsed));
  }
  return output;
}

[[nodiscard]] MachineCosts machine_costs(const std::vector<std::int64_t>& parameters,
                                         std::uint8_t language) {
  const auto value = [&](std::size_t index) {
    return index < parameters.size() ? parameters[index] : std::numeric_limits<std::int64_t>::max();
  };
  const auto pair = [&](std::size_t index) {
    return MachineBudget{value(index), value(index + 1U)};
  };
  const auto constr_index = language == 0U ? 175U : (language == 1U ? 185U : 193U);
  MachineCosts output;
  output.apply = pair(17U);
  output.builtin = pair(19U);
  output.constant = pair(21U);
  output.delay = pair(23U);
  output.force = pair(25U);
  output.lambda = pair(27U);
  output.startup = pair(29U);
  output.variable = pair(31U);
  output.constr = pair(constr_index);
  output.case_cost = pair(constr_index + 2U);
  return output;
}

[[nodiscard]] SemanticsVariant semantics_variant(std::uint8_t language, std::uint64_t protocol) {
  if (protocol < 9U) {
    return SemanticsVariant::a;
  }
  if (protocol < 11U) {
    return language == 2U ? SemanticsVariant::c : SemanticsVariant::b;
  }
  return language == 2U ? SemanticsVariant::e : SemanticsVariant::d;
}

[[nodiscard]] std::uint8_t maximum_builtin(std::uint8_t language, std::uint64_t protocol) {
  if (language == 0U) {
    return protocol >= 9U ? (protocol >= 11U ? 100U : 87U) : 50U;
  }
  if (language == 1U) {
    return protocol >= 11U ? 100U : (protocol >= 9U ? 87U : 53U);
  }
  return protocol >= 11U ? 100U : 87U;
}

[[nodiscard]] core::VoidResult validate_builtins(const UplcTerm& root, std::uint8_t maximum) {
  std::vector<const UplcTerm*> stack{&root};
  while (!stack.empty()) {
    const auto* current = stack.back();
    stack.pop_back();
    const auto valid = std::visit(
        [&](const auto& node) -> bool {
          using Node = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<Node, Builtin>) {
            return builtin_tag(node) <= maximum;
          } else if constexpr (std::is_same_v<Node, UplcDelay> || std::is_same_v<Node, UplcForce>) {
            stack.push_back(node.term.get());
          } else if constexpr (std::is_same_v<Node, UplcLambda>) {
            stack.push_back(node.body.get());
          } else if constexpr (std::is_same_v<Node, UplcApply>) {
            stack.push_back(node.function.get());
            stack.push_back(node.argument.get());
          } else if constexpr (std::is_same_v<Node, UplcConstr>) {
            for (const auto& field : node.fields) {
              stack.push_back(&field);
            }
          } else if constexpr (std::is_same_v<Node, UplcCase>) {
            stack.push_back(node.scrutinee.get());
            for (const auto& branch : node.branches) {
              stack.push_back(&branch);
            }
          }
          return true;
        },
        current->node());
    if (!valid) {
      return std::unexpected(evaluation("builtin is unavailable for this language/protocol"));
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::Result<core::Bytes> encode_redeemer(const LedgerRedeemer& redeemer,
                                                        MachineBudget budget) {
  return core::cbor::encode_cbor(
      Cbor::array({
          Cbor::unsigned_integer(core::BigInteger(static_cast<std::uint64_t>(redeemer.tag))),
          Cbor::unsigned_integer(core::BigInteger(redeemer.index)),
          redeemer.data,
          Cbor::array({
              Cbor::unsigned_integer(core::BigInteger(static_cast<std::uint64_t>(budget.memory))),
              Cbor::unsigned_integer(core::BigInteger(static_cast<std::uint64_t>(budget.cpu))),
          }),
      }),
      {.mode = core::cbor::Mode::canonical});
}

}  // namespace

core::Result<std::vector<PhaseTwoRawEvaluation>> eval_phase_two_raw(
    core::ByteSpan transaction_cbor, std::span<const PhaseTwoUtxo> utxo_bytes,
    core::ByteSpan cost_models_cbor, MachineBudget maximum, std::array<std::int64_t, 3> slot_config,
    std::uint64_t protocol_major, bool run_phase_one) {
  if (protocol_major < 5U || protocol_major > 11U) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::unsupported,
                           "protocol major " + std::to_string(protocol_major) + " is unsupported"));
  }
  if (maximum.cpu < 0 || maximum.memory < 0) {
    return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                              "phase-two maximum budget must be nonnegative"));
  }
  if (slot_config[2] <= 0) {
    return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                              "slot length milliseconds must be positive"));
  }
  auto transaction = core::cbor::decode_cbor(transaction_cbor);
  if (!transaction) {
    return std::unexpected(transaction.error());
  }
  const auto* transaction_fields = transaction->as_array();
  if (transaction_fields == nullptr || transaction_fields->values.size() < 3U) {
    return std::unexpected(malformed("transaction must be a ledger transaction array"));
  }
  const auto* body = transaction_fields->values[0].as_map();
  const auto* witnesses = transaction_fields->values[1].as_map();
  if (body == nullptr || witnesses == nullptr) {
    return std::unexpected(malformed("transaction body and witnesses must be maps"));
  }
  auto redeemers = parse_redeemers(map_get(*witnesses, 5U));
  if (!redeemers) {
    return std::unexpected(redeemers.error());
  }
  auto utxos = parse_utxos(utxo_bytes);
  if (!utxos) {
    return std::unexpected(utxos.error());
  }
  auto scripts = parse_scripts(*witnesses, *utxos);
  if (!scripts) {
    return std::unexpected(scripts.error());
  }
  auto cost_models = parse_cost_models(cost_models_cbor);
  if (!cost_models) {
    return std::unexpected(cost_models.error());
  }

  if (run_phase_one) {
    std::set<std::string> supplied;
    for (const auto& redeemer : *redeemers) {
      supplied.insert(pointer_key(redeemer));
    }
    std::set<std::string> needed;
    const auto* inputs = set_values(map_get(*body, 0U));
    if (inputs != nullptr) {
      auto sorted = *inputs;
      std::ranges::sort(sorted, cbor_less);
      for (std::size_t index = 0U; index < sorted.size(); ++index) {
        const auto* utxo = find_utxo(sorted[index], *utxos);
        if (utxo == nullptr) {
          continue;
        }
        const auto* address = payment_script_hash(utxo->output);
        if (address == nullptr) {
          continue;
        }
        const core::Bytes hash(address->begin() + 1, address->begin() + 29);
        if (std::ranges::any_of(*scripts,
                                [&](const Script& script) { return script.hash == hash; })) {
          needed.insert("0:" + std::to_string(index));
        }
      }
    }
    const auto known = [&](const std::optional<core::Bytes>& hash) {
      return hash && std::ranges::any_of(
                         *scripts, [&](const Script& script) { return script.hash == *hash; });
    };
    const auto collect = [&](std::uint8_t purpose, std::size_t count) -> core::VoidResult {
      for (std::size_t index = 0U; index < count; ++index) {
        const LedgerRedeemer candidate{purpose, static_cast<std::uint64_t>(index), Cbor::null()};
        auto hash = purpose_script_hash(candidate, *body, *utxos);
        if (!hash) {
          return std::unexpected(hash.error());
        }
        if (known(*hash)) {
          needed.insert(pointer_key(candidate));
        }
      }
      return std::monostate{};
    };
    if (const auto* mint = map_get(*body, 9U); mint != nullptr && mint->as_map() != nullptr) {
      auto collected = collect(1U, mint->as_map()->entries.size());
      if (!collected) {
        return std::unexpected(collected.error());
      }
    }
    for (const auto [purpose, key] :
         std::array<std::pair<std::uint8_t, std::uint64_t>, 2>{{{2U, 4U}, {5U, 20U}}}) {
      const auto* value = map_get(*body, key);
      const auto* values = value == nullptr ? nullptr : value->as_array();
      if (values != nullptr) {
        auto collected = collect(purpose, values->values.size());
        if (!collected) {
          return std::unexpected(collected.error());
        }
      }
    }
    for (const auto [purpose, key] :
         std::array<std::pair<std::uint8_t, std::uint64_t>, 2>{{{3U, 5U}, {4U, 19U}}}) {
      const auto* value = map_get(*body, key);
      const auto* map = value == nullptr ? nullptr : value->as_map();
      if (map != nullptr) {
        auto collected = collect(purpose, map->entries.size());
        if (!collected) {
          return std::unexpected(collected.error());
        }
      }
    }
    for (const auto& pointer : supplied) {
      if (!needed.contains(pointer)) {
        return std::unexpected(evaluation("extra redeemer " + pointer));
      }
    }
    for (const auto& pointer : needed) {
      if (!supplied.contains(pointer)) {
        return std::unexpected(evaluation("missing redeemer " + pointer));
      }
    }
  }

  std::vector<PhaseTwoRawEvaluation> output;
  for (const auto& redeemer : *redeemers) {
    auto script = resolve_script(redeemer, *body, *scripts, *utxos);
    if (!script) {
      return std::unexpected(script.error());
    }
    const auto minimum_protocol =
        (*script)->language == 0U ? 5U : ((*script)->language == 1U ? 7U : 9U);
    if (protocol_major < minimum_protocol) {
      return std::unexpected(core::CardanoError(
          core::ErrorCode::unsupported, "Plutus V" + std::to_string((*script)->language + 1U) +
                                            " is unavailable at protocol " +
                                            std::to_string(protocol_major)));
    }
    const auto model = cost_models->find((*script)->language);
    if (model == cost_models->end()) {
      return std::unexpected(evaluation("missing Plutus V" +
                                        std::to_string((*script)->language + 1U) + " cost model"));
    }
    ProgramDecodeOptions decode_options;
    decode_options.protocol_major = protocol_major;
    decode_options.enforce_data_wire_limit = (*script)->language != 0U;
    if (protocol_major >= 11U) {
      decode_options.max_universe_header = 32U;
      decode_options.max_constr_fields = 1'024U;
    }
    auto program = decode_program_envelope_compatible(
        (*script)->bytes, static_cast<std::uint64_t>((*script)->language) + 1U, decode_options);
    if (!program) {
      return std::unexpected(program.error());
    }
    if (protocol_major < 11U && program->version.minor >= 1U) {
      return std::unexpected(evaluation("UPLC 1.1.0 is unavailable before protocol 11"));
    }
    auto valid_builtins =
        validate_builtins(program->term, maximum_builtin((*script)->language, protocol_major));
    if (!valid_builtins) {
      return std::unexpected(valid_builtins.error());
    }

    auto redeemer_data = validate_plutus_data_node(redeemer.data);
    if (!redeemer_data) {
      return std::unexpected(redeemer_data.error());
    }
    std::vector<detail::ContextRedeemer> context_redeemers;
    context_redeemers.reserve(redeemers->size());
    for (const auto& candidate : *redeemers) {
      context_redeemers.push_back({candidate.tag, candidate.index, candidate.data});
    }
    std::vector<detail::ContextUtxo> context_utxos;
    context_utxos.reserve(utxos->size());
    for (const auto& candidate : *utxos) {
      context_utxos.push_back({candidate.input, candidate.output});
    }
    std::optional<Cbor> datum;
    if (redeemer.tag == 0U) {
      auto resolved_datum = spending_datum(redeemer, *body, *witnesses, *utxos);
      if (!resolved_datum) {
        return std::unexpected(resolved_datum.error());
      }
      datum = std::move(*resolved_datum);
    }
    auto context = detail::make_script_context(
        {redeemer.tag, redeemer.index, redeemer.data}, context_redeemers, *body, *witnesses,
        context_utxos, datum, slot_config, protocol_major, (*script)->language);
    if (!context) {
      return std::unexpected(context.error());
    }
    std::vector<Data> arguments;
    if ((*script)->language == 0U && datum) {
      auto validated = validate_plutus_data_node(*datum);
      if (!validated) {
        return std::unexpected(validated.error());
      }
      arguments.push_back(std::move(*validated));
    }
    if ((*script)->language != 2U) {
      arguments.push_back(std::move(*redeemer_data));
    }
    arguments.push_back(std::move(*context));
    auto term = program->term;
    for (auto& argument : arguments) {
      term = UplcTerm::apply(std::move(term),
                             UplcTerm::constant(UplcConstant::data(std::move(argument))));
    }
    auto result = evaluate_program(
        {program->version, std::move(term)}, maximum,
        semantics_variant((*script)->language, protocol_major),
        machine_costs(model->second, (*script)->language),
        make_builtin_cost_model(static_cast<PlutusLanguage>((*script)->language), model->second,
                                semantics_variant((*script)->language, protocol_major)));
    if (!result) {
      return std::unexpected(result.error());
    }
    if ((*script)->language == 2U) {
      const auto* constant = std::get_if<UplcConstant>(&result->result.node());
      if (constant == nullptr || constant->type().tag() != UplcTypeTag::unit) {
        return std::unexpected(evaluation("Plutus V3 script returned a non-Unit value"));
      }
    }
    auto encoded = encode_redeemer(redeemer, result->spent);
    if (!encoded) {
      return std::unexpected(encoded.error());
    }
    output.push_back({std::move(*encoded), {result->spent, std::move(result->logs)}});
  }
  return output;
}

}  // namespace cardano::plutus
