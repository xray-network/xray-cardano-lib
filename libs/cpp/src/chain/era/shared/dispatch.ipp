
[[nodiscard]] core::VoidResult validate_map(std::string_view name, const CborValue& value) {
  const auto* map = value.as_map();
  if (map == nullptr) {
    return std::unexpected(model_error(name, "must be a CBOR map"));
  }
  std::set<std::string> keys;
  for (std::size_t index = 0; index < map->entries.size(); ++index) {
    auto encoded =
        core::cbor::encode_cbor(map->entries[index].first, {.mode = core::cbor::Mode::canonical});
    if (!encoded) return std::unexpected(encoded.error().at(index));
    if (!keys.insert(core::bytes_to_hex(*encoded)).second) {
      return std::unexpected(core::CardanoError(core::ErrorCode::duplicate_key,
                                                std::string(name) + ": duplicate map key"));
    }
  }
  return std::monostate{};
}

using Json = nlohmann::json;

[[nodiscard]] core::CardanoError json_error(std::string_view name, std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_encoding,
                            std::string(name) + " JSON: " + std::move(message));
}

[[nodiscard]] core::Result<core::BigInteger> json_nonnegative_integer(std::string_view name,
                                                                      const Json& value,
                                                                      std::string_view field) {
  if (value.is_number_unsigned()) {
    return core::BigInteger(value.get<std::uint64_t>());
  }
  if (value.is_number_integer()) {
    const auto number = value.get<std::int64_t>();
    if (number >= 0) return core::BigInteger(number);
  }
  if (value.is_number_float()) {
    const auto number = value.get<double>();
    if (std::isfinite(number) && number >= 0.0 && number <= 9'007'199'254'740'991.0 &&
        std::trunc(number) == number) {
      return core::BigInteger(static_cast<std::uint64_t>(number));
    }
  }
  if (value.is_string()) {
    auto parsed = core::BigInteger::from_decimal(value.get<std::string>());
    if (parsed && !parsed->is_negative()) return parsed;
  }
  return std::unexpected(json_error(name, std::string(field) + " must be nonnegative integer"));
}

