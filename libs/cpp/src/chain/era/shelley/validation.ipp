
[[nodiscard]] core::VoidResult validate_certificate(std::string_view name, const CborValue& value) {
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(model_error(name, "certificate must be a nonempty array"));
  }
  auto kind = unsigned_value(name, fields->values[0], "certificate kind");
  if (!kind) return std::unexpected(kind.error());
  std::size_t expected = 0;
  if (name == "Certificate") {
    static constexpr std::array<std::size_t, 19> lengths{2, 2, 3, 10, 3, 0, 0, 3, 3, 3,
                                                         4, 4, 4, 5,  3, 3, 4, 3, 3};
    if (*kind >= lengths.size() || lengths[*kind] == 0) {
      return std::unexpected(model_error(name, "certificate kind is not valid for Conway"));
    }
    expected = lengths[*kind];
  } else {
    static constexpr std::array<std::size_t, 7> lengths{2, 2, 3, 10, 3, 4, 2};
    if (*kind >= lengths.size()) {
      return std::unexpected(model_error(name, "certificate kind must be in 0..6"));
    }
    expected = lengths[*kind];
  }
  auto shape = require_array_size(name, value, expected, expected);
  if (!shape) return shape;
  if (*kind != 3 && *kind != 4 && *kind != 5 && *kind != 6) {
    auto credential = validate_credential(name, fields->values[1]);
    if (!credential) return std::unexpected(credential.error().at(1U));
  }
  if (*kind == 2 || *kind == 4 || *kind == 10 || *kind == 11 || *kind == 13) {
    const std::size_t index = *kind == 4 ? 1 : 2;
    auto pool = require_bytes(name, fields->values[index], 28, "pool hash");
    if (!pool) return std::unexpected(pool.error().at(index));
  }
  if (*kind == 3U) {
    auto pool = require_bytes(name, fields->values[1], 28, "pool operator");
    auto vrf = require_bytes(name, fields->values[2], 32, "pool VRF key hash");
    auto pledge = unsigned_value(name, fields->values[3], "pool pledge");
    auto cost = unsigned_value(name, fields->values[4], "pool cost");
    auto margin = validate_rational(name, fields->values[5], true);
    auto reward = require_bytes_range(name, fields->values[6], 1, 65'535, "reward account");
    if (!pool || !vrf || !pledge || !cost || !margin || !reward) {
      return std::unexpected(!pool     ? pool.error()
                             : !vrf    ? vrf.error()
                             : !pledge ? pledge.error()
                             : !cost   ? cost.error()
                             : !margin ? margin.error()
                                       : reward.error());
    }
    auto owners = set_array(name, fields->values[7], "pool owners");
    if (!owners) return std::unexpected(owners.error().at(7U));
    for (std::size_t index = 0; index < (*owners)->values.size(); ++index) {
      auto owner = require_bytes(name, (*owners)->values[index], 28, "pool owner");
      if (!owner) return std::unexpected(owner.error().at(7U).at(index));
    }
    const auto* relays = fields->values[8].as_array();
    if (relays == nullptr) {
      return std::unexpected(model_error(name, "pool relays must be an array").at(8U));
    }
    for (std::size_t index = 0; index < relays->values.size(); ++index) {
      auto relay = validate_relay(name, relays->values[index]);
      if (!relay) return std::unexpected(relay.error().at(8U).at(index));
    }
    const auto& metadata = fields->values[9];
    if (!is_null(metadata)) {
      auto valid = validate_pool_metadata(
          name == "Certificate" ? "PoolMetadata" : "ShelleyPoolMetadata", metadata);
      if (!valid) return std::unexpected(valid.error().at(9U));
    }
  }
  if (*kind == 5U) {
    auto genesis = require_bytes(name, fields->values[1], 28U, "genesis hash");
    auto delegate = require_bytes(name, fields->values[2], 28U, "genesis delegate hash");
    auto vrf = require_bytes(name, fields->values[3], 32U, "genesis delegate VRF hash");
    if (!genesis || !delegate || !vrf) {
      return std::unexpected(!genesis ? genesis.error().at(1U)
                                      : (!delegate ? delegate.error().at(2U) : vrf.error().at(3U)));
    }
  }
  if (*kind == 6U) {
    auto mir = validate_mir(name, fields->values[1]);
    if (!mir) return std::unexpected(mir.error().at(1U));
  }
  if (*kind == 14U) {
    auto credential = validate_credential(name, fields->values[2]);
    if (!credential) return std::unexpected(credential.error().at(2U));
  }
  if (*kind == 9U || *kind == 10U || *kind == 12U || *kind == 13U) {
    const auto index = (*kind == 9U || *kind == 12U) ? 2U : 3U;
    auto drep = validate_drep(name, fields->values[index]);
    if (!drep) return std::unexpected(drep.error().at(index));
  }
  if (*kind == 15U || *kind == 16U || *kind == 18U) {
    const auto index = *kind == 16U ? 3U : 2U;
    if (!is_null(fields->values[index])) {
      auto anchor = validate_anchor(name, fields->values[index]);
      if (!anchor) return std::unexpected(anchor.error().at(index));
    }
  }
  std::optional<std::size_t> coin_index;
  if (*kind == 7U || *kind == 8U || *kind == 16U || *kind == 17U) coin_index = 2U;
  if (*kind == 11U || *kind == 12U) coin_index = 3U;
  if (*kind == 13U) coin_index = 4U;
  if (coin_index) {
    auto coin = unsigned_value(name, fields->values[*coin_index], "certificate coin");
    if (!coin) return std::unexpected(coin.error().at(*coin_index));
  }
  if (*kind == 4U) {
    auto epoch = unsigned_value(name, fields->values[2], "retirement epoch");
    if (!epoch) return std::unexpected(epoch.error().at(2U));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_multiasset(std::string_view name, const CborValue& value,
                                                   bool mint) {
  const auto* policies = value.as_map();
  if (policies == nullptr || (mint && policies->entries.empty())) {
    return std::unexpected(
        model_error(name, mint ? "mint must be a nonempty map" : "multiasset must be a map"));
  }
  for (std::size_t policy_index = 0; policy_index < policies->entries.size(); ++policy_index) {
    const auto& [policy, assets_value] = policies->entries[policy_index];
    auto policy_hash = require_bytes(name, policy, 28, "policy id");
    if (!policy_hash) return std::unexpected(policy_hash.error().at(policy_index));
    const auto* assets = assets_value.as_map();
    if (assets == nullptr || assets->entries.empty()) {
      return std::unexpected(
          model_error(name, "policy asset map must be nonempty").at(policy_index));
    }
    for (std::size_t asset_index = 0; asset_index < assets->entries.size(); ++asset_index) {
      const auto& [asset_name, quantity] = assets->entries[asset_index];
      auto bounded_name = require_bytes_range(name, asset_name, 0, 32, "asset name");
      if (!bounded_name) {
        return std::unexpected(bounded_name.error().at(policy_index).at(asset_index));
      }
      if (mint) {
        bool valid_quantity = false;
        if (const auto* positive = quantity.as_unsigned()) {
          auto converted = positive->value.to_int64();
          valid_quantity = converted && *converted != 0;
        } else if (const auto* negative = quantity.as_negative()) {
          auto converted = negative->value.to_int64();
          valid_quantity = converted && *converted != 0;
        }
        if (!valid_quantity) {
          return std::unexpected(model_error(name, "mint quantity must be a nonzero int64")
                                     .at(policy_index)
                                     .at(asset_index));
        }
      } else {
        auto coin = unsigned_value(name, quantity, "asset quantity");
        if (!coin) {
          return std::unexpected(coin.error().at(policy_index).at(asset_index));
        }
      }
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_value(std::string_view name, const CborValue& value) {
  if (value.as_unsigned() != nullptr) return std::monostate{};
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto coin = unsigned_value(name, fields[0], "coin");
  if (!coin) return std::unexpected(coin.error().at(0U));
  auto assets = validate_multiasset(name, fields[1], false);
  return assets ? core::VoidResult(std::monostate{}) : std::unexpected(assets.error().at(1U));
}

[[nodiscard]] core::VoidResult validate_datum_option(std::string_view name,
                                                     const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto kind = unsigned_value(name, fields[0], "datum option kind");
  if (!kind || *kind > 1) {
    return std::unexpected(kind ? model_error(name, "datum option kind must be 0 or 1")
                                : kind.error());
  }
  if (*kind == 0) return require_bytes(name, fields[1], 32, "datum hash");
  auto datum = validate_plutus_data_node(fields[1]);
  return datum ? core::VoidResult(std::monostate{}) : std::unexpected(datum.error().at(1U));
}

[[nodiscard]] core::VoidResult validate_script(std::string_view name, const CborValue& value,
                                               std::uint64_t maximum_kind) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto kind = unsigned_value(name, fields[0], "script kind");
  if (!kind || *kind > maximum_kind) {
    return std::unexpected(kind ? model_error(name, "script kind is out of range") : kind.error());
  }
  if (*kind == 0) return validate_native_script(name, fields[1]);
  if (fields[1].as_byte_string() == nullptr) {
    return std::unexpected(model_error(name, "Plutus script must be bytes"));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_script_ref(std::string_view name, const CborValue& value,
                                                   std::uint64_t maximum_kind) {
  const auto* tag = value.as_tag();
  if (tag == nullptr || tag->value == nullptr) {
    return std::unexpected(model_error(name, "script reference must be tagged embedded CBOR"));
  }
  auto tag_number = tag->tag.to_uint64();
  if (!tag_number || *tag_number != 24U) {
    return std::unexpected(model_error(name, "script reference tag must be 24"));
  }
  auto embedded = core::cbor::decode_embedded_cbor(value);
  if (!embedded) return std::unexpected(embedded.error());
  return validate_script(name, *embedded, maximum_kind);
}

enum class OutputEra : std::uint8_t { shelley, mary, alonzo, babbage, conway };

[[nodiscard]] core::VoidResult validate_transaction_output(std::string_view name,
                                                           const CborValue& value, OutputEra era) {
  if (const auto* fields = value.as_array()) {
    const std::size_t maximum =
        era == OutputEra::alonzo || era == OutputEra::babbage || era == OutputEra::conway ? 3U : 2U;
    auto shape = require_array_size(name, value, 2, maximum);
    if (!shape) return shape;
    auto address = require_bytes_range(name, fields->values[0], 1, 65'535, "address");
    if (!address) return std::unexpected(address.error().at(0U));
    auto amount = validate_value(name, fields->values[1]);
    if (!amount) return std::unexpected(amount.error().at(1U));
    if (fields->values.size() == 3U) {
      auto datum = require_bytes(name, fields->values[2], 32, "datum hash");
      if (!datum) return std::unexpected(datum.error().at(2U));
    }
    return std::monostate{};
  }
  if (era != OutputEra::babbage && era != OutputEra::conway) {
    return std::unexpected(model_error(name, "map output is unavailable in this era"));
  }
  const auto* map = value.as_map();
  if (map == nullptr) return std::unexpected(model_error(name, "transaction output is invalid"));
  auto keys = validate_numeric_keys(name, *map, {0, 1, 2, 3}, {0, 1});
  if (!keys) return keys;
  auto address_field = numeric_field(name, *map, 0);
  auto value_field = numeric_field(name, *map, 1);
  if (!address_field || !value_field) {
    return std::unexpected(!address_field ? address_field.error() : value_field.error());
  }
  auto address = require_bytes_range(name, **address_field, 1, 65'535, "address");
  if (!address) return address;
  auto amount = validate_value(name, **value_field);
  if (!amount) return amount;
  auto datum = numeric_field(name, *map, 2);
  if (!datum) return std::unexpected(datum.error());
  if (*datum != nullptr) {
    auto valid = validate_datum_option(name, **datum);
    if (!valid) return valid;
  }
  auto reference = numeric_field(name, *map, 3);
  if (!reference) return std::unexpected(reference.error());
  if (*reference != nullptr) {
    auto valid = validate_script_ref(name, **reference, era == OutputEra::conway ? 3U : 2U);
    if (!valid) return valid;
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_outputs(std::string_view name, const CborValue& value,
                                                OutputEra era) {
  const auto* outputs = value.as_array();
  if (outputs == nullptr) return std::unexpected(model_error(name, "outputs must be an array"));
  for (std::size_t index = 0; index < outputs->values.size(); ++index) {
    auto valid = validate_transaction_output(name, outputs->values[index], era);
    if (!valid) return std::unexpected(valid.error().at(index));
  }
  return std::monostate{};
}

enum class BodyEra : std::uint8_t { shelley, allegra, mary, alonzo, babbage, conway };

[[nodiscard]] std::uint64_t maximum_protocol_major(BodyEra era) {
  switch (era) {
    case BodyEra::shelley:
      return 3U;
    case BodyEra::allegra:
      return 4U;
    case BodyEra::mary:
      return 5U;
    case BodyEra::alonzo:
      return 7U;
    case BodyEra::babbage:
      return 9U;
    case BodyEra::conway:
      return 12U;
  }
}

[[nodiscard]] OutputEra output_era(BodyEra era) {
  switch (era) {
    case BodyEra::shelley:
    case BodyEra::allegra:
      return OutputEra::shelley;
    case BodyEra::mary:
      return OutputEra::mary;
    case BodyEra::alonzo:
      return OutputEra::alonzo;
    case BodyEra::babbage:
      return OutputEra::babbage;
    case BodyEra::conway:
      return OutputEra::conway;
  }
}

[[nodiscard]] core::VoidResult validate_transaction_body(std::string_view name,
                                                         const CborValue& value, BodyEra era) {
  const auto* map = value.as_map();
  if (map == nullptr) return std::unexpected(model_error(name, "transaction body must be a map"));
  core::VoidResult keys = std::monostate{};
  switch (era) {
    case BodyEra::shelley:
      keys = validate_numeric_keys(name, *map, {0, 1, 2, 3, 4, 5, 6, 7}, {0, 1, 2});
      break;
    case BodyEra::allegra:
      keys = validate_numeric_keys(name, *map, {0, 1, 2, 3, 4, 5, 6, 7, 8}, {0, 1, 2});
      break;
    case BodyEra::mary:
      keys = validate_numeric_keys(name, *map, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, {0, 1, 2});
      break;
    case BodyEra::alonzo:
      keys = validate_numeric_keys(name, *map, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 14, 15},
                                   {0, 1, 2});
      break;
    case BodyEra::babbage:
      keys = validate_numeric_keys(
          name, *map, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 14, 15, 16, 17, 18}, {0, 1, 2});
      break;
    case BodyEra::conway:
      keys = validate_numeric_keys(
          name, *map, {0, 1, 2, 3, 4, 5, 7, 8, 9, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22},
          {0, 1, 2});
      break;
  }
  if (!keys) return keys;
  auto inputs = numeric_field(name, *map, 0);
  auto outputs_field = numeric_field(name, *map, 1);
  auto fee = numeric_field(name, *map, 2);
  if (!inputs || !outputs_field || !fee) {
    return std::unexpected(!inputs ? inputs.error()
                                   : (!outputs_field ? outputs_field.error() : fee.error()));
  }
  auto input_valid = validate_transaction_input_set(name, **inputs, "inputs", true);
  if (!input_valid) return std::unexpected(input_valid.error().at(0U));
  auto output_valid = validate_outputs(name, **outputs_field, output_era(era));
  if (!output_valid) return std::unexpected(output_valid.error().at(1U));
  auto fee_valid = unsigned_value(name, **fee, "fee");
  if (!fee_valid) return std::unexpected(fee_valid.error().at(2U));
  for (const auto key : {3U, 8U, 17U, 21U}) {
    auto field = numeric_field(name, *map, key);
    if (!field) return std::unexpected(field.error());
    if (*field != nullptr) {
      auto valid = unsigned_value(name, **field, "unsigned body field");
      if (!valid) return std::unexpected(valid.error().at(key));
    }
  }
  auto donation = numeric_field(name, *map, 22);
  if (!donation) return std::unexpected(donation.error());
  if (*donation != nullptr) {
    auto valid = unsigned_value(name, **donation, "donation");
    if (!valid || *valid == 0U) {
      return std::unexpected(valid ? model_error(name, "donation must be positive").at(22U)
                                   : valid.error().at(22U));
    }
  }
  for (const auto key : {13U, 18U}) {
    auto field = numeric_field(name, *map, key);
    if (!field) return std::unexpected(field.error());
    if (*field != nullptr) {
      auto valid =
          validate_transaction_input_set(name, **field, "transaction input set", key == 13U);
      if (!valid) return std::unexpected(valid.error().at(key));
    }
  }
  auto mint = numeric_field(name, *map, 9);
  if (!mint) return std::unexpected(mint.error());
  if (*mint != nullptr) {
    auto valid = validate_multiasset(name, **mint, true);
    if (!valid) return std::unexpected(valid.error().at(9U));
  }
  auto certificates = numeric_field(name, *map, 4);
  if (!certificates) return std::unexpected(certificates.error());
  if (*certificates != nullptr) {
    auto values = set_array(name, **certificates, "certificates");
    if (!values) return std::unexpected(values.error().at(4U));
    if ((*values)->values.empty() && era == BodyEra::conway) {
      return std::unexpected(model_error(name, "Conway certificates must be nonempty").at(4U));
    }
    for (std::size_t index = 0; index < (*values)->values.size(); ++index) {
      auto valid = validate_certificate(
          era == BodyEra::conway ? "Certificate" : "ShelleyCertificate", (*values)->values[index]);
      if (!valid) return std::unexpected(valid.error().at(4U).at(index));
    }
  }
  auto withdrawals = numeric_field(name, *map, 5);
  if (!withdrawals) return std::unexpected(withdrawals.error());
  if (*withdrawals != nullptr) {
    auto valid = validate_withdrawals(name, **withdrawals, true);
    if (!valid) return std::unexpected(valid.error().at(5U));
  }
  auto update = numeric_field(name, *map, 6);
  if (!update) return std::unexpected(update.error());
  if (*update != nullptr) {
    std::string_view update_name = "ShelleyUpdate";
    if (era == BodyEra::alonzo) update_name = "AlonzoUpdate";
    if (era == BodyEra::babbage) update_name = "BabbageUpdate";
    auto valid = validate_era_update(update_name, **update);
    if (!valid) return std::unexpected(valid.error().at(6U));
  }
  for (const auto key : {7U, 11U}) {
    auto field = numeric_field(name, *map, key);
    if (!field) return std::unexpected(field.error());
    if (*field != nullptr) {
      auto valid = require_bytes(name, **field, 32, "body hash");
      if (!valid) return std::unexpected(valid.error().at(key));
    }
  }
  auto signers = numeric_field(name, *map, 14);
  if (!signers) return std::unexpected(signers.error());
  if (*signers != nullptr) {
    auto values = set_array(name, **signers, "required signers");
    if (!values) return std::unexpected(values.error().at(14U));
    for (std::size_t index = 0; index < (*values)->values.size(); ++index) {
      auto hash = require_bytes(name, (*values)->values[index], 28, "required signer");
      if (!hash) return std::unexpected(hash.error().at(14U).at(index));
    }
  }
  auto network = numeric_field(name, *map, 15);
  if (!network) return std::unexpected(network.error());
  if (*network != nullptr) {
    auto valid = require_unsigned_bound(name, **network, 1U, "network id");
    if (!valid) return std::unexpected(valid.error().at(15U));
  }
  auto collateral_return = numeric_field(name, *map, 16);
  if (!collateral_return) return std::unexpected(collateral_return.error());
  if (*collateral_return != nullptr) {
    auto valid = validate_transaction_output(name, **collateral_return, output_era(era));
    if (!valid) return std::unexpected(valid.error().at(16U));
  }
  auto votes = numeric_field(name, *map, 19);
  if (!votes) return std::unexpected(votes.error());
  if (*votes != nullptr) {
    auto valid = validate_voting_procedures(name, **votes);
    if (!valid) return std::unexpected(valid.error().at(19U));
  }
  auto proposals = numeric_field(name, *map, 20);
  if (!proposals) return std::unexpected(proposals.error());
  if (*proposals != nullptr) {
    auto values = set_array(name, **proposals, "proposal procedures");
    if (!values) return std::unexpected(values.error().at(20U));
    if ((*values)->values.empty()) {
      return std::unexpected(model_error(name, "proposal procedures must be nonempty").at(20U));
    }
    for (std::size_t index = 0; index < (*values)->values.size(); ++index) {
      auto valid = validate_proposal(name, (*values)->values[index]);
      if (!valid) return std::unexpected(valid.error().at(20U).at(index));
    }
  }
  return std::monostate{};
}
