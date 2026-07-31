#include <cstddef>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cardano/plutus/data.hpp"

namespace cardano::plutus {
namespace {

using JsonValue = nlohmann::json;

[[nodiscard]] core::CardanoError data_error(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::Result<chain::PlutusData> from_json_node(const JsonValue& json,
                                                             std::size_t depth,
                                                             std::size_t max_depth,
                                                             std::size_t& values,
                                                             std::size_t max_values) {
  if (depth > max_depth) {
    return std::unexpected(core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                                              "Data JSON nesting exceeds the configured limit"));
  }
  if (++values > max_values) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::resource_limit_exceeded,
                           "Data JSON value count exceeds the configured limit"));
  }
  if (!json.is_object() || json.size() != 1U) {
    return std::unexpected(data_error("Data JSON must contain exactly one variant property"));
  }
  if (json.contains("int")) {
    const auto& integer = json.at("int");
    core::Result<core::BigInteger> parsed =
        std::unexpected(data_error("Data JSON int must be an integer or decimal string"));
    if (integer.is_number_unsigned()) {
      parsed = core::BigInteger(integer.get<std::uint64_t>());
    } else if (integer.is_number_integer()) {
      parsed = core::BigInteger(integer.get<std::int64_t>());
    } else if (integer.is_string()) {
      parsed = core::BigInteger::from_decimal(integer.get<std::string>());
    }
    return parsed ? core::Result<chain::PlutusData>(chain::PlutusData::integer(std::move(*parsed)))
                  : std::unexpected(parsed.error());
  }
  if (json.contains("bytes")) {
    if (!json.at("bytes").is_string()) {
      return std::unexpected(data_error("Data JSON bytes must be hexadecimal text"));
    }
    auto bytes = core::hex_to_bytes(json.at("bytes").get<std::string>());
    return bytes ? core::Result<chain::PlutusData>(chain::PlutusData::bytes(std::move(*bytes)))
                 : std::unexpected(bytes.error());
  }
  if (json.contains("list")) {
    if (!json.at("list").is_array()) {
      return std::unexpected(data_error("Data JSON list must be an array"));
    }
    std::vector<chain::PlutusData> items;
    items.reserve(json.at("list").size());
    for (std::size_t index = 0; index < json.at("list").size(); ++index) {
      auto item = from_json_node(json.at("list")[index], depth + 1U, max_depth, values, max_values);
      if (!item) {
        return std::unexpected(item.error().at(index));
      }
      items.push_back(std::move(*item));
    }
    return chain::PlutusData::list(std::move(items));
  }
  if (json.contains("map")) {
    if (!json.at("map").is_array()) {
      return std::unexpected(data_error("Data JSON map must be an array"));
    }
    std::vector<std::pair<chain::PlutusData, chain::PlutusData>> entries;
    entries.reserve(json.at("map").size());
    for (std::size_t index = 0; index < json.at("map").size(); ++index) {
      const auto& entry = json.at("map")[index];
      if (!entry.is_object() || entry.size() != 2U || !entry.contains("k") ||
          !entry.contains("v")) {
        return std::unexpected(data_error("Data JSON map entry must be {k, v}"));
      }
      auto key = from_json_node(entry.at("k"), depth + 1U, max_depth, values, max_values);
      auto value = from_json_node(entry.at("v"), depth + 1U, max_depth, values, max_values);
      if (!key || !value) {
        return std::unexpected((!key ? key.error() : value.error()).at(index));
      }
      entries.emplace_back(std::move(*key), std::move(*value));
    }
    return chain::PlutusData::map(std::move(entries));
  }
  if (json.contains("constructor")) {
    const auto& constructor = json.at("constructor");
    if (!constructor.is_object() || constructor.size() != 2U ||
        !constructor.contains("alternative") || !constructor.contains("fields") ||
        !constructor.at("fields").is_array()) {
      return std::unexpected(
          data_error("Data JSON constructor must contain alternative and fields"));
    }
    core::Result<core::BigInteger> alternative =
        std::unexpected(data_error("constructor alternative must be unsigned"));
    if (constructor.at("alternative").is_number_unsigned()) {
      alternative = core::BigInteger(constructor.at("alternative").get<std::uint64_t>());
    } else if (constructor.at("alternative").is_string()) {
      alternative =
          core::BigInteger::from_decimal(constructor.at("alternative").get<std::string>());
    }
    if (!alternative || alternative->is_negative()) {
      return std::unexpected(alternative ? data_error("constructor alternative must be unsigned")
                                         : alternative.error());
    }
    std::vector<chain::PlutusData> fields;
    fields.reserve(constructor.at("fields").size());
    for (std::size_t index = 0; index < constructor.at("fields").size(); ++index) {
      auto field = from_json_node(constructor.at("fields")[index], depth + 1U, max_depth, values,
                                  max_values);
      if (!field) {
        return std::unexpected(field.error().at(index));
      }
      fields.push_back(std::move(*field));
    }
    return chain::PlutusData::constr(std::move(*alternative), std::move(fields));
  }
  return std::unexpected(data_error("unknown Data JSON variant"));
}

