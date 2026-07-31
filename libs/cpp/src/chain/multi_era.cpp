#include "cardano/chain/multi_era.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cardano/chain/ledger.hpp"
#include "cardano/crypto/primitives.hpp"

namespace cardano::chain {
namespace {

[[nodiscard]] core::CardanoError structure_error(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::Result<std::uint64_t> unsigned_value(const core::cbor::Value& value,
                                                         std::string_view description) {
  const auto* integer = value.as_unsigned();
  if (integer == nullptr) {
    return std::unexpected(
        structure_error(std::string(description) + " must be an unsigned integer"));
  }
  auto converted = integer->value.to_uint64();
  if (!converted) {
    return std::unexpected(converted.error());
  }
  return *converted;
}

[[nodiscard]] core::Result<core::Bytes> required_hash(const core::cbor::Value& value,
                                                      std::string_view description) {
  const auto* bytes = value.as_byte_string();
  if (bytes == nullptr || bytes->value.size() != 32U) {
    return std::unexpected(structure_error(std::string(description) + " must be exactly 32 bytes"));
  }
  return bytes->value;
}

[[nodiscard]] core::VoidResult require_bytes(const core::cbor::Value& value, std::size_t size,
                                             std::string_view description) {
  const auto* bytes = value.as_byte_string();
  if (bytes == nullptr || bytes->value.size() != size) {
    return std::unexpected(
        structure_error(std::string(description) + " has the wrong byte length"));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult require_vrf_certificate(const core::cbor::Value& value) {
  const auto* certificate = value.as_array();
  if (certificate == nullptr || certificate->values.size() != 2U) {
    return std::unexpected(structure_error("VRF certificate must be [output, proof]"));
  }
  auto output = require_bytes(certificate->values[0], 64U, "VRF output");
  if (!output) {
    return output;
  }
  return require_bytes(certificate->values[1], 80U, "VRF proof");
}

[[nodiscard]] const core::cbor::Value* map_unsigned_key(const core::cbor::Value& value,
                                                        std::uint64_t wanted) {
  const auto* map = value.as_map();
  if (map == nullptr) {
    return nullptr;
  }
  for (const auto& [key, item] : map->entries) {
    const auto* number = key.as_unsigned();
    if (number == nullptr) {
      continue;
    }
    const auto converted = number->value.to_uint64();
    if (converted && *converted == wanted) {
      return &item;
    }
  }
  return nullptr;
}

[[nodiscard]] const core::cbor::ArrayValue* optional_set_array(const core::cbor::Value& value) {
  if (const auto* bare = value.as_array()) {
    return bare;
  }
  const auto* tag = value.as_tag();
  if (tag == nullptr || tag->value == nullptr || tag->tag != core::BigInteger(std::uint64_t{258})) {
    return nullptr;
  }
  return tag->value->as_array();
}

[[nodiscard]] core::VoidResult validate_transaction_body_common(const core::cbor::Value& body) {
  if (body.as_map() == nullptr) {
    return std::unexpected(structure_error("post-Byron transaction body must be a map"));
  }
  const auto* certificates = map_unsigned_key(body, 4U);
  if (certificates == nullptr) {
    return std::monostate{};
  }
  const auto* list = optional_set_array(*certificates);
  if (list == nullptr) {
    return std::unexpected(
        structure_error("transaction certificates must be an array or tag-258 array"));
  }
  for (std::size_t index = 0; index < list->values.size(); ++index) {
    const auto* certificate = list->values[index].as_array();
    if (certificate == nullptr || certificate->values.empty()) {
      return std::unexpected(structure_error("transaction certificate must be a nonempty array"));
    }
    auto kind = unsigned_value(certificate->values[0], "certificate discriminator");
    if (!kind) {
      return std::unexpected(kind.error().at(index));
    }
    if (*kind == 2U) {
      if (certificate->values.size() != 3U) {
        return std::unexpected(
            structure_error("stake-delegation certificate must contain three fields"));
      }
      auto pool_hash = require_bytes(certificate->values[2], 28U, "stake-delegation pool key hash");
      if (!pool_hash) {
        return std::unexpected(pool_hash.error().at(index));
      }
    }
  }
  return std::monostate{};
}

[[nodiscard]] core::Result<core::cbor::Value> later_header_body(const core::cbor::Value& header,
                                                                std::uint8_t network_tag) {
  const auto* header_array = header.as_array();
  if (header_array == nullptr || header_array->values.size() != 2U) {
    return std::unexpected(
        structure_error("post-Byron header must be [header-body, kes-signature]"));
  }
  const auto* body = header_array->values[0].as_array();
  const std::size_t body_hash_index = network_tag >= 6U ? 7U : 8U;
  if (body == nullptr || body->values.size() <= body_hash_index) {
    return std::unexpected(structure_error("post-Byron header body omits mandatory fields"));
  }
  auto body_hash = required_hash(body->values[body_hash_index], "header body hash");
  if (!body_hash) {
    return std::unexpected(body_hash.error().at(body_hash_index));
  }
  if (!std::holds_alternative<core::cbor::NullValue>(body->values[2].node())) {
    auto previous = required_hash(body->values[2], "previous block hash");
    if (!previous) {
      return std::unexpected(previous.error().at(2U));
    }
  }
  for (const auto index : {3U, 4U}) {
    auto key = require_bytes(body->values[index], 32U, "header verification key");
    if (!key) {
      return std::unexpected(key.error().at(index));
    }
  }
  if (network_tag >= 6U) {
    auto vrf = require_vrf_certificate(body->values[5]);
    if (!vrf) {
      return std::unexpected(vrf.error().at(5U));
    }
    auto size = unsigned_value(body->values[6], "block body size");
    if (!size || *size > std::numeric_limits<std::uint32_t>::max()) {
      return std::unexpected(size ? structure_error("block body size exceeds uint32")
                                  : size.error());
    }
    const auto* operational = body->values[8].as_array();
    const auto* protocol = body->values[9].as_array();
    if (operational == nullptr || operational->values.size() != 4U || protocol == nullptr ||
        protocol->values.size() != 2U) {
      return std::unexpected(
          structure_error("header operational certificate or protocol version is malformed"));
    }
    auto hot_key = require_bytes(operational->values[0], 32U, "operational certificate hot key");
    auto sequence = unsigned_value(operational->values[1], "operational certificate sequence");
    auto period = unsigned_value(operational->values[2], "operational certificate KES period");
    auto signature =
        require_bytes(operational->values[3], 64U, "operational certificate signature");
    auto major = unsigned_value(protocol->values[0], "protocol major");
    auto minor = unsigned_value(protocol->values[1], "protocol minor");
    if (!hot_key || !sequence || !period || !signature || !major || !minor) {
      return std::unexpected(
          structure_error("header operational certificate or protocol version is malformed"));
    }
  } else {
    for (const auto index : {5U, 6U}) {
      auto vrf = require_vrf_certificate(body->values[index]);
      if (!vrf) {
        return std::unexpected(vrf.error().at(index));
      }
    }
    auto size = unsigned_value(body->values[7], "block body size");
    if (!size || *size > std::numeric_limits<std::uint32_t>::max()) {
      return std::unexpected(size ? structure_error("block body size exceeds uint32")
                                  : size.error());
    }
    if (body->values.size() < 15U) {
      return std::unexpected(structure_error("Shelley-through-Alonzo header body is incomplete"));
    }
    auto hot_key = require_bytes(body->values[9], 32U, "operational certificate hot key");
    auto sequence = unsigned_value(body->values[10], "operational certificate sequence");
    auto period = unsigned_value(body->values[11], "operational certificate KES period");
    auto signature = require_bytes(body->values[12], 64U, "operational certificate signature");
    auto major = unsigned_value(body->values[13], "protocol major");
    auto minor = unsigned_value(body->values[14], "protocol minor");
    if (!hot_key || !sequence || !period || !signature || !major || !minor) {
      return std::unexpected(
          structure_error("header operational certificate or protocol version is malformed"));
    }
  }
  auto kes_signature = require_bytes(header_array->values[1], 448U, "KES signature");
  if (!kes_signature) {
    return std::unexpected(kes_signature.error().at(1U));
  }
  return header_array->values[0];
}

[[nodiscard]] core::Result<core::cbor::Value> validate_envelope(const core::cbor::Value& envelope,
                                                                std::uint8_t& tag,
                                                                core::cbor::Value& header) {
  const auto* explicit_block = envelope.as_array();
  if (explicit_block == nullptr || explicit_block->values.size() != 2U) {
    return std::unexpected(structure_error("explicit-network block must be [network-tag, block]"));
  }
  auto tag_value = unsigned_value(explicit_block->values[0], "explicit-network block tag");
  if (!tag_value || *tag_value > 7U) {
    return std::unexpected(tag_value ? structure_error("explicit-network block tag must be in 0..7")
                                     : tag_value.error());
  }
  tag = static_cast<std::uint8_t>(*tag_value);
  const auto& block = explicit_block->values[1];
  const auto* block_array = block.as_array();
  if (block_array == nullptr || block_array->values.empty()) {
    return std::unexpected(structure_error("era block must be a nonempty array"));
  }
  if (tag <= 1U) {
    header = block_array->values[0];
    if (header.as_array() == nullptr) {
      return std::unexpected(structure_error("Byron block header must be an array"));
    }
    return block;
  }
  if (block_array->values.size() < 4U) {
    return std::unexpected(structure_error(
        "post-Byron block must contain header, bodies, witnesses, and auxiliary data"));
  }
  header = block_array->values[0];
  auto body = later_header_body(header, tag);
  if (!body) {
    return std::unexpected(body.error().at(0U));
  }
  const auto* bodies = block_array->values[1].as_array();
  const auto* witnesses = block_array->values[2].as_array();
  const auto* auxiliary = block_array->values[3].as_map();
  if (bodies == nullptr || witnesses == nullptr || auxiliary == nullptr ||
      bodies->values.size() != witnesses->values.size()) {
    return std::unexpected(
        structure_error("post-Byron block transaction collections are inconsistent"));
  }
  for (std::size_t index = 0; index < bodies->values.size(); ++index) {
    auto valid = validate_transaction_body_common(bodies->values[index]);
    if (!valid) {
      return std::unexpected(valid.error().at(index).at(1U));
    }
  }
  for (const auto& [index, value] : auxiliary->entries) {
    auto decoded_index = unsigned_value(index, "auxiliary-data index");
    if (!decoded_index || *decoded_index >= bodies->values.size()) {
      return std::unexpected(
          structure_error("auxiliary-data index is outside the transaction body array"));
    }
    (void)value;
  }
  if (tag >= 5U) {
    if (block_array->values.size() < 5U || block_array->values[4].as_array() == nullptr) {
      return std::unexpected(
          structure_error("Alonzo-or-later block must contain an invalid-transaction index array"));
    }
    for (const auto& invalid : block_array->values[4].as_array()->values) {
      auto decoded_index = unsigned_value(invalid, "invalid-transaction index");
      if (!decoded_index || *decoded_index >= bodies->values.size()) {
        return std::unexpected(
            structure_error("invalid-transaction index is outside the transaction body array"));
      }
    }
  }
  return block;
}

}  // namespace

MultiEraCertificate::MultiEraCertificate(MultiEraCertificateKind kind, core::cbor::Value value)
    : kind_(kind), value_(std::move(value)) {}
core::Result<MultiEraCertificate> MultiEraCertificate::from_cbor(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  const auto* array = value->as_array();
  if (array == nullptr || array->values.empty()) {
    return std::unexpected(structure_error("certificate must be a nonempty array"));
  }
  auto kind = unsigned_value(array->values[0], "certificate kind");
  if (!kind || *kind > 18) {
    return std::unexpected(kind ? structure_error("certificate kind must be in 0..18")
                                : kind.error());
  }
  return MultiEraCertificate(static_cast<MultiEraCertificateKind>(*kind), std::move(*value));
}
core::Result<MultiEraCertificate> MultiEraCertificate::from_json(std::string_view json) {
  auto value = cbor_value_from_json(json);
  if (!value) return std::unexpected(value.error());
  auto encoded = core::cbor::encode_cbor(*value);
  return encoded ? from_cbor(*encoded) : std::unexpected(encoded.error());
}
MultiEraCertificateKind MultiEraCertificate::kind() const noexcept { return kind_; }
const core::cbor::Value& MultiEraCertificate::cbor() const noexcept { return value_; }
core::Result<core::Bytes> MultiEraCertificate::to_cbor(core::cbor::Mode mode) const {
  return core::cbor::encode_cbor(value_, {.mode = mode});
}
core::Result<std::string> MultiEraCertificate::to_json() const {
  return cbor_value_to_json(value_, true);
}

MultiEraProtocolParamUpdate::MultiEraProtocolParamUpdate(core::cbor::Value value)
    : value_(std::move(value)) {}
core::Result<MultiEraProtocolParamUpdate> MultiEraProtocolParamUpdate::from_cbor(
    core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  if (value->as_map() == nullptr) {
    return std::unexpected(structure_error("protocol parameter update must be a map"));
  }
  return MultiEraProtocolParamUpdate(std::move(*value));
}
const core::cbor::Value& MultiEraProtocolParamUpdate::cbor() const noexcept { return value_; }
core::Result<core::Bytes> MultiEraProtocolParamUpdate::to_cbor(core::cbor::Mode mode) const {
  return core::cbor::encode_cbor(value_, {.mode = mode});
}
core::Result<std::optional<std::uint64_t>> MultiEraProtocolParamUpdate::numeric(
    std::uint64_t key) const {
  const auto* value = map_unsigned_key(value_, key);
  if (value == nullptr) return std::nullopt;
  auto number = unsigned_value(*value, "protocol parameter value");
  return number ? core::Result<std::optional<std::uint64_t>>(*number)
                : std::unexpected(number.error());
}
core::Result<std::optional<std::uint64_t>> MultiEraProtocolParamUpdate::minfee_a() const {
  return numeric(0);
}
core::Result<std::optional<std::uint64_t>> MultiEraProtocolParamUpdate::minfee_b() const {
  return numeric(1);
}
core::Result<std::optional<std::uint64_t>> MultiEraProtocolParamUpdate::max_transaction_size()
    const {
  return numeric(3);
}
core::Result<std::optional<std::uint64_t>> MultiEraProtocolParamUpdate::collateral_percentage()
    const {
  return numeric(23);
}
core::Result<std::optional<std::uint64_t>> MultiEraProtocolParamUpdate::max_collateral_inputs()
    const {
  return numeric(24);
}

MultiEraUpdate::MultiEraUpdate(core::cbor::Value value, core::cbor::Value proposed_updates,
                               std::uint64_t epoch)
    : value_(std::move(value)), proposed_updates_(std::move(proposed_updates)), epoch_(epoch) {}
core::Result<MultiEraUpdate> MultiEraUpdate::from_cbor(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  const auto* array = value->as_array();
  if (array == nullptr || array->values.size() != 2 || array->values[0].as_map() == nullptr) {
    return std::unexpected(structure_error("update must be [proposed-updates, epoch]"));
  }
  auto epoch = unsigned_value(array->values[1], "update epoch");
  return epoch ? core::Result<MultiEraUpdate>(
                     MultiEraUpdate(std::move(*value), array->values[0], *epoch))
               : std::unexpected(epoch.error());
}
std::uint64_t MultiEraUpdate::epoch() const noexcept { return epoch_; }
const core::cbor::Value& MultiEraUpdate::proposed_updates() const noexcept {
  return proposed_updates_;
}
core::Result<core::Bytes> MultiEraUpdate::to_cbor(core::cbor::Mode mode) const {
  return core::cbor::encode_cbor(value_, {.mode = mode});
}

MultiEraBlockHeader::MultiEraBlockHeader(std::uint8_t network_tag, core::cbor::Value header)
    : network_tag_(network_tag), header_(std::move(header)) {}

const core::cbor::Value& MultiEraBlockHeader::cbor() const noexcept { return header_; }

core::Result<core::Bytes> MultiEraBlockHeader::to_cbor(core::cbor::Mode mode) const {
  return core::cbor::encode_cbor(header_, core::cbor::EncodeOptions{.mode = mode});
}

core::Result<std::uint64_t> MultiEraBlockHeader::block_number() const {
  const auto* header = header_.as_array();
  if (network_tag_ >= 2U) {
    auto body = later_header_body(header_, network_tag_);
    if (!body) {
      return std::unexpected(body.error());
    }
    return unsigned_value(body->as_array()->values[0], "block number");
  }
  if (header == nullptr || header->values.size() < 4U) {
    return std::unexpected(structure_error("Byron header is malformed"));
  }
  const auto* consensus = header->values[3].as_array();
  if (consensus == nullptr) {
    return std::unexpected(structure_error("Byron consensus data must be an array"));
  }
  if (network_tag_ == 0U) {
    if (consensus->values.size() < 2U || consensus->values[1].as_array() == nullptr ||
        consensus->values[1].as_array()->values.empty()) {
      return std::unexpected(structure_error("Byron EBB difficulty is malformed"));
    }
    return unsigned_value(consensus->values[1].as_array()->values[0], "Byron EBB block number");
  }
  if (consensus->values.size() < 3U || consensus->values[2].as_array() == nullptr ||
      consensus->values[2].as_array()->values.empty()) {
    return std::unexpected(structure_error("Byron main-block difficulty is malformed"));
  }
  return unsigned_value(consensus->values[2].as_array()->values[0], "Byron block number");
}

core::Result<std::uint64_t> MultiEraBlockHeader::slot() const {
  if (network_tag_ >= 2U) {
    auto body = later_header_body(header_, network_tag_);
    if (!body) {
      return std::unexpected(body.error());
    }
    return unsigned_value(body->as_array()->values[1], "slot");
  }
  const auto* header = header_.as_array();
  if (header == nullptr || header->values.size() < 4U || header->values[3].as_array() == nullptr ||
      header->values[3].as_array()->values.empty()) {
    return std::unexpected(structure_error("Byron slot data is malformed"));
  }
  const auto& slot_id = header->values[3].as_array()->values[0];
  if (network_tag_ == 0U) {
    auto epoch = unsigned_value(slot_id, "Byron EBB epoch");
    if (!epoch || *epoch > std::numeric_limits<std::uint64_t>::max() / 21600U) {
      return std::unexpected(epoch ? core::CardanoError(core::ErrorCode::out_of_range,
                                                        "Byron EBB slot overflows uint64")
                                   : epoch.error());
    }
    return *epoch * 21600U;
  }
  const auto* pair = slot_id.as_array();
  if (pair == nullptr || pair->values.size() != 2U) {
    return std::unexpected(structure_error("Byron slot id must be [epoch, slot]"));
  }
  auto epoch = unsigned_value(pair->values[0], "Byron epoch");
  auto local = unsigned_value(pair->values[1], "Byron local slot");
  if (!epoch || !local || *epoch > (std::numeric_limits<std::uint64_t>::max() - *local) / 21600U) {
    return std::unexpected(structure_error("Byron absolute slot overflows uint64"));
  }
  return *epoch * 21600U + *local;
}

core::Result<std::optional<core::Bytes>> MultiEraBlockHeader::previous_hash() const {
  const auto* header = header_.as_array();
  if (network_tag_ >= 2U) {
    auto body = later_header_body(header_, network_tag_);
    if (!body) {
      return std::unexpected(body.error());
    }
    const auto& previous = body->as_array()->values[2];
    if (std::holds_alternative<core::cbor::NullValue>(previous.node())) {
      return std::optional<core::Bytes>{};
    }
    auto bytes = required_hash(previous, "previous block hash");
    if (!bytes) {
      return std::unexpected(bytes.error());
    }
    return std::optional<core::Bytes>(std::move(*bytes));
  }
  if (header == nullptr || header->values.size() < 2U) {
    return std::unexpected(structure_error("Byron header is malformed"));
  }
  auto bytes = required_hash(header->values[1], "Byron previous block hash");
  if (!bytes) {
    return std::unexpected(bytes.error());
  }
  return std::optional<core::Bytes>(std::move(*bytes));
}

core::Result<core::Bytes> MultiEraBlockHeader::hash() const {
  auto encoded = to_cbor(core::cbor::Mode::preserve);
  if (!encoded) {
    return std::unexpected(encoded.error());
  }
  if (network_tag_ <= 1U) {
    core::Bytes domain{core::Byte{0x82}, static_cast<core::Byte>(network_tag_)};
    domain.insert(domain.end(), encoded->begin(), encoded->end());
    return crypto::blake2b256(domain);
  }
  return crypto::blake2b256(*encoded);
}

MultiEraTransactionInput::MultiEraTransactionInput(bool byron, core::cbor::Value value)
    : byron_(byron), value_(std::move(value)) {}

const core::cbor::Value& MultiEraTransactionInput::cbor() const noexcept { return value_; }

namespace {

[[nodiscard]] core::Result<core::cbor::Value> input_reference(const core::cbor::Value& input,
                                                              bool byron) {
  const auto* pair = input.as_array();
  if (pair == nullptr || pair->values.size() != 2U) {
    return std::unexpected(structure_error("transaction input must be a two-item array"));
  }
  if (!byron) {
    return input;
  }
  const auto* tag = pair->values[1].as_tag();
  if (tag != nullptr && tag->value != nullptr && tag->tag == core::BigInteger(std::uint64_t{24})) {
    auto decoded = core::cbor::decode_embedded_cbor(pair->values[1]);
    if (!decoded) {
      return std::unexpected(decoded.error());
    }
    return *decoded;
  }
  const auto* bytes = pair->values[1].as_byte_string();
  if (bytes == nullptr) {
    return std::unexpected(
        structure_error("Byron transaction input must contain embedded CBOR bytes"));
  }
  return core::cbor::decode_cbor(bytes->value);
}

}  // namespace

core::Result<core::Bytes> MultiEraTransactionInput::transaction_hash() const {
  auto reference = input_reference(value_, byron_);
  if (!reference) {
    return std::unexpected(reference.error());
  }
  const auto* pair = reference->as_array();
  if (pair == nullptr || pair->values.size() != 2U) {
    return std::unexpected(structure_error("transaction input reference must be [hash, index]"));
  }
  return required_hash(pair->values[0], "transaction input hash");
}

core::Result<std::uint64_t> MultiEraTransactionInput::index() const {
  auto reference = input_reference(value_, byron_);
  if (!reference) {
    return std::unexpected(reference.error());
  }
  const auto* pair = reference->as_array();
  if (pair == nullptr || pair->values.size() != 2U) {
    return std::unexpected(structure_error("transaction input reference must be [hash, index]"));
  }
  return unsigned_value(pair->values[1], "transaction input index");
}

MultiEraTransactionOutput::MultiEraTransactionOutput(bool byron, core::cbor::Value value)
    : byron_(byron), value_(std::move(value)) {}

const core::cbor::Value& MultiEraTransactionOutput::cbor() const noexcept { return value_; }

core::Result<core::cbor::Value> MultiEraTransactionOutput::address() const {
  if (const auto* array = value_.as_array(); array != nullptr && array->values.size() >= 2U) {
    return array->values[0];
  }
  if (!byron_) {
    if (const auto* address = map_unsigned_key(value_, 0U)) {
      return *address;
    }
  }
  return std::unexpected(structure_error("transaction output has no address"));
}

core::Result<core::cbor::Value> MultiEraTransactionOutput::value() const {
  if (const auto* array = value_.as_array(); array != nullptr && array->values.size() >= 2U) {
    if (byron_ && array->values[1].as_unsigned() == nullptr) {
      return std::unexpected(structure_error("Byron transaction output amount must be unsigned"));
    }
    return array->values[1];
  }
  if (!byron_) {
    if (const auto* amount = map_unsigned_key(value_, 1U)) {
      return *amount;
    }
  }
  return std::unexpected(structure_error("transaction output has no value"));
}

MultiEraTransactionBody::MultiEraTransactionBody(std::uint8_t network_tag, core::cbor::Value body)
    : network_tag_(network_tag), body_(std::move(body)) {}

const core::cbor::Value& MultiEraTransactionBody::cbor() const noexcept { return body_; }

core::Result<core::Bytes> MultiEraTransactionBody::to_cbor(core::cbor::Mode mode) const {
  return core::cbor::encode_cbor(body_, core::cbor::EncodeOptions{.mode = mode});
}

core::Result<core::Bytes> MultiEraTransactionBody::hash() const {
  auto encoded = to_cbor(core::cbor::Mode::preserve);
  return encoded ? core::Result<core::Bytes>(crypto::blake2b256(*encoded))
                 : std::unexpected(encoded.error());
}

MultiEraTransactionBodyKind MultiEraTransactionBody::kind() const noexcept {
  if (network_tag_ <= 1U) {
    return MultiEraTransactionBodyKind::byron;
  }
  return static_cast<MultiEraTransactionBodyKind>(network_tag_ - 1U);
}

std::optional<core::cbor::Value> MultiEraTransactionBody::field(std::uint64_t key) const {
  if (network_tag_ <= 1U) {
    const auto* array = body_.as_array();
    if (array == nullptr || key >= array->values.size()) {
      return std::nullopt;
    }
    return array->values[static_cast<std::size_t>(key)];
  }
  const auto* value = map_unsigned_key(body_, key);
  return value == nullptr ? std::nullopt : std::optional<core::cbor::Value>(*value);
}

core::Result<std::vector<MultiEraTransactionInput>> MultiEraTransactionBody::inputs() const {
  auto input_field = field(0U);
  if (!input_field) {
    return std::unexpected(structure_error("transaction body has no inputs"));
  }
  const auto* inputs = optional_set_array(*input_field);
  if (inputs == nullptr) {
    return std::unexpected(structure_error("transaction inputs must be an array or tag-258 array"));
  }
  std::vector<MultiEraTransactionInput> output;
  output.reserve(inputs->values.size());
  for (const auto& input : inputs->values) {
    output.push_back(MultiEraTransactionInput(network_tag_ <= 1U, input));
  }
  return output;
}

core::Result<std::vector<MultiEraTransactionOutput>> MultiEraTransactionBody::outputs() const {
  auto output_field = field(1U);
  if (!output_field || output_field->as_array() == nullptr) {
    return std::unexpected(structure_error("transaction outputs must be an array"));
  }
  std::vector<MultiEraTransactionOutput> output;
  output.reserve(output_field->as_array()->values.size());
  for (const auto& value : output_field->as_array()->values) {
    output.push_back(MultiEraTransactionOutput(network_tag_ <= 1U, value));
  }
  return output;
}

core::Result<std::optional<std::uint64_t>> MultiEraTransactionBody::fee() const {
  if (network_tag_ <= 1U) {
    return std::optional<std::uint64_t>{};
  }
  auto fee_value = field(2U);
  if (!fee_value) {
    return std::optional<std::uint64_t>{};
  }
  auto fee = unsigned_value(*fee_value, "transaction fee");
  return fee ? core::Result<std::optional<std::uint64_t>>(std::optional<std::uint64_t>(*fee))
             : std::unexpected(fee.error());
}

core::Result<std::vector<MultiEraCertificate>> MultiEraTransactionBody::certificates() const {
  if (network_tag_ <= 1U) {
    return std::vector<MultiEraCertificate>{};
  }
  auto value = field(4);
  if (!value) return std::vector<MultiEraCertificate>{};
  const auto* array = optional_set_array(*value);
  if (array == nullptr) {
    return std::unexpected(
        structure_error("transaction certificates must be an array or tag-258 array"));
  }
  std::vector<MultiEraCertificate> output;
  output.reserve(array->values.size());
  for (const auto& certificate : array->values) {
    auto encoded = core::cbor::encode_cbor(certificate);
    if (!encoded) return std::unexpected(encoded.error());
    auto decoded = MultiEraCertificate::from_cbor(*encoded);
    if (!decoded) return std::unexpected(decoded.error());
    output.push_back(std::move(*decoded));
  }
  return output;
}

core::Result<std::optional<MultiEraUpdate>> MultiEraTransactionBody::update() const {
  if (network_tag_ <= 1U) return std::nullopt;
  auto value = field(6);
  if (!value) return std::nullopt;
  auto encoded = core::cbor::encode_cbor(*value);
  if (!encoded) return std::unexpected(encoded.error());
  auto decoded = MultiEraUpdate::from_cbor(*encoded);
  return decoded ? core::Result<std::optional<MultiEraUpdate>>(std::move(*decoded))
                 : std::unexpected(decoded.error());
}

MultiEraBlock::MultiEraBlock(std::uint8_t network_tag, core::cbor::Value envelope,
                             core::cbor::Value block, core::cbor::Value header)
    : network_tag_(network_tag),
      envelope_(std::move(envelope)),
      block_(std::move(block)),
      header_(std::move(header)) {}

core::Result<MultiEraBlock> MultiEraBlock::from_cbor(core::ByteSpan bytes,
                                                     core::cbor::DecodeOptions options) {
  auto envelope = core::cbor::decode_cbor(bytes, options);
  if (!envelope) {
    return std::unexpected(envelope.error());
  }
  std::uint8_t tag = 0;
  core::cbor::Value header = core::cbor::Value::null();
  auto block = validate_envelope(*envelope, tag, header);
  if (!block) {
    return std::unexpected(block.error());
  }
  return MultiEraBlock(tag, std::move(*envelope), std::move(*block), std::move(header));
}

core::Result<MultiEraBlock> MultiEraBlock::from_cbor_hex(std::string_view hex,
                                                         core::cbor::DecodeOptions options) {
  auto bytes = core::hex_to_bytes(hex);
  return bytes ? from_cbor(*bytes, options) : std::unexpected(bytes.error());
}

MultiEraBlockKind MultiEraBlock::kind() const noexcept {
  if (network_tag_ <= 1U) {
    return MultiEraBlockKind::byron;
  }
  return static_cast<MultiEraBlockKind>(network_tag_ - 1U);
}

std::uint8_t MultiEraBlock::network_tag() const noexcept { return network_tag_; }

const core::cbor::Value& MultiEraBlock::cbor() const noexcept { return envelope_; }

core::Result<core::Bytes> MultiEraBlock::to_cbor(core::cbor::Mode mode) const {
  return core::cbor::encode_cbor(envelope_, core::cbor::EncodeOptions{.mode = mode});
}

MultiEraBlockHeader MultiEraBlock::header() const {
  return MultiEraBlockHeader(network_tag_, header_);
}

core::Result<std::vector<MultiEraTransactionBody>> MultiEraBlock::transaction_bodies() const {
  std::vector<MultiEraTransactionBody> output;
  const auto* block = block_.as_array();
  if (block == nullptr) {
    return std::unexpected(structure_error("block is not an array"));
  }
  if (network_tag_ == 0U) {
    return output;
  }
  if (network_tag_ >= 2U) {
    const auto* bodies = block->values[1].as_array();
    if (bodies == nullptr) {
      return std::unexpected(structure_error("transaction bodies must be an array"));
    }
    output.reserve(bodies->values.size());
    for (const auto& body : bodies->values) {
      output.push_back(MultiEraTransactionBody(network_tag_, body));
    }
    return output;
  }
  if (block->values.size() < 2U || block->values[1].as_array() == nullptr ||
      block->values[1].as_array()->values.empty() ||
      block->values[1].as_array()->values[0].as_array() == nullptr) {
    return std::unexpected(structure_error("Byron transaction payload is malformed"));
  }
  const auto& payload = block->values[1].as_array()->values[0].as_array()->values;
  output.reserve(payload.size());
  for (const auto& auxiliary : payload) {
    const auto* pair = auxiliary.as_array();
    if (pair == nullptr || pair->values.empty()) {
      return std::unexpected(structure_error("Byron transaction auxiliary value is malformed"));
    }
    output.push_back(MultiEraTransactionBody(network_tag_, pair->values[0]));
  }
  return output;
}

}  // namespace cardano::chain
