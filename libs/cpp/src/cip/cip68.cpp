#include "cardano/cip/cip68.hpp"

#include <set>
#include <string_view>

namespace cardano::cip::cip68 {
namespace c67 = experimental::cip67;
namespace {

[[nodiscard]] core::CardanoError metadata_error(std::string message) {
  return core::CardanoError(core::ErrorCode::invalid_structure, std::move(message));
}

[[nodiscard]] core::Bytes text_bytes(std::string_view text) {
  core::Bytes output;
  output.reserve(text.size());
  for (const auto character : text) output.push_back(static_cast<core::Byte>(character));
  return output;
}

[[nodiscard]] core::Result<TokenClass> classify(const chain::AssetName& name) {
  auto split = c67::split_asset_name(name);
  if (!split) return std::unexpected(split.error());
  switch (split->label.value()) {
    case 222:
      return TokenClass::nft;
    case 333:
      return TokenClass::ft;
    case 444:
      return TokenClass::rft;
    default:
      return std::unexpected(
          core::CardanoError(core::ErrorCode::invalid_argument, "unsupported CIP-68 token class"));
  }
}

struct DataBudget {
  Limits limits;
  std::size_t nodes{};
  std::size_t map_entries{};
  std::size_t bytes{};
};

[[nodiscard]] core::VoidResult check_limits(const chain::PlutusData& data, DataBudget& budget,
                                            std::size_t depth) {
  if (depth > budget.limits.max_depth || ++budget.nodes > budget.limits.max_nodes) {
    return std::unexpected(core::CardanoError(core::ErrorCode::resource_limit_exceeded,
                                              "CIP-68 Data depth/node limit exceeded"));
  }
  const auto& node = data.node();
  if (const auto* value = std::get_if<core::Bytes>(&node)) {
    if (value->size() > budget.limits.max_byte_string_bytes -
                            std::min(budget.bytes, budget.limits.max_byte_string_bytes)) {
      return std::unexpected(core::CardanoError(core::ErrorCode::resource_limit_exceeded,
                                                "CIP-68 byte-string limit exceeded"));
    }
    budget.bytes += value->size();
  } else if (const auto* list = std::get_if<std::shared_ptr<chain::PlutusData::List>>(&node)) {
    for (const auto& item : **list) {
      auto valid = check_limits(item, budget, depth + 1U);
      if (!valid) return valid;
    }
  } else if (const auto* map = std::get_if<std::shared_ptr<chain::PlutusMap>>(&node)) {
    if ((*map)->entries.size() > budget.limits.max_map_entries -
                                     std::min(budget.map_entries, budget.limits.max_map_entries)) {
      return std::unexpected(core::CardanoError(core::ErrorCode::resource_limit_exceeded,
                                                "CIP-68 map-entry limit exceeded"));
    }
    budget.map_entries += (*map)->entries.size();
    for (const auto& [key, value] : (*map)->entries) {
      auto key_valid = check_limits(key, budget, depth + 1U);
      if (!key_valid) return key_valid;
      auto value_valid = check_limits(value, budget, depth + 1U);
      if (!value_valid) return value_valid;
    }
  } else if (const auto* constructor =
                 std::get_if<std::shared_ptr<chain::ConstrPlutusData>>(&node)) {
    for (const auto& field : (*constructor)->fields) {
      auto valid = check_limits(field, budget, depth + 1U);
      if (!valid) return valid;
    }
  }
  return std::monostate{};
}

[[nodiscard]] const core::Bytes* as_bytes(const chain::PlutusData& data) {
  return std::get_if<core::Bytes>(&data.node());
}

[[nodiscard]] const chain::PlutusMap* as_map(const chain::PlutusData& data) {
  const auto* pointer = std::get_if<std::shared_ptr<chain::PlutusMap>>(&data.node());
  return pointer == nullptr ? nullptr : pointer->get();
}

[[nodiscard]] const chain::PlutusData::List* as_list(const chain::PlutusData& data) {
  const auto* pointer = std::get_if<std::shared_ptr<chain::PlutusData::List>>(&data.node());
  return pointer == nullptr ? nullptr : pointer->get();
}

[[nodiscard]] core::Result<const chain::PlutusData*> unique_map_value(const chain::PlutusData& data,
                                                                      core::ByteSpan key,
                                                                      std::string_view label) {
  const auto* map = as_map(data);
  if (map == nullptr)
    return std::unexpected(metadata_error(std::string(label) + " container must be a map"));
  const chain::PlutusData* result = nullptr;
  for (const auto& [candidate, value] : map->entries) {
    const auto* bytes = as_bytes(candidate);
    if (bytes == nullptr || !std::ranges::equal(*bytes, key)) continue;
    if (result != nullptr)
      return std::unexpected(metadata_error(std::string(label) + " path is duplicated"));
    result = &value;
  }
  if (result == nullptr)
    return std::unexpected(metadata_error(std::string(label) + " path is missing"));
  return result;
}

[[nodiscard]] core::Result<core::Bytes> uri_bytes(const chain::PlutusData& data,
                                                  std::uint64_t version) {
  if (const auto* bytes = as_bytes(data)) return *bytes;
  const auto* chunks = as_list(data);
  if (version < 3U || chunks == nullptr || chunks->empty()) {
    return std::unexpected(metadata_error("chunked CIP-68 URI requires version 3 or 4"));
  }
  core::Bytes output;
  for (const auto& chunk : *chunks) {
    const auto* bytes = as_bytes(chunk);
    if (bytes == nullptr) return std::unexpected(metadata_error("URI chunks must be byte strings"));
    output.insert(output.end(), bytes->begin(), bytes->end());
  }
  return output;
}

[[nodiscard]] core::VoidResult validate_uri(const chain::PlutusData& data, std::uint64_t version) {
  auto bytes = uri_bytes(data, version);
  if (!bytes) return std::unexpected(bytes.error());
  if (!core::is_valid_utf8(*bytes))
    return std::unexpected(metadata_error("CIP-68 URI is not UTF-8"));
  const std::string text(reinterpret_cast<const char*>(bytes->data()), bytes->size());
  const bool allowed = text.starts_with("https://") || text.starts_with("ipfs:") ||
                       text.starts_with("ar:") ||
                       (text.starts_with("data:") && text.find(',') > 5U);
  if (!allowed) return std::unexpected(metadata_error("unsupported or malformed CIP-68 URI"));
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_files(const chain::PlutusData& data,
                                              std::uint64_t version) {
  const auto* files = as_list(data);
  if (files == nullptr) return std::unexpected(metadata_error("files must be a list"));
  static const core::Bytes media_type = text_bytes("mediaType");
  static const core::Bytes source = text_bytes("src");
  static const core::Bytes name = text_bytes("name");
  for (const auto& file : *files) {
    auto media = unique_map_value(file, media_type, "file mediaType");
    auto src = unique_map_value(file, source, "file src");
    if (!media || !src) return std::unexpected(!media ? media.error() : src.error());
    if (as_bytes(**media) == nullptr)
      return std::unexpected(metadata_error("file mediaType must be bytes"));
    auto uri = validate_uri(**src, version);
    if (!uri) return uri;
    const auto* map = as_map(file);
    std::size_t names = 0U;
    for (const auto& [key, value] : map->entries)
      if (const auto* bytes = as_bytes(key); bytes != nullptr && *bytes == name) {
        ++names;
        if (as_bytes(value) == nullptr)
          return std::unexpected(metadata_error("file name must be bytes"));
      }
    if (names > 1U) return std::unexpected(metadata_error("duplicate file name"));
  }
  return std::monostate{};
}

[[nodiscard]] core::VoidResult validate_metadata(const chain::PlutusData& data,
                                                 TokenClass token_class, std::uint64_t version) {
  const auto* map = as_map(data);
  if (map == nullptr) return std::unexpected(metadata_error("CIP-68 metadata must be a map"));
  std::map<std::string, const chain::PlutusData*> known;
  const std::set<std::string> recognized{"name",   "image", "description", "files",
                                         "ticker", "url",   "decimals",    "logo"};
  for (const auto& [key, value] : map->entries) {
    const auto* bytes = as_bytes(key);
    if (bytes == nullptr || !core::is_valid_utf8(*bytes)) continue;
    const std::string text(reinterpret_cast<const char*>(bytes->data()), bytes->size());
    if (!recognized.contains(text)) continue;
    if (!known.emplace(text, &value).second)
      return std::unexpected(metadata_error("duplicate known metadata key: " + text));
  }
  const std::initializer_list<std::string_view> required =
      token_class == TokenClass::ft ? std::initializer_list<std::string_view>{"name", "description"}
                                    : std::initializer_list<std::string_view>{"name", "image"};
  for (const auto key : required)
    if (!known.contains(std::string(key)))
      return std::unexpected(metadata_error("missing required metadata key: " + std::string(key)));
  for (const auto* key : {"name", "description", "ticker"})
    if (known.contains(key) && as_bytes(*known[key]) == nullptr)
      return std::unexpected(metadata_error(std::string(key) + " must be bytes"));
  for (const auto* key : {"image", "url", "logo"})
    if (known.contains(key)) {
      auto uri = validate_uri(*known[key], version);
      if (!uri) return uri;
    }
  if (known.contains("decimals") &&
      !std::holds_alternative<core::BigInteger>(known["decimals"]->node()))
    return std::unexpected(metadata_error("decimals must be an integer"));
  if (known.contains("files")) {
    auto valid = validate_files(*known["files"], version);
    if (!valid) return valid;
  }
  return std::monostate{};
}

}  // namespace

core::Result<TokenIdentity> reference_identity(const TokenIdentity& user) {
  auto kind = classify(user.name);
  if (!kind) return std::unexpected(kind.error());
  auto split = c67::split_asset_name(user.name);
  auto name = c67::make_asset_name(c67::AssetNameLabel(reference_label), split->content);
  if (!name) return std::unexpected(name.error());
  return TokenIdentity{user.policy, *name};
}

core::Result<Relationship> validate_relationship(const TokenIdentity& user,
                                                 const TokenIdentity& reference,
                                                 std::uint64_t quantity, std::size_t count) {
  auto kind = classify(user.name);
  if (!kind) return std::unexpected(kind.error());
  auto expected = reference_identity(user);
  if (!expected || expected->policy != reference.policy || expected->name != reference.name ||
      quantity != 1U || count != 1U) {
    return std::unexpected(metadata_error("invalid CIP-68 reference relationship"));
  }
  return Relationship{user, reference, *kind};
}

Datum::Datum(chain::PlutusData data, chain::PlutusData metadata, std::uint64_t version,
             chain::PlutusData extra)
    : data_(std::move(data)),
      metadata_(std::move(metadata)),
      version_(version),
      extra_(std::move(extra)) {}

Datum Datum::make(chain::PlutusData metadata, std::uint64_t version, chain::PlutusData extra) {
  auto data = chain::PlutusData::constr(
      core::BigInteger(std::uint64_t{0}),
      {metadata, chain::PlutusData::integer(core::BigInteger(version)), extra});
  return Datum(std::move(data), std::move(metadata), version, std::move(extra));
}

core::Result<Datum> Datum::parse(chain::PlutusData data, TokenClass token_class_value) {
  const auto* constructor = std::get_if<std::shared_ptr<chain::ConstrPlutusData>>(&data.node());
  if (constructor == nullptr || (*constructor)->alternative != core::BigInteger(std::uint64_t{0}) ||
      (*constructor)->fields.size() != 3U) {
    return std::unexpected(metadata_error("CIP-68 datum must be constructor 0 with three fields"));
  }
  const auto* integer = std::get_if<core::BigInteger>(&(*constructor)->fields[1].node());
  if (integer == nullptr)
    return std::unexpected(metadata_error("CIP-68 version must be an integer"));
  auto version = core::as_uint64(*integer);
  if (!version || *version < 1U || *version > 4U ||
      (token_class_value == TokenClass::rft && *version < 2U)) {
    return std::unexpected(
        core::CardanoError(core::ErrorCode::invalid_argument, "unsupported CIP-68 class/version"));
  }
  auto metadata = (*constructor)->fields[0];
  auto extra = (*constructor)->fields[2];
  return Datum(std::move(data), std::move(metadata), *version, std::move(extra));
}

core::VoidResult Datum::validate(TokenClass token_class_value, const TokenIdentity& user,
                                 Limits limits) const {
  auto actual_class = classify(user.name);
  if (!actual_class || *actual_class != token_class_value) {
    return std::unexpected(metadata_error("CIP-68 user identity has the wrong class"));
  }
  if (token_class_value == TokenClass::rft && version_ < 2U) {
    return std::unexpected(metadata_error("RFT metadata starts at version 2"));
  }
  DataBudget budget{limits};
  auto bounded = check_limits(data_, budget, 0U);
  if (!bounded) return bounded;
  const chain::PlutusData* selected = &metadata_;
  static const core::Bytes label = text_bytes("721");
  if (version_ == 4U) {
    auto level1 = unique_map_value(metadata_, label, "721");
    if (!level1) return std::unexpected(level1.error());
    const auto policy = user.policy.to_bytes();
    auto level2 = unique_map_value(**level1, policy, "policy");
    if (!level2) return std::unexpected(level2.error());
    auto split = c67::split_asset_name(user.name);
    if (!split) return std::unexpected(split.error());
    auto level3 = unique_map_value(**level2, split->content, "asset content");
    if (!level3) return std::unexpected(level3.error());
    selected = *level3;
  } else if (as_map(metadata_) != nullptr) {
    for (const auto& [key, unused] : as_map(metadata_)->entries) {
      static_cast<void>(unused);
      const auto* bytes = as_bytes(key);
      if (bytes != nullptr && *bytes == label)
        return std::unexpected(metadata_error("direct metadata must not contain 721"));
    }
  }
  return validate_metadata(*selected, token_class_value, version_);
}

std::uint64_t Datum::version() const noexcept { return version_; }
MetadataFormat Datum::metadata_format() const noexcept {
  return version_ == 4U ? MetadataFormat::nested_721 : MetadataFormat::direct;
}
const chain::PlutusData& Datum::metadata() const noexcept { return metadata_; }
const chain::PlutusData& Datum::extra() const noexcept { return extra_; }
const chain::PlutusData& Datum::to_data() const noexcept { return data_; }

}  // namespace cardano::cip::cip68
