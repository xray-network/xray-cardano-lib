#include "cardano/plutus/blueprint.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <set>
#include <unordered_set>

#include "cardano/chain/ledger.hpp"

namespace cardano::plutus {
namespace {

using JsonValue = nlohmann::json;

[[nodiscard]] core::CardanoError blueprint_error(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::CardanoError limit_error(std::string message) {
  return core::CardanoError(core::ErrorCode::resource_limit_exceeded, std::move(message));
}

[[nodiscard]] bool known_purpose(std::string_view purpose) {
  return purpose == "spend" || purpose == "mint" || purpose == "withdraw" || purpose == "publish";
}

[[nodiscard]] core::Result<std::vector<std::string>> purposes(const JsonValue& value) {
  std::vector<std::string> output;
  if (!value.contains("purpose")) return output;
  const auto& purpose = value["purpose"];
  const JsonValue* alternatives = nullptr;
  if (purpose.is_string()) {
    output.push_back(purpose.get<std::string>());
  } else if (purpose.is_object() && purpose.contains("oneOf") && purpose["oneOf"].is_array()) {
    alternatives = &purpose["oneOf"];
    if (alternatives->empty()) return std::unexpected(blueprint_error("purpose oneOf is empty"));
    for (const auto& item : *alternatives) {
      if (!item.is_string()) return std::unexpected(blueprint_error("purpose must be a string"));
      output.push_back(item.get<std::string>());
    }
  } else {
    return std::unexpected(blueprint_error("invalid blueprint purpose"));
  }
  std::set<std::string> unique;
  for (const auto& item : output) {
    if (!known_purpose(item) || !unique.insert(item).second) {
      return std::unexpected(blueprint_error("invalid or duplicate blueprint purpose"));
    }
  }
  return output;
}

[[nodiscard]] bool exact_integer(const JsonValue& value, core::BigInteger& output) {
  std::string text;
  if (value.is_number_integer())
    text = std::to_string(value.get<std::int64_t>());
  else if (value.is_number_unsigned())
    text = std::to_string(value.get<std::uint64_t>());
  else if (value.is_string())
    text = value.get<std::string>();
  else
    return false;
  auto parsed = core::BigInteger::from_decimal(text);
  if (!parsed) return false;
  output = std::move(*parsed);
  return true;
}

[[nodiscard]] core::VoidResult validate_schema_shape(const JsonValue& schema,
                                                     const JsonValue& definitions, bool parameter,
                                                     std::size_t depth, std::size_t& nodes,
                                                     const BlueprintLimits& limits,
                                                     std::set<const JsonValue*>& active) {
  if (depth > limits.max_depth || ++nodes > limits.max_nodes) {
    return std::unexpected(limit_error("blueprint schema limit exceeded"));
  }
  if (!schema.is_object()) return std::unexpected(blueprint_error("schema must be an object"));
  if (!active.insert(&schema).second) return std::monostate{};
  const auto leave = [&] { active.erase(&schema); };
  if (schema.contains("$ref")) {
    if (!schema["$ref"].is_string()) {
      leave();
      return std::unexpected(blueprint_error("$ref must be a string"));
    }
    const auto reference = schema["$ref"].get<std::string>();
    constexpr std::string_view prefix = "#/definitions/";
    if (!reference.starts_with(prefix)) {
      leave();
      return std::unexpected(blueprint_error("only #/definitions references are allowed"));
    }
    const auto name = reference.substr(prefix.size());
    if (name.empty() || name.find('/') != std::string::npos || !definitions.contains(name)) {
      leave();
      return std::unexpected(blueprint_error("blueprint reference does not resolve"));
    }
  }
  std::size_t applicators = 0;
  for (const auto* keyword : {"allOf", "anyOf", "oneOf"}) {
    if (!schema.contains(keyword)) continue;
    ++applicators;
    const auto& branches = schema[keyword];
    if (!branches.is_array() || branches.empty()) {
      leave();
      return std::unexpected(blueprint_error(std::string(keyword) + " must be a nonempty array"));
    }
    for (const auto& branch : branches) {
      auto valid =
          validate_schema_shape(branch, definitions, parameter, depth + 1, nodes, limits, active);
      if (!valid) {
        leave();
        return valid;
      }
    }
  }
  if (schema.contains("not")) {
    ++applicators;
    auto valid = validate_schema_shape(schema["not"], definitions, parameter, depth + 1, nodes,
                                       limits, active);
    if (!valid) {
      leave();
      return valid;
    }
  }
  if (applicators > 1U) {
    leave();
    return std::unexpected(blueprint_error("schema has multiple applicators"));
  }
  if (!schema.contains("dataType")) {
    leave();
    return std::monostate{};
  }
  if (!schema["dataType"].is_string()) {
    leave();
    return std::unexpected(blueprint_error("dataType must be a string"));
  }
  const auto type = schema["dataType"].get<std::string>();
  const std::set<std::string> data_types{"integer", "bytes", "list", "map", "constructor"};
  const std::set<std::string> builtin_types{"#unit",   "#boolean", "#integer", "#bytes",
                                            "#string", "#pair",    "#list"};
  if (!data_types.contains(type) && !(parameter && builtin_types.contains(type))) {
    leave();
    return std::unexpected(blueprint_error("unsupported blueprint dataType"));
  }
  for (const auto* keyword :
       {"minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum", "multipleOf"}) {
    if (!schema.contains(keyword)) continue;
    core::BigInteger integer;
    if (!exact_integer(schema[keyword], integer) ||
        (std::string_view(keyword) == "multipleOf" &&
         integer <= core::BigInteger(std::int64_t{0}))) {
      leave();
      return std::unexpected(blueprint_error(std::string(keyword) + " must be an exact integer"));
    }
  }
  for (const auto* keyword : {"minLength", "maxLength", "minItems", "maxItems"}) {
    if (!schema.contains(keyword)) continue;
    if (!schema[keyword].is_number_unsigned()) {
      leave();
      return std::unexpected(blueprint_error(std::string(keyword) + " must be nonnegative"));
    }
  }
  if (type == "list" || type == "#list") {
    if (!schema.contains("items")) {
      leave();
      return std::unexpected(blueprint_error("list schema requires items"));
    }
    if (schema["items"].is_array()) {
      for (const auto& item : schema["items"]) {
        auto valid = validate_schema_shape(item, definitions, type == "list" ? parameter : false,
                                           depth + 1, nodes, limits, active);
        if (!valid) {
          leave();
          return valid;
        }
      }
    } else {
      auto valid =
          validate_schema_shape(schema["items"], definitions, type == "list" ? parameter : false,
                                depth + 1, nodes, limits, active);
      if (!valid) {
        leave();
        return valid;
      }
    }
  }
  if (type == "map") {
    if (!schema.contains("keys") || !schema.contains("values")) {
      leave();
      return std::unexpected(blueprint_error("map schema requires keys and values"));
    }
    for (const auto* key : {"keys", "values"}) {
      auto valid =
          validate_schema_shape(schema[key], definitions, false, depth + 1, nodes, limits, active);
      if (!valid) {
        leave();
        return valid;
      }
    }
  }
  if (type == "constructor") {
    core::BigInteger index;
    if (!schema.contains("index") || !exact_integer(schema["index"], index) ||
        index.is_negative() || !schema.contains("fields") || !schema["fields"].is_array()) {
      leave();
      return std::unexpected(
          blueprint_error("constructor schema requires nonnegative index and fields"));
    }
    for (const auto& field : schema["fields"]) {
      auto valid =
          validate_schema_shape(field, definitions, false, depth + 1, nodes, limits, active);
      if (!valid) {
        leave();
        return valid;
      }
    }
  }
  if (type == "#pair") {
    if (!schema.contains("left") || !schema.contains("right")) {
      leave();
      return std::unexpected(blueprint_error("pair schema requires left and right"));
    }
    for (const auto* key : {"left", "right"}) {
      auto valid =
          validate_schema_shape(schema[key], definitions, false, depth + 1, nodes, limits, active);
      if (!valid) {
        leave();
        return valid;
      }
    }
  }
  leave();
  return std::monostate{};
}

[[nodiscard]] core::Result<BlueprintArgument> parse_argument(const JsonValue& value, bool parameter,
                                                             const JsonValue& definitions,
                                                             const BlueprintLimits& limits,
                                                             std::size_t& nodes) {
  if (!value.is_object() || !value.contains("schema") || !value["schema"].is_object()) {
    return std::unexpected(blueprint_error("blueprint argument requires schema"));
  }
  std::set<const JsonValue*> active;
  auto schema_valid =
      validate_schema_shape(value["schema"], definitions, parameter, 0U, nodes, limits, active);
  if (!schema_valid) return std::unexpected(schema_valid.error());
  auto parsed_purposes = purposes(value);
  if (!parsed_purposes) return std::unexpected(parsed_purposes.error());
  BlueprintArgument result{{value["schema"].dump()}, std::move(*parsed_purposes), std::nullopt};
  if (value.contains("title")) {
    if (!value["title"].is_string())
      return std::unexpected(blueprint_error("argument title must be a string"));
    result.title = value["title"].get<std::string>();
  }
  if (value.contains("description") && !value["description"].is_string()) {
    return std::unexpected(blueprint_error("argument description must be a string"));
  }
  return result;
}

struct Evaluation {
  const JsonValue& definitions;
  BlueprintLimits limits;
  std::size_t steps{};
  std::vector<BlueprintDiagnostic> diagnostics;
  std::set<std::string> active;

  void add(bool quiet, std::string code, std::string path, std::string message) {
    if (!quiet) diagnostics.push_back({std::move(code), std::move(path), std::move(message)});
  }

  [[nodiscard]] const JsonValue* resolve(const JsonValue& schema) const {
    if (!schema.contains("$ref") || !schema["$ref"].is_string()) return nullptr;
    const auto reference = schema["$ref"].get<std::string>();
    constexpr std::string_view prefix = "#/definitions/";
    if (!reference.starts_with(prefix)) return nullptr;
    const auto name = reference.substr(prefix.size());
    if (name.find('/') != std::string::npos || !definitions.contains(name)) return nullptr;
    return &definitions[name];
  }

  [[nodiscard]] bool match_data(const JsonValue& schema, const Data& data, std::string path,
                                std::size_t depth, bool quiet = false) {
    if (depth > limits.max_depth || ++steps > limits.max_evaluation_steps) {
      add(quiet, "LIMIT", path, "blueprint evaluation limit exceeded");
      return false;
    }
    const auto state = std::to_string(reinterpret_cast<std::uintptr_t>(&schema)) + ":" +
                       data.to_cbor_hex().value_or("invalid");
    if (!active.insert(state).second) {
      add(quiet, "REFERENCE_CYCLE", path, "reference did not consume a Data node");
      return false;
    }
    const auto leave = [&] { active.erase(state); };
    if (schema.contains("$ref")) {
      const auto* target = resolve(schema);
      if (target == nullptr) {
        leave();
        add(quiet, "REFERENCE", path, "reference does not resolve");
        return false;
      }
      const auto result = match_data(*target, data, std::move(path), depth + 1, quiet);
      leave();
      return result;
    }
    if (schema.contains("allOf")) {
      bool result = true;
      for (const auto& branch : schema["allOf"])
        result = match_data(branch, data, path, depth + 1, quiet) && result;
      leave();
      return result;
    }
    for (const auto* keyword : {"anyOf", "oneOf"})
      if (schema.contains(keyword)) {
        std::size_t matches = 0;
        for (const auto& branch : schema[keyword])
          if (match_data(branch, data, path, depth + 1, true)) ++matches;
        const bool result = std::string_view(keyword) == "anyOf" ? matches > 0U : matches == 1U;
        if (!result)
          add(quiet, std::string(keyword), path, std::string(keyword) + " did not match");
        leave();
        return result;
      }
    if (schema.contains("not")) {
      const bool result = !match_data(schema["not"], data, path, depth + 1, true);
      if (!result) add(quiet, "not", path, "not schema matched");
      leave();
      return result;
    }
    if (!schema.contains("dataType")) {
      leave();
      return true;
    }
    const auto type = schema["dataType"].get<std::string>();
    const auto& node = data.to_plutus_data().node();
    bool result = true;
    if (type == "integer") {
      const auto* integer = std::get_if<core::BigInteger>(&node);
      if (integer == nullptr) {
        add(quiet, "TYPE", path, "expected integer Data");
        result = false;
      } else {
        for (const auto* keyword : {"minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum"})
          if (schema.contains(keyword)) {
            core::BigInteger bound;
            const bool parsed = exact_integer(schema[keyword], bound);
            static_cast<void>(parsed);
            const bool valid = std::string_view(keyword) == "minimum"            ? *integer >= bound
                               : std::string_view(keyword) == "maximum"          ? *integer <= bound
                               : std::string_view(keyword) == "exclusiveMinimum" ? *integer > bound
                                                                                 : *integer < bound;
            if (!valid) {
              add(quiet, keyword, path, "integer bound failed");
              result = false;
            }
          }
        if (schema.contains("multipleOf")) {
          core::BigInteger divisor;
          const bool parsed = exact_integer(schema["multipleOf"], divisor);
          static_cast<void>(parsed);
          if ((*integer % divisor) != core::BigInteger(std::int64_t{0})) {
            add(quiet, "multipleOf", path, "integer multiple failed");
            result = false;
          }
        }
      }
    } else if (type == "bytes") {
      const auto* bytes = std::get_if<core::Bytes>(&node);
      if (bytes == nullptr) {
        add(quiet, "TYPE", path, "expected bytes Data");
        result = false;
      } else {
        if (schema.contains("minLength") &&
            bytes->size() < schema["minLength"].get<std::size_t>()) {
          add(quiet, "minLength", path, "byte string is too short");
          result = false;
        }
        if (schema.contains("maxLength") &&
            bytes->size() > schema["maxLength"].get<std::size_t>()) {
          add(quiet, "maxLength", path, "byte string is too long");
          result = false;
        }
        if (schema.contains("enum")) {
          const auto hex = core::bytes_to_hex(*bytes);
          const bool found = std::ranges::any_of(schema["enum"], [&](const auto& item) {
            auto text = item.template get<std::string>();
            std::ranges::transform(text, text.begin(), [](unsigned char character) {
              return static_cast<char>(std::tolower(character));
            });
            return text == hex;
          });
          if (!found) {
            add(quiet, "enum", path, "byte string is outside enum");
            result = false;
          }
        }
      }
    } else if (type == "list") {
      const auto* list = std::get_if<std::shared_ptr<chain::PlutusData::List>>(&node);
      if (list == nullptr) {
        add(quiet, "TYPE", path, "expected list Data");
        result = false;
      } else {
        const auto count = (*list)->size();
        if (schema.contains("minItems") && count < schema["minItems"].get<std::size_t>()) {
          add(quiet, "minItems", path, "list is too short");
          result = false;
        }
        if (schema.contains("maxItems") && count > schema["maxItems"].get<std::size_t>()) {
          add(quiet, "maxItems", path, "list is too long");
          result = false;
        }
        if (schema.contains("uniqueItems") && schema["uniqueItems"] == true) {
          std::set<std::string> unique;
          for (const auto& item : **list) {
            auto encoded = item.to_cbor(core::cbor::Mode::canonical);
            if (!encoded || !unique.insert(core::bytes_to_hex(*encoded)).second) {
              add(quiet, "uniqueItems", path, "list contains duplicates");
              result = false;
              break;
            }
          }
        }
        if (schema.contains("items")) {
          if (schema["items"].is_array()) {
            for (std::size_t index = 0; index < std::min(count, schema["items"].size()); ++index)
              result = match_data(schema["items"][index], Data::from_plutus_data((**list)[index]),
                                  path + "/" + std::to_string(index), depth + 1, quiet) &&
                       result;
          } else {
            for (std::size_t index = 0; index < count; ++index)
              result = match_data(schema["items"], Data::from_plutus_data((**list)[index]),
                                  path + "/" + std::to_string(index), depth + 1, quiet) &&
                       result;
          }
        }
      }
    } else if (type == "map") {
      const auto* map = std::get_if<std::shared_ptr<chain::PlutusMap>>(&node);
      if (map == nullptr) {
        add(quiet, "TYPE", path, "expected map Data");
        result = false;
      } else {
        const auto count = (*map)->entries.size();
        if (schema.contains("minItems") && count < schema["minItems"].get<std::size_t>()) {
          add(quiet, "minItems", path, "map is too small");
          result = false;
        }
        if (schema.contains("maxItems") && count > schema["maxItems"].get<std::size_t>()) {
          add(quiet, "maxItems", path, "map is too large");
          result = false;
        }
        for (std::size_t index = 0; index < count; ++index) {
          result = match_data(schema["keys"], Data::from_plutus_data((*map)->entries[index].first),
                              path + "/" + std::to_string(index) + "/key", depth + 1, quiet) &&
                   result;
          result =
              match_data(schema["values"], Data::from_plutus_data((*map)->entries[index].second),
                         path + "/" + std::to_string(index), depth + 1, quiet) &&
              result;
        }
      }
    } else if (type == "constructor") {
      const auto* constructor = std::get_if<std::shared_ptr<chain::ConstrPlutusData>>(&node);
      if (constructor == nullptr) {
        add(quiet, "TYPE", path, "expected constructor Data");
        result = false;
      } else {
        core::BigInteger index;
        const bool parsed = exact_integer(schema["index"], index);
        static_cast<void>(parsed);
        if ((*constructor)->alternative != index) {
          add(quiet, "index", path, "constructor index differs");
          result = false;
        }
        const auto& fields = schema["fields"];
        if ((*constructor)->fields.size() != fields.size()) {
          add(quiet, "fields", path, "constructor field count differs");
          result = false;
        }
        for (std::size_t position = 0;
             position < std::min((*constructor)->fields.size(), fields.size()); ++position)
          result =
              match_data(fields[position], Data::from_plutus_data((*constructor)->fields[position]),
                         path + "/" + std::to_string(position), depth + 1, quiet) &&
              result;
      }
    } else {
      add(quiet, "TYPE", path, "builtin schema requires a UPLC constant");
      result = false;
    }
    leave();
    return result;
  }
};

}  // namespace

ContractBlueprint::ContractBlueprint(
    BlueprintPreamble preamble, std::vector<BlueprintValidator> validators,
    std::vector<std::pair<std::string, BlueprintSchema>> definitions, std::string json)
    : preamble_(std::move(preamble)),
      validators_(std::move(validators)),
      definitions_(std::move(definitions)),
      json_(std::move(json)) {}

core::Result<ContractBlueprint> ContractBlueprint::parse(std::string_view text,
                                                         BlueprintLimits limits) {
  if (text.size() > limits.max_string_bytes)
    return std::unexpected(limit_error("blueprint JSON exceeds byte limit"));
  JsonValue root;
  std::vector<std::set<std::string>> keys;
  std::size_t nodes = 0U;
  try {
    root =
        JsonValue::parse(text, [&](int depth, JsonValue::parse_event_t event, JsonValue& parsed) {
          if (++nodes > limits.max_nodes || depth > static_cast<int>(limits.max_depth))
            throw std::range_error("blueprint JSON limit exceeded");
          if (event == JsonValue::parse_event_t::object_start) {
            const auto key_depth = static_cast<std::size_t>(depth) + 1U;
            if (keys.size() <= key_depth) keys.resize(key_depth + 1U);
            keys[key_depth].clear();
          }
          if (event == JsonValue::parse_event_t::key) {
            if (depth < 0) throw std::invalid_argument("invalid JSON key depth");
            const auto key_depth = static_cast<std::size_t>(depth);
            if (keys.size() <= key_depth) keys.resize(key_depth + 1U);
            const auto key = parsed.get<std::string>();
            if (!keys[key_depth].insert(key).second)
              throw std::invalid_argument("duplicate JSON object key: " + key);
          }
          return true;
        });
  } catch (const std::range_error& exception) {
    return std::unexpected(limit_error(exception.what()));
  } catch (const std::exception& exception) {
    return std::unexpected(blueprint_error(exception.what()));
  }
  if (!root.is_object() || !root.contains("preamble") || !root.contains("validators") ||
      !root["validators"].is_array())
    return std::unexpected(blueprint_error("blueprint requires preamble and validators"));
  if (root.contains("$schema") &&
      root["$schema"] != "https://cips.cardano.org/cips/cip57/schemas/plutus-blueprint.json")
    return std::unexpected(blueprint_error("unsupported blueprint $schema"));
  if (root.contains("$vocabulary")) {
    if (!root["$vocabulary"].is_object())
      return std::unexpected(blueprint_error("$vocabulary must be an object"));
    for (const auto& [name, required] : root["$vocabulary"].items())
      if (required == true && name.find("json-schema.org/draft/2020-12") == std::string::npos &&
          name != "https://cips.cardano.org/cips/cip57")
        return std::unexpected(blueprint_error("unsupported required vocabulary"));
  }
  const auto& p = root["preamble"];
  if (!p.is_object()) return std::unexpected(blueprint_error("invalid blueprint preamble"));
  for (const auto& [key, unused] : p.items()) {
    static_cast<void>(unused);
    if (key != "title" && key != "description" && key != "version" && key != "plutusVersion" &&
        key != "compiler" && key != "license")
      return std::unexpected(blueprint_error("unknown preamble property: " + key));
  }
  if (!p.contains("title") || !p["title"].is_string() || !p.contains("version") ||
      !p["version"].is_string() || !p.contains("plutusVersion") || !p["plutusVersion"].is_string())
    return std::unexpected(blueprint_error("invalid blueprint preamble"));
  BlueprintPreamble preamble{p["title"],   p["version"], p["plutusVersion"],
                             std::nullopt, std::nullopt, std::nullopt};
  if (preamble.plutus_version != "v1" && preamble.plutus_version != "v2" &&
      preamble.plutus_version != "v3")
    return std::unexpected(blueprint_error("unsupported Plutus version"));
  if (p.contains("description")) {
    if (!p["description"].is_string())
      return std::unexpected(blueprint_error("preamble description must be a string"));
    preamble.description = p["description"].get<std::string>();
  }
  if (p.contains("license")) {
    if (!p["license"].is_string())
      return std::unexpected(blueprint_error("preamble license must be a string"));
    preamble.license = p["license"].get<std::string>();
  }
  if (p.contains("compiler")) {
    const auto& compiler = p["compiler"];
    if (!compiler.is_object() || !compiler.contains("name") || !compiler["name"].is_string())
      return std::unexpected(blueprint_error("compiler requires name"));
    for (const auto& [key, unused] : compiler.items()) {
      static_cast<void>(unused);
      if (key != "name" && key != "version")
        return std::unexpected(blueprint_error("unknown compiler property"));
    }
    BlueprintCompiler parsed{compiler["name"], std::nullopt};
    if (compiler.contains("version")) {
      if (!compiler["version"].is_string())
        return std::unexpected(blueprint_error("compiler version must be a string"));
      parsed.version = compiler["version"].get<std::string>();
    }
    preamble.compiler = std::move(parsed);
  }
  const JsonValue definitions = root.value("definitions", JsonValue::object());
  if (!definitions.is_object() ||
      root["validators"].size() + definitions.size() > limits.max_declarations)
    return std::unexpected(limit_error("blueprint declarations exceed limit"));
  std::vector<std::pair<std::string, BlueprintSchema>> retained_definitions;
  std::size_t schema_nodes = 0U;
  std::set<const JsonValue*> active;
  for (const auto& [name, schema] : definitions.items()) {
    auto valid = validate_schema_shape(schema, definitions, true, 0U, schema_nodes, limits, active);
    if (!valid) return std::unexpected(valid.error());
    retained_definitions.emplace_back(name, BlueprintSchema{schema.dump()});
  }
  std::vector<BlueprintValidator> validators;
  for (const auto& item : root["validators"]) {
    if (!item.is_object() || !item.contains("title") || !item["title"].is_string() ||
        !item.contains("redeemer"))
      return std::unexpected(blueprint_error("invalid blueprint validator"));
    auto redeemer = parse_argument(item["redeemer"], false, definitions, limits, schema_nodes);
    if (!redeemer) return std::unexpected(redeemer.error());
    BlueprintValidator validator{item["title"], std::move(*redeemer), std::nullopt, {},
                                 std::nullopt,  std::nullopt,         std::nullopt};
    if (item.contains("description")) {
      if (!item["description"].is_string())
        return std::unexpected(blueprint_error("validator description must be a string"));
      validator.description = item["description"].get<std::string>();
    }
    if (item.contains("datum")) {
      auto datum = parse_argument(item["datum"], false, definitions, limits, schema_nodes);
      if (!datum) return std::unexpected(datum.error());
      validator.datum = std::move(*datum);
    }
    if (item.contains("parameters")) {
      if (!item["parameters"].is_array())
        return std::unexpected(blueprint_error("parameters must be an array"));
      for (const auto& value : item["parameters"]) {
        auto parameter = parse_argument(value, true, definitions, limits, schema_nodes);
        if (!parameter) return std::unexpected(parameter.error());
        validator.parameters.push_back(std::move(*parameter));
      }
    }
    std::vector<const std::vector<std::string>*> purpose_sets{&validator.redeemer.purposes};
    if (validator.datum) purpose_sets.push_back(&validator.datum->purposes);
    for (const auto& parameter : validator.parameters) purpose_sets.push_back(&parameter.purposes);
    for (std::size_t left = 0; left < purpose_sets.size(); ++left)
      for (std::size_t right = left + 1; right < purpose_sets.size(); ++right)
        for (const auto& purpose : *purpose_sets[left])
          if (std::ranges::find(*purpose_sets[right], purpose) != purpose_sets[right]->end())
            return std::unexpected(blueprint_error("validator purpose alternatives overlap"));
    if (item.contains("hash")) {
      if (!item["hash"].is_string() || item["hash"].get<std::string>().size() != 56U ||
          !core::hex_to_bytes(item["hash"].get<std::string>()))
        return std::unexpected(blueprint_error("invalid validator hash"));
      validator.hash = item["hash"].get<std::string>();
    }
    if (item.contains("compiledCode")) {
      if (!item["compiledCode"].is_string() || !validator.hash)
        return std::unexpected(blueprint_error("compiledCode requires hash"));
      validator.compiled_code = item["compiledCode"].get<std::string>();
      auto bytes = core::hex_to_bytes(*validator.compiled_code);
      if (!bytes) return std::unexpected(bytes.error());
      auto program = decode_program_envelope(*bytes, {.max_depth = limits.max_depth,
                                                      .max_nodes = limits.max_nodes,
                                                      .require_complete_input = true});
      if (!program) return std::unexpected(program.error());
      const std::uint8_t prefix =
          preamble.plutus_version == "v1" ? 1U : (preamble.plutus_version == "v2" ? 2U : 3U);
      if (chain::hash_script(prefix, *bytes).to_hex() != *validator.hash)
        return std::unexpected(blueprint_error("validator hash does not match compiledCode"));
    }
    validators.push_back(std::move(validator));
  }
  return ContractBlueprint(std::move(preamble), std::move(validators),
                           std::move(retained_definitions), root.dump());
}

const BlueprintPreamble& ContractBlueprint::preamble() const noexcept { return preamble_; }
const std::vector<BlueprintValidator>& ContractBlueprint::validators() const noexcept {
  return validators_;
}
const std::vector<std::pair<std::string, BlueprintSchema>>& ContractBlueprint::definitions()
    const noexcept {
  return definitions_;
}
std::string ContractBlueprint::to_json() const { return json_; }

std::vector<BlueprintDiagnostic> ContractBlueprint::validate_data(const BlueprintSchema& schema,
                                                                  const Data& value,
                                                                  BlueprintLimits limits) const {
  JsonValue rule;
  JsonValue definitions = JsonValue::object();
  try {
    rule = JsonValue::parse(schema.json);
    for (const auto& [name, definition] : definitions_)
      definitions[name] = JsonValue::parse(definition.json);
  } catch (...) {
    return {{"SCHEMA", "", "schema JSON is invalid"}};
  }
  Evaluation evaluation{definitions, limits, 0U, {}, {}};
  const bool matched = evaluation.match_data(rule, value, "", 0U);
  static_cast<void>(matched);
  return evaluation.diagnostics;
}

std::vector<BlueprintDiagnostic> ContractBlueprint::validate_constant(
    const BlueprintSchema& schema, const UplcConstant& value, BlueprintLimits limits) const {
  JsonValue rule;
  JsonValue definitions = JsonValue::object();
  try {
    rule = JsonValue::parse(schema.json);
    for (const auto& [name, definition] : definitions_)
      definitions[name] = JsonValue::parse(definition.json);
  } catch (...) {
    return {{"SCHEMA", "", "schema JSON is invalid"}};
  }
  if (!rule.contains("dataType") || !rule["dataType"].is_string()) return {};
  const auto type = rule["dataType"].get<std::string>();
  const auto expected = type == "#unit"      ? UplcTypeTag::unit
                        : type == "#boolean" ? UplcTypeTag::boolean
                        : type == "#integer" ? UplcTypeTag::integer
                        : type == "#bytes"   ? UplcTypeTag::byte_string
                        : type == "#string"  ? UplcTypeTag::string
                        : type == "#pair"    ? UplcTypeTag::proto_pair
                        : type == "#list"    ? UplcTypeTag::proto_list
                                             : UplcTypeTag::data;
  if (value.type().tag() != expected)
    return {{"TYPE", "", "UPLC constant does not match builtin schema"}};
  if (type == "#pair") {
    const auto* pair = std::get_if<UplcPair>(&value.value());
    if (pair == nullptr) return {{"TYPE", "", "pair constant is malformed"}};
    std::vector<BlueprintDiagnostic> output;
    for (const auto& [key, item] :
         {std::pair{"left", &pair->first()}, std::pair{"right", &pair->second()}}) {
      if (item->type().tag() != UplcTypeTag::data) {
        output.push_back({"TYPE", std::string("/") + key, "pair member must be Data"});
        continue;
      }
      const auto* data = std::get_if<Data>(&item->value());
      if (data != nullptr) {
        Evaluation evaluation{definitions, limits, 0U, {}, {}};
        const bool matched = evaluation.match_data(rule[key], *data, std::string("/") + key, 0U);
        static_cast<void>(matched);
        output.insert(output.end(), evaluation.diagnostics.begin(), evaluation.diagnostics.end());
      }
    }
    return output;
  }
  if (type == "#list") {
    const auto* items = std::get_if<UplcConstant::Items>(&value.value());
    if (items == nullptr) return {{"TYPE", "", "list constant is malformed"}};
    std::vector<BlueprintDiagnostic> output;
    for (std::size_t index = 0; index < items->size(); ++index) {
      const auto* data = std::get_if<Data>(&(*items)[index].value());
      if ((*items)[index].type().tag() != UplcTypeTag::data || data == nullptr) {
        output.push_back({"TYPE", "/" + std::to_string(index), "list member must be Data"});
        continue;
      }
      Evaluation evaluation{definitions, limits, 0U, {}, {}};
      const bool matched =
          evaluation.match_data(rule["items"], *data, "/" + std::to_string(index), 0U);
      static_cast<void>(matched);
      output.insert(output.end(), evaluation.diagnostics.begin(), evaluation.diagnostics.end());
    }
    return output;
  }
  return {};
}

}  // namespace cardano::plutus