[[nodiscard]] core::Result<JsonValue> to_json_node(const chain::PlutusData& data, std::size_t depth,
                                                   std::size_t max_depth, std::size_t& values,
                                                   std::size_t max_values) {
  if (depth > max_depth) {
    return std::unexpected(core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                                              "Data JSON nesting exceeds the configured limit"));
  }
  if (++values > max_values) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::resource_limit_exceeded,
                           "Data JSON value count exceeds the configured limit"));
  }
  return std::visit(
      [&](const auto& node) -> core::Result<JsonValue> {
        using Node = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<Node, core::BigInteger>) {
          return JsonValue{{"int", node.to_decimal()}};
        } else if constexpr (std::is_same_v<Node, core::Bytes>) {
          return JsonValue{{"bytes", core::bytes_to_hex(node)}};
        } else if constexpr (std::is_same_v<Node, std::shared_ptr<chain::PlutusData::List>>) {
          JsonValue items = JsonValue::array();
          for (std::size_t index = 0; index < node->size(); ++index) {
            auto item = to_json_node((*node)[index], depth + 1U, max_depth, values, max_values);
            if (!item) {
              return std::unexpected(item.error().at(index));
            }
            items.push_back(std::move(*item));
          }
          return JsonValue{{"list", std::move(items)}};
        } else if constexpr (std::is_same_v<Node, std::shared_ptr<chain::PlutusMap>>) {
          JsonValue entries = JsonValue::array();
          for (std::size_t index = 0; index < node->entries.size(); ++index) {
            auto key =
                to_json_node(node->entries[index].first, depth + 1U, max_depth, values, max_values);
            auto value = to_json_node(node->entries[index].second, depth + 1U, max_depth, values,
                                      max_values);
            if (!key || !value) {
              return std::unexpected((!key ? key.error() : value.error()).at(index));
            }
            entries.push_back(JsonValue{{"k", std::move(*key)}, {"v", std::move(*value)}});
          }
          return JsonValue{{"map", std::move(entries)}};
        } else {
          JsonValue fields = JsonValue::array();
          for (std::size_t index = 0; index < node->fields.size(); ++index) {
            auto field =
                to_json_node(node->fields[index], depth + 1U, max_depth, values, max_values);
            if (!field) {
              return std::unexpected(field.error().at(index));
            }
            fields.push_back(std::move(*field));
          }
          return JsonValue{
              {"constructor", JsonValue{{"alternative", node->alternative.to_decimal()},
                                        {"fields", std::move(fields)}}}};
        }
      },
      data.node());
}

}  // namespace

Data::Data(chain::PlutusData value) : value_(std::move(value)) {}

Data Data::constr(core::BigInteger alternative, std::vector<Data> fields) {
  std::vector<chain::PlutusData> converted;
  converted.reserve(fields.size());
  for (auto& field : fields) {
    converted.push_back(std::move(field.value_));
  }
  return Data(chain::PlutusData::constr(std::move(alternative), std::move(converted)));
}

Data Data::integer(core::BigInteger value) {
  return Data(chain::PlutusData::integer(std::move(value)));
}

Data Data::bytes(core::Bytes value) { return Data(chain::PlutusData::bytes(std::move(value))); }

Data Data::list(std::vector<Data> values) {
  std::vector<chain::PlutusData> converted;
  converted.reserve(values.size());
  for (auto& value : values) {
    converted.push_back(std::move(value.value_));
  }
  return Data(chain::PlutusData::list(std::move(converted)));
}

Data Data::map(std::vector<std::pair<Data, Data>> entries) {
  std::vector<std::pair<chain::PlutusData, chain::PlutusData>> converted;
  converted.reserve(entries.size());
  for (auto& [key, value] : entries) {
    converted.emplace_back(std::move(key.value_), std::move(value.value_));
  }
  return Data(chain::PlutusData::map(std::move(converted)));
}

core::Result<Data> Data::from_cbor(core::ByteSpan bytes, core::cbor::DecodeOptions options) {
  auto value = chain::PlutusData::from_cbor(bytes, options);
  return value ? core::Result<Data>(Data(std::move(*value))) : std::unexpected(value.error());
}