[[nodiscard]] Json integer_json(const core::BigInteger& value) {
  static const auto safe_max = core::BigInteger(std::uint64_t{9'007'199'254'740'991ULL});
  if (!value.is_negative() && value <= safe_max) {
    return Json(static_cast<double>(value.to_uint64().value()));
  }
  return Json(value.to_decimal());
}

[[nodiscard]] core::Result<CborValue> specialized_rational_from_json(std::string_view name,
                                                                     const Json& parsed) {
  if (!parsed.is_object() || parsed.size() != 2U) {
    return std::unexpected(json_error(name, "must be a two-field object"));
  }
  const auto numerator_key = name == "UnitInterval" ? "start" : "numerator";
  const auto denominator_key = name == "UnitInterval" ? "end" : "denominator";
  if (!parsed.contains(numerator_key) || !parsed.contains(denominator_key)) {
    return std::unexpected(json_error(name, "missing rational fields"));
  }
  auto numerator = json_nonnegative_integer(name, parsed.at(numerator_key), numerator_key);
  auto denominator = json_nonnegative_integer(name, parsed.at(denominator_key), denominator_key);
  if (!numerator || !denominator) {
    return std::unexpected(!numerator ? numerator.error() : denominator.error());
  }
  return CborValue::tag(core::BigInteger(std::uint64_t{30}),
                        CborValue::array({CborValue::unsigned_integer(*numerator),
                                          CborValue::unsigned_integer(*denominator)}));
}

[[nodiscard]] core::Result<Json> specialized_rational_to_json(std::string_view name,
                                                              const CborValue& value) {
  const CborValue* candidate = &value;
  if (const auto* tag = value.as_tag(); tag != nullptr && tag->value != nullptr) {
    candidate = tag->value.get();
  }
  auto shape = require_array_size(name, *candidate, 2, 2);
  if (!shape) return std::unexpected(shape.error());
  const auto* numerator = candidate->as_array()->values[0].as_unsigned();
  const auto* denominator = candidate->as_array()->values[1].as_unsigned();
  if (numerator == nullptr || denominator == nullptr) {
    return std::unexpected(json_error(name, "rational fields are invalid"));
  }
  if (name == "UnitInterval") {
    return Json{{"start", integer_json(numerator->value)},
                {"end", integer_json(denominator->value)}};
  }
  return Json{{"numerator", integer_json(numerator->value)},
              {"denominator", integer_json(denominator->value)}};
}

[[nodiscard]] core::Result<CborValue> specialized_anchor_from_json(std::string_view name,
                                                                   const Json& parsed) {
  if (!parsed.is_object() || parsed.size() != 2U || !parsed.contains("anchor_url") ||
      !parsed.at("anchor_url").is_string() || !parsed.contains("anchor_doc_hash") ||
      !parsed.at("anchor_doc_hash").is_string()) {
    return std::unexpected(json_error(name, "invalid anchor fields"));
  }
  auto hash = core::hex_to_bytes(parsed.at("anchor_doc_hash").get<std::string>());
  if (!hash) return std::unexpected(hash.error());
  return CborValue::array({
      CborValue::text_string(parsed.at("anchor_url").get<std::string>()),
      CborValue::byte_string(std::move(*hash)),
  });
}

[[nodiscard]] core::Result<Json> specialized_anchor_to_json(std::string_view name,
                                                            const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return std::unexpected(shape.error());
  const auto* url = value.as_array()->values[0].as_text_string();
  const auto* hash = value.as_array()->values[1].as_byte_string();
  if (url == nullptr || hash == nullptr) {
    return std::unexpected(json_error(name, "invalid anchor value"));
  }
  return Json{{"anchor_url", url->value}, {"anchor_doc_hash", core::bytes_to_hex(hash->value)}};
}

[[nodiscard]] core::Result<CborValue> specialized_protocol_version_from_json(std::string_view name,
                                                                             const Json& parsed) {
  if (!parsed.is_object() || parsed.size() != 2U || !parsed.contains("major") ||
      !parsed.contains("minor")) {
    return std::unexpected(json_error(name, "invalid protocol version fields"));
  }
  auto major = json_nonnegative_integer(name, parsed.at("major"), "major");
  auto minor = json_nonnegative_integer(name, parsed.at("minor"), "minor");
  if (!major || !minor) return std::unexpected(!major ? major.error() : minor.error());
  return CborValue::array(
      {CborValue::unsigned_integer(*major), CborValue::unsigned_integer(*minor)});
}

[[nodiscard]] core::Result<Json> specialized_protocol_version_to_json(std::string_view name,
                                                                      const CborValue& value) {
  auto shape = require_array_size(name, value, 2, 2);
  if (!shape) return std::unexpected(shape.error());
  const auto* major = value.as_array()->values[0].as_unsigned();
  const auto* minor = value.as_array()->values[1].as_unsigned();
  if (major == nullptr || minor == nullptr) {
    return std::unexpected(json_error(name, "invalid protocol version value"));
  }
  return Json{{"major", integer_json(major->value)}, {"minor", integer_json(minor->value)}};
}

[[nodiscard]] core::Result<core::Bytes> json_byte_array(std::string_view name, const Json& value,
                                                        std::string_view field) {
  if (!value.is_array()) {
    return std::unexpected(json_error(name, std::string(field) + " must be a byte array"));
  }
  core::Bytes bytes;
  bytes.reserve(value.size());
  for (const auto& item : value) {
    if (!item.is_number_unsigned() || item.get<std::uint64_t>() > UINT8_MAX) {
      return std::unexpected(json_error(name, std::string(field) + " contains an invalid byte"));
    }
    bytes.push_back(static_cast<core::Byte>(item.get<std::uint8_t>()));
  }
  return bytes;
}

[[nodiscard]] Json bytes_json(core::ByteSpan bytes) {
  Json output = Json::array();
  for (const auto byte : bytes) output.push_back(std::to_integer<std::uint8_t>(byte));
  return output;
}

[[nodiscard]] core::Result<CborValue> native_script_from_json(std::string_view name,
                                                              const Json& parsed,
                                                              std::size_t depth = 0U) {
  if (depth > 128U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                                              std::string(name) + " JSON exceeds depth 128"));
  }
  if (!parsed.is_object() || parsed.size() != 1U) {
    return std::unexpected(json_error(name, "native script must have one variant"));
  }
  const auto iterator = parsed.begin();
  const auto& variant = iterator.key();
  const auto& fields = iterator.value();
  if (!fields.is_object()) {
    return std::unexpected(json_error(name, "native script variant must be an object"));
  }
  if (variant == "ScriptPubkey") {
    if (fields.size() != 1U || !fields.contains("ed25519_key_hash") ||
        !fields.at("ed25519_key_hash").is_string()) {
      return std::unexpected(json_error(name, "invalid ScriptPubkey fields"));
    }
    auto hash = core::hex_to_bytes(fields.at("ed25519_key_hash").get<std::string>());
    if (!hash) return std::unexpected(hash.error());
    return CborValue::array({
        CborValue::unsigned_integer(core::BigInteger(std::uint64_t{0})),
        CborValue::byte_string(std::move(*hash)),
    });
  }
  const bool all = variant == "ScriptAll";
  const bool any = variant == "ScriptAny";
  const bool threshold = variant == "ScriptNOfK";
  if (all || any || threshold) {
    if (!fields.contains("native_scripts") || !fields.at("native_scripts").is_array() ||
        fields.size() != (threshold ? 2U : 1U)) {
      return std::unexpected(json_error(name, "invalid native script list fields"));
    }
    std::vector<CborValue> children;
    children.reserve(fields.at("native_scripts").size());
    for (const auto& child : fields.at("native_scripts")) {
      auto converted = native_script_from_json(name, child, depth + 1U);
      if (!converted) return std::unexpected(converted.error());
      children.push_back(std::move(*converted));
    }
    const auto kind = threshold ? 3U : (all ? 1U : 2U);
    std::vector<CborValue> output{
        CborValue::unsigned_integer(core::BigInteger(static_cast<std::uint64_t>(kind)))};
    if (threshold) {
      if (!fields.contains("n")) {
        return std::unexpected(json_error(name, "ScriptNOfK is missing n"));
      }
      auto required = json_nonnegative_integer(name, fields.at("n"), "n");
      if (!required) return std::unexpected(required.error());
      output.push_back(CborValue::unsigned_integer(std::move(*required)));
    }
    output.push_back(CborValue::array(std::move(children)));
    return CborValue::array(std::move(output));
  }
  if (variant == "ScriptInvalidBefore" || variant == "ScriptInvalidHereafter") {
    const auto field = variant == "ScriptInvalidBefore" ? "before" : "after";
    if (fields.size() != 1U || !fields.contains(field)) {
      return std::unexpected(json_error(name, "invalid timelock fields"));
    }
    auto slot = json_nonnegative_integer(name, fields.at(field), field);
    if (!slot) return std::unexpected(slot.error());
    const auto kind = variant == "ScriptInvalidBefore" ? 4U : 5U;
    return CborValue::array({
        CborValue::unsigned_integer(core::BigInteger(static_cast<std::uint64_t>(kind))),
        CborValue::unsigned_integer(std::move(*slot)),
    });
  }
  return std::unexpected(json_error(name, "unknown native script variant"));
}

