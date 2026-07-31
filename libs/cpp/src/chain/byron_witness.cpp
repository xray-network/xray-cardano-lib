#include <algorithm>
#include <nlohmann/json.hpp>
#include <utility>

#include "cardano/chain/byron.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/crypto/primitives.hpp"

namespace cardano::chain {
namespace {

[[nodiscard]] core::CardanoError invalid(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::cbor::Value cbor_bytes(core::ByteSpan bytes) {
  return core::cbor::Value::byte_string(core::Bytes(bytes.begin(), bytes.end()));
}

}  // namespace

Vkeywitness::Vkeywitness(crypto::PublicKey public_key, crypto::Ed25519Signature signature)
    : public_key_(std::move(public_key)), signature_(std::move(signature)) {}
core::Result<Vkeywitness> Vkeywitness::from_cbor(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  const auto* array = value->as_array();
  if (array == nullptr || array->values.size() != 2 ||
      array->values[0].as_byte_string() == nullptr ||
      array->values[1].as_byte_string() == nullptr) {
    return std::unexpected(invalid("vkey witness must be [public-key, signature]"));
  }
  auto public_key = crypto::PublicKey::from_bytes(array->values[0].as_byte_string()->value);
  auto signature = crypto::Ed25519Signature::from_bytes(array->values[1].as_byte_string()->value);
  if (!public_key) return std::unexpected(public_key.error());
  if (!signature) return std::unexpected(signature.error());
  return Vkeywitness(std::move(*public_key), std::move(*signature));
}
core::Result<Vkeywitness> Vkeywitness::from_json(std::string_view json) {
  try {
    const auto value = nlohmann::json::parse(json);
    if (!value.is_object() || !value.contains("public_key") ||
        !value.at("public_key").is_string() || !value.contains("signature") ||
        !value.at("signature").is_string()) {
      return std::unexpected(invalid("invalid vkey witness JSON"));
    }
    auto public_key = crypto::PublicKey::from_hex(value.at("public_key").get<std::string>());
    auto signature = crypto::Ed25519Signature::from_hex(value.at("signature").get<std::string>());
    if (!public_key) return std::unexpected(public_key.error());
    if (!signature) return std::unexpected(signature.error());
    return Vkeywitness(std::move(*public_key), std::move(*signature));
  } catch (const nlohmann::json::exception& error) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding,
                           std::string("invalid vkey witness JSON: ") + error.what()));
  }
}
const crypto::PublicKey& Vkeywitness::public_key() const noexcept { return public_key_; }
const crypto::Ed25519Signature& Vkeywitness::signature() const noexcept { return signature_; }
core::Result<core::Bytes> Vkeywitness::to_cbor() const {
  return core::cbor::encode_cbor(core::cbor::Value::array({
      cbor_bytes(public_key_.to_bytes()),
      cbor_bytes(signature_.span()),
  }));
}
std::string Vkeywitness::to_json() const {
  return nlohmann::json{
      {"public_key", public_key_.to_hex()},
      {"signature", signature_.to_hex()},
  }
      .dump();
}

BootstrapWitness::BootstrapWitness(crypto::PublicKey public_key, crypto::Ed25519Signature signature,
                                   std::array<core::Byte, 32> chain_code, AddrAttributes attributes)
    : public_key_(std::move(public_key)),
      signature_(std::move(signature)),
      chain_code_(chain_code),
      attributes_(std::move(attributes)) {}

core::Result<BootstrapWitness> BootstrapWitness::create(crypto::PublicKey public_key,
                                                        crypto::Ed25519Signature signature,
                                                        core::ByteSpan chain_code,
                                                        AddrAttributes attributes) {
  if (chain_code.size() != 32) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "bootstrap witness chain code must be 32 bytes"));
  }
  std::array<core::Byte, 32> owned{};
  std::ranges::copy(chain_code, owned.begin());
  auto encoded_attributes = attributes.to_cbor();
  if (!encoded_attributes) {
    return std::unexpected(encoded_attributes.error());
  }
  auto normalized_attributes = AddrAttributes::from_cbor(*encoded_attributes);
  if (!normalized_attributes) {
    return std::unexpected(normalized_attributes.error());
  }
  return BootstrapWitness(std::move(public_key), std::move(signature), owned,
                          std::move(*normalized_attributes));
}

