
[[nodiscard]] core::VoidResult validate_byron_address(std::string_view name,
                                                      const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto content = decode_byron_embedded(name, fields[0]);
  if (!content) return std::unexpected(content.error().at(0U));
  auto content_shape = require_array_size(name, *content, 3, 3);
  if (!content_shape) return std::unexpected(content_shape.error().at(0U));
  auto address_id = require_bytes(name, content->as_array()->values[0], 28, "Byron address id");
  if (!address_id) return std::unexpected(address_id.error().at(0U));
  const auto* attributes = content->as_array()->values[1].as_map();
  if (attributes == nullptr) {
    return std::unexpected(model_error(name, "Byron address attributes must be a map").at(0U));
  }
  std::set<std::uint64_t> attribute_keys;
  for (std::size_t index = 0; index < attributes->entries.size(); ++index) {
    auto key = unsigned_value(name, attributes->entries[index].first, "Byron address attribute");
    if (!key || (*key != 1U && *key != 2U) ||
        attributes->entries[index].second.as_byte_string() == nullptr) {
      return std::unexpected(
          !key ? key.error().at(0U).at(index)
               : model_error(name, "invalid Byron address attribute").at(0U).at(index));
    }
    if (!attribute_keys.insert(*key).second) {
      return std::unexpected(
          core::CardanoError(core::ErrorCode::duplicate_key,
                             std::string(name) + ": duplicate Byron address attribute"));
    }
  }
  auto kind = unsigned_value(name, content->as_array()->values[2], "Byron address kind");
  if (!kind) return std::unexpected(kind.error().at(0U));
  auto crc = unsigned_value(name, fields[1], "Byron address checksum");
  return crc ? core::VoidResult(std::monostate{}) : std::unexpected(crc.error().at(1U));
}

