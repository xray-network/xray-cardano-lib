
core::Result<CborValue> era_model_value_from_json(std::string_view name, std::string_view json) {
  try {
    const auto parsed = Json::parse(json);
    if (name == "NativeScript" || name == "MultisigScript") {
      return native_script_from_json(name, parsed);
    }
    const std::array<std::pair<std::string_view, std::string_view>, 10> script_variants{{
        {"ScriptPubkey", "ScriptPubkey"},
        {"ScriptAll", "ScriptAll"},
        {"ScriptAny", "ScriptAny"},
        {"ScriptNOfK", "ScriptNOfK"},
        {"ScriptInvalidBefore", "ScriptInvalidBefore"},
        {"ScriptInvalidHereafter", "ScriptInvalidHereafter"},
        {"MultisigPubkey", "ScriptPubkey"},
        {"MultisigAll", "ScriptAll"},
        {"MultisigAny", "ScriptAny"},
        {"MultisigNOfK", "ScriptNOfK"},
    }};
    for (const auto& [model, variant] : script_variants) {
      if (name == model) return native_script_from_json(name, Json{{variant, parsed}});
    }
    if (name == "Rational" || name == "UnitInterval") {
      return specialized_rational_from_json(name, parsed);
    }
    if (name == "Anchor") return specialized_anchor_from_json(name, parsed);
    if (name == "ProtocolVersion" || name == "ProtocolVersionStruct") {
      return specialized_protocol_version_from_json(name, parsed);
    }
    if (name == "VRFCert") {
      if (!parsed.is_object() || parsed.size() != 2U || !parsed.contains("output") ||
          !parsed.contains("proof")) {
        return std::unexpected(json_error(name, "invalid VRF certificate fields"));
      }
      auto output = json_byte_array(name, parsed.at("output"), "output");
      auto proof = json_byte_array(name, parsed.at("proof"), "proof");
      if (!output || !proof) return std::unexpected(!output ? output.error() : proof.error());
      return CborValue::array(
          {CborValue::byte_string(std::move(*output)), CborValue::byte_string(std::move(*proof))});
    }
    if (name == "Ipv4" || name == "Ipv6") {
      if (!parsed.is_string()) {
        return std::unexpected(json_error(name, "IP address must be a string"));
      }
      core::Bytes bytes(name == "Ipv4" ? 4U : 16U);
      const auto family = name == "Ipv4" ? AF_INET : AF_INET6;
      if (inet_pton(family, parsed.get<std::string>().c_str(), bytes.data()) != 1) {
        return std::unexpected(json_error(name, "invalid IP address"));
      }
      return CborValue::byte_string(std::move(bytes));
    }
    if (is_one_of(name, {"KESSignature", "PlutusV1Script", "PlutusV2Script", "PlutusV3Script"})) {
      if (!parsed.is_string()) {
        return std::unexpected(json_error(name, "byte value must be lowercase hex string"));
      }
      auto bytes = core::hex_to_bytes(parsed.get<std::string>());
      return bytes ? core::Result<CborValue>(CborValue::byte_string(std::move(*bytes)))
                   : std::unexpected(bytes.error());
    }
    if (is_one_of(name, {"DNSName", "ShelleyDNSName", "Url"})) {
      if (!parsed.is_string()) {
        return std::unexpected(json_error(name, "text value must be a string"));
      }
      return CborValue::text_string(parsed.get<std::string>());
    }
  } catch (const Json::exception& error) {
    return std::unexpected(json_error(name, error.what()));
  }
  return cbor_value_from_json(json);
}

core::Result<std::string> era_model_value_to_json(std::string_view name, const CborValue& value) {
  if (name == "NativeScript" || name == "MultisigScript") {
    auto converted = native_script_to_json(name, value);
    return converted ? core::Result<std::string>(converted->dump())
                     : std::unexpected(converted.error());
  }
  const std::array<std::pair<std::string_view, std::string_view>, 10> script_variants{{
      {"ScriptPubkey", "ScriptPubkey"},
      {"ScriptAll", "ScriptAll"},
      {"ScriptAny", "ScriptAny"},
      {"ScriptNOfK", "ScriptNOfK"},
      {"ScriptInvalidBefore", "ScriptInvalidBefore"},
      {"ScriptInvalidHereafter", "ScriptInvalidHereafter"},
      {"MultisigPubkey", "ScriptPubkey"},
      {"MultisigAll", "ScriptAll"},
      {"MultisigAny", "ScriptAny"},
      {"MultisigNOfK", "ScriptNOfK"},
  }};
  for (const auto& [model, variant] : script_variants) {
    if (name != model) continue;
    auto converted = native_script_to_json(name, value);
    if (!converted) return std::unexpected(converted.error());
    return converted->at(variant).dump();
  }
  if (name == "Rational" || name == "UnitInterval") {
    auto converted = specialized_rational_to_json(name, value);
    return converted ? core::Result<std::string>(converted->dump())
                     : std::unexpected(converted.error());
  }
  if (name == "Anchor") {
    auto converted = specialized_anchor_to_json(name, value);
    return converted ? core::Result<std::string>(converted->dump())
                     : std::unexpected(converted.error());
  }
  if (name == "ProtocolVersion" || name == "ProtocolVersionStruct") {
    auto converted = specialized_protocol_version_to_json(name, value);
    return converted ? core::Result<std::string>(converted->dump())
                     : std::unexpected(converted.error());
  }
  if (name == "VRFCert") {
    auto shape = require_array_size(name, value, 2, 2);
    if (!shape) return std::unexpected(shape.error());
    const auto* output = value.as_array()->values[0].as_byte_string();
    const auto* proof = value.as_array()->values[1].as_byte_string();
    if (output == nullptr || proof == nullptr) {
      return std::unexpected(json_error(name, "invalid VRF certificate value"));
    }
    return Json{{"output", bytes_json(output->value)}, {"proof", bytes_json(proof->value)}}.dump();
  }
  if (name == "Ipv4" || name == "Ipv6") {
    const auto* bytes = value.as_byte_string();
    const auto expected = name == "Ipv4" ? 4U : 16U;
    if (bytes == nullptr || bytes->value.size() != expected) {
      return std::unexpected(json_error(name, "invalid IP address bytes"));
    }
    std::array<char, INET6_ADDRSTRLEN> buffer{};
    const auto family = name == "Ipv4" ? AF_INET : AF_INET6;
    if (inet_ntop(family, bytes->value.data(), buffer.data(), buffer.size()) == nullptr) {
      return std::unexpected(json_error(name, "cannot format IP address"));
    }
    return Json(std::string(buffer.data())).dump();
  }
  if (is_one_of(name, {"KESSignature", "PlutusV1Script", "PlutusV2Script", "PlutusV3Script"})) {
    const auto* bytes = value.as_byte_string();
    if (bytes == nullptr) return std::unexpected(json_error(name, "invalid byte value"));
    return Json(core::bytes_to_hex(bytes->value)).dump();
  }
  if (is_one_of(name, {"DNSName", "ShelleyDNSName", "Url"})) {
    const auto* text = value.as_text_string();
    if (text == nullptr) return std::unexpected(json_error(name, "invalid text value"));
    return Json(text->value).dump();
  }
  return cbor_value_to_json(value, true);
}

}  // namespace cardano::chain::detail