core::Result<BootstrapWitness> BootstrapWitness::from_cbor(core::ByteSpan bytes) {
  auto decoded = core::cbor::decode_cbor(bytes);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  const auto* array = decoded->as_array();
  if (array == nullptr || array->values.size() != 4) {
    return std::unexpected(invalid("bootstrap witness must be a four-item array"));
  }
  for (const auto& value : array->values) {
    if (value.as_byte_string() == nullptr) {
      return std::unexpected(invalid("bootstrap witness fields must be byte strings"));
    }
  }
  auto public_key = crypto::PublicKey::from_bytes(array->values[0].as_byte_string()->value);
  auto signature = crypto::Ed25519Signature::from_bytes(array->values[1].as_byte_string()->value);
  auto attributes = AddrAttributes::from_cbor(array->values[3].as_byte_string()->value);
  if (!public_key) {
    return std::unexpected(public_key.error());
  }
  if (!signature) {
    return std::unexpected(signature.error());
  }
  if (!attributes) {
    return std::unexpected(attributes.error());
  }
  return create(std::move(*public_key), std::move(*signature),
                array->values[2].as_byte_string()->value, std::move(*attributes));
}

core::Result<BootstrapWitness> BootstrapWitness::from_json(std::string_view json) {
  try {
    const auto value = nlohmann::json::parse(json);
    if (!value.is_object()) {
      return std::unexpected(invalid("bootstrap witness JSON must be an object"));
    }
    for (const auto* field : {"public_key", "signature", "chain_code", "attributes"}) {
      if (!value.contains(field) || !value.at(field).is_string()) {
        return std::unexpected(invalid(std::string("bootstrap witness JSON field ") + field +
                                       " must be a hex string"));
      }
    }
    auto encoded = core::hex_to_bytes(value.at("attributes").get<std::string>());
    auto chain_code = core::hex_to_bytes(value.at("chain_code").get<std::string>());
    auto public_key = crypto::PublicKey::from_hex(value.at("public_key").get<std::string>());
    auto signature = crypto::Ed25519Signature::from_hex(value.at("signature").get<std::string>());
    if (!encoded) {
      return std::unexpected(encoded.error());
    }
    if (!chain_code) {
      return std::unexpected(chain_code.error());
    }
    if (!public_key) {
      return std::unexpected(public_key.error());
    }
    if (!signature) {
      return std::unexpected(signature.error());
    }
    auto attributes = AddrAttributes::from_cbor(*encoded);
    if (!attributes) {
      return std::unexpected(attributes.error());
    }
    return create(std::move(*public_key), std::move(*signature), *chain_code,
                  std::move(*attributes));
  } catch (const nlohmann::json::exception& error) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_encoding,
                           std::string("invalid bootstrap witness JSON: ") + error.what()));
  }
}

const crypto::PublicKey& BootstrapWitness::public_key() const noexcept { return public_key_; }
const crypto::Ed25519Signature& BootstrapWitness::signature() const noexcept { return signature_; }
core::Bytes BootstrapWitness::chain_code() const {
  return core::Bytes(chain_code_.begin(), chain_code_.end());
}
const AddrAttributes& BootstrapWitness::attributes() const noexcept { return attributes_; }

core::Result<AddressContent> BootstrapWitness::to_address_content() const {
  core::Bytes extended = public_key_.to_bytes();
  extended.insert(extended.end(), chain_code_.begin(), chain_code_.end());
  auto public_key = crypto::Bip32PublicKey::from_bytes(extended);
  return public_key
             ? AddressContent::hash_and_create(ByronAddrType::public_key,
                                               SpendingData::public_key(*public_key), attributes_)
             : std::unexpected(public_key.error());
}

