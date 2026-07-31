#include "phase_two_context.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cardano/crypto/primitives.hpp"

namespace cardano::plutus::detail {
namespace {

using Cbor = core::cbor::Value;
using Map = core::cbor::MapValue;

struct ContextFailure {
  core::CardanoError error;
};

[[noreturn]] void fail(std::string message) {
  throw ContextFailure{core::CardanoError(core::ErrorCode::evaluation, std::move(message))};
}

[[nodiscard]] Cbor integer(core::BigInteger value) {
  return value.is_negative() ? Cbor::negative_integer(std::move(value))
                             : Cbor::unsigned_integer(std::move(value));
}

[[nodiscard]] Cbor natural(std::uint64_t value) {
  return Cbor::unsigned_integer(core::BigInteger(value));
}

[[nodiscard]] Cbor bytes(core::Bytes value) { return Cbor::byte_string(std::move(value)); }

[[nodiscard]] Cbor array(std::vector<Cbor> values) { return Cbor::array(std::move(values)); }

[[nodiscard]] Cbor map(std::vector<std::pair<Cbor, Cbor>> entries) {
  return Cbor::map(std::move(entries));
}

[[nodiscard]] Cbor constr(std::uint64_t alternative, std::vector<Cbor> fields) {
  if (alternative <= 6U) {
    return Cbor::tag(core::BigInteger(121U + alternative), array(std::move(fields)));
  }
  if (alternative <= 127U) {
    return Cbor::tag(core::BigInteger(1280U + alternative - 7U), array(std::move(fields)));
  }
  return Cbor::tag(core::BigInteger(std::uint64_t{102}),
                   array({natural(alternative), array(std::move(fields))}));
}

[[nodiscard]] Cbor nothing() { return constr(1U, {}); }
[[nodiscard]] Cbor just(Cbor value) { return constr(0U, {std::move(value)}); }
[[nodiscard]] Cbor boolean_data(bool value) { return constr(value ? 1U : 0U, {}); }

[[nodiscard]] const Cbor* get(const Map& value, std::uint64_t key) {
  for (const auto& [candidate, item] : value.entries) {
    const auto* number = candidate.as_unsigned();
    if (number != nullptr && number->value == core::BigInteger(key)) {
      return &item;
    }
  }
  return nullptr;
}

[[nodiscard]] const std::vector<Cbor>* set_values(const Cbor* value) {
  if (value == nullptr) {
    return nullptr;
  }
  if (const auto* values = value->as_array()) {
    return &values->values;
  }
  const auto* tag = value->as_tag();
  if (tag == nullptr || tag->tag != core::BigInteger(std::uint64_t{258})) {
    return nullptr;
  }
  const auto* values = tag->value->as_array();
  return values == nullptr ? nullptr : &values->values;
}

[[nodiscard]] core::Bytes canonical(const Cbor& value) {
  auto encoded = core::cbor::encode_cbor(value, {.mode = core::cbor::Mode::canonical});
  if (!encoded) {
    throw ContextFailure{encoded.error()};
  }
  return std::move(*encoded);
}

[[nodiscard]] bool less(const Cbor& left, const Cbor& right) {
  return canonical(left) < canonical(right);
}

[[nodiscard]] const core::cbor::ArrayValue& ledger_array(const Cbor* value, std::string label) {
  const auto* fields = value == nullptr ? nullptr : value->as_array();
  if (fields == nullptr) {
    fail(std::move(label) + " must be an array");
  }
  return *fields;
}

[[nodiscard]] const core::Bytes& byte_value(const Cbor* value, std::string label) {
  const auto* result = value == nullptr ? nullptr : value->as_byte_string();
  if (result == nullptr) {
    fail(std::move(label) + " must be bytes");
  }
  return result->value;
}

[[nodiscard]] core::BigInteger integer_value(const Cbor* value) {
  if (value != nullptr) {
    if (const auto* positive = value->as_unsigned()) {
      return positive->value;
    }
    if (const auto* negative = value->as_negative()) {
      return negative->value;
    }
  }
  fail("expected integer");
}

[[nodiscard]] std::uint64_t unsigned_value(const Cbor* value, std::string label) {
  const auto* positive = value == nullptr ? nullptr : value->as_unsigned();
  if (positive == nullptr) {
    fail(std::move(label) + " must be unsigned");
  }
  auto converted = positive->value.to_uint64();
  if (!converted) {
    fail(std::move(label) + " is out of range");
  }
  return *converted;
}

[[nodiscard]] Cbor credential(bool script, core::Bytes hash) {
  if (hash.size() != 28U) {
    fail("credential hash must contain 28 bytes");
  }
  return constr(script ? 1U : 0U, {bytes(std::move(hash))});
}

[[nodiscard]] Cbor credential_node(const Cbor* value) {
  const auto& fields = ledger_array(value, "ledger credential").values;
  if (fields.size() != 2U) {
    fail("invalid ledger credential");
  }
  const auto kind = unsigned_value(&fields[0], "credential kind");
  if (kind > 1U) {
    fail("invalid ledger credential");
  }
  return credential(kind == 1U, byte_value(&fields[1], "credential hash"));
}

[[nodiscard]] Cbor credential_from_reward(const Cbor& value) {
  const auto& account = byte_value(&value, "reward account");
  if (account.size() != 29U) {
    fail("invalid reward account");
  }
  const auto kind = std::to_integer<std::uint8_t>(account.front()) >> 4U;
  if (kind != 0x0eU && kind != 0x0fU) {
    fail("invalid reward account");
  }
  return credential(kind == 0x0fU, core::Bytes(account.begin() + 1, account.end()));
}

[[nodiscard]] Cbor staking_credential_from_reward(const Cbor& value) {
  return constr(0U, {credential_from_reward(value)});
}

[[nodiscard]] std::array<core::BigInteger, 4> decode_pointer(core::ByteSpan value) {
  std::size_t offset = 0U;
  const auto read = [&]() {
    core::BigInteger output(std::uint64_t{0});
    for (std::size_t groups = 0U;; ++groups) {
      if (offset >= value.size() || groups >= 10U) {
        fail("invalid pointer address");
      }
      const auto octet = std::to_integer<std::uint8_t>(value[offset++]);
      output *= core::BigInteger(std::uint64_t{128});
      output += core::BigInteger(static_cast<std::uint64_t>(octet & 0x7fU));
      if ((octet & 0x80U) == 0U) {
        return output;
      }
    }
  };
  auto slot = read();
  auto transaction = read();
  auto certificate = read();
  return {std::move(slot), std::move(transaction), std::move(certificate),
          core::BigInteger(static_cast<std::uint64_t>(offset))};
}

[[nodiscard]] Cbor address_data(const core::Bytes& address) {
  if (address.size() < 29U) {
    fail("Byron or malformed address cannot appear in a Plutus context");
  }
  const auto kind = static_cast<std::uint8_t>(std::to_integer<std::uint8_t>(address.front()) >> 4U);
  auto payment =
      credential(kind % 2U == 1U, core::Bytes(address.begin() + 1, address.begin() + 29));
  Cbor stake = nothing();
  if (kind <= 3U) {
    if (address.size() != 57U) {
      fail("invalid base address");
    }
    stake = just(
        constr(0U, {credential(kind >= 2U, core::Bytes(address.begin() + 29, address.end()))}));
  } else if (kind == 4U || kind == 5U) {
    const auto pointer = decode_pointer(core::ByteSpan(address).subspan(29U));
    const auto consumed = pointer[3].to_uint64();
    if (!consumed || *consumed != address.size() - 29U) {
      fail("invalid pointer address");
    }
    stake = just(constr(1U, {integer(pointer[0]), integer(pointer[1]), integer(pointer[2])}));
  } else if (kind == 6U || kind == 7U) {
    if (address.size() != 29U) {
      fail("invalid enterprise address");
    }
  } else {
    fail("Byron or reward address cannot appear in a transaction output");
  }
  return constr(0U, {std::move(payment), std::move(stake)});
}

struct LedgerValue {
  core::BigInteger coin;
  std::vector<std::pair<Cbor, Cbor>> assets;
};

[[nodiscard]] LedgerValue read_value(const Cbor* value, bool require_coin) {
  if (value == nullptr) {
    if (require_coin) {
      fail("missing coin value");
    }
    return {core::BigInteger(std::uint64_t{0}), {}};
  }
  if (const auto* coin = value->as_unsigned()) {
    return {coin->value, {}};
  }
  const auto* fields = value->as_array();
  if (fields == nullptr || fields->values.size() != 2U ||
      fields->values[0].as_unsigned() == nullptr || fields->values[1].as_map() == nullptr) {
    fail("invalid ledger value");
  }
  return {fields->values[0].as_unsigned()->value, fields->values[1].as_map()->entries};
}

[[nodiscard]] Cbor value_data(const Cbor* value, bool include_ada, bool require_coin) {
  auto decoded = read_value(value, require_coin);
  std::vector<std::pair<Cbor, Cbor>> entries;
  if (include_ada) {
    entries.emplace_back(bytes({}), map({{bytes({}), integer(decoded.coin)}}));
  }
  for (auto& [policy, tokens] : decoded.assets) {
    const auto* policy_bytes = policy.as_byte_string();
    const auto* token_map = tokens.as_map();
    if (policy_bytes == nullptr || token_map == nullptr) {
      fail("invalid multiasset value");
    }
    std::vector<std::pair<Cbor, Cbor>> quantities;
    for (const auto& [asset, quantity] : token_map->entries) {
      const auto* asset_bytes = asset.as_byte_string();
      if (asset_bytes == nullptr) {
        fail("invalid asset name");
      }
      quantities.emplace_back(bytes(asset_bytes->value), integer(integer_value(&quantity)));
    }
    entries.emplace_back(bytes(policy_bytes->value), map(std::move(quantities)));
  }
  return map(std::move(entries));
}

[[nodiscard]] Cbor mint_data(const Cbor* value, bool include_ada) {
  if (value == nullptr) {
    return value_data(nullptr, include_ada, false);
  }
  const auto* assets = value->as_map();
  if (assets == nullptr) {
    fail("mint must be a multiasset map");
  }
  auto wrapped = array({natural(0U), map(assets->entries)});
  return value_data(&wrapped, include_ada, true);
}

[[nodiscard]] const Cbor* output_field(const Cbor& output, std::uint64_t key, std::size_t index) {
  if (const auto* fields = output.as_map()) {
    return get(*fields, key);
  }
  const auto* fields = output.as_array();
  return fields != nullptr && index < fields->values.size() ? &fields->values[index] : nullptr;
}

[[nodiscard]] Cbor output_datum(const Cbor& output) {
  if (const auto* fields = output.as_array()) {
    if (fields->values.size() > 2U) {
      if (const auto* hash = fields->values[2].as_byte_string()) {
        return constr(1U, {bytes(hash->value)});
      }
    }
    return constr(0U, {});
  }
  const auto* fields = output.as_map();
  if (fields == nullptr) {
    return constr(0U, {});
  }
  const auto* datum = get(*fields, 2U);
  if (datum == nullptr) {
    return constr(0U, {});
  }
  const auto* option = datum->as_array();
  if (option == nullptr || option->values.size() != 2U) {
    fail("invalid Babbage output datum option");
  }
  const auto kind = unsigned_value(&option->values[0], "output datum kind");
  if (kind == 0U && option->values[1].as_byte_string() != nullptr) {
    return constr(1U, {bytes(option->values[1].as_byte_string()->value)});
  }
  if (kind == 1U) {
    return constr(2U, {option->values[1]});
  }
  fail("invalid Babbage output datum option");
}

[[nodiscard]] Cbor reference_script_hash(const Cbor& output) {
  const auto* fields = output.as_map();
  const auto* reference = fields == nullptr ? nullptr : get(*fields, 3U);
  if (reference == nullptr) {
    return nothing();
  }
  const auto* tag = reference->as_tag();
  const auto* embedded = tag == nullptr ? nullptr : tag->value->as_byte_string();
  if (tag == nullptr || tag->tag != core::BigInteger(std::uint64_t{24}) || embedded == nullptr) {
    fail("invalid reference script");
  }
  auto decoded = core::cbor::decode_cbor(embedded->value);
  if (!decoded) {
    throw ContextFailure{decoded.error()};
  }
  const auto* script = decoded->as_array();
  if (script == nullptr || script->values.size() != 2U ||
      script->values[0].as_unsigned() == nullptr || script->values[1].as_byte_string() == nullptr) {
    fail("invalid reference script");
  }
  const auto prefix = unsigned_value(&script->values[0], "reference script language");
  if (prefix > 3U) {
    fail("invalid reference script language");
  }
  core::Bytes hash_input{static_cast<std::byte>(prefix)};
  const auto& program = script->values[1].as_byte_string()->value;
  hash_input.insert(hash_input.end(), program.begin(), program.end());
  return just(bytes(crypto::blake2b224(hash_input)));
}

[[nodiscard]] Cbor tx_out(const Cbor& output, bool v2) {
  const auto* address = output_field(output, 0U, 0U);
  const auto* value = output_field(output, 1U, 1U);
  if (address == nullptr || address->as_byte_string() == nullptr || value == nullptr) {
    fail("invalid transaction output");
  }
  if (!v2) {
    Cbor datum = nothing();
    if (const auto* fields = output.as_array(); fields != nullptr && fields->values.size() > 2U &&
                                                fields->values[2].as_byte_string() != nullptr) {
      datum = just(bytes(fields->values[2].as_byte_string()->value));
    }
    return constr(0U, {address_data(address->as_byte_string()->value),
                       value_data(value, true, true), std::move(datum)});
  }
  return constr(0U, {address_data(address->as_byte_string()->value), value_data(value, true, true),
                     output_datum(output), reference_script_hash(output)});
}

[[nodiscard]] Cbor tx_out_ref(const Cbor& value, bool v3) {
  const auto& fields = ledger_array(&value, "transaction input").values;
  if (fields.size() != 2U || fields[0].as_byte_string() == nullptr ||
      fields[1].as_unsigned() == nullptr) {
    fail("invalid transaction input");
  }
  auto id = bytes(fields[0].as_byte_string()->value);
  if (!v3) {
    id = constr(0U, {std::move(id)});
  }
  return constr(0U, {std::move(id), integer(fields[1].as_unsigned()->value)});
}

[[nodiscard]] const ContextUtxo* find_utxo(const Cbor& input,
                                           const std::vector<ContextUtxo>& utxos) {
  const auto identity = canonical(input);
  for (const auto& utxo : utxos) {
    if (canonical(utxo.input) == identity) {
      return &utxo;
    }
  }
  return nullptr;
}

[[nodiscard]] std::vector<Cbor> input_infos(const Map& body, const std::vector<ContextUtxo>& utxos,
                                            std::uint64_t key, bool v2_output, bool v3_id = false) {
  const auto* inputs = get(body, key);
  if (inputs == nullptr) {
    return {};
  }
  const auto* values = set_values(inputs);
  if (values == nullptr) {
    fail("transaction input field must be a set");
  }
  auto sorted = *values;
  std::ranges::sort(sorted, less);
  std::vector<Cbor> output;
  for (const auto& input : sorted) {
    const auto* resolved = find_utxo(input, utxos);
    if (resolved == nullptr) {
      fail("transaction input does not exist in the supplied UTxO");
    }
    output.push_back(constr(0U, {tx_out_ref(input, v3_id), tx_out(resolved->output, v2_output)}));
  }
  return output;
}

[[nodiscard]] std::vector<Cbor> body_array(const Map& body, std::uint64_t key, std::string label) {
  const auto* value = get(body, key);
  if (value == nullptr) {
    return {};
  }
  const auto* values = value->as_array();
  if (values == nullptr) {
    fail(std::move(label) + " must be an array");
  }
  return values->values;
}

[[nodiscard]] std::vector<std::pair<Cbor, Cbor>> withdrawals(const Map& body) {
  const auto* value = get(body, 5U);
  if (value == nullptr) {
    return {};
  }
  const auto* entries = value->as_map();
  if (entries == nullptr) {
    fail("withdrawals must be a map");
  }
  auto output = entries->entries;
  std::ranges::sort(
      output, [](const auto& left, const auto& right) { return less(left.first, right.first); });
  return output;
}

[[nodiscard]] std::vector<Cbor> sorted_set(const Cbor* value, std::string label) {
  if (value == nullptr) {
    return {};
  }
  const auto* values = set_values(value);
  if (values == nullptr) {
    fail(std::move(label) + " must be a set");
  }
  auto output = *values;
  std::ranges::sort(output, less);
  return output;
}

[[nodiscard]] Cbor validity_range(const Map& body, std::array<std::int64_t, 3> slots) {
  const auto convert = [&](const Cbor* value) {
    const auto slot = unsigned_value(value, "validity bound");
    return core::BigInteger(slots[0]) +
           (core::BigInteger(slot) - core::BigInteger(slots[1])) * core::BigInteger(slots[2]);
  };
  const auto* lower = get(body, 8U);
  const auto* upper = get(body, 3U);
  auto lower_bound = lower == nullptr
                         ? constr(0U, {constr(0U, {}), boolean_data(true)})
                         : constr(0U, {constr(1U, {integer(convert(lower))}), boolean_data(true)});
  auto upper_bound = upper == nullptr
                         ? constr(0U, {constr(2U, {}), boolean_data(true)})
                         : constr(0U, {constr(1U, {integer(convert(upper))}), boolean_data(false)});
  return constr(0U, {std::move(lower_bound), std::move(upper_bound)});
}

[[nodiscard]] Cbor tx_id(const Map& body, bool v3) {
  auto encoded = core::cbor::encode_cbor(Cbor(std::make_shared<Map>(body)),
                                         {.mode = core::cbor::Mode::preserve});
  if (!encoded) {
    throw ContextFailure{encoded.error()};
  }
  auto hash = bytes(crypto::blake2b256(*encoded));
  return v3 ? hash : constr(0U, {std::move(hash)});
}

[[nodiscard]] Cbor legacy_certificate(const Cbor& certificate, std::uint64_t protocol) {
  const auto& fields = ledger_array(&certificate, "certificate").values;
  const auto tag = unsigned_value(fields.empty() ? nullptr : &fields[0], "certificate tag");
  const auto stake = [&]() {
    return constr(0U, {credential_node(fields.size() > 1U ? &fields[1] : nullptr)});
  };
  switch (tag) {
    case 0U:
    case 7U:
      return constr(0U, {stake()});
    case 1U:
    case 8U:
      return constr(1U, {stake()});
    case 2U:
      return constr(2U, {stake(), bytes(byte_value(fields.size() > 2U ? &fields[2] : nullptr,
                                                   "stake pool hash"))});
    case 3U:
      return constr(
          3U, {bytes(byte_value(fields.size() > 1U ? &fields[1] : nullptr, "stake pool hash")),
               bytes(byte_value(fields.size() > 2U ? &fields[2] : nullptr, "VRF hash"))});
    case 4U:
      return constr(
          4U, {bytes(byte_value(fields.size() > 1U ? &fields[1] : nullptr, "stake pool hash")),
               integer(integer_value(fields.size() > 2U ? &fields[2] : nullptr))});
    case 5U:
      return constr(5U, {});
    case 6U:
      return constr(6U, {});
    default:
      fail("certificate " + std::to_string(tag) +
           " cannot be represented in a Plutus V1/V2 context at protocol " +
           std::to_string(protocol));
  }
}

[[nodiscard]] Cbor drep_data(const Cbor* value) {
  const auto& fields = ledger_array(value, "DRep").values;
  const auto tag = unsigned_value(fields.empty() ? nullptr : &fields[0], "DRep tag");
  if (tag == 0U || tag == 1U) {
    return constr(0U, {credential(tag == 1U, byte_value(fields.size() > 1U ? &fields[1] : nullptr,
                                                        "DRep hash"))});
  }
  if (tag == 2U) {
    return constr(1U, {});
  }
  if (tag == 3U) {
    return constr(2U, {});
  }
  fail("invalid DRep");
}

[[nodiscard]] Cbor v3_certificate(const Cbor& certificate, std::uint64_t protocol) {
  const auto& fields = ledger_array(&certificate, "certificate").values;
  const auto field = [&](std::size_t index) -> const Cbor* {
    return index < fields.size() ? &fields[index] : nullptr;
  };
  const auto tag = unsigned_value(field(0U), "certificate tag");
  const auto cred = [&]() { return credential_node(field(1U)); };
  const auto pool = [&](std::size_t index = 2U) {
    return bytes(byte_value(field(index), "stake pool hash"));
  };
  switch (tag) {
    case 0U:
      return constr(0U, {cred(), nothing()});
    case 1U:
      return constr(1U, {cred(), nothing()});
    case 2U:
      return constr(2U, {cred(), constr(0U, {pool()})});
    case 3U:
      return constr(7U, {pool(1U), bytes(byte_value(field(2U), "VRF hash"))});
    case 4U:
      return constr(8U, {pool(1U), integer(integer_value(field(2U)))});
    case 7U:
      return constr(0U,
                    {cred(), protocol == 9U ? nothing() : just(integer(integer_value(field(2U))))});
    case 8U:
      return constr(1U,
                    {cred(), protocol == 9U ? nothing() : just(integer(integer_value(field(2U))))});
    case 9U:
      return constr(2U, {cred(), constr(1U, {drep_data(field(2U))})});
    case 10U:
      return constr(2U, {cred(), constr(2U, {pool(), drep_data(field(3U))})});
    case 11U:
      return constr(3U, {cred(), constr(0U, {pool()}), integer(integer_value(field(3U)))});
    case 12U:
      return constr(
          3U, {cred(), constr(1U, {drep_data(field(2U))}), integer(integer_value(field(3U)))});
    case 13U:
      return constr(3U, {cred(), constr(2U, {pool(), drep_data(field(3U))}),
                         integer(integer_value(field(4U)))});
    case 14U:
      return constr(9U, {credential_node(field(1U)), credential_node(field(2U))});
    case 15U:
      return constr(10U, {credential_node(field(1U))});
    case 16U:
      return constr(4U, {credential_node(field(1U)), integer(integer_value(field(2U)))});
    case 17U:
      return constr(6U, {credential_node(field(1U)), integer(integer_value(field(2U)))});
    case 18U:
      return constr(5U, {credential_node(field(1U))});
    default:
      fail("unsupported Conway certificate " + std::to_string(tag));
  }
}

[[nodiscard]] Cbor voter_data(const Cbor& value) {
  const auto& fields = ledger_array(&value, "voter").values;
  const auto tag = unsigned_value(fields.empty() ? nullptr : &fields[0], "voter tag");
  const auto* hash = fields.size() > 1U ? &fields[1] : nullptr;
  if (tag <= 1U) {
    return constr(0U, {credential(tag == 1U, byte_value(hash, "committee voter hash"))});
  }
  if (tag == 2U || tag == 3U) {
    return constr(1U, {credential(tag == 3U, byte_value(hash, "DRep voter hash"))});
  }
  if (tag == 4U) {
    return constr(2U, {bytes(byte_value(hash, "stake pool hash"))});
  }
  fail("invalid voter");
}

[[nodiscard]] Cbor governance_action_id(const Cbor& value) {
  const auto& fields = ledger_array(&value, "governance action id").values;
  if (fields.size() < 2U) {
    fail("invalid governance action id");
  }
  return constr(0U, {bytes(byte_value(&fields[0], "governance action transaction id")),
                     integer(integer_value(&fields[1]))});
}

[[nodiscard]] Cbor maybe_script_hash(const Cbor* value) {
  return value == nullptr || std::holds_alternative<core::cbor::NullValue>(value->node())
             ? nothing()
             : just(bytes(byte_value(value, "script hash")));
}

[[nodiscard]] Cbor maybe_governance_action(const Cbor* value) {
  return value == nullptr || std::holds_alternative<core::cbor::NullValue>(value->node())
             ? nothing()
             : just(governance_action_id(*value));
}

[[nodiscard]] Cbor rational_data(const Cbor* value) {
  const core::cbor::ArrayValue* values = nullptr;
  if (value != nullptr && value->as_tag() != nullptr &&
      value->as_tag()->tag == core::BigInteger(std::uint64_t{30})) {
    values = value->as_tag()->value->as_array();
  } else if (value != nullptr) {
    values = value->as_array();
  }
  if (values == nullptr || values->values.size() != 2U) {
    fail("invalid rational");
  }
  return constr(
      0U, {integer(integer_value(&values->values[0])), integer(integer_value(&values->values[1]))});
}

[[nodiscard]] Cbor ledger_parameter_data(const Cbor& value) {
  if (value.as_unsigned() != nullptr || value.as_negative() != nullptr ||
      value.as_byte_string() != nullptr || value.as_map() != nullptr) {
    return value;
  }
  if (const auto* values = value.as_array()) {
    std::vector<Cbor> translated;
    for (const auto& item : values->values) {
      translated.push_back(ledger_parameter_data(item));
    }
    return array(std::move(translated));
  }
  fail("unsupported changed parameter value");
}

[[nodiscard]] Cbor changed_parameters(const Cbor* value) {
  if (value == nullptr) {
    fail("missing changed parameters");
  }
  const auto* entries = value->as_map();
  if (entries == nullptr) {
    return *value;
  }
  std::vector<std::pair<Cbor, Cbor>> output;
  for (const auto& [key, item] : entries->entries) {
    output.emplace_back(integer(integer_value(&key)), ledger_parameter_data(item));
  }
  return map(std::move(output));
}

[[nodiscard]] Cbor governance_action_data(const Cbor& value) {
  const auto& fields = ledger_array(&value, "governance action").values;
  const auto field = [&](std::size_t index) -> const Cbor* {
    return index < fields.size() ? &fields[index] : nullptr;
  };
  const auto tag = unsigned_value(field(0U), "governance action tag");
  switch (tag) {
    case 0U:
      return constr(0U, {maybe_governance_action(field(1U)), changed_parameters(field(2U)),
                         maybe_script_hash(field(3U))});
    case 1U: {
      const auto& version = ledger_array(field(2U), "protocol version").values;
      return constr(
          1U, {maybe_governance_action(field(1U)),
               constr(0U, {integer(integer_value(version.empty() ? nullptr : &version[0])),
                           integer(integer_value(version.size() < 2U ? nullptr : &version[1]))})});
    }
    case 2U: {
      const auto* withdrawals = field(1U) == nullptr ? nullptr : field(1U)->as_map();
      if (withdrawals == nullptr) {
        fail("treasury withdrawals must be a map");
      }
      std::vector<std::pair<Cbor, Cbor>> entries;
      for (const auto& [account, amount] : withdrawals->entries) {
        entries.emplace_back(credential_from_reward(account), integer(integer_value(&amount)));
      }
      return constr(2U, {map(std::move(entries)), maybe_script_hash(field(2U))});
    }
    case 3U:
      return constr(3U, {maybe_governance_action(field(1U))});
    case 4U: {
      const auto* removed = field(2U) == nullptr ? nullptr : field(2U)->as_array();
      const auto* added = field(3U) == nullptr ? nullptr : field(3U)->as_map();
      if (removed == nullptr || added == nullptr) {
        fail("invalid committee update");
      }
      std::vector<Cbor> removed_data;
      for (const auto& item : removed->values) {
        removed_data.push_back(constr(0U, {credential_node(&item)}));
      }
      std::vector<std::pair<Cbor, Cbor>> added_data;
      for (const auto& [cred, epoch] : added->entries) {
        added_data.emplace_back(constr(0U, {credential_node(&cred)}),
                                integer(integer_value(&epoch)));
      }
      return constr(4U, {maybe_governance_action(field(1U)), array(std::move(removed_data)),
                         map(std::move(added_data)), rational_data(field(4U))});
    }
    case 5U: {
      const auto& constitution = ledger_array(field(2U), "constitution").values;
      return constr(
          5U,
          {maybe_governance_action(field(1U)),
           constr(0U, {maybe_script_hash(constitution.size() < 2U ? nullptr : &constitution[1])})});
    }
    case 6U:
      return constr(6U, {});
    default:
      fail("invalid governance action");
  }
}

[[nodiscard]] Cbor proposal_data(const Cbor& value) {
  const auto& fields = ledger_array(&value, "proposal procedure").values;
  if (fields.size() < 3U) {
    fail("invalid proposal procedure");
  }
  return constr(0U, {integer(integer_value(&fields[0])), credential_from_reward(fields[1]),
                     governance_action_data(fields[2])});
}

[[nodiscard]] Cbor votes_data(const Cbor* value) {
  if (value == nullptr) {
    return map({});
  }
  const auto* voters = value->as_map();
  if (voters == nullptr) {
    fail("voting procedures must be a map");
  }
  std::vector<std::pair<Cbor, Cbor>> output;
  for (const auto& [voter, procedures] : voters->entries) {
    const auto* procedure_map = procedures.as_map();
    if (procedure_map == nullptr) {
      fail("voting procedure must be a map");
    }
    std::vector<std::pair<Cbor, Cbor>> translated;
    for (const auto& [action, procedure] : procedure_map->entries) {
      const auto& fields = ledger_array(&procedure, "voting procedure").values;
      const auto vote = unsigned_value(fields.empty() ? nullptr : &fields[0], "vote");
      if (vote > 2U) {
        fail("invalid vote");
      }
      translated.emplace_back(governance_action_id(action), constr(vote, {}));
    }
    output.emplace_back(voter_data(voter), map(std::move(translated)));
  }
  return map(std::move(output));
}

[[nodiscard]] Cbor script_purpose(const ContextRedeemer& redeemer, const Map& body,
                                  const std::vector<ContextUtxo>& utxos, std::uint8_t language,
                                  std::uint64_t protocol) {
  static_cast<void>(utxos);
  const auto indexed_set = [&](const Cbor* value, std::string label) {
    const auto* values = set_values(value);
    if (values == nullptr) {
      fail(std::move(label) + " set is missing");
    }
    auto sorted = *values;
    std::ranges::sort(sorted, less);
    if (redeemer.index >= sorted.size()) {
      fail(std::move(label) + " index is out of range");
    }
    return sorted[static_cast<std::size_t>(redeemer.index)];
  };
  switch (redeemer.tag) {
    case 0U:
      return constr(1U, {tx_out_ref(indexed_set(get(body, 0U), "spending input"), language == 2U)});
    case 1U: {
      const auto* mint = get(body, 9U);
      const auto* assets = mint == nullptr ? nullptr : mint->as_map();
      if (assets == nullptr) {
        fail("minting redeemer points to no policy");
      }
      std::vector<Cbor> policies;
      for (const auto& [policy, tokens] : assets->entries) {
        static_cast<void>(tokens);
        policies.push_back(policy);
      }
      std::ranges::sort(policies, less);
      if (redeemer.index >= policies.size() ||
          policies[static_cast<std::size_t>(redeemer.index)].as_byte_string() == nullptr) {
        fail("minting redeemer points to no policy");
      }
      return constr(
          0U, {bytes(policies[static_cast<std::size_t>(redeemer.index)].as_byte_string()->value)});
    }
    case 2U: {
      const auto certificates = body_array(body, 4U, "certificate list");
      if (redeemer.index >= certificates.size()) {
        fail("certificate index is out of range");
      }
      const auto& certificate = certificates[static_cast<std::size_t>(redeemer.index)];
      const auto translated = language == 2U ? v3_certificate(certificate, protocol)
                                             : legacy_certificate(certificate, protocol);
      return language == 2U ? constr(3U, {natural(redeemer.index), translated})
                            : constr(3U, {translated});
    }
    case 3U: {
      const auto entries = withdrawals(body);
      if (redeemer.index >= entries.size()) {
        fail("rewarding redeemer points to no withdrawal");
      }
      auto credential_value =
          language == 2U
              ? credential_from_reward(entries[static_cast<std::size_t>(redeemer.index)].first)
              : staking_credential_from_reward(
                    entries[static_cast<std::size_t>(redeemer.index)].first);
      return constr(2U, {std::move(credential_value)});
    }
    case 4U: {
      if (language != 2U) {
        fail("voting purpose is unavailable to Plutus V1/V2");
      }
      const auto* votes = get(body, 19U);
      const auto* entries = votes == nullptr ? nullptr : votes->as_map();
      if (entries == nullptr) {
        fail("voting redeemer points to no voter");
      }
      std::vector<Cbor> voters;
      for (const auto& [voter, procedures] : entries->entries) {
        static_cast<void>(procedures);
        voters.push_back(voter);
      }
      std::ranges::sort(voters, less);
      if (redeemer.index >= voters.size()) {
        fail("voting redeemer points to no voter");
      }
      return constr(4U, {voter_data(voters[static_cast<std::size_t>(redeemer.index)])});
    }
    case 5U: {
      if (language != 2U) {
        fail("proposing purpose is unavailable to Plutus V1/V2");
      }
      const auto proposals = body_array(body, 20U, "proposal list");
      if (redeemer.index >= proposals.size()) {
        fail("proposal index is out of range");
      }
      return constr(5U, {natural(redeemer.index),
                         proposal_data(proposals[static_cast<std::size_t>(redeemer.index)])});
    }
    default:
      fail("invalid redeemer purpose");
  }
}

[[nodiscard]] Cbor datum_witness_map(const Map& witnesses) {
  const auto* datum_value = get(witnesses, 4U);
  const auto* datum_set = set_values(datum_value);
  if (datum_value != nullptr && datum_set == nullptr) {
    fail("datum witnesses must be a set");
  }
  auto datums = datum_set == nullptr ? std::vector<Cbor>{} : *datum_set;
  std::ranges::sort(datums, [](const Cbor& left, const Cbor& right) {
    auto left_data = validate_plutus_data_node(left);
    auto right_data = validate_plutus_data_node(right);
    if (!left_data || !right_data) {
      fail("invalid datum witness");
    }
    auto left_cbor = encode_plutus_data(*left_data);
    auto right_cbor = encode_plutus_data(*right_data);
    if (!left_cbor || !right_cbor) {
      fail("invalid datum witness");
    }
    return crypto::blake2b256(*left_cbor) < crypto::blake2b256(*right_cbor);
  });
  std::vector<std::pair<Cbor, Cbor>> entries;
  for (auto& datum : datums) {
    auto validated = validate_plutus_data_node(datum);
    if (!validated) {
      throw ContextFailure{validated.error()};
    }
    auto encoded = encode_plutus_data(*validated);
    if (!encoded) {
      throw ContextFailure{encoded.error()};
    }
    entries.emplace_back(bytes(crypto::blake2b256(*encoded)), std::move(datum));
  }
  return map(std::move(entries));
}

[[nodiscard]] Cbor make_tx_info(const std::vector<ContextRedeemer>& redeemers, const Map& body,
                                const Map& witnesses, const std::vector<ContextUtxo>& utxos,
                                std::array<std::int64_t, 3> slots, std::uint64_t protocol,
                                std::uint8_t language) {
  std::vector<Cbor> outputs;
  for (const auto& output : body_array(body, 1U, "transaction outputs")) {
    outputs.push_back(tx_out(output, language != 0U));
  }
  std::vector<Cbor> certificates;
  for (const auto& certificate : body_array(body, 4U, "transaction certificates")) {
    certificates.push_back(language == 2U ? v3_certificate(certificate, protocol)
                                          : legacy_certificate(certificate, protocol));
  }
  std::vector<Cbor> signatories;
  for (const auto& signer : sorted_set(get(body, 14U), "required signers")) {
    signatories.push_back(bytes(byte_value(&signer, "required signer")));
  }
  std::vector<std::pair<Cbor, Cbor>> withdrawal_entries;
  std::vector<Cbor> withdrawal_list;
  for (const auto& [account, amount] : withdrawals(body)) {
    auto translated =
        language == 2U ? credential_from_reward(account) : staking_credential_from_reward(account);
    auto quantity = integer(integer_value(&amount));
    if (language == 0U) {
      withdrawal_list.push_back(constr(0U, {std::move(translated), std::move(quantity)}));
    } else {
      withdrawal_entries.emplace_back(std::move(translated), std::move(quantity));
    }
  }
  std::vector<std::pair<Cbor, Cbor>> redeemer_entries;
  if (language != 0U) {
    for (const auto& redeemer : redeemers) {
      redeemer_entries.emplace_back(script_purpose(redeemer, body, utxos, language, protocol),
                                    redeemer.data);
    }
  }
  auto datum_map = datum_witness_map(witnesses);
  if (language == 0U) {
    std::vector<Cbor> datum_list;
    for (const auto& [hash, datum] : datum_map.as_map()->entries) {
      datum_list.push_back(constr(0U, {hash, datum}));
    }
    return constr(0U, {array(input_infos(body, utxos, 0U, false)), array(std::move(outputs)),
                       value_data(get(body, 2U), true, true), mint_data(get(body, 9U), true),
                       array(std::move(certificates)), array(std::move(withdrawal_list)),
                       validity_range(body, slots), array(std::move(signatories)),
                       array(std::move(datum_list)), tx_id(body, false)});
  }
  if (language == 1U) {
    return constr(0U, {array(input_infos(body, utxos, 0U, true)),
                       array(input_infos(body, utxos, 18U, true)), array(std::move(outputs)),
                       value_data(get(body, 2U), true, true), mint_data(get(body, 9U), true),
                       array(std::move(certificates)), map(std::move(withdrawal_entries)),
                       validity_range(body, slots), array(std::move(signatories)),
                       map(std::move(redeemer_entries)), std::move(datum_map), tx_id(body, false)});
  }
  std::vector<Cbor> proposals;
  for (const auto& proposal : body_array(body, 20U, "proposal procedures")) {
    proposals.push_back(proposal_data(proposal));
  }
  const auto* treasury = get(body, 21U);
  const auto* donation = get(body, 22U);
  auto donation_data = nothing();
  if (donation != nullptr) {
    const auto* quantity = donation->as_unsigned();
    if (quantity != nullptr && !quantity->value.is_zero()) {
      donation_data = just(integer(quantity->value));
    }
  }
  return constr(0U, {array(input_infos(body, utxos, 0U, true, true)),
                     array(input_infos(body, utxos, 18U, true, true)), array(std::move(outputs)),
                     integer(integer_value(get(body, 2U))), mint_data(get(body, 9U), false),
                     array(std::move(certificates)), map(std::move(withdrawal_entries)),
                     validity_range(body, slots), array(std::move(signatories)),
                     map(std::move(redeemer_entries)), std::move(datum_map), tx_id(body, true),
                     votes_data(get(body, 19U)), array(std::move(proposals)),
                     treasury == nullptr ? nothing() : just(integer(integer_value(treasury))),
                     std::move(donation_data)});
}

void assert_v1_features(const Map& body, const std::vector<ContextUtxo>& utxos) {
  const auto* references = set_values(get(body, 18U));
  if (references != nullptr && !references->empty()) {
    fail("reference inputs are unavailable to Plutus V1");
  }
  auto outputs = body_array(body, 1U, "transaction outputs");
  for (const auto& utxo : utxos) {
    outputs.push_back(utxo.output);
  }
  for (const auto& output : outputs) {
    const auto* fields = output.as_map();
    if (fields == nullptr) {
      continue;
    }
    const auto* datum = get(*fields, 2U);
    const auto* option = datum == nullptr ? nullptr : datum->as_array();
    if (option != nullptr && !option->values.empty() &&
        option->values[0].as_unsigned() != nullptr &&
        option->values[0].as_unsigned()->value == core::BigInteger(std::uint64_t{1})) {
      fail("inline datums are unavailable to Plutus V1");
    }
    if (get(*fields, 3U) != nullptr) {
      fail("reference scripts are unavailable to Plutus V1");
    }
  }
}

void assert_disjoint_inputs(const Map& body) {
  const auto* inputs = set_values(get(body, 0U));
  const auto* references = set_values(get(body, 18U));
  if (inputs == nullptr || references == nullptr) {
    return;
  }
  for (const auto& input : *inputs) {
    const auto encoded = canonical(input);
    if (std::ranges::any_of(
            *references, [&](const Cbor& reference) { return canonical(reference) == encoded; })) {
      fail("reference inputs must be disjoint from spending inputs at protocol 11");
    }
  }
}

}  // namespace

core::Result<Data> make_script_context(
    const ContextRedeemer& current, const std::vector<ContextRedeemer>& redeemers, const Map& body,
    const Map& witnesses, const std::vector<ContextUtxo>& utxos, std::optional<Cbor> current_datum,
    std::array<std::int64_t, 3> slot_config, std::uint64_t protocol, std::uint8_t language) {
  try {
    if (language == 0U && protocol >= 7U) {
      assert_v1_features(body, utxos);
    }
    if (language == 2U && protocol >= 11U) {
      assert_disjoint_inputs(body);
    }
    auto tx_info = make_tx_info(redeemers, body, witnesses, utxos, slot_config, protocol, language);
    auto purpose = script_purpose(current, body, utxos, language, protocol);
    Cbor context = Cbor::null();
    if (language == 2U) {
      const auto* purpose_tag = purpose.as_tag();
      if (purpose_tag == nullptr) {
        fail("invalid script purpose");
      }
      auto script_fields = ledger_array(purpose_tag->value.get(), "script purpose").values;
      if (current.tag == 0U) {
        script_fields.push_back(current_datum ? just(std::move(*current_datum)) : nothing());
      }
      context =
          constr(0U, {std::move(tx_info), current.data,
                      constr(current.tag == 0U ? 1U : current.tag, std::move(script_fields))});
    } else {
      context = constr(0U, {std::move(tx_info), std::move(purpose)});
    }
    auto validated = validate_plutus_data_node(context);
    if (!validated) {
      return std::unexpected(validated.error());
    }
    return std::move(*validated);
  } catch (const ContextFailure& failure) {
    return std::unexpected(failure.error);
  }
}

}  // namespace cardano::plutus::detail