[[nodiscard]] core::VoidResult validate_byron_tx_in(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto kind = require_unsigned_bound(name, fields[0], UINT8_MAX, "Byron input kind");
  if (!kind) return kind;
  auto embedded = decode_byron_embedded(name, fields[1]);
  if (!embedded) return std::unexpected(embedded.error().at(1U));
  if (unsigned_value(name, fields[0], "Byron input kind").value() == 0U) {
    auto pointer = require_array_size(name, *embedded, 2, 2);
    if (!pointer) return std::unexpected(pointer.error().at(1U));
    auto hash = require_bytes(name, embedded->as_array()->values[0], 32, "Byron transaction id");
    auto index =
        require_unsigned_bound(name, embedded->as_array()->values[1], UINT32_MAX, "Byron index");
    if (!hash || !index) {
      return std::unexpected(!hash ? hash.error().at(1U) : index.error().at(1U));
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_tx_out(std::string_view name,
                                                     const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  auto address = validate_byron_address(name, value.as_array()->values[0]);
  if (!address) return std::unexpected(address.error().at(0U));
  auto amount = unsigned_value(name, value.as_array()->values[1], "Byron output amount");
  return amount ? core::VoidResult(std::monostate{}) : std::unexpected(amount.error().at(1U));
}

[[nodiscard]] core::VoidResult validate_byron_tx(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 3, 3);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  const auto* inputs = fields[0].as_array();
  const auto* outputs = fields[1].as_array();
  if (inputs == nullptr || inputs->values.empty() || outputs == nullptr ||
      outputs->values.empty() || fields[2].as_map() == nullptr) {
    return std::unexpected(model_error(name, "invalid Byron transaction collections"));
  }
  for (std::size_t index = 0; index < inputs->values.size(); ++index) {
    auto input = validate_byron_tx_in(name, inputs->values[index]);
    if (!input) return std::unexpected(input.error().at(0U).at(index));
  }
  for (std::size_t index = 0; index < outputs->values.size(); ++index) {
    auto output = validate_byron_tx_out(name, outputs->values[index]);
    if (!output) return std::unexpected(output.error().at(1U).at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_witness(std::string_view name,
                                                      const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto kind = require_unsigned_bound(name, fields[0], UINT8_MAX, "Byron witness kind");
  if (!kind) return kind;
  auto embedded = decode_byron_embedded(name, fields[1]);
  if (!embedded) return std::unexpected(embedded.error().at(1U));
  const auto kind_value = unsigned_value(name, fields[0], "Byron witness kind").value();
  if (kind_value == 0U || kind_value == 2U) {
    auto witness = require_array_size(name, *embedded, 2, 2);
    if (!witness || embedded->as_array()->values[0].as_byte_string() == nullptr ||
        embedded->as_array()->values[1].as_byte_string() == nullptr) {
      return std::unexpected(
          witness ? model_error(name, "Byron key witness fields must be bytes").at(1U)
                  : witness.error().at(1U));
    }
  } else if (kind_value == 1U) {
    auto witness = require_array_size(name, *embedded, 2, 2);
    if (!witness) return std::unexpected(witness.error().at(1U));
    for (const auto& script : embedded->as_array()->values) {
      auto pair = require_array_size(name, script, 2, 2);
      if (!pair) return std::unexpected(pair.error().at(1U));
      auto version =
          require_unsigned_bound(name, script.as_array()->values[0], UINT16_MAX, "script version");
      if (!version || script.as_array()->values[1].as_byte_string() == nullptr) {
        return std::unexpected(version ? model_error(name, "script must be bytes").at(1U)
                                       : version.error().at(1U));
      }
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_tx_aux(std::string_view name,
                                                     const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  auto transaction = validate_byron_tx(name, value.as_array()->values[0]);
  if (!transaction) return std::unexpected(transaction.error().at(0U));
  const auto* witnesses = value.as_array()->values[1].as_array();
  if (witnesses == nullptr) {
    return std::unexpected(model_error(name, "Byron witnesses must be an array").at(1U));
  }
  for (std::size_t index = 0; index < witnesses->values.size(); ++index) {
    auto witness = validate_byron_witness(name, witnesses->values[index]);
    if (!witness) return std::unexpected(witness.error().at(1U).at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_version(std::string_view name,
                                                      const CborValue& value) {
  auto shape = require_array_size(name, value, 3, 3);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto major = require_unsigned_bound(name, fields[0], UINT16_MAX, "version major");
  auto minor = require_unsigned_bound(name, fields[1], UINT16_MAX, "version minor");
  auto alt = require_unsigned_bound(name, fields[2], UINT8_MAX, "version alt");
  if (!major || !minor || !alt) {
    return std::unexpected(!major ? major.error() : (!minor ? minor.error() : alt.error()));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_delegation(std::string_view name,
                                                         const CborValue& value) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  core::VoidResult epoch = std::monostate{};
  if (fields[0].as_array() != nullptr) {
    auto range = require_array_size(name, fields[0], 2, 2);
    if (!range) {
      epoch = std::unexpected(range.error());
    } else {
      for (const auto& bound : fields[0].as_array()->values) {
        auto number = unsigned_value(name, bound, "delegation epoch range");
        if (!number) {
          epoch = std::unexpected(number.error());
          break;
        }
      }
    }
  } else {
    auto number = unsigned_value(name, fields[0], "delegation epoch");
    if (!number) epoch = std::unexpected(number.error());
  }
  if (!epoch || fields[1].as_byte_string() == nullptr || fields[2].as_byte_string() == nullptr ||
      fields[3].as_byte_string() == nullptr) {
    return std::unexpected(epoch ? model_error(name, "delegation keys/signature must be bytes")
                                 : epoch.error());
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_ssc_cert(std::string_view name,
                                                       const CborValue& value,
                                                       bool allow_legacy = false) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  const bool official =
      fields[2].as_unsigned() != nullptr && fields[0].as_byte_string() != nullptr &&
      fields[1].as_byte_string() != nullptr && fields[3].as_byte_string() != nullptr;
  bool legacy = false;
  if (allow_legacy) {
    std::size_t integers = 0U;
    std::size_t byte_strings = 0U;
    for (const auto& field : fields) {
      if (field.as_unsigned() != nullptr) ++integers;
      if (field.as_byte_string() != nullptr) ++byte_strings;
    }
    legacy = integers == 1U && byte_strings == 3U;
  }
  if (!official && !legacy) {
    return std::unexpected(model_error(name, "invalid official or legacy SSC certificate"));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_ssc_certs(std::string_view name,
                                                        const CborValue& value) {
  auto certificates = set_array(name, value, "SSC certificates");
  if (!certificates) return std::unexpected(certificates.error());
  for (std::size_t index = 0; index < (*certificates)->values.size(); ++index) {
    auto certificate = validate_byron_ssc_cert(name, (*certificates)->values[index], true);
    if (!certificate) return std::unexpected(certificate.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_ssc(std::string_view name, const CborValue& value) {
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(model_error(name, "SSC payload must be a nonempty array"));
  }
  auto kind = require_unsigned_bound(name, fields->values[0], 3U, "SSC kind");
  if (!kind) return kind;
  const auto kind_value = unsigned_value(name, fields->values[0], "SSC kind").value();
  const auto expected = kind_value == 3U ? 2U : 3U;
  auto shape = require_array_size(name, value, expected, expected);
  if (!shape) return shape;
  const auto cert_index = kind_value == 3U ? 1U : 2U;
  auto certificates = validate_byron_ssc_certs(name, fields->values[cert_index]);
  if (!certificates) return std::unexpected(certificates.error().at(cert_index));
  if (kind_value == 0U) {
    auto commitments = set_array(name, fields->values[1], "SSC commitments");
    if (!commitments) return std::unexpected(commitments.error().at(1U));
    for (std::size_t index = 0; index < (*commitments)->values.size(); ++index) {
      auto commitment = validate_ssc_signed_commitment(name, (*commitments)->values[index]);
      if (!commitment) return std::unexpected(commitment.error().at(1U).at(index));
    }
  } else if (kind_value == 1U) {
    const auto* openings = fields->values[1].as_map();
    if (openings == nullptr) {
      return std::unexpected(model_error(name, "SSC openings must be a map").at(1U));
    }
    for (std::size_t index = 0; index < openings->entries.size(); ++index) {
      auto stakeholder = require_bytes(name, openings->entries[index].first, 28U, "stakeholder id");
      if (!stakeholder || openings->entries[index].second.as_byte_string() == nullptr) {
        return std::unexpected(
            !stakeholder ? stakeholder.error().at(1U).at(index)
                         : model_error(name, "SSC opening must be bytes").at(1U).at(index));
      }
    }
  } else if (kind_value == 2U) {
    auto shares = validate_ssc_shares(name, fields->values[1]);
    if (!shares) return std::unexpected(shares.error().at(1U));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_update_vote(std::string_view name,
                                                          const CborValue& value) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto proposal = require_bytes(name, fields[1], 32, "update proposal id");
  if (!proposal || fields[0].as_byte_string() == nullptr ||
      !std::holds_alternative<core::cbor::BooleanValue>(fields[2].node()) ||
      fields[3].as_byte_string() == nullptr) {
    return std::unexpected(proposal ? model_error(name, "invalid update vote fields")
                                    : proposal.error());
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_update_data(std::string_view name,
                                                          const CborValue& value) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  for (std::size_t index = 0; index < value.as_array()->values.size(); ++index) {
    auto hash = require_bytes(name, value.as_array()->values[index], 32, "update data hash");
    if (!hash) return std::unexpected(hash.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_software_version(std::string_view name,
                                                               const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  if (value.as_array()->values[0].as_text_string() == nullptr) {
    return std::unexpected(model_error(name, "software application name must be text"));
  }
  return require_unsigned_bound(name, value.as_array()->values[1], UINT32_MAX, "software version");
}

[[nodiscard]] core::VoidResult validate_byron_update_proposal(std::string_view name,
                                                              const CborValue& value) {
  auto shape = require_array_size(name, value, 7, 7);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto version = validate_byron_version(name, fields[0]);
  auto modifications = require_array_size(name, fields[1], 14, 14);
  auto software = validate_byron_software_version(name, fields[2]);
  if (!version || !modifications || !software) {
    return std::unexpected(!version ? version.error()
                                    : (!modifications ? modifications.error() : software.error()));
  }
  for (std::size_t index = 0; index < fields[1].as_array()->values.size(); ++index) {
    auto optional = require_array_size(name, fields[1].as_array()->values[index], 0, 1);
    if (!optional) return std::unexpected(optional.error().at(1U).at(index));
  }
  const auto* data = fields[3].as_map();
  if (data == nullptr || fields[4].as_map() == nullptr || fields[5].as_byte_string() == nullptr ||
      fields[6].as_byte_string() == nullptr) {
    return std::unexpected(model_error(name, "invalid update proposal fields"));
  }
  for (std::size_t index = 0; index < data->entries.size(); ++index) {
    if (data->entries[index].first.as_text_string() == nullptr) {
      return std::unexpected(model_error(name, "update system tag must be text").at(3U).at(index));
    }
    auto update = validate_byron_update_data(name, data->entries[index].second);
    if (!update) return std::unexpected(update.error().at(3U).at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_update(std::string_view name,
                                                     const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  const auto* proposal = fields[0].as_array();
  const auto* votes = fields[1].as_array();
  if (proposal == nullptr || proposal->values.size() > 1U || votes == nullptr) {
    return std::unexpected(model_error(name, "invalid Byron update collections"));
  }
  if (!proposal->values.empty()) {
    auto valid = validate_byron_update_proposal(name, proposal->values[0]);
    if (!valid) return std::unexpected(valid.error().at(0U).at(0U));
  }
  for (std::size_t index = 0; index < votes->values.size(); ++index) {
    auto vote = validate_byron_update_vote(name, votes->values[index]);
    if (!vote) return std::unexpected(vote.error().at(1U).at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_block_signature(std::string_view name,
                                                              const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto kind = require_unsigned_bound(name, fields[0], 2U, "block signature kind");
  if (!kind) return kind;
  const auto kind_value = unsigned_value(name, fields[0], "block signature kind").value();
  if (kind_value == 0U) {
    return fields[1].as_byte_string() != nullptr
               ? core::VoidResult(std::monostate{})
               : std::unexpected(model_error(name, "block signature must be bytes"));
  }
  auto delegation_signature = require_array_size(name, fields[1], 2, 2);
  if (!delegation_signature) return delegation_signature;
  auto delegation = validate_byron_delegation(name, fields[1].as_array()->values[0]);
  if (!delegation || fields[1].as_array()->values[1].as_byte_string() == nullptr) {
    return std::unexpected(delegation ? model_error(name, "delegation signature must be bytes")
                                      : delegation.error());
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_consensus(std::string_view name,
                                                        const CborValue& value) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto slot_shape = require_array_size(name, fields[0], 2, 2);
  if (!slot_shape) return std::unexpected(slot_shape.error().at(0U));
  for (const auto& slot : fields[0].as_array()->values) {
    auto number = unsigned_value(name, slot, "slot id");
    if (!number) return std::unexpected(number.error().at(0U));
  }
  if (fields[1].as_byte_string() == nullptr) {
    return std::unexpected(model_error(name, "slot leader must be bytes").at(1U));
  }
  auto difficulty_shape = require_array_size(name, fields[2], 1, 1);
  if (!difficulty_shape) return std::unexpected(difficulty_shape.error().at(2U));
  auto difficulty = unsigned_value(name, fields[2].as_array()->values[0], "difficulty");
  if (!difficulty) return std::unexpected(difficulty.error().at(2U));
  auto signature = validate_byron_block_signature(name, fields[3]);
  return signature ? core::VoidResult(std::monostate{}) : std::unexpected(signature.error().at(3U));
}

[[nodiscard]] core::VoidResult validate_byron_ssc_proof(std::string_view name,
                                                        const CborValue& value) {
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(model_error(name, "SSC proof must be a nonempty array"));
  }
  auto kind = require_unsigned_bound(name, fields->values[0], 3U, "SSC proof kind");
  if (!kind) return kind;
  const auto kind_value = unsigned_value(name, fields->values[0], "SSC proof kind").value();
  const auto expected = kind_value == 3U ? 2U : 3U;
  auto shape = require_array_size(name, value, expected, expected);
  if (!shape) return shape;
  for (std::size_t index = 1; index < fields->values.size(); ++index) {
    auto hash = require_bytes(name, fields->values[index], 32, "SSC proof hash");
    if (!hash) return std::unexpected(hash.error().at(index));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_body_proof(std::string_view name,
                                                         const CborValue& value) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto tx_proof = require_array_size(name, fields[0], 3, 3);
  if (!tx_proof) return std::unexpected(tx_proof.error().at(0U));
  auto count = require_unsigned_bound(name, fields[0].as_array()->values[0], UINT32_MAX,
                                      "transaction count");
  auto root = require_bytes(name, fields[0].as_array()->values[1], 32, "transaction root");
  auto witnesses = require_bytes(name, fields[0].as_array()->values[2], 32, "witness root");
  auto ssc = validate_byron_ssc_proof(name, fields[1]);
  auto delegation = require_bytes(name, fields[2], 32, "delegation proof");
  auto update = require_bytes(name, fields[3], 32, "update proof");
  if (!count || !root || !witnesses || !ssc || !delegation || !update) {
    return std::unexpected(!count        ? count.error()
                           : !root       ? root.error()
                           : !witnesses  ? witnesses.error()
                           : !ssc        ? ssc.error()
                           : !delegation ? delegation.error()
                                         : update.error());
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_header(std::string_view name,
                                                     const CborValue& value) {
  auto shape = require_array_size(name, value, 5, 5);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  auto magic = require_unsigned_bound(name, fields[0], UINT32_MAX, "protocol magic");
  auto previous = require_bytes(name, fields[1], 32, "previous block");
  auto proof = validate_byron_body_proof(name, fields[2]);
  auto consensus = validate_byron_consensus(name, fields[3]);
  auto extra = require_array_size(name, fields[4], 4, 4);
  if (!magic || !previous || !proof || !consensus || !extra) {
    return std::unexpected(!magic       ? magic.error()
                           : !previous  ? previous.error()
                           : !proof     ? proof.error()
                           : !consensus ? consensus.error()
                                        : extra.error());
  }
  auto version = validate_byron_version(name, fields[4].as_array()->values[0]);
  auto software = require_array_size(name, fields[4].as_array()->values[1], 2, 2);
  if (!version || !software ||
      fields[4].as_array()->values[1].as_array()->values[0].as_text_string() == nullptr ||
      fields[4].as_array()->values[2].as_map() == nullptr) {
    return std::unexpected(
        !version ? version.error()
                 : (!software ? software.error() : model_error(name, "invalid header extra")));
  }
  auto software_version = require_unsigned_bound(
      name, fields[4].as_array()->values[1].as_array()->values[1], UINT32_MAX, "software version");
  auto extra_proof = require_bytes(name, fields[4].as_array()->values[3], 32, "header extra proof");
  if (!software_version || !extra_proof) {
    return std::unexpected(!software_version ? software_version.error() : extra_proof.error());
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_body(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 4, 4);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  const auto* transactions = fields[0].as_array();
  const auto* delegations = fields[2].as_array();
  if (transactions == nullptr || delegations == nullptr) {
    return std::unexpected(model_error(name, "invalid Byron block body collections"));
  }
  for (const auto& transaction : transactions->values) {
    auto valid = validate_byron_tx_aux(name, transaction);
    if (!valid) return valid;
  }
  auto ssc = validate_byron_ssc(name, fields[1]);
  if (!ssc) return std::unexpected(ssc.error().at(1U));
  for (const auto& delegation_value : delegations->values) {
    auto delegation = validate_byron_delegation(name, delegation_value);
    if (!delegation) return delegation;
  }
  auto update = validate_byron_update(name, fields[3]);
  return update ? core::VoidResult(std::monostate{}) : std::unexpected(update.error().at(3U));
}

[[nodiscard]] core::VoidResult validate_byron_script_pair(std::string_view name,
                                                          const CborValue& value) {
  auto shape = require_array_size(name, value, 2U, 2U);
  if (!shape) return shape;
  auto version =
      require_unsigned_bound(name, value.as_array()->values[0], UINT16_MAX, "script version");
  if (!version) return std::unexpected(version.error().at(0U));
  if (value.as_array()->values[1].as_byte_string() == nullptr) {
    return std::unexpected(model_error(name, "script payload must be bytes").at(1U));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_inner_witness(std::string_view name,
                                                            const CborValue& value,
                                                            std::uint64_t kind) {
  auto shape = require_array_size(name, value, 2U, 2U);
  if (!shape) return shape;
  if (kind == 1U) {
    auto validator = validate_byron_script_pair(name, value.as_array()->values[0]);
    auto redeemer = validate_byron_script_pair(name, value.as_array()->values[1]);
    if (!validator || !redeemer) {
      return std::unexpected(!validator ? validator.error().at(0U) : redeemer.error().at(1U));
    }
    return std::monostate{};
  }
  if (value.as_array()->values[0].as_byte_string() == nullptr ||
      value.as_array()->values[1].as_byte_string() == nullptr) {
    return std::unexpected(model_error(name, "key witness fields must be bytes"));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_vss_proof(std::string_view name, const CborValue& value) {
  auto shape = require_array_size(name, value, 4U, 4U);
  if (!shape) return shape;
  const auto& fields = value.as_array()->values;
  for (std::size_t index = 0; index < 3U; ++index) {
    if (fields[index].as_byte_string() == nullptr) {
      return std::unexpected(model_error(name, "VSS proof component must be bytes").at(index));
    }
  }
  const auto* commitments = fields[3].as_array();
  if (commitments == nullptr) {
    return std::unexpected(model_error(name, "VSS proof commitments must be an array").at(3U));
  }
  for (std::size_t index = 0; index < commitments->values.size(); ++index) {
    if (commitments->values[index].as_byte_string() == nullptr) {
      return std::unexpected(
          model_error(name, "VSS proof commitment must be bytes").at(3U).at(index));
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_ssc_commitment(std::string_view name,
                                                       const CborValue& value) {
  auto shape = require_array_size(name, value, 2U, 2U);
  if (!shape) return shape;
  const auto* encrypted = value.as_array()->values[0].as_map();
  if (encrypted == nullptr) {
    return std::unexpected(model_error(name, "SSC encrypted shares must be a map").at(0U));
  }
  for (std::size_t index = 0; index < encrypted->entries.size(); ++index) {
    if (encrypted->entries[index].first.as_byte_string() == nullptr) {
      return std::unexpected(model_error(name, "VSS public key must be bytes").at(0U).at(index));
    }
    auto share = require_array_size(name, encrypted->entries[index].second, 1U, 1U);
    if (!share ||
        encrypted->entries[index].second.as_array()->values[0].as_byte_string() == nullptr) {
      return std::unexpected(
          !share ? share.error().at(0U).at(index)
                 : model_error(name, "encrypted share must be bytes").at(0U).at(index));
    }
  }
  auto proof = validate_vss_proof(name, value.as_array()->values[1]);
  return proof ? core::VoidResult(std::monostate{}) : std::unexpected(proof.error().at(1U));
}

[[nodiscard]] core::VoidResult validate_ssc_signed_commitment(std::string_view name,
                                                              const CborValue& value) {
  auto shape = require_array_size(name, value, 3U, 3U);
  if (!shape) return shape;
  if (value.as_array()->values[0].as_byte_string() == nullptr ||
      value.as_array()->values[2].as_byte_string() == nullptr) {
    return std::unexpected(model_error(name, "SSC key/signature must be bytes"));
  }
  auto commitment = validate_ssc_commitment(name, value.as_array()->values[1]);
  return commitment ? core::VoidResult(std::monostate{})
                    : std::unexpected(commitment.error().at(1U));
}

[[nodiscard]] core::VoidResult validate_ssc_shares(std::string_view name, const CborValue& value) {
  const auto* shares = value.as_map();
  if (shares == nullptr) {
    return std::unexpected(model_error(name, "SSC shares must be a map"));
  }
  for (std::size_t index = 0; index < shares->entries.size(); ++index) {
    auto key = require_bytes(name, shares->entries[index].first, 28U, "address id");
    if (!key) return std::unexpected(key.error().at(index));
    if (const auto* legacy = shares->entries[index].second.as_map()) {
      for (std::size_t legacy_index = 0; legacy_index < legacy->entries.size(); ++legacy_index) {
        auto address = require_bytes(name, legacy->entries[legacy_index].first, 28U, "address id");
        const auto* decrypted = legacy->entries[legacy_index].second.as_array();
        if (!address || decrypted == nullptr) {
          return std::unexpected(!address
                                     ? address.error().at(index).at(legacy_index)
                                     : model_error(name, "legacy decrypted shares must be an array")
                                           .at(index)
                                           .at(legacy_index));
        }
        for (std::size_t share_index = 0; share_index < decrypted->values.size(); ++share_index) {
          if (decrypted->values[share_index].as_byte_string() == nullptr) {
            return std::unexpected(model_error(name, "legacy decrypted share must be bytes")
                                       .at(index)
                                       .at(legacy_index)
                                       .at(share_index));
          }
        }
      }
      continue;
    }
    auto submap = require_array_size(name, shares->entries[index].second, 2U, 2U);
    if (!submap) return std::unexpected(submap.error().at(index));
    auto address =
        require_bytes(name, shares->entries[index].second.as_array()->values[0], 28U, "address id");
    const auto* decrypted = shares->entries[index].second.as_array()->values[1].as_array();
    if (!address || decrypted == nullptr) {
      return std::unexpected(
          !address ? address.error().at(index)
                   : model_error(name, "decrypted shares must be an array").at(index));
    }
    for (std::size_t share_index = 0; share_index < decrypted->values.size(); ++share_index) {
      if (decrypted->values[share_index].as_byte_string() == nullptr) {
        return std::unexpected(
            model_error(name, "decrypted share must be bytes").at(index).at(share_index));
      }
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_byron_model(std::string_view name, const CborValue& value) {
  if (name == "ByronTxIn" || name == "ByronTxInRegular" || name == "ByronTxInGenesis") {
    return validate_byron_tx_in(name, value);
  }
  if (name == "ByronTxOut") return validate_byron_tx_out(name, value);
  if (name == "ByronTxOutPtr") {
    auto shape = require_array_size(name, value, 2U, 2U);
    if (!shape) return shape;
    auto hash = require_bytes(name, value.as_array()->values[0], 32U, "Byron transaction id");
    auto index =
        require_unsigned_bound(name, value.as_array()->values[1], UINT32_MAX, "Byron output index");
    if (!hash || !index) {
      return std::unexpected(!hash ? hash.error().at(0U) : index.error().at(1U));
    }
    return std::monostate{};
  }
  if (name == "ByronTxProof") {
    auto shape = require_array_size(name, value, 3U, 3U);
    if (!shape) return shape;
    auto count =
        require_unsigned_bound(name, value.as_array()->values[0], UINT32_MAX, "transaction count");
    auto root = require_bytes(name, value.as_array()->values[1], 32U, "transaction root");
    auto witnesses = require_bytes(name, value.as_array()->values[2], 32U, "witness root");
    if (!count || !root || !witnesses) {
      return std::unexpected(!count ? count.error().at(0U)
                                    : (!root ? root.error().at(1U) : witnesses.error().at(2U)));
    }
    return std::monostate{};
  }
  if (name == "ByronTx") return validate_byron_tx(name, value);
  if (name == "ByronTxWitness") return validate_byron_witness(name, value);
  if (name == "ByronPkWitness" || name == "ByronRedeemWitness") {
    return validate_byron_inner_witness(name, value, 0U);
  }
  if (name == "ByronScriptWitness") return validate_byron_inner_witness(name, value, 1U);
  if (name == "ByronPkWitnessEntry" || name == "ByronRedeemerWitnessEntry" ||
      name == "ByronScriptWitnessEntry") {
    return validate_byron_witness(name, value);
  }
  if (name == "ByronValidatorScript" || name == "ByronRedeemerScript") {
    return validate_byron_script_pair(name, value);
  }
  if (name == "TxAux") return validate_byron_tx_aux(name, value);
  if (name == "TxPayload") {
    const auto* payload = value.as_array();
    if (payload == nullptr) return std::unexpected(model_error(name, "payload must be an array"));
    for (std::size_t index = 0; index < payload->values.size(); ++index) {
      auto valid = validate_byron_tx_aux(name, payload->values[index]);
      if (!valid) return std::unexpected(valid.error().at(index));
    }
  }
  if (name == "ByronBlockVersion") return validate_byron_version(name, value);
  if (name == "ByronDifficulty") {
    auto shape = require_array_size(name, value, 1, 1);
    if (!shape) return shape;
    auto number = unsigned_value(name, value.as_array()->values[0], "difficulty");
    return number ? core::VoidResult(std::monostate{}) : std::unexpected(number.error());
  }
  if (name == "ByronSlotId" || name == "EpochRange") {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return shape;
    for (const auto& field : value.as_array()->values) {
      auto number = unsigned_value(name, field, "Byron integer");
      if (!number) return std::unexpected(number.error());
    }
    return std::monostate{};
  }
  if (name == "EbbConsensusData") {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return shape;
    auto epoch = unsigned_value(name, value.as_array()->values[0], "epoch");
    auto difficulty = validate_byron_model("ByronDifficulty", value.as_array()->values[1]);
    if (!epoch || !difficulty) {
      return std::unexpected(!epoch ? epoch.error() : difficulty.error());
    }
    return std::monostate{};
  }
  if (name == "ByronDelegation" || name == "LightWeightDlg") {
    return validate_byron_delegation(name, value);
  }
  if (name == "ByronDelegationSignature" || name == "LightWeightDelegationSignature") {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return shape;
    auto delegation = validate_byron_delegation(name, value.as_array()->values[0]);
    if (!delegation || value.as_array()->values[1].as_byte_string() == nullptr) {
      return std::unexpected(delegation ? model_error(name, "delegation signature must be bytes")
                                        : delegation.error());
    }
    return std::monostate{};
  }
  if (name == "SscCert") return validate_byron_ssc_cert(name, value);
  if (name == "VssEncryptedShare") {
    auto shape = require_array_size(name, value, 1U, 1U);
    if (!shape) return shape;
    return value.as_array()->values[0].as_byte_string() != nullptr
               ? core::VoidResult(std::monostate{})
               : std::unexpected(model_error(name, "encrypted share must be bytes").at(0U));
  }
  if (name == "VssProof") return validate_vss_proof(name, value);
  if (name == "SscCommitment") return validate_ssc_commitment(name, value);
  if (name == "SscSignedCommitment") return validate_ssc_signed_commitment(name, value);
  if (name == "SscSharesSubmap") return validate_ssc_shares(name, value);
  if (name == "Ssc" || name == "SscCommitmentsPayload" || name == "SscOpeningsPayload" ||
      name == "SscSharesPayload" || name == "SscCertificatesPayload") {
    return validate_byron_ssc(name, value);
  }
  if (name == "ByronUpdateVote") return validate_byron_update_vote(name, value);
  if (name == "ByronUpdateData") return validate_byron_update_data(name, value);
  if (name == "ByronSoftwareVersion") return validate_byron_software_version(name, value);
  if (name == "ByronUpdateProposal") return validate_byron_update_proposal(name, value);
  if (name == "ByronUpdate") return validate_byron_update(name, value);
  if (name == "Bvermod") {
    auto shape = require_array_size(name, value, 14, 14);
    if (!shape) return shape;
    for (const auto& field : value.as_array()->values) {
      auto optional = require_array_size(name, field, 0, 1);
      if (!optional) return optional;
    }
    return std::monostate{};
  }
  if (name == "SoftForkRule") {
    auto shape = require_array_size(name, value, 3, 3);
    if (!shape) return shape;
    for (const auto& field : value.as_array()->values) {
      auto number = unsigned_value(name, field, "soft fork rule");
      if (!number) return std::unexpected(number.error());
    }
    return std::monostate{};
  }
  if (name == "ByronTxFeePolicy" || name == "StdFeePolicy") {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return shape;
    auto kind = require_unsigned_bound(name, value.as_array()->values[0], UINT8_MAX, "fee kind");
    if (!kind) return kind;
    auto embedded = decode_byron_embedded(name, value.as_array()->values[1]);
    if (!embedded) return std::unexpected(embedded.error());
    if (unsigned_value(name, value.as_array()->values[0], "fee kind").value() == 0U) {
      auto fee = require_array_size(name, *embedded, 2, 2);
      if (!fee) return fee;
      for (const auto& integer : embedded->as_array()->values) {
        if (integer.as_unsigned() == nullptr && integer.as_negative() == nullptr) {
          return std::unexpected(model_error(name, "fee coefficient must be an integer"));
        }
      }
    }
    return std::monostate{};
  }
  if (name == "ByronBlockSignature" || name == "ByronBlockSignatureNormal" ||
      name == "ByronBlockSignatureProxyLight" || name == "ByronBlockSignatureProxyHeavy") {
    return validate_byron_block_signature(name, value);
  }
  if (name == "ByronBlockConsensusData") return validate_byron_consensus(name, value);
  if (name == "SscProof" || name == "SscCommitmentsProof" || name == "SscOpeningsProof" ||
      name == "SscSharesProof" || name == "SscCertificatesProof") {
    return validate_byron_ssc_proof(name, value);
  }
  if (name == "ByronBodyProof") return validate_byron_body_proof(name, value);
  if (name == "ByronBlockHeader") return validate_byron_header(name, value);
  if (name == "BlockHeaderExtraData") {
    auto shape = require_array_size(name, value, 4U, 4U);
    if (!shape) return shape;
    auto version = validate_byron_version(name, value.as_array()->values[0]);
    auto software = validate_byron_software_version(name, value.as_array()->values[1]);
    auto attributes = validate_map(name, value.as_array()->values[2]);
    auto proof = require_bytes(name, value.as_array()->values[3], 32U, "header extra proof");
    if (!version || !software || !attributes || !proof) {
      return std::unexpected(!version      ? version.error().at(0U)
                             : !software   ? software.error().at(1U)
                             : !attributes ? attributes.error().at(2U)
                                           : proof.error().at(3U));
    }
    return std::monostate{};
  }
  if (name == "ByronBlockBody") return validate_byron_body(name, value);
  if (name == "ByronMainBlock") {
    auto shape = require_array_size(name, value, 3, 3);
    if (!shape) return shape;
    auto header = validate_byron_header(name, value.as_array()->values[0]);
    auto body = validate_byron_body(name, value.as_array()->values[1]);
    auto extra = require_array_size(name, value.as_array()->values[2], 1, 1);
    if (!header || !body || !extra ||
        value.as_array()->values[2].as_array()->values[0].as_map() == nullptr) {
      return std::unexpected(
          !header ? header.error()
                  : (!body ? body.error()
                           : (!extra ? extra.error() : model_error(name, "invalid block extra"))));
    }
    return std::monostate{};
  }
  if (name == "EbbHead") {
    auto shape = require_array_size(name, value, 5, 5);
    if (!shape) return shape;
    const auto& fields = value.as_array()->values;
    auto magic = require_unsigned_bound(name, fields[0], UINT32_MAX, "protocol magic");
    auto previous = require_bytes(name, fields[1], 32, "previous block");
    auto proof = require_bytes(name, fields[2], 32, "body proof");
    auto consensus_shape = require_array_size(name, fields[3], 2, 2);
    auto extra = require_array_size(name, fields[4], 1, 1);
    if (!magic || !previous || !proof || !consensus_shape || !extra ||
        fields[4].as_array()->values[0].as_map() == nullptr) {
      return std::unexpected(
          !magic             ? magic.error()
          : !previous        ? previous.error()
          : !proof           ? proof.error()
          : !consensus_shape ? consensus_shape.error()
                             : (!extra ? extra.error() : model_error(name, "invalid EBB extra")));
    }
    auto epoch = unsigned_value(name, fields[3].as_array()->values[0], "epoch");
    auto difficulty_shape = require_array_size(name, fields[3].as_array()->values[1], 1, 1);
    if (!epoch || !difficulty_shape) {
      return std::unexpected(!epoch ? epoch.error() : difficulty_shape.error());
    }
    auto difficulty =
        unsigned_value(name, fields[3].as_array()->values[1].as_array()->values[0], "difficulty");
    return difficulty ? core::VoidResult(std::monostate{}) : std::unexpected(difficulty.error());
  }
  if (name == "ByronEbBlock") {
    auto shape = require_array_size(name, value, 3, 3);
    if (!shape) return shape;
    auto header = validate_byron_model("EbbHead", value.as_array()->values[0]);
    const auto* stakeholders = value.as_array()->values[1].as_array();
    auto extra = require_array_size(name, value.as_array()->values[2], 1, 1);
    if (!header || stakeholders == nullptr || !extra ||
        value.as_array()->values[2].as_array()->values[0].as_map() == nullptr) {
      return std::unexpected(!header ? header.error()
                                     : model_error(name, "invalid EBB body/extra"));
    }
    for (const auto& stakeholder : stakeholders->values) {
      auto hash = require_bytes(name, stakeholder, 28, "stakeholder id");
      if (!hash) return hash;
    }
    return std::monostate{};
  }
  if (name == "ByronBlock") {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return shape;
    auto kind = require_unsigned_bound(name, value.as_array()->values[0], 1U, "Byron block kind");
    if (!kind) return kind;
    const auto kind_value =
        unsigned_value(name, value.as_array()->values[0], "Byron block kind").value();
    return validate_byron_model(kind_value == 0U ? "ByronEbBlock" : "ByronMainBlock",
                                value.as_array()->values[1]);
  }
  return std::monostate{};
}

[[nodiscard]] bool is_one_of(std::string_view name, std::initializer_list<std::string_view> names) {
  return std::ranges::find(names, name) != names.end();
}
