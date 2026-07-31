#include <algorithm>
#include <limits>
#include <set>
#include <utility>

#include "cardano/chain/address.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/crypto/primitives.hpp"

namespace cardano::chain {
namespace {

[[nodiscard]] core::CardanoError invalid(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::Result<std::uint64_t> unsigned_value(const core::cbor::Value& value,
                                                         std::string_view name) {
  const auto* integer = value.as_unsigned();
  if (integer == nullptr) {
    return std::unexpected(invalid(std::string(name) + " must be unsigned"));
  }
  return integer->value.to_uint64();
}

[[nodiscard]] core::cbor::Value cbor_bytes(core::ByteSpan value) {
  return core::cbor::Value::byte_string(core::Bytes(value.begin(), value.end()));
}

[[nodiscard]] core::cbor::Value cbor_uint(std::uint64_t value) {
  return core::cbor::Value::unsigned_integer(core::BigInteger(value));
}

[[nodiscard]] core::Result<StakeholderId> stakeholder_id(const crypto::Bip32PublicKey& public_key) {
  auto encoded = core::cbor::encode_cbor(cbor_bytes(public_key.to_bytes()));
  if (!encoded) {
    return std::unexpected(encoded.error());
  }
  const auto digest = crypto::blake2b224(crypto::sha3_256(*encoded));
  return StakeholderId::from_bytes(digest);
}

[[nodiscard]] core::Result<AddressId> make_address_id(ByronAddrType kind,
                                                      const SpendingData& spending_data,
                                                      const AddrAttributes& attributes) {
  auto spending = spending_data.to_cbor();
  auto attrs = attributes.to_cbor();
  if (!spending) {
    return std::unexpected(spending.error());
  }
  if (!attrs) {
    return std::unexpected(attrs.error());
  }
  auto spending_node = core::cbor::decode_cbor(*spending);
  auto attrs_node = core::cbor::decode_cbor(*attrs);
  if (!spending_node) {
    return std::unexpected(spending_node.error());
  }
  if (!attrs_node) {
    return std::unexpected(attrs_node.error());
  }
  auto encoded = core::cbor::encode_cbor(core::cbor::Value::array({
      cbor_uint(static_cast<std::uint8_t>(kind)),
      std::move(*spending_node),
      std::move(*attrs_node),
  }));
  if (!encoded) {
    return std::unexpected(encoded.error());
  }
  return AddressId::from_bytes(crypto::blake2b224(crypto::sha3_256(*encoded)));
}

}  // namespace

HDAddressPayload::HDAddressPayload(core::Bytes bytes) : bytes_(std::move(bytes)) {}
core::Bytes HDAddressPayload::get() const { return bytes_; }

SpendingData::SpendingData(SpendingDataKind kind, core::Bytes bytes)
    : kind_(kind), bytes_(std::move(bytes)) {}

SpendingData SpendingData::public_key(const crypto::Bip32PublicKey& value) {
  return SpendingData(SpendingDataKind::public_key, value.to_bytes());
}

SpendingData SpendingData::script(const ByronScript& value) {
  return SpendingData(SpendingDataKind::script, value.to_bytes());
}

SpendingData SpendingData::redeem(const crypto::PublicKey& value) {
  return SpendingData(SpendingDataKind::redeem, value.to_bytes());
}

core::Result<SpendingData> SpendingData::from_cbor(core::ByteSpan encoded) {
  auto decoded = core::cbor::decode_cbor(encoded);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  const auto* array = decoded->as_array();
  if (array == nullptr || array->values.size() != 2 ||
      array->values[1].as_byte_string() == nullptr) {
    return std::unexpected(invalid("Byron spending data must be a two-item array"));
  }
  auto kind = unsigned_value(array->values[0], "Byron spending data kind");
  if (!kind || *kind > 2) {
    return std::unexpected(kind ? invalid("unknown Byron spending data kind") : kind.error());
  }
  const auto value = array->values[1].as_byte_string()->value;
  const auto expected = *kind == 0 ? std::size_t{64} : std::size_t{32};
  if (value.size() != expected) {
    return std::unexpected(core::CardanoError(core::ErrorCode::invalid_length,
                                              "Byron spending data has an invalid length"));
  }
  return SpendingData(static_cast<SpendingDataKind>(*kind), value);
}

SpendingDataKind SpendingData::kind() const noexcept { return kind_; }
core::Bytes SpendingData::bytes() const { return bytes_; }
core::Result<core::Bytes> SpendingData::to_cbor() const {
  return core::cbor::encode_cbor(core::cbor::Value::array({
      cbor_uint(static_cast<std::uint8_t>(kind_)),
      cbor_bytes(bytes_),
  }));
}

StakeDistribution::StakeDistribution(std::optional<StakeholderId> stakeholder)
    : stakeholder_(std::move(stakeholder)) {}

StakeDistribution StakeDistribution::single_key(StakeholderId stakeholder) {
  return StakeDistribution(std::move(stakeholder));
}

StakeDistribution StakeDistribution::bootstrap_era() { return StakeDistribution(std::nullopt); }

core::Result<StakeDistribution> StakeDistribution::from_cbor(core::ByteSpan encoded) {
  auto decoded = core::cbor::decode_cbor(encoded);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  const auto* array = decoded->as_array();
  if (array == nullptr || array->values.empty()) {
    return std::unexpected(invalid("invalid Byron stake distribution"));
  }
  auto kind = unsigned_value(array->values[0], "stake distribution kind");
  if (!kind) {
    return std::unexpected(kind.error());
  }
  if (*kind == 1 && array->values.size() == 1) {
    return bootstrap_era();
  }
  if (*kind == 0 && array->values.size() == 2 && array->values[1].as_byte_string() != nullptr) {
    auto id = StakeholderId::from_bytes(array->values[1].as_byte_string()->value);
    return id ? core::Result<StakeDistribution>(single_key(std::move(*id)))
              : std::unexpected(id.error());
  }
  return std::unexpected(invalid("invalid Byron stake distribution"));
}

StakeDistributionKind StakeDistribution::kind() const noexcept {
  return stakeholder_ ? StakeDistributionKind::single_key : StakeDistributionKind::bootstrap_era;
}

const std::optional<StakeholderId>& StakeDistribution::stakeholder() const noexcept {
  return stakeholder_;
}

core::Result<core::Bytes> StakeDistribution::to_cbor() const {
  std::vector<core::cbor::Value> values;
  values.push_back(cbor_uint(stakeholder_ ? 0 : 1));
  if (stakeholder_) {
    values.push_back(cbor_bytes(stakeholder_->span()));
  }
  return core::cbor::encode_cbor(core::cbor::Value::array(std::move(values)));
}

AddrAttributes AddrAttributes::bootstrap_era(std::optional<HDAddressPayload> derivation_path,
                                             std::optional<core::ProtocolMagic> protocol_magic) {
  AddrAttributes result;
  result.stake_distribution_ = StakeDistribution::bootstrap_era();
  result.derivation_path_ = std::move(derivation_path);
  if (protocol_magic && protocol_magic->value() != core::BYRON_MAINNET_NETWORK_MAGIC) {
    result.protocol_magic_ = protocol_magic->value();
  }
  return result;
}

AddrAttributes AddrAttributes::single_key(const crypto::Bip32PublicKey& public_key,
                                          std::optional<HDAddressPayload> derivation_path,
                                          core::ProtocolMagic protocol_magic) {
  AddrAttributes result;
  auto id = stakeholder_id(public_key);
  if (id) {
    result.stake_distribution_ = StakeDistribution::single_key(std::move(*id));
  }
  result.derivation_path_ = std::move(derivation_path);
  result.protocol_magic_ = protocol_magic.value();
  return result;
}

core::Result<AddrAttributes> AddrAttributes::from_cbor(core::ByteSpan encoded) {
  auto decoded = core::cbor::decode_cbor(encoded);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  const auto* map = decoded->as_map();
  if (map == nullptr) {
    return std::unexpected(invalid("Byron address attributes must be a map"));
  }
  AddrAttributes result;
  std::set<std::uint64_t> seen;
  for (const auto& [key_node, value_node] : map->entries) {
    auto key = unsigned_value(key_node, "Byron address attribute key");
    const auto* value = value_node.as_byte_string();
    if (!key || *key > 2 || value == nullptr || !seen.insert(*key).second) {
      return std::unexpected(
          key ? invalid("unknown, duplicated, or malformed Byron address attribute") : key.error());
    }
    auto embedded = core::cbor::decode_cbor(value->value);
    if (!embedded) {
      return std::unexpected(embedded.error());
    }
    if (*key == 0) {
      auto distribution = StakeDistribution::from_cbor(value->value);
      if (!distribution) {
        return std::unexpected(distribution.error());
      }
      result.stake_distribution_ = std::move(*distribution);
    } else if (*key == 1) {
      const auto* payload = embedded->as_byte_string();
      if (payload == nullptr) {
        return std::unexpected(invalid("Byron HD payload must embed a byte string"));
      }
      result.derivation_path_ = HDAddressPayload(payload->value);
    } else {
      auto magic = unsigned_value(*embedded, "Byron protocol magic");
      if (!magic || *magic > std::numeric_limits<std::uint32_t>::max()) {
        return std::unexpected(magic ? core::CardanoError(core::ErrorCode::out_of_range,
                                                          "Byron protocol magic exceeds uint32")
                                     : magic.error());
      }
      result.protocol_magic_ = static_cast<std::uint32_t>(*magic);
    }
  }
  return result;
}

const std::optional<StakeDistribution>& AddrAttributes::stake_distribution() const noexcept {
  return stake_distribution_;
}
const std::optional<HDAddressPayload>& AddrAttributes::derivation_path() const noexcept {
  return derivation_path_;
}
std::optional<core::ProtocolMagic> AddrAttributes::protocol_magic() const noexcept {
  return protocol_magic_ ? std::optional<core::ProtocolMagic>(core::ProtocolMagic(*protocol_magic_))
                         : std::nullopt;
}
void AddrAttributes::set_stake_distribution(StakeDistribution value) {
  stake_distribution_ = std::move(value);
}
void AddrAttributes::set_derivation_path(HDAddressPayload value) {
  derivation_path_ = std::move(value);
}
void AddrAttributes::set_protocol_magic(core::ProtocolMagic value) {
  protocol_magic_ = value.value();
}

core::Result<core::Bytes> AddrAttributes::to_cbor() const {
  std::vector<std::pair<core::cbor::Value, core::cbor::Value>> entries;
  if (stake_distribution_ && stake_distribution_->kind() == StakeDistributionKind::single_key) {
    auto encoded = stake_distribution_->to_cbor();
    if (!encoded) {
      return std::unexpected(encoded.error());
    }
    entries.emplace_back(cbor_uint(0), cbor_bytes(*encoded));
  }
  if (derivation_path_) {
    auto embedded = core::cbor::encode_cbor(cbor_bytes(derivation_path_->get()));
    if (!embedded) {
      return std::unexpected(embedded.error());
    }
    entries.emplace_back(cbor_uint(1), cbor_bytes(*embedded));
  }
  if (protocol_magic_) {
    auto embedded = core::cbor::encode_cbor(cbor_uint(*protocol_magic_));
    if (!embedded) {
      return std::unexpected(embedded.error());
    }
    entries.emplace_back(cbor_uint(2), cbor_bytes(*embedded));
  }
  return core::cbor::encode_cbor(core::cbor::Value::map(std::move(entries)));
}

AddressContent::AddressContent(AddressId address_id, AddrAttributes attributes, ByronAddrType kind)
    : address_id_(std::move(address_id)), attributes_(std::move(attributes)), kind_(kind) {}

core::Result<AddressContent> AddressContent::from_cbor(core::ByteSpan encoded) {
  auto decoded = core::cbor::decode_cbor(encoded);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  const auto* array = decoded->as_array();
  if (array == nullptr || array->values.size() != 3 ||
      array->values[0].as_byte_string() == nullptr || array->values[1].as_map() == nullptr) {
    return std::unexpected(invalid("invalid Byron address content"));
  }
  auto id = AddressId::from_bytes(array->values[0].as_byte_string()->value);
  auto kind = unsigned_value(array->values[2], "Byron address type");
  auto attrs_bytes = core::cbor::encode_cbor(array->values[1]);
  if (!id) {
    return std::unexpected(id.error());
  }
  if (!kind || *kind > 2) {
    return std::unexpected(kind ? invalid("unknown Byron address type") : kind.error());
  }
  if (!attrs_bytes) {
    return std::unexpected(attrs_bytes.error());
  }
  auto attrs = AddrAttributes::from_cbor(*attrs_bytes);
  if (!attrs) {
    return std::unexpected(attrs.error());
  }
  return AddressContent(std::move(*id), std::move(*attrs), static_cast<ByronAddrType>(*kind));
}

core::Result<AddressContent> AddressContent::create(AddressId address_id, AddrAttributes attributes,
                                                    ByronAddrType kind) {
  return AddressContent(std::move(address_id), std::move(attributes), kind);
}

core::Result<AddressContent> AddressContent::hash_and_create(ByronAddrType kind,
                                                             const SpendingData& spending_data,
                                                             const AddrAttributes& attributes) {
  auto id = make_address_id(kind, spending_data, attributes);
  return id ? core::Result<AddressContent>(AddressContent(*id, attributes, kind))
            : std::unexpected(id.error());
}

core::Result<AddressContent> AddressContent::redeem(
    const crypto::PublicKey& public_key, std::optional<core::ProtocolMagic> protocol_magic) {
  return hash_and_create(ByronAddrType::redeem, SpendingData::redeem(public_key),
                         AddrAttributes::bootstrap_era(std::nullopt, protocol_magic));
}

core::Result<AddressContent> AddressContent::simple(
    const crypto::Bip32PublicKey& public_key, std::optional<core::ProtocolMagic> protocol_magic) {
  return hash_and_create(ByronAddrType::public_key, SpendingData::public_key(public_key),
                         AddrAttributes::bootstrap_era(std::nullopt, protocol_magic));
}

core::Result<AddressContent> AddressContent::icarus_from_key(
    const crypto::Bip32PublicKey& public_key, core::ProtocolMagic protocol_magic) {
  return simple(public_key, protocol_magic.value() == core::BYRON_MAINNET_NETWORK_MAGIC
                                ? std::nullopt
                                : std::optional<core::ProtocolMagic>(protocol_magic));
}

const AddressId& AddressContent::address_id() const noexcept { return address_id_; }
const AddrAttributes& AddressContent::attributes() const noexcept { return attributes_; }
ByronAddrType AddressContent::type() const noexcept { return kind_; }
core::ProtocolMagic AddressContent::protocol_magic() const noexcept {
  return attributes_.protocol_magic().value_or(
      core::ProtocolMagic(core::BYRON_MAINNET_NETWORK_MAGIC));
}
std::optional<std::uint8_t> AddressContent::network_id() const noexcept {
  const auto magic = protocol_magic().value();
  if (magic == core::BYRON_MAINNET_NETWORK_MAGIC) {
    return 1;
  }
  if (magic == core::BYRON_TESTNET_NETWORK_MAGIC || magic == core::PREPROD_NETWORK_MAGIC ||
      magic == core::PREVIEW_NETWORK_MAGIC || magic == core::SANCHO_TESTNET_NETWORK_MAGIC) {
    return 0;
  }
  return std::nullopt;
}

core::Result<bool> AddressContent::identical_with(const crypto::Bip32PublicKey& public_key) const {
  auto candidate =
      hash_and_create(ByronAddrType::public_key, SpendingData::public_key(public_key), attributes_);
  return candidate ? core::Result<bool>(*candidate == *this) : std::unexpected(candidate.error());
}

core::Result<core::Bytes> AddressContent::to_cbor() const {
  auto attrs = attributes_.to_cbor();
  if (!attrs) {
    return std::unexpected(attrs.error());
  }
  auto attrs_node = core::cbor::decode_cbor(*attrs);
  if (!attrs_node) {
    return std::unexpected(attrs_node.error());
  }
  return core::cbor::encode_cbor(core::cbor::Value::array({
      cbor_bytes(address_id_.span()),
      std::move(*attrs_node),
      cbor_uint(static_cast<std::uint8_t>(kind_)),
  }));
}

void Crc32::update(core::ByteSpan bytes_value) noexcept {
  for (const auto byte : bytes_value) {
    state_ ^= std::to_integer<std::uint8_t>(byte);
    for (unsigned bit = 0; bit < 8; ++bit) {
      state_ = (state_ >> 1U) ^ (0xedb88320U & (0U - (state_ & 1U)));
    }
  }
}
std::uint32_t Crc32::finalize() const noexcept { return ~state_; }

core::Result<ByronAddress> ByronAddress::create(const AddressContent& content,
                                                std::uint32_t checksum_value) {
  auto content_bytes = content.to_cbor();
  if (!content_bytes) {
    return std::unexpected(content_bytes.error());
  }
  if (crc32(*content_bytes) != checksum_value) {
    return std::unexpected(core::CardanoError(core::ErrorCode::checksum_mismatch,
                                              "Byron address CRC32 does not match content"));
  }
  auto envelope = core::cbor::encode_cbor(core::cbor::Value::array({
      core::cbor::Value::tag(core::BigInteger(std::uint64_t{24}), cbor_bytes(*content_bytes)),
      core::cbor::Value::unsigned_integer(
          core::BigInteger(static_cast<std::uint64_t>(checksum_value)),
          core::cbor::HeadWidth::four),
  }));
  return envelope ? core::Result<ByronAddress>(ByronAddress(
                        std::move(*envelope), content.protocol_magic().value(), checksum_value))
                  : std::unexpected(envelope.error());
}

core::Result<ByronAddress> ByronAddress::from_content(const AddressContent& content) {
  auto content_bytes = content.to_cbor();
  return content_bytes ? create(content, crc32(*content_bytes))
                       : std::unexpected(content_bytes.error());
}

core::Result<AddressContent> ByronAddress::content() const {
  auto outer = core::cbor::decode_cbor(envelope_);
  if (!outer) {
    return std::unexpected(outer.error());
  }
  const auto* array = outer->as_array();
  if (array == nullptr || array->values.empty() || array->values[0].as_tag() == nullptr ||
      array->values[0].as_tag()->value == nullptr ||
      array->values[0].as_tag()->value->as_byte_string() == nullptr) {
    return std::unexpected(invalid("invalid Byron address envelope"));
  }
  return AddressContent::from_cbor(array->values[0].as_tag()->value->as_byte_string()->value);
}

}  // namespace cardano::chain