[[nodiscard]] core::Result<Json> native_script_to_json(std::string_view name,
                                                       const CborValue& value,
                                                       std::size_t depth = 0U) {
  if (depth > 128U) {
    return std::unexpected(core::CardanoError(core::ErrorCode::depth_limit_exceeded,
                                              std::string(name) + " JSON exceeds depth 128"));
  }
  const auto* fields = value.as_array();
  if (fields == nullptr || fields->values.empty()) {
    return std::unexpected(json_error(name, "invalid native script value"));
  }
  auto kind = unsigned_value(name, fields->values[0], "script kind");
  if (!kind || *kind > 5U) {
    return std::unexpected(kind ? json_error(name, "script kind is out of range") : kind.error());
  }
  if (*kind == 0U) {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return std::unexpected(shape.error());
    const auto* hash = fields->values[1].as_byte_string();
    if (hash == nullptr) return std::unexpected(json_error(name, "invalid signer hash"));
    return Json{{"ScriptPubkey", Json{{"ed25519_key_hash", core::bytes_to_hex(hash->value)}}}};
  }
  if (*kind == 4U || *kind == 5U) {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return std::unexpected(shape.error());
    const auto* slot = fields->values[1].as_unsigned();
    if (slot == nullptr) return std::unexpected(json_error(name, "invalid timelock slot"));
    const auto variant = *kind == 4U ? "ScriptInvalidBefore" : "ScriptInvalidHereafter";
    const auto field = *kind == 4U ? "before" : "after";
    return Json{{variant, Json{{field, integer_json(slot->value)}}}};
  }
  const auto child_index = *kind == 3U ? 2U : 1U;
  const auto* children = fields->values[child_index].as_array();
  if (children == nullptr) return std::unexpected(json_error(name, "invalid native script list"));
  Json scripts = Json::array();
  for (const auto& child : children->values) {
    auto converted = native_script_to_json(name, child, depth + 1U);
    if (!converted) return std::unexpected(converted.error());
    scripts.push_back(std::move(*converted));
  }
  const auto variant = *kind == 1U ? "ScriptAll" : (*kind == 2U ? "ScriptAny" : "ScriptNOfK");
  Json inner{{"native_scripts", std::move(scripts)}};
  if (*kind == 3U) {
    const auto* required = fields->values[1].as_unsigned();
    if (required == nullptr) return std::unexpected(json_error(name, "invalid threshold"));
    inner["n"] = integer_json(required->value);
  }
  return Json{{variant, std::move(inner)}};
}

}  // namespace