core::Result<Data> Data::from_cbor_hex(std::string_view hex, core::cbor::DecodeOptions options) {
  auto bytes = core::hex_to_bytes(hex);
  return bytes ? from_cbor(*bytes, options) : std::unexpected(bytes.error());
}

core::Result<Data> Data::from_json(std::string_view json, std::size_t max_depth,
                                   std::size_t max_values) {
  JsonValue parsed;
  try {
    parsed = JsonValue::parse(json);
  } catch (const JsonValue::exception& error) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                              std::string("invalid Data JSON: ") + error.what()));
  }
  std::size_t values = 0U;
  auto data = from_json_node(parsed, 0U, max_depth, values, max_values);
  return data ? core::Result<Data>(Data(std::move(*data))) : std::unexpected(data.error());
}

Data Data::from_plutus_data(chain::PlutusData value) { return Data(std::move(value)); }

core::Result<core::Bytes> Data::to_cbor(core::cbor::Mode mode) const {
  return value_.to_cbor(mode);
}

core::Result<std::string> Data::to_cbor_hex(core::cbor::Mode mode) const {
  auto encoded = to_cbor(mode);
  return encoded ? core::Result<std::string>(core::bytes_to_hex(*encoded))
                 : std::unexpected(encoded.error());
}

core::Result<std::string> Data::to_json(std::size_t max_depth, std::size_t max_values) const {
  std::size_t values = 0U;
  auto json = to_json_node(value_, 0U, max_depth, values, max_values);
  return json ? core::Result<std::string>(json->dump()) : std::unexpected(json.error());
}

const chain::PlutusData& Data::to_plutus_data() const noexcept { return value_; }

struct DataSchema::State {
  SchemaKind kind{SchemaKind::any};
  std::optional<core::BigInteger> minimum_integer;
  std::optional<core::BigInteger> maximum_integer;
  std::size_t minimum_size{};
  std::optional<std::size_t> maximum_size;
  std::optional<core::BigInteger> alternative;
  std::vector<DataSchema> children;
};

DataSchema::DataSchema(std::shared_ptr<const State> state) : state_(std::move(state)) {}

DataSchema DataSchema::any() { return DataSchema(std::make_shared<State>()); }

DataSchema DataSchema::integer(std::optional<core::BigInteger> minimum,
                               std::optional<core::BigInteger> maximum) {
  auto state = std::make_shared<State>();
  state->kind = SchemaKind::integer;
  state->minimum_integer = std::move(minimum);
  state->maximum_integer = std::move(maximum);
  return DataSchema(std::move(state));
}

DataSchema DataSchema::bytes(std::size_t minimum, std::optional<std::size_t> maximum) {
  auto state = std::make_shared<State>();
  state->kind = SchemaKind::bytes;
  state->minimum_size = minimum;
  state->maximum_size = maximum;
  return DataSchema(std::move(state));
}

DataSchema DataSchema::boolean() {
  auto state = std::make_shared<State>();
  state->kind = SchemaKind::boolean;
  return DataSchema(std::move(state));
}

DataSchema DataSchema::list(DataSchema item, std::size_t minimum,
                            std::optional<std::size_t> maximum) {
  auto state = std::make_shared<State>();
  state->kind = SchemaKind::list;
  state->minimum_size = minimum;
  state->maximum_size = maximum;
  state->children.push_back(std::move(item));
  return DataSchema(std::move(state));
}

DataSchema DataSchema::tuple(std::vector<DataSchema> items) {
  auto state = std::make_shared<State>();
  state->kind = SchemaKind::tuple;
  state->children = std::move(items);
  return DataSchema(std::move(state));
}

DataSchema DataSchema::map(DataSchema key, DataSchema value) {
  auto state = std::make_shared<State>();
  state->kind = SchemaKind::map;
  state->children.push_back(std::move(key));
  state->children.push_back(std::move(value));
  return DataSchema(std::move(state));
}

DataSchema DataSchema::nullable(DataSchema item) {
  auto state = std::make_shared<State>();
  state->kind = SchemaKind::nullable;
  state->children.push_back(std::move(item));
  return DataSchema(std::move(state));
}

DataSchema DataSchema::constructor(core::BigInteger alternative, std::vector<DataSchema> fields) {
  auto state = std::make_shared<State>();
  state->kind = SchemaKind::constructor;
  state->alternative = std::move(alternative);
  state->children = std::move(fields);
  return DataSchema(std::move(state));
}

SchemaKind DataSchema::kind() const noexcept { return state_->kind; }

