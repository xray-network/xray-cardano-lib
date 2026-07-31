
[[nodiscard]] core::VoidResult validate_anchor(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  const auto* url = fields[0].as_text_string();
  if (url == nullptr || url->value.size() > 128U) {
    return std::unexpected(model_error(name, "anchor URL must be UTF-8 text of at most 128 bytes"));
  }
  return require_bytes(name, fields[1], 32, "anchor document hash");
}

[[nodiscard]] core::VoidResult validate_drep(std::string_view name, const CborValue& value) {
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(model_error(name, "DRep must be a nonempty array"));
  }
  auto kind = unsigned_value(name, fields->values[0], "DRep kind");
  if (!kind || *kind > 3U) {
    return std::unexpected(kind ? model_error(name, "DRep kind must be in 0..3") : kind.error());
  }
  const auto expected = *kind <= 1U ? 2U : 1U;
  auto shape = require_array_size(name, value, expected, expected);
  if (!shape) return shape;
  return *kind <= 1U ? require_bytes(name, fields->values[1], 28, "DRep credential")
                     : core::VoidResult(std::monostate{});
}

[[nodiscard]] core::VoidResult validate_gov_action_id(std::string_view name,
                                                      const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto hash = require_bytes(name, fields[0], 32, "transaction id");
  if (!hash) return hash;
  return require_unsigned_bound(name, fields[1], 65'535U, "action index");
}

[[nodiscard]] core::VoidResult validate_protocol_version(std::string_view name,
                                                         const CborValue& value,
                                                         std::uint64_t max_major) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto major = require_unsigned_bound(name, fields[0], max_major, "protocol major");
  if (!major) return major;
  return require_unsigned_bound(name, fields[1], UINT32_MAX, "protocol minor");
}