core::Result<core::Bytes> BootstrapWitness::to_cbor() const {
  auto attributes = attributes_.to_cbor();
  if (!attributes) {
    return std::unexpected(attributes.error());
  }
  return core::cbor::encode_cbor(core::cbor::Value::array({
      cbor_bytes(public_key_.to_bytes()),
      cbor_bytes(signature_.span()),
      cbor_bytes(chain_code_),
      cbor_bytes(*attributes),
  }));
}

std::string BootstrapWitness::to_json() const {
  const auto attributes = attributes_.to_cbor();
  return nlohmann::json{
      {"public_key", public_key_.to_hex()},
      {"signature", signature_.to_hex()},
      {"chain_code", core::bytes_to_hex(chain_code_)},
      {"attributes", attributes ? core::bytes_to_hex(*attributes) : std::string{}},
  }
      .dump();
}

ByronGenesisRedeem::ByronGenesisRedeem(crypto::TransactionHash transaction_id, ByronAddress address)
    : transaction_id_(std::move(transaction_id)), address_(std::move(address)) {}
const crypto::TransactionHash& ByronGenesisRedeem::transaction_id() const noexcept {
  return transaction_id_;
}
const ByronAddress& ByronGenesisRedeem::address() const noexcept { return address_; }

core::Result<ByronGenesisRedeem> genesis_txid_byron(
    const crypto::PublicKey& public_key, std::optional<core::ProtocolMagic> protocol_magic) {
  auto content = AddressContent::redeem(public_key, protocol_magic);
  if (!content) {
    return std::unexpected(content.error());
  }
  auto address = ByronAddress::from_content(*content);
  if (!address) {
    return std::unexpected(address.error());
  }
  auto transaction_id =
      crypto::TransactionHash::from_bytes(crypto::blake2b256(address->to_bytes()));
  return transaction_id ? core::Result<ByronGenesisRedeem>(
                              ByronGenesisRedeem(std::move(*transaction_id), std::move(*address)))
                        : std::unexpected(transaction_id.error());
}

core::Result<BootstrapWitness> make_icarus_bootstrap_witness(
    const crypto::TransactionHash& transaction_body_hash, const ByronAddress& address,
    const crypto::Bip32PrivateKey& key) {
  auto bip32_public = key.public_key();
  auto signature = key.sign(transaction_body_hash.span());
  auto content = address.content();
  if (!bip32_public) {
    return std::unexpected(bip32_public.error());
  }
  if (!signature) {
    return std::unexpected(signature.error());
  }
  if (!content) {
    return std::unexpected(content.error());
  }
  return BootstrapWitness::create(bip32_public->public_key(), std::move(*signature),
                                  key.chain_code(), content->attributes());
}

core::Result<BootstrapWitness> make_daedalus_bootstrap_witness(
    const crypto::TransactionHash& transaction_body_hash, const ByronAddress& address,
    const crypto::LegacyDaedalusPrivateKey& key) {
  auto extended_public = crypto::legacy_public_key(key);
  auto signature = crypto::legacy_sign(key, transaction_body_hash.span());
  auto chain_code = key.chain_code();
  auto content = address.content();
  if (!extended_public) {
    return std::unexpected(extended_public.error());
  }
  if (!signature) {
    return std::unexpected(signature.error());
  }
  if (!chain_code) {
    return std::unexpected(chain_code.error());
  }
  if (!content) {
    return std::unexpected(content.error());
  }
  auto public_key = crypto::PublicKey::from_bytes(core::ByteSpan(*extended_public).first(32));
  return public_key ? BootstrapWitness::create(std::move(*public_key), std::move(*signature),
                                               *chain_code, content->attributes())
                    : std::unexpected(public_key.error());
}

}  // namespace cardano::chain