core::VoidResult DataSchema::validate(const Data& data, std::size_t max_depth,
                                      std::size_t max_values) const {
  struct Frame {
    const DataSchema* schema;
    const chain::PlutusData* data;
    std::size_t depth;
  };
  std::vector<Frame> pending{{this, &data.to_plutus_data(), 0U}};
  std::size_t values = 0U;
  while (!pending.empty()) {
    const auto frame = pending.back();
    pending.pop_back();
    if (frame.depth > max_depth) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                             "Data schema nesting exceeds the configured limit"));
    }
    if (++values > max_values) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::resource_limit_exceeded,
                             "Data schema value count exceeds the configured limit"));
    }
    const auto& node = frame.data->node();
    const auto& state = *frame.schema->state_;
    if (state.kind == SchemaKind::any) {
      continue;
    }
    if (state.kind == SchemaKind::integer) {
      const auto* integer = std::get_if<core::BigInteger>(&node);
      if (integer == nullptr || (state.minimum_integer && *integer < *state.minimum_integer) ||
          (state.maximum_integer && *integer > *state.maximum_integer)) {
        return std::unexpected(data_error("Data integer does not satisfy its schema"));
      }
      continue;
    }
    if (state.kind == SchemaKind::bytes) {
      const auto* bytes = std::get_if<core::Bytes>(&node);
      if (bytes == nullptr || bytes->size() < state.minimum_size ||
          (state.maximum_size && bytes->size() > *state.maximum_size)) {
        return std::unexpected(data_error("Data bytes do not satisfy their schema"));
      }
      continue;
    }
    if (state.kind == SchemaKind::list || state.kind == SchemaKind::tuple) {
      const auto* list = std::get_if<std::shared_ptr<chain::PlutusData::List>>(&node);
      if (list == nullptr) {
        return std::unexpected(data_error("Data value is not a list"));
      }
      if (state.kind == SchemaKind::list) {
        if ((*list)->size() < state.minimum_size ||
            (state.maximum_size && (*list)->size() > *state.maximum_size)) {
          return std::unexpected(data_error("Data list length does not satisfy its schema"));
        }
        for (const auto& item : **list) {
          pending.push_back(Frame{&state.children[0], &item, frame.depth + 1U});
        }
      } else {
        if ((*list)->size() != state.children.size()) {
          return std::unexpected(data_error("Data tuple has the wrong length"));
        }
        for (std::size_t index = 0; index < (*list)->size(); ++index) {
          pending.push_back(Frame{&state.children[index], &(**list)[index], frame.depth + 1U});
        }
      }
      continue;
    }
    if (state.kind == SchemaKind::map) {
      const auto* map = std::get_if<std::shared_ptr<chain::PlutusMap>>(&node);
      if (map == nullptr) {
        return std::unexpected(data_error("Data value is not a map"));
      }
      for (const auto& [key, value] : (*map)->entries) {
        pending.push_back(Frame{&state.children[0], &key, frame.depth + 1U});
        pending.push_back(Frame{&state.children[1], &value, frame.depth + 1U});
      }
      continue;
    }
    const auto* constructor = std::get_if<std::shared_ptr<chain::ConstrPlutusData>>(&node);
    if (constructor == nullptr) {
      return std::unexpected(data_error("Data value is not a constructor"));
    }
    if (state.kind == SchemaKind::boolean) {
      if (!(*constructor)->fields.empty() ||
          ((*constructor)->alternative != core::BigInteger(std::uint64_t{0}) &&
           (*constructor)->alternative != core::BigInteger(std::uint64_t{1}))) {
        return std::unexpected(
            data_error("Data boolean must be constructor 0 or 1 without fields"));
      }
      continue;
    }
    if (state.kind == SchemaKind::nullable) {
      if ((*constructor)->alternative == core::BigInteger(std::uint64_t{1}) &&
          (*constructor)->fields.empty()) {
        continue;
      }
      if ((*constructor)->alternative != core::BigInteger(std::uint64_t{0}) ||
          (*constructor)->fields.size() != 1U) {
        return std::unexpected(
            data_error("nullable Data must be constructor 0 with one field or constructor 1"));
      }
      pending.push_back(Frame{&state.children[0], &(*constructor)->fields[0], frame.depth + 1U});
      continue;
    }
    if (!state.alternative || (*constructor)->alternative != *state.alternative ||
        (*constructor)->fields.size() != state.children.size()) {
      return std::unexpected(data_error("Data constructor does not satisfy its schema"));
    }
    for (std::size_t index = 0; index < (*constructor)->fields.size(); ++index) {
      pending.push_back(
          Frame{&state.children[index], &(*constructor)->fields[index], frame.depth + 1U});
    }
  }
  return std::monostate{};
}

}  // namespace cardano::plutus