[[nodiscard]] core::VoidResult validate_gov_action(std::string_view name, const CborValue& value) {
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(model_error(name, "governance action must be a nonempty array"));
  }
  auto kind = unsigned_value(name, fields->values[0], "governance action kind");
  if (!kind || *kind > 6U) {
    return std::unexpected(kind ? model_error(name, "governance action kind must be in 0..6")
                                : kind.error());
  }
  static constexpr std::array<std::size_t, 7> lengths{4, 3, 3, 2, 5, 3, 1};
  auto shape = require_array_size(name, value, lengths[*kind], lengths[*kind]);
  if (!shape) return shape;
  if ((*kind == 0U || *kind == 1U || *kind == 3U || *kind == 4U || *kind == 5U) &&
      !is_null(fields->values[1])) {
    auto action_id = validate_gov_action_id(name, fields->values[1]);
    if (!action_id) return std::unexpected(action_id.error().at(1U));
  }
  if (*kind == 1U) {
    auto version = validate_protocol_version(name, fields->values[2], 12U);
    if (!version) return std::unexpected(version.error().at(2U));
  } else if (*kind == 2U) {
    const auto* withdrawals = fields->values[1].as_map();
    if (withdrawals == nullptr || withdrawals->entries.empty()) {
      return std::unexpected(model_error(name, "treasury withdrawals must be a nonempty map"));
    }
    for (const auto& [account, coin] : withdrawals->entries) {
      if (account.as_byte_string() == nullptr) {
        return std::unexpected(model_error(name, "reward account must be bytes"));
      }
      auto amount = unsigned_value(name, coin, "treasury withdrawal");
      if (!amount) return std::unexpected(amount.error());
    }
    auto guardrail =
        validate_optional_hash(name, fields->values[2], 28U, "treasury guardrails script hash");
    if (!guardrail) return std::unexpected(guardrail.error().at(2U));
  } else if (*kind == 0U) {
    auto parameters =
        validate_protocol_param_update("ProtocolParamUpdate", fields->values[2], true);
    auto guardrail = validate_optional_hash(name, fields->values[3], 28U, "guardrails script hash");
    if (!parameters || !guardrail) {
      return std::unexpected(!parameters ? parameters.error().at(2U) : guardrail.error().at(3U));
    }
  } else if (*kind == 4U) {
    auto removed = set_array(name, fields->values[2], "removed committee members");
    if (!removed) return std::unexpected(removed.error().at(2U));
    for (std::size_t index = 0; index < (*removed)->values.size(); ++index) {
      auto credential = validate_credential(name, (*removed)->values[index]);
      if (!credential) return std::unexpected(credential.error().at(2U).at(index));
    }
    const auto* added = fields->values[3].as_map();
    if (added == nullptr) {
      return std::unexpected(model_error(name, "committee additions must be a map").at(3U));
    }
    for (std::size_t index = 0; index < added->entries.size(); ++index) {
      auto credential = validate_credential(name, added->entries[index].first);
      auto epoch = unsigned_value(name, added->entries[index].second, "committee epoch");
      if (!credential || !epoch) {
        return std::unexpected(!credential ? credential.error().at(3U).at(index)
                                           : epoch.error().at(3U).at(index));
      }
    }
    auto quorum = validate_rational(name, fields->values[4], true);
    if (!quorum) return std::unexpected(quorum.error().at(4U));
  } else if (*kind == 5U) {
    auto constitution = require_array_size(name, fields->values[2], 2, 2);
    if (!constitution) return std::unexpected(constitution.error().at(2U));
    auto anchor = validate_anchor(name, fields->values[2].as_array()->values[0]);
    if (!anchor) return std::unexpected(anchor.error().at(2U).at(0U));
    const auto& guardrail = fields->values[2].as_array()->values[1];
    if (!is_null(guardrail)) {
      auto hash = require_bytes(name, guardrail, 28, "constitution script hash");
      if (!hash) return std::unexpected(hash.error().at(2U).at(1U));
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_proposal(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto deposit = unsigned_value(name, fields[0], "proposal deposit");
  if (!deposit) return std::unexpected(deposit.error().at(0U));
  auto reward = require_bytes_range(name, fields[1], 1, 65'535, "reward account");
  if (!reward) return std::unexpected(reward.error().at(1U));
  auto action = validate_gov_action(name, fields[2]);
  if (!action) return std::unexpected(action.error().at(2U));
  auto anchor = validate_anchor(name, fields[3]);
  return anchor ? core::VoidResult(std::monostate{}) : std::unexpected(anchor.error().at(3U));
}

[[nodiscard]] core::VoidResult validate_voter(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto kind = require_unsigned_bound(name, fields[0], 4U, "voter kind");
  if (!kind) return kind;
  return require_bytes(name, fields[1], 28, "voter credential");
}

[[nodiscard]] core::VoidResult validate_voting_procedure(std::string_view name,
                                                         const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto vote = require_unsigned_bound(name, fields[0], 2U, "vote");
  if (!vote) return vote;
  return is_null(fields[1]) ? core::VoidResult(std::monostate{}) : validate_anchor(name, fields[1]);
}

[[nodiscard]] core::VoidResult validate_voting_procedures(std::string_view name,
                                                          const CborValue& value) {
  const auto* voters = value.as_map();
  if (voters == nullptr || voters->entries.empty()) {
    return std::unexpected(model_error(name, "voting procedures must be a nonempty map"));
  }
  for (const auto& [voter_value, action_values] : voters->entries) {
    auto voter = validate_voter(name, voter_value);
    if (!voter) return voter;
    const auto* actions = action_values.as_map();
    if (actions == nullptr || actions->entries.empty()) {
      return std::unexpected(model_error(name, "voter action map must be nonempty"));
    }
    for (const auto& [action_id_value, procedure_value] : actions->entries) {
      auto action_id = validate_gov_action_id(name, action_id_value);
      if (!action_id) return action_id;
      auto procedure = validate_voting_procedure(name, procedure_value);
      if (!procedure) return procedure;
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_relay(std::string_view name, const CborValue& value) {
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(model_error(name, "relay must be a nonempty array"));
  }
  auto kind = require_unsigned_bound(name, fields->values[0], 2U, "relay kind");
  if (!kind) return kind;
  auto kind_value = unsigned_value(name, fields->values[0], "relay kind").value();
  const std::size_t expected = kind_value == 0U ? 4U : (kind_value == 1U ? 3U : 2U);
  auto shape = require_array_size(name, value, expected, expected);
  if (!shape) return shape;
  if (kind_value <= 1U && !is_null(fields->values[1])) {
    auto port = require_unsigned_bound(name, fields->values[1], 65'535U, "relay port");
    if (!port) return port;
  }
  if (kind_value == 0U) {
    if (!is_null(fields->values[2])) {
      auto ipv4 = require_bytes(name, fields->values[2], 4, "IPv4");
      if (!ipv4) return ipv4;
    }
    if (!is_null(fields->values[3])) {
      auto ipv6 = require_bytes(name, fields->values[3], 16, "IPv6");
      if (!ipv6) return ipv6;
    }
  } else {
    const auto& dns_value = fields->values[kind_value == 1U ? 2U : 1U];
    const auto* dns = dns_value.as_text_string();
    const auto maximum = name.find("Shelley") != std::string_view::npos ||
                                 name.find("Allegra") != std::string_view::npos ||
                                 name.find("Mary") != std::string_view::npos ||
                                 name.find("Alonzo") != std::string_view::npos ||
                                 name.find("Babbage") != std::string_view::npos
                             ? 64U
                             : 128U;
    if (dns == nullptr || dns->value.size() > maximum) {
      return std::unexpected(model_error(
          name, "DNS name must be text of at most " + std::to_string(maximum) + " bytes"));
    }
  }
  return std::monostate{};
}
