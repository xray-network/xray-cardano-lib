
[[nodiscard]] core::VoidResult validate_header(std::string_view name, const CborValue& value,
                                               BodyEra era) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  const bool compact_vrf = era == BodyEra::babbage || era == BodyEra::conway;
  const auto body_size = compact_vrf ? 10U : 11U;
  auto body = require_array_size(name, fields[0], body_size, body_size);
  if (!body) return std::unexpected(body.error().at(0U));
  auto signature = require_bytes(name, fields[1], 448, "KES signature");
  if (!signature) return std::unexpected(signature.error().at(1U));
  const auto& body_fields = fields[0].as_array()->values;
  const std::array<std::size_t, 3> unsigned_indices{0U, 1U,
                                                    compact_vrf ? std::size_t{6} : std::size_t{7}};
  for (const auto index : unsigned_indices) {
    auto number = unsigned_value(name, body_fields[index], "header integer");
    if (!number) return std::unexpected(number.error().at(0U).at(index));
  }
  if (!is_null(body_fields[2])) {
    auto previous = require_bytes(name, body_fields[2], 32, "previous block hash");
    if (!previous) return std::unexpected(previous.error().at(0U).at(2U));
  }
  auto issuer = require_bytes(name, body_fields[3], 32, "issuer vkey");
  if (!issuer) return std::unexpected(issuer.error().at(0U).at(3U));
  const auto body_hash_index = compact_vrf ? 7U : 8U;
  auto body_hash = require_bytes(name, body_fields[body_hash_index], 32, "block body hash");
  if (!body_hash) {
    return std::unexpected(body_hash.error().at(0U).at(body_hash_index));
  }
  if (compact_vrf) {
    auto vrf = require_bytes(name, body_fields[4], 32, "VRF vkey");
    if (!vrf) return std::unexpected(vrf.error().at(0U).at(4U));
    auto vrf_cert = require_array_size(name, body_fields[5], 2, 2);
    if (!vrf_cert) return std::unexpected(vrf_cert.error().at(0U).at(5U));
    const bool output_valid = body_fields[5].as_array()->values[0].as_byte_string() != nullptr;
    auto proof = require_bytes(name, body_fields[5].as_array()->values[1], 80, "VRF proof");
    if (!output_valid || !proof) {
      return std::unexpected(
          !output_valid ? model_error(name, "VRF output must be bytes").at(0U).at(5U).at(0U)
                        : proof.error().at(0U).at(5U).at(1U));
    }
    auto version = validate_protocol_version(name, body_fields[9], maximum_protocol_major(era));
    if (!version) return std::unexpected(version.error().at(0U).at(9U));
  } else {
    for (const auto index : {5U, 6U}) {
      auto vrf_cert = require_array_size(name, body_fields[index], 2, 2);
      if (!vrf_cert) return std::unexpected(vrf_cert.error().at(0U).at(index));
      if (body_fields[index].as_array()->values[0].as_byte_string() == nullptr) {
        return std::unexpected(
            model_error(name, "VRF output must be bytes").at(0U).at(index).at(0U));
      }
      auto proof = require_bytes(name, body_fields[index].as_array()->values[1], 80, "VRF proof");
      if (!proof) return std::unexpected(proof.error().at(0U).at(index).at(1U));
    }
    auto version = validate_protocol_version(name, body_fields[10], maximum_protocol_major(era));
    if (!version) return std::unexpected(version.error().at(0U).at(10U));
  }
  const auto operational_index = compact_vrf ? 8U : 9U;
  auto operational = require_array_size(name, body_fields[operational_index], 4, 4);
  if (!operational) {
    return std::unexpected(operational.error().at(0U).at(operational_index));
  }
  const auto& operational_fields = body_fields[operational_index].as_array()->values;
  auto hot_key = require_bytes(name, operational_fields[0], 32, "operational KES key");
  auto sequence = unsigned_value(name, operational_fields[1], "operational sequence");
  auto period = unsigned_value(name, operational_fields[2], "operational KES period");
  auto operational_signature =
      require_bytes(name, operational_fields[3], 64, "operational signature");
  if (!hot_key || !sequence || !period || !operational_signature) {
    return std::unexpected(!hot_key    ? hot_key.error().at(0U).at(operational_index)
                           : !sequence ? sequence.error().at(0U).at(operational_index)
                           : !period   ? period.error().at(0U).at(operational_index)
                                     : operational_signature.error().at(0U).at(operational_index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_block(std::string_view name, const CborValue& value,
                                              BodyEra era) {
  const bool alonzo_or_later =
      era == BodyEra::alonzo || era == BodyEra::babbage || era == BodyEra::conway;
  auto shape =
      require_array_size(name, value, alonzo_or_later ? 5U : 4U, alonzo_or_later ? 5U : 4U);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto header = validate_header(name, fields[0], era);
  if (!header) return std::unexpected(header.error().at(0U));
  const auto* bodies = fields[1].as_array();
  const auto* witnesses = fields[2].as_array();
  const auto* auxiliary = fields[3].as_map();
  if (bodies == nullptr || witnesses == nullptr || auxiliary == nullptr) {
    return std::unexpected(model_error(name, "block body collections have invalid shapes"));
  }
  if (bodies->values.size() != witnesses->values.size()) {
    return std::unexpected(model_error(name, "transaction body and witness counts differ"));
  }
  for (std::size_t index = 0; index < bodies->values.size(); ++index) {
    auto body = validate_transaction_body(name, bodies->values[index], era);
    if (!body) return std::unexpected(body.error().at(1U).at(index));
    auto witness = validate_witness_set(name, witnesses->values[index], era);
    if (!witness) return std::unexpected(witness.error().at(2U).at(index));
  }
  for (const auto& [index_value, auxiliary_value] : auxiliary->entries) {
    auto bounded_index =
        require_unsigned_bound(name, index_value, UINT16_MAX, "auxiliary-data transaction index");
    auto index = unsigned_value(name, index_value, "auxiliary-data transaction index");
    if (!bounded_index || !index || *index >= bodies->values.size()) {
      return std::unexpected(
          !bounded_index ? bounded_index.error()
                         : (!index ? index.error()
                                   : model_error(name, "auxiliary-data index is out of range")));
    }
    core::VoidResult valid = std::monostate{};
    if (era == BodyEra::shelley) {
      valid = validate_metadata_map(name, auxiliary_value);
    } else {
      const auto maximum_key =
          era == BodyEra::allegra || era == BodyEra::mary
              ? 1U
              : (era == BodyEra::alonzo ? 2U : (era == BodyEra::babbage ? 3U : 4U));
      valid = validate_auxiliary_data(name, auxiliary_value, maximum_key);
    }
    if (!valid) return std::unexpected(valid.error().at(3U).at(*index));
  }
  if (alonzo_or_later) {
    const auto* invalid = fields[4].as_array();
    if (invalid == nullptr) {
      return std::unexpected(model_error(name, "invalid transaction indices must be an array"));
    }
    for (const auto& index_value : invalid->values) {
      auto bounded_index =
          require_unsigned_bound(name, index_value, UINT16_MAX, "invalid transaction index");
      auto index = unsigned_value(name, index_value, "invalid transaction index");
      if (!bounded_index || !index || *index >= bodies->values.size()) {
        return std::unexpected(
            !bounded_index
                ? bounded_index.error()
                : (!index ? index.error()
                          : model_error(name, "invalid transaction index is out of range")));
      }
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_ex_units(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  for (const auto& field : value.as_array()->values) {
    auto number = unsigned_value(name, field, "execution units");
    if (!number || *number > static_cast<std::uint64_t>(INT64_MAX)) {
      return std::unexpected(number ? model_error(name, "execution units exceed int64")
                                    : number.error());
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_witness_set(std::string_view name, const CborValue& value,
                                                    BodyEra era) {
  const auto* map = value.as_map();
  if (map == nullptr) return std::unexpected(model_error(name, "witness set must be a map"));
  core::VoidResult keys = std::monostate{};
  if (era == BodyEra::shelley || era == BodyEra::allegra || era == BodyEra::mary) {
    keys = validate_numeric_keys(name, *map, {0, 1, 2});
  } else if (era == BodyEra::alonzo) {
    keys = validate_numeric_keys(name, *map, {0, 1, 2, 3, 4, 5});
  } else if (era == BodyEra::babbage) {
    keys = validate_numeric_keys(name, *map, {0, 1, 2, 3, 4, 5, 6});
  } else {
    keys = validate_numeric_keys(name, *map, {0, 1, 2, 3, 4, 5, 6, 7});
  }
  if (!keys) return keys;
  auto vkeys = numeric_field(name, *map, 0);
  if (!vkeys) return std::unexpected(vkeys.error());
  if (*vkeys != nullptr) {
    auto witnesses = set_array(name, **vkeys, "vkey witnesses");
    if (!witnesses) return std::unexpected(witnesses.error());
    if (era == BodyEra::conway && (*witnesses)->values.empty()) {
      return std::unexpected(model_error(name, "vkey witnesses must be nonempty"));
    }
    for (const auto& witness : (*witnesses)->values) {
      auto shape = require_array_size(name, witness, 2, 2);
      if (!shape) return shape;
      auto public_key = require_bytes(name, witness.as_array()->values[0], 32, "vkey");
      auto signature = require_bytes(name, witness.as_array()->values[1], 64, "signature");
      if (!public_key || !signature) {
        return std::unexpected(!public_key ? public_key.error() : signature.error());
      }
    }
  }
  auto native = numeric_field(name, *map, 1);
  if (!native) return std::unexpected(native.error());
  if (*native != nullptr) {
    auto scripts = set_array(name, **native, "native scripts");
    if (!scripts) return std::unexpected(scripts.error());
    if (era == BodyEra::conway && (*scripts)->values.empty()) {
      return std::unexpected(model_error(name, "native scripts must be nonempty"));
    }
    for (const auto& script : (*scripts)->values) {
      auto valid = validate_native_script(name, script);
      if (!valid) return valid;
    }
  }
  auto bootstrap = numeric_field(name, *map, 2);
  if (!bootstrap) return std::unexpected(bootstrap.error());
  if (*bootstrap != nullptr) {
    auto witnesses = set_array(name, **bootstrap, "bootstrap witnesses");
    if (!witnesses) return std::unexpected(witnesses.error());
    if (era == BodyEra::conway && (*witnesses)->values.empty()) {
      return std::unexpected(model_error(name, "bootstrap witnesses must be nonempty"));
    }
    for (const auto& witness : (*witnesses)->values) {
      auto shape = require_array_size(name, witness, 4, 4);
      if (!shape) return shape;
      const auto& fields = witness.as_array()->values;
      for (const auto [index, size] :
           std::array<std::pair<std::size_t, std::size_t>, 3>{{{0, 32}, {1, 64}, {2, 32}}}) {
        auto bytes = require_bytes(name, fields[index], size, "bootstrap witness field");
        if (!bytes) return bytes;
      }
      if (fields[3].as_byte_string() == nullptr) {
        return std::unexpected(model_error(name, "bootstrap attributes must be bytes"));
      }
    }
  }
  for (const auto key : {3U, 6U, 7U}) {
    auto scripts_field = numeric_field(name, *map, key);
    if (!scripts_field) return std::unexpected(scripts_field.error());
    if (*scripts_field != nullptr) {
      auto scripts = set_array(name, **scripts_field, "Plutus scripts");
      if (!scripts) return std::unexpected(scripts.error());
      if (era == BodyEra::conway && (*scripts)->values.empty()) {
        return std::unexpected(model_error(name, "Plutus scripts must be nonempty"));
      }
      for (const auto& script : (*scripts)->values) {
        if (script.as_byte_string() == nullptr) {
          return std::unexpected(model_error(name, "Plutus script witness must be bytes"));
        }
      }
    }
  }
  auto datums = numeric_field(name, *map, 4);
  if (!datums) return std::unexpected(datums.error());
  if (*datums != nullptr) {
    auto values = set_array(name, **datums, "Plutus data");
    if (!values) return std::unexpected(values.error());
    if (era == BodyEra::conway && (*values)->values.empty()) {
      return std::unexpected(model_error(name, "Plutus data must be nonempty"));
    }
    for (const auto& datum : (*values)->values) {
      auto valid = validate_plutus_data_node(datum);
      if (!valid) return std::unexpected(valid.error());
    }
  }
  auto redeemers = numeric_field(name, *map, 5);
  if (!redeemers) return std::unexpected(redeemers.error());
  if (*redeemers != nullptr) {
    if (era == BodyEra::conway) {
      auto valid = validate_redeemers(name, **redeemers, 5U);
      if (!valid) return valid;
      return std::monostate{};
    }
    if (const auto* legacy = (**redeemers).as_array()) {
      for (const auto& redeemer : legacy->values) {
        auto shape = require_array_size(name, redeemer, 4, 4);
        if (!shape) return shape;
        const auto& fields = redeemer.as_array()->values;
        auto tag = require_unsigned_bound(name, fields[0], era == BodyEra::conway ? 5U : 3U,
                                          "redeemer tag");
        auto index = require_unsigned_bound(name, fields[1], UINT32_MAX, "redeemer index");
        auto data = validate_plutus_data_node(fields[2]);
        auto units = validate_ex_units(name, fields[3]);
        if (!tag || !index || !data || !units) {
          return std::unexpected(!tag     ? tag.error()
                                 : !index ? index.error()
                                 : !data  ? data.error()
                                          : units.error());
        }
      }
    } else if (const auto* redeemer_map = (**redeemers).as_map();
               redeemer_map != nullptr && era != BodyEra::alonzo) {
      for (const auto& [key, value_field] : redeemer_map->entries) {
        auto key_shape = require_array_size(name, key, 2, 2);
        auto value_shape = require_array_size(name, value_field, 2, 2);
        if (!key_shape || !value_shape) {
          return std::unexpected(!key_shape ? key_shape.error() : value_shape.error());
        }
        auto tag = require_unsigned_bound(name, key.as_array()->values[0],
                                          era == BodyEra::conway ? 5U : 3U, "redeemer tag");
        auto index =
            require_unsigned_bound(name, key.as_array()->values[1], UINT32_MAX, "redeemer index");
        auto data = validate_plutus_data_node(value_field.as_array()->values[0]);
        auto units = validate_ex_units(name, value_field.as_array()->values[1]);
        if (!tag || !index || !data || !units) {
          return std::unexpected(!tag     ? tag.error()
                                 : !index ? index.error()
                                 : !data  ? data.error()
                                          : units.error());
        }
      }
    } else {
      return std::unexpected(model_error(name, "redeemers have an invalid era shape"));
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_transaction(std::string_view name, const CborValue& value,
                                                    BodyEra era) {
  const bool alonzo_or_later =
      era == BodyEra::alonzo || era == BodyEra::babbage || era == BodyEra::conway;
  auto shape =
      require_array_size(name, value, alonzo_or_later ? 4U : 3U, alonzo_or_later ? 4U : 3U);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto body = validate_transaction_body(name, fields[0], era);
  if (!body) return std::unexpected(body.error().at(0U));
  auto witness = validate_witness_set(name, fields[1], era);
  if (!witness) return std::unexpected(witness.error().at(1U));
  const std::size_t auxiliary_index = alonzo_or_later ? 3U : 2U;
  if (alonzo_or_later && !std::holds_alternative<core::cbor::BooleanValue>(fields[2].node())) {
    return std::unexpected(model_error(name, "transaction validity must be boolean").at(2U));
  }
  if (!is_null(fields[auxiliary_index])) {
    const auto maximum_key =
        era == BodyEra::allegra || era == BodyEra::mary
            ? 1U
            : (era == BodyEra::alonzo ? 2U : (era == BodyEra::babbage ? 3U : 4U));
    auto auxiliary = era == BodyEra::shelley
                         ? validate_metadata_map(name, fields[auxiliary_index])
                         : validate_auxiliary_data(name, fields[auxiliary_index], maximum_key);
    if (!auxiliary) return std::unexpected(auxiliary.error().at(auxiliary_index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_metadatum(std::string_view name, const CborValue& value,
                                                  std::size_t depth = 0U) {
  if (depth > 128U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                                              std::string(name) + ": metadata exceeds depth 128"));
  }
  if (value.as_unsigned() != nullptr || value.as_negative() != nullptr) return std::monostate{};
  if (const auto* bytes = value.as_byte_string()) {
    return bytes->value.size() <= 64U
               ? core::VoidResult(std::monostate{})
               : std::unexpected(model_error(name, "metadata bytes exceed 64 bytes"));
  }
  if (const auto* text = value.as_text_string()) {
    return text->value.size() <= 64U
               ? core::VoidResult(std::monostate{})
               : std::unexpected(model_error(name, "metadata text exceeds 64 UTF-8 bytes"));
  }
  if (const auto* array = value.as_array()) {
    for (std::size_t index = 0; index < array->values.size(); ++index) {
      auto valid = validate_metadatum(name, array->values[index], depth + 1U);
      if (!valid) return std::unexpected(valid.error().at(index));
    }
    return std::monostate{};
  }
  if (const auto* map = value.as_map()) {
    for (std::size_t index = 0; index < map->entries.size(); ++index) {
      auto key = validate_metadatum(name, map->entries[index].first, depth + 1U);
      auto item = validate_metadatum(name, map->entries[index].second, depth + 1U);
      if (!key || !item) {
        return std::unexpected(!key ? key.error().at(index) : item.error().at(index));
      }
    }
    return std::monostate{};
  }
  return std::unexpected(model_error(name, "invalid transaction metadatum"));
}

[[nodiscard]] core::VoidResult validate_metadata_map(std::string_view name,
                                                     const CborValue& value) {
  const auto* metadata = value.as_map();
  if (metadata == nullptr) return std::unexpected(model_error(name, "metadata must be a map"));
  for (std::size_t index = 0; index < metadata->entries.size(); ++index) {
    auto label = unsigned_value(name, metadata->entries[index].first, "metadata label");
    if (!label) return std::unexpected(label.error().at(index));
    auto datum = validate_metadatum(name, metadata->entries[index].second);
    if (!datum) return std::unexpected(datum.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_auxiliary_data(std::string_view name,
                                                       const CborValue& value,
                                                       std::uint64_t maximum_plutus_key) {
  if (value.as_map() != nullptr) return validate_metadata_map(name, value);
  if (const auto* fields = value.as_array()) {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return shape;
    auto metadata = validate_metadata_map(name, fields->values[0]);
    if (!metadata) return std::unexpected(metadata.error().at(0U));
    const auto* scripts = fields->values[1].as_array();
    if (scripts == nullptr) {
      return std::unexpected(model_error(name, "auxiliary scripts must be an array").at(1U));
    }
    for (std::size_t index = 0; index < scripts->values.size(); ++index) {
      auto script = validate_native_script(name, scripts->values[index]);
      if (!script) return std::unexpected(script.error().at(1U).at(index));
    }
    return std::monostate{};
  }
  if (maximum_plutus_key == 1U) {
    return std::unexpected(model_error(name, "tagged auxiliary data is unavailable in this era"));
  }
  const auto* tag = value.as_tag();
  if (tag == nullptr || tag->value == nullptr) {
    return std::unexpected(model_error(name, "invalid auxiliary data shape"));
  }
  auto tag_number = tag->tag.to_uint64();
  if (!tag_number || *tag_number != 259U || tag->value->as_map() == nullptr) {
    return std::unexpected(model_error(name, "format auxiliary data must be tag 259 map"));
  }
  const auto* map = tag->value->as_map();
  core::VoidResult keys =
      maximum_plutus_key == 2U
          ? validate_numeric_keys(name, *map, {0, 1, 2})
          : (maximum_plutus_key == 3U ? validate_numeric_keys(name, *map, {0, 1, 2, 3})
                                      : validate_numeric_keys(name, *map, {0, 1, 2, 3, 4}));
  if (!keys) return keys;
  for (const auto& [key_value, field] : map->entries) {
    auto key = unsigned_value(name, key_value, "auxiliary key");
    if (!key) return std::unexpected(key.error());
    if (*key == 0U) {
      auto metadata = validate_metadata_map(name, field);
      if (!metadata) return metadata;
    } else {
      const auto* scripts = field.as_array();
      if (scripts == nullptr) {
        return std::unexpected(model_error(name, "auxiliary scripts must be an array"));
      }
      for (const auto& script : scripts->values) {
        if (*key == 1U) {
          auto valid = validate_native_script(name, script);
          if (!valid) return valid;
        } else if (script.as_byte_string() == nullptr) {
          return std::unexpected(model_error(name, "Plutus auxiliary script must be bytes"));
        }
      }
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_cost_models(std::string_view name, const CborValue& value) {
  const auto* map = value.as_map();
  if (map == nullptr) return std::unexpected(model_error(name, "cost models must be a map"));
  std::set<std::uint64_t> languages;
  for (const auto& [language_value, model_value] : map->entries) {
    auto language = require_unsigned_bound(name, language_value, 255U, "language");
    if (!language) return language;
    const auto language_number = unsigned_value(name, language_value, "language").value();
    if (!languages.insert(language_number).second) {
      return std::unexpected(core::CardanoError(
          core::ErrorCode::duplicate_key, std::string(name) + ": duplicate cost-model language"));
    }
    if (name == "AlonzoProtocolParamUpdate" && language_number != 0U) {
      return std::unexpected(model_error(name, "Alonzo cost-model language must be 0"));
    }
    if (name == "BabbageProtocolParamUpdate" && language_number > 1U) {
      return std::unexpected(model_error(name, "Babbage cost-model language must be 0 or 1"));
    }
    const auto* costs = model_value.as_array();
    if (costs == nullptr) {
      return std::unexpected(model_error(name, "cost model must be an integer array"));
    }
    const auto required =
        name == "AlonzoProtocolParamUpdate"
            ? std::optional<std::size_t>{166U}
            : (name == "BabbageProtocolParamUpdate"
                   ? std::optional<std::size_t>{language_number == 0U ? 166U : 175U}
                   : std::nullopt);
    if (required && costs->values.size() != *required) {
      return std::unexpected(model_error(name, "cost model has the wrong parameter count"));
    }
    for (const auto& cost : costs->values) {
      bool valid = false;
      if (const auto* positive = cost.as_unsigned()) {
        valid = positive->value.to_int64().has_value();
      } else if (const auto* negative = cost.as_negative()) {
        valid = negative->value.to_int64().has_value();
      }
      if (!valid) return std::unexpected(model_error(name, "cost model value must be int64"));
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_protocol_param_update(std::string_view name,
                                                              const CborValue& value, bool conway) {
  const auto* map = value.as_map();
  if (map == nullptr) {
    return std::unexpected(model_error(name, "protocol parameter update must be a map"));
  }
  core::VoidResult keys = std::monostate{};
  if (conway) {
    keys = validate_numeric_keys(name, *map,
                                 {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 16, 17, 18,
                                  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33});
  } else if (name == "BabbageProtocolParamUpdate") {
    keys = validate_numeric_keys(
        name, *map, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  } else if (name == "AlonzoProtocolParamUpdate") {
    keys = validate_numeric_keys(name, *map, {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                                              12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  } else {
    keys = validate_numeric_keys(name, *map,
                                 {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
  }
  if (!keys) return keys;
  for (const auto& [key_value, parameter] : map->entries) {
    auto key = unsigned_value(name, key_value, "protocol parameter key");
    if (!key) return std::unexpected(key.error());
    if (*key == 9U || *key == 10U || *key == 11U || *key == 12U || *key == 33U) {
      auto rational = validate_rational(name, parameter, *key != 9U && *key != 33U);
      if (!rational) return std::unexpected(rational.error().at(*key));
    } else if (*key == 13U) {
      const auto* nonce = parameter.as_array();
      if (nonce == nullptr || nonce->values.empty()) {
        return std::unexpected(model_error(name, "nonce must be a nonempty array").at(*key));
      }
      auto kind = require_unsigned_bound(name, nonce->values[0], 1U, "nonce kind");
      if (!kind) return std::unexpected(kind.error().at(*key));
      auto kind_value = unsigned_value(name, nonce->values[0], "nonce kind").value();
      const auto expected = kind_value == 0U ? 1U : 2U;
      auto shape = require_array_size(name, parameter, expected, expected);
      if (!shape) return std::unexpected(shape.error().at(*key));
      if (kind_value == 1U) {
        auto hash = require_bytes(name, nonce->values[1], 32, "nonce hash");
        if (!hash) return std::unexpected(hash.error().at(*key));
      }
    } else if (*key == 14U) {
      const auto maximum = conway ? 12U
                                  : (name == "BabbageProtocolParamUpdate"
                                         ? 9U
                                         : (name == "AlonzoProtocolParamUpdate" ? 7U : 3U));
      auto version = validate_protocol_version(name, parameter, maximum);
      if (!version) return std::unexpected(version.error().at(*key));
    } else if (*key == 18U) {
      auto models = validate_cost_models(name, parameter);
      if (!models) return std::unexpected(models.error().at(*key));
    } else if (*key == 19U) {
      auto prices = require_array_size(name, parameter, 2, 2);
      if (!prices) return std::unexpected(prices.error().at(*key));
      for (const auto& price : parameter.as_array()->values) {
        auto rational = validate_rational(name, price, true);
        if (!rational) return std::unexpected(rational.error().at(*key));
      }
    } else if (*key == 20U || *key == 21U) {
      auto units = validate_ex_units(name, parameter);
      if (!units) return std::unexpected(units.error().at(*key));
    } else if (*key == 25U || *key == 26U) {
      const auto expected = *key == 25U ? 5U : 10U;
      auto thresholds = require_array_size(name, parameter, expected, expected);
      if (!thresholds) return std::unexpected(thresholds.error().at(*key));
      for (const auto& threshold : parameter.as_array()->values) {
        auto unit = validate_rational(name, threshold, true);
        if (!unit) return std::unexpected(unit.error().at(*key));
      }
    } else {
      auto number = unsigned_value(name, parameter, "protocol parameter");
      if (!number) return std::unexpected(number.error().at(*key));
      const bool uint32_field = *key == 2U || *key == 3U || *key == 7U || *key == 22U ||
                                *key == 28U || *key == 29U || *key == 32U;
      const bool uint16_field =
          *key == 4U || *key == 8U || *key == 23U || *key == 24U || *key == 27U;
      if ((uint32_field && *number > UINT32_MAX) || (uint16_field && *number > UINT16_MAX)) {
        return std::unexpected(model_error(name, "protocol parameter is out of range").at(*key));
      }
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_proposed_updates(std::string_view name,
                                                         const CborValue& value) {
  const auto* map = value.as_map();
  if (map == nullptr) {
    return std::unexpected(model_error(name, "proposed updates must be a map"));
  }
  std::string_view parameter_name = "ShelleyProtocolParamUpdate";
  if (name.find("Alonzo") != std::string_view::npos) parameter_name = "AlonzoProtocolParamUpdate";
  if (name.find("Babbage") != std::string_view::npos) {
    parameter_name = "BabbageProtocolParamUpdate";
  }
  for (std::size_t index = 0; index < map->entries.size(); ++index) {
    auto genesis = require_bytes(name, map->entries[index].first, 28, "genesis hash");
    if (!genesis) return std::unexpected(genesis.error().at(index));
    auto parameters =
        validate_protocol_param_update(parameter_name, map->entries[index].second, false);
    if (!parameters) return std::unexpected(parameters.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_era_update(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  auto proposals = validate_proposed_updates(name, value.as_array()->values[0]);
  auto epoch = unsigned_value(name, value.as_array()->values[1], "update epoch");
  if (!proposals || !epoch) {
    return std::unexpected(!proposals ? proposals.error().at(0U) : epoch.error().at(1U));
  }
  return std::monostate{};
}

[[nodiscard]] core::Result<CborValue> decode_byron_embedded(std::string_view name,
                                                            const CborValue& value) {
  const auto* tag = value.as_tag();
  if (tag == nullptr || tag->value == nullptr) {
    return std::unexpected(model_error(name, "Byron embedded value must use tag 24"));
  }
  auto number = tag->tag.to_uint64();
  if (!number || *number != 24U || tag->value->as_byte_string() == nullptr) {
    return std::unexpected(model_error(name, "Byron embedded value must use tag 24 bytes"));
  }
  return core::cbor::decode_embedded_cbor(value);
}