core::VoidResult validate_era_model(std::string_view name, EraWireShape shape,
                                    const CborValue& value) {
  if (shape == EraWireShape::array && value.as_array() == nullptr) {
    return std::unexpected(model_error(name, "must be a CBOR array"));
  }
  if (shape == EraWireShape::map) {
    auto valid = validate_map(name, value);
    if (!valid) return valid;
  } else if (value.as_map() != nullptr) {
    auto valid = validate_map(name, value);
    if (!valid) return valid;
  }

  if (is_one_of(name, {"ByronTxIn",
                       "ByronTxInRegular",
                       "ByronTxInGenesis",
                       "ByronTxOut",
                       "ByronTxOutPtr",
                       "ByronTxProof",
                       "ByronTx",
                       "ByronTxWitness",
                       "ByronPkWitness",
                       "ByronPkWitnessEntry",
                       "ByronRedeemWitness",
                       "ByronRedeemerWitnessEntry",
                       "ByronScriptWitness",
                       "ByronScriptWitnessEntry",
                       "ByronValidatorScript",
                       "ByronRedeemerScript",
                       "TxAux",
                       "TxPayload",
                       "ByronBlockVersion",
                       "ByronDifficulty",
                       "ByronSlotId",
                       "EpochRange",
                       "EbbConsensusData",
                       "ByronDelegation",
                       "LightWeightDlg",
                       "ByronDelegationSignature",
                       "LightWeightDelegationSignature",
                       "SscCert",
                       "SscCommitment",
                       "SscSignedCommitment",
                       "SscSharesSubmap",
                       "VssEncryptedShare",
                       "VssProof",
                       "Ssc",
                       "SscCommitmentsPayload",
                       "SscOpeningsPayload",
                       "SscSharesPayload",
                       "SscCertificatesPayload",
                       "ByronUpdateVote",
                       "ByronUpdateData",
                       "ByronSoftwareVersion",
                       "ByronUpdateProposal",
                       "ByronUpdate",
                       "Bvermod",
                       "SoftForkRule",
                       "ByronTxFeePolicy",
                       "StdFeePolicy",
                       "ByronBlockSignature",
                       "ByronBlockSignatureNormal",
                       "ByronBlockSignatureProxyLight",
                       "ByronBlockSignatureProxyHeavy",
                       "ByronBlockConsensusData",
                       "SscProof",
                       "SscCommitmentsProof",
                       "SscOpeningsProof",
                       "SscSharesProof",
                       "SscCertificatesProof",
                       "ByronBodyProof",
                       "BlockHeaderExtraData",
                       "ByronBlockHeader",
                       "ByronBlockBody",
                       "ByronMainBlock",
                       "EbbHead",
                       "ByronEbBlock",
                       "ByronBlock"})) {
    return validate_byron_model(name, value);
  }
  if (name == "ShelleyTransactionBody") {
    return validate_transaction_body(name, value, BodyEra::shelley);
  }
  if (name == "AllegraTransactionBody") {
    return validate_transaction_body(name, value, BodyEra::allegra);
  }
  if (name == "MaryTransactionBody") {
    return validate_transaction_body(name, value, BodyEra::mary);
  }
  if (name == "AlonzoTransactionBody") {
    return validate_transaction_body(name, value, BodyEra::alonzo);
  }
  if (name == "BabbageTransactionBody") {
    return validate_transaction_body(name, value, BodyEra::babbage);
  }
  if (name == "TransactionBody") {
    return validate_transaction_body(name, value, BodyEra::conway);
  }
  if (name == "ShelleyTransactionOutput") {
    return validate_transaction_output(name, value, OutputEra::shelley);
  }
  if (name == "MaryTransactionOutput") {
    return validate_transaction_output(name, value, OutputEra::mary);
  }
  if (name == "AlonzoFormatTxOut") {
    return validate_transaction_output(name, value, OutputEra::alonzo);
  }
  if (name == "BabbageTransactionOutput") {
    return validate_transaction_output(name, value, OutputEra::babbage);
  }
  if (name == "BabbageFormatTxOut") {
    if (value.as_map() == nullptr) {
      return std::unexpected(model_error(name, "Babbage-format output must be a map"));
    }
    return validate_transaction_output(name, value, OutputEra::babbage);
  }
  if (name == "TransactionOutput") {
    return validate_transaction_output(name, value, OutputEra::conway);
  }
  if (name == "ConwayFormatTxOut") {
    if (value.as_map() == nullptr) {
      return std::unexpected(model_error(name, "Conway-format output must be a map"));
    }
    return validate_transaction_output(name, value, OutputEra::conway);
  }
  if (name == "ShelleyTransactionWitnessSet") {
    return validate_witness_set(name, value, BodyEra::shelley);
  }
  if (name == "AllegraTransactionWitnessSet") {
    return validate_witness_set(name, value, BodyEra::allegra);
  }
  if (name == "AlonzoTransactionWitnessSet") {
    return validate_witness_set(name, value, BodyEra::alonzo);
  }
  if (name == "BabbageTransactionWitnessSet") {
    return validate_witness_set(name, value, BodyEra::babbage);
  }
  if (name == "TransactionWitnessSet") {
    return validate_witness_set(name, value, BodyEra::conway);
  }
  if (name == "ShelleyTransaction") {
    return validate_transaction(name, value, BodyEra::shelley);
  }
  if (name == "AllegraTransaction") {
    return validate_transaction(name, value, BodyEra::allegra);
  }
  if (name == "MaryTransaction") {
    return validate_transaction(name, value, BodyEra::mary);
  }
  if (name == "AlonzoTransaction") {
    return validate_transaction(name, value, BodyEra::alonzo);
  }
  if (name == "BabbageTransaction") {
    return validate_transaction(name, value, BodyEra::babbage);
  }
  if (name == "Transaction") {
    return validate_transaction(name, value, BodyEra::conway);
  }
  if (name == "ShelleyBlock") return validate_block(name, value, BodyEra::shelley);
  if (name == "AllegraBlock") return validate_block(name, value, BodyEra::allegra);
  if (name == "MaryBlock") return validate_block(name, value, BodyEra::mary);
  if (name == "AlonzoBlock") return validate_block(name, value, BodyEra::alonzo);
  if (name == "BabbageBlock") return validate_block(name, value, BodyEra::babbage);
  if (name == "Block") return validate_block(name, value, BodyEra::conway);
  if (name == "ShelleyHeader") return validate_header(name, value, BodyEra::shelley);
  if (name == "Header") return validate_header(name, value, BodyEra::conway);
  if (name == "ShelleyHeaderBody" || name == "HeaderBody") {
    auto synthetic = CborValue::array({value, CborValue::byte_string(core::Bytes(448U))});
    return validate_header(name, synthetic,
                           name == "HeaderBody" ? BodyEra::conway : BodyEra::shelley);
  }
  if (name == "OperationalCert") {
    auto valid = require_array_size(name, value, 4, 4);
    if (!valid) return valid;
    const auto& fields = value.as_array()->values;
    auto key = require_bytes(name, fields[0], 32, "operational KES key");
    auto sequence = unsigned_value(name, fields[1], "sequence");
    auto period = unsigned_value(name, fields[2], "KES period");
    auto signature = require_bytes(name, fields[3], 64, "operational signature");
    if (!key || !sequence || !period || !signature) {
      return std::unexpected(!key        ? key.error()
                             : !sequence ? sequence.error()
                             : !period   ? period.error()
                                         : signature.error());
    }
    return std::monostate{};
  }
  if (name == "VRFCert") {
    auto valid = require_array_size(name, value, 2, 2);
    if (!valid) return valid;
    if (value.as_array()->values[0].as_byte_string() == nullptr) {
      return std::unexpected(model_error(name, "VRF output must be bytes"));
    }
    return require_bytes(name, value.as_array()->values[1], 80, "VRF proof");
  }
  if (is_one_of(name, {"PoolParams", "ShelleyPoolParams"})) {
    const auto* fields = value.as_array();
    if (fields == nullptr || fields->values.size() != 9U) {
      return std::unexpected(model_error(name, "pool parameters must contain 9 fields"));
    }
    std::vector<CborValue> certificate;
    certificate.reserve(10U);
    certificate.push_back(CborValue::unsigned_integer(core::BigInteger(std::uint64_t{3})));
    certificate.insert(certificate.end(), fields->values.begin(), fields->values.end());
    return validate_certificate(name == "PoolParams" ? "Certificate" : "ShelleyCertificate",
                                CborValue::array(std::move(certificate)));
  }
  if (is_one_of(name, {"RegCert",
                       "UnregCert",
                       "VoteDelegCert",
                       "StakeVoteDelegCert",
                       "StakeRegDelegCert",
                       "VoteRegDelegCert",
                       "StakeVoteRegDelegCert",
                       "AuthCommitteeHotCert",
                       "ResignCommitteeColdCert",
                       "RegDrepCert",
                       "UnregDrepCert",
                       "UpdateDrepCert",
                       "StakeRegistration",
                       "StakeDeregistration",
                       "StakeDelegation",
                       "PoolRegistration",
                       "PoolRetirement",
                       "GenesisKeyDelegation",
                       "MoveInstantaneousRewardsCert",
                       "ShelleyMoveInstantaneousRewardsCert",
                       "ShelleyPoolRegistration",
                       "AllegraCertificate",
                       "ShelleyCertificate"})) {
    return validate_certificate(
        is_one_of(name, {"AllegraCertificate", "ShelleyCertificate", "StakeRegistration",
                         "StakeDeregistration", "StakeDelegation", "PoolRegistration",
                         "PoolRetirement", "GenesisKeyDelegation", "MoveInstantaneousRewardsCert"})
            ? "ShelleyCertificate"
            : "Certificate",
        value);
  }
  if (is_one_of(name, {"Certificate", "AllegraCertificate", "ShelleyCertificate"})) {
    return validate_certificate(name, value);
  }
  if (is_one_of(name, {"NativeScript", "MultisigScript"})) {
    if (name == "MultisigScript") {
      const auto* fields = value.as_array();
      if (fields == nullptr || fields->values.empty()) {
        return std::unexpected(model_error(name, "multisig script must be a nonempty array"));
      }
      auto kind = unsigned_value(name, fields->values[0], "multisig kind");
      if (!kind || *kind > 3U) {
        return std::unexpected(kind ? model_error(name, "multisig kind must be in 0..3")
                                    : kind.error());
      }
    }
    return validate_native_script(name, value);
  }
  if (is_one_of(name, {"ScriptPubkey", "ScriptAll", "ScriptAny", "ScriptNOfK",
                       "ScriptInvalidBefore", "ScriptInvalidHereafter", "MultisigPubkey",
                       "MultisigAll", "MultisigAny", "MultisigNOfK"})) {
    return validate_native_script(name, value);
  }
  if (name == "Script") {
    return validate_script(name, value, 3U);
  }
  if (name == "BabbageScript") {
    return validate_script(name, value, 2U);
  }
  if (name == "ScriptRef") {
    return validate_script_ref(name, value, 3U);
  }
  if (name == "BabbageScriptRef") {
    return validate_script_ref(name, value, 2U);
  }
  if (name == "DatumOption") {
    return validate_datum_option(name, value);
  }
  if (name == "GovActionId") {
    return validate_gov_action_id(name, value);
  }
  if (name == "Voter") return validate_voter(name, value);
  if (name == "DRep") return validate_drep(name, value);
  if (name == "Anchor") return validate_anchor(name, value);
  if (name == "GovAction") return validate_gov_action(name, value);
  if (is_one_of(name,
                {"ParameterChangeAction", "HardForkInitiationAction", "TreasuryWithdrawalsAction",
                 "NoConfidence", "UpdateCommittee", "NewConstitution"})) {
    return validate_gov_action(name, value);
  }
  if (name == "ProposalProcedure") return validate_proposal(name, value);
  if (name == "Constitution") {
    auto shape = require_array_size(name, value, 2U, 2U);
    if (!shape) return shape;
    auto anchor = validate_anchor(name, value.as_array()->values[0]);
    auto guardrail =
        validate_optional_hash(name, value.as_array()->values[1], 28U, "guardrails script hash");
    if (!anchor || !guardrail) {
      return std::unexpected(!anchor ? anchor.error().at(0U) : guardrail.error().at(1U));
    }
    return std::monostate{};
  }
  if (name == "VotingProcedure") return validate_voting_procedure(name, value);
  if (name == "VotingProcedures") return validate_voting_procedures(name, value);
  if (is_one_of(name, {"Relay", "ShelleyRelay", "SingleHostAddr", "SingleHostName", "MultiHostName",
                       "ShelleySingleHostName", "ShelleyMultiHostName"})) {
    return validate_relay(name, value);
  }
  if (is_one_of(name, {"MoveInstantaneousReward", "ShelleyMoveInstantaneousReward", "MIRAction"})) {
    return validate_mir(name, value);
  }
  if (name == "Metadata") return validate_metadata_map(name, value);
  if (name == "TransactionMetadatum") return validate_metadatum(name, value);
  if (name == "ShelleyMAFormatAuxData") {
    if (value.as_array() == nullptr) {
      return std::unexpected(model_error(name, "Shelley-MA auxiliary data must use array format"));
    }
    return validate_auxiliary_data(name, value, 1U);
  }
  if (name == "AllegraAuxiliaryData") {
    return validate_auxiliary_data(name, value, 1U);
  }
  if (name == "AlonzoFormatAuxData") {
    if (value.as_tag() == nullptr) {
      return std::unexpected(model_error(name, "Alonzo auxiliary map must use tag 259"));
    }
    return validate_auxiliary_data(name, value, 2U);
  }
  if (name == "AlonzoAuxiliaryData") {
    return validate_auxiliary_data(name, value, 2U);
  }
  if (name == "BabbageFormatAuxData") {
    if (value.as_tag() == nullptr) {
      return std::unexpected(model_error(name, "Babbage auxiliary map must use tag 259"));
    }
    return validate_auxiliary_data(name, value, 3U);
  }
  if (name == "BabbageAuxiliaryData") {
    return validate_auxiliary_data(name, value, 3U);
  }
  if (name == "ConwayFormatAuxData") {
    if (value.as_tag() == nullptr) {
      return std::unexpected(model_error(name, "Conway auxiliary map must use tag 259"));
    }
    return validate_auxiliary_data(name, value, 4U);
  }
  if (name == "AuxiliaryData") {
    return validate_auxiliary_data(name, value, 4U);
  }
  if (is_one_of(name, {"ShelleyProposedProtocolParameterUpdates",
                       "AlonzoProposedProtocolParameterUpdates",
                       "BabbageProposedProtocolParameterUpdates"})) {
    return validate_proposed_updates(name, value);
  }
  if (is_one_of(name, {"ShelleyUpdate", "AlonzoUpdate", "BabbageUpdate"})) {
    return validate_era_update(name, value);
  }
  if (name == "MultiAsset") return validate_multiasset(name, value, false);
  if (name == "Mint") return validate_multiasset(name, value, true);
  if (name == "MetadatumMap") {
    if (value.as_map() == nullptr) {
      return std::unexpected(model_error(name, "metadata map must be a map"));
    }
    return validate_metadatum(name, value);
  }
  if (name == "Withdrawals") return validate_withdrawals(name, value, true);
  if (name == "ProtocolParamUpdate") {
    return validate_protocol_param_update(name, value, true);
  }
  if (is_one_of(name, {"ShelleyProtocolParamUpdate", "AlonzoProtocolParamUpdate",
                       "BabbageProtocolParamUpdate"})) {
    return validate_protocol_param_update(name, value, false);
  }
  if (name == "CostModels") return validate_cost_models(name, value);
  if (name == "PoolVotingThresholds") return validate_thresholds(name, value, 5U);
  if (name == "DRepVotingThresholds") return validate_thresholds(name, value, 10U);
  if (name == "Rational" || name == "UnitInterval") {
    return validate_rational(name, value, name == "UnitInterval");
  }
  if (name == "Nonce") return validate_nonce(name, value);
  if (name == "PoolMetadata") return validate_pool_metadata(name, value);
  if (name == "AlonzoRedeemer") return validate_legacy_redeemer(name, value, 3U);
  if (name == "LegacyRedeemer") return validate_legacy_redeemer(name, value, 5U);
  if (name == "RedeemerKey") return validate_redeemer_key(name, value, 5U);
  if (name == "RedeemerVal") return validate_redeemer_value(name, value);
  if (name == "Redeemers") return validate_redeemers(name, value, 5U);
  if (is_one_of(name, {"ProtocolVersion", "ProtocolVersionStruct"})) {
    return validate_protocol_version(name, value, name == "ProtocolVersion" ? 12U : 3U);
  }
  if (name == "DNSName" || name == "Url") {
    const auto* text = value.as_text_string();
    if (text == nullptr || text->value.size() > 128U) {
      return std::unexpected(model_error(name, "text length must be at most 128 bytes"));
    }
    return std::monostate{};
  }
  if (name == "ShelleyDNSName") {
    const auto* text = value.as_text_string();
    if (text == nullptr || text->value.size() > 64U) {
      return std::unexpected(model_error(name, "text length must be at most 64 bytes"));
    }
    return std::monostate{};
  }
  if (name == "Ipv4") return require_bytes(name, value, 4, "IPv4");
  if (name == "Ipv6") return require_bytes(name, value, 16, "IPv6");
  if (name == "KESSignature") {
    return require_bytes(name, value, 448, "KES signature");
  }
  if (is_one_of(name, {"PlutusV1Script", "PlutusV2Script", "PlutusV3Script"})) {
    if (value.as_byte_string() == nullptr) {
      return std::unexpected(model_error(name, "must be bytes"));
    }
  }
  if (name == "Vote") {
    auto vote = unsigned_value(name, value, "vote");
    if (!vote || *vote > 2) {
      return std::unexpected(vote ? model_error(name, "vote must be in 0..2") : vote.error());
    }
  }
  return std::monostate{};
}
