#include "cardano/cip/cip25.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <utility>

namespace cardano::cip::cip25 {
namespace {

using core::BigInteger;
using core::CardanoError;
using core::ErrorCode;
using core::Result;
using core::cbor::ArrayValue;
using core::cbor::MapValue;
using core::cbor::Value;
using Json = nlohmann::json;

CardanoError structure_error(std::string message) {
  return CardanoError(ErrorCode::invalid_structure, std::move(message));
}

Result<const MapValue*> require_map(const Value& value, std::string_view name) {
  const auto* map = value.as_map();
  if (map == nullptr) {
    return std::unexpected(structure_error(std::string(name) + " must be a CBOR map"));
  }
  return map;
}

Result<const ArrayValue*> require_array(const Value& value, std::string_view name) {
  const auto* array = value.as_array();
  if (array == nullptr) {
    return std::unexpected(structure_error(std::string(name) + " must be a CBOR array"));
  }
  return array;
}

Result<String64> parse_string64(const Value& value, std::string_view name) {
  const auto* text = value.as_text_string();
  if (text == nullptr) {
    return std::unexpected(structure_error(std::string(name) + " must be CBOR text"));
  }
  return String64::from_string(text->value);
}

Result<ChunkableString> parse_chunkable(const Value& value, std::string_view name) {
  if (const auto* text = value.as_text_string(); text != nullptr) {
    return ChunkableString::single(text->value);
  }
  auto array = require_array(value, name);
  if (!array) {
    return std::unexpected(array.error());
  }
  std::vector<std::string> chunks;
  chunks.reserve((*array)->values.size());
  for (const auto& item : (*array)->values) {
    const auto* text = item.as_text_string();
    if (text == nullptr) {
      return std::unexpected(structure_error(std::string(name) + " chunks must be CBOR text"));
    }
    chunks.push_back(text->value);
  }
  return ChunkableString::chunked(std::move(chunks));
}

Value encode_string64(const String64& value) { return Value::text_string(value.value()); }

Value encode_chunkable(const ChunkableString& value) {
  if (value.kind() == ChunkableStringKind::single) {
    return Value::text_string(value.chunks().front().value());
  }
  std::vector<Value> chunks;
  chunks.reserve(value.chunks().size());
  for (const auto& chunk : value.chunks()) {
    chunks.push_back(encode_string64(chunk));
  }
  return Value::array(std::move(chunks));
}

Result<std::map<std::string, const Value*>> named_map(const Value& value, std::string_view name) {
  auto map = require_map(value, name);
  if (!map) {
    return std::unexpected(map.error());
  }
  std::map<std::string, const Value*> fields;
  for (const auto& [key, item] : (*map)->entries) {
    const auto* text = key.as_text_string();
    if (text == nullptr) {
      continue;
    }
    if (!fields.emplace(text->value, &item).second) {
      return std::unexpected(CardanoError(ErrorCode::duplicate_key,
                                          std::string(name) + " contains a duplicate text key"));
    }
  }
  return fields;
}

Result<FileDetails> parse_file(const Value& value) {
  auto fields = named_map(value, "CIP-25 file");
  if (!fields) {
    return std::unexpected(fields.error());
  }
  for (const auto required : {"name", "mediaType", "src"}) {
    if (!fields->contains(required)) {
      return std::unexpected(structure_error(std::string("CIP-25 file is missing ") + required));
    }
  }
  auto name = parse_string64(*fields->at("name"), "file name");
  auto media = parse_string64(*fields->at("mediaType"), "file mediaType");
  auto src = parse_chunkable(*fields->at("src"), "file src");
  if (!name) return std::unexpected(name.error());
  if (!media) return std::unexpected(media.error());
  if (!src) return std::unexpected(src.error());
  return FileDetails{std::move(*name), std::move(*media), std::move(*src)};
}

Result<MetadataDetails> parse_details(const Value& value) {
  auto fields = named_map(value, "CIP-25 details");
  if (!fields) {
    return std::unexpected(fields.error());
  }
  if (!fields->contains("name") || !fields->contains("image")) {
    return std::unexpected(structure_error("CIP-25 details require name and image"));
  }
  auto name = parse_string64(*fields->at("name"), "asset name");
  auto image = parse_chunkable(*fields->at("image"), "asset image");
  if (!name) return std::unexpected(name.error());
  if (!image) return std::unexpected(image.error());

  MetadataDetails details{std::move(*name), std::move(*image), std::nullopt, std::nullopt,
                          std::nullopt};
  if (fields->contains("files")) {
    auto array = require_array(*fields->at("files"), "CIP-25 files");
    if (!array) return std::unexpected(array.error());
    std::vector<FileDetails> files;
    files.reserve((*array)->values.size());
    for (const auto& item : (*array)->values) {
      auto file = parse_file(item);
      if (!file) return std::unexpected(file.error());
      files.push_back(std::move(*file));
    }
    details.files = std::move(files);
  }
  if (fields->contains("mediaType")) {
    auto media = parse_string64(*fields->at("mediaType"), "asset mediaType");
    if (!media) return std::unexpected(media.error());
    details.media_type = std::move(*media);
  }
  if (fields->contains("description")) {
    auto description = parse_chunkable(*fields->at("description"), "asset description");
    if (!description) return std::unexpected(description.error());
    details.description = std::move(*description);
  }
  return details;
}

Value encode_file(const FileDetails& file) {
  return Value::map({
      {Value::text_string("name"), encode_string64(file.name)},
      {Value::text_string("mediaType"), encode_string64(file.media_type)},
      {Value::text_string("src"), encode_chunkable(file.src)},
  });
}

Value encode_details(const MetadataDetails& details) {
  std::vector<std::pair<Value, Value>> entries;
  entries.emplace_back(Value::text_string("name"), encode_string64(details.name));
  if (details.files) {
    std::vector<Value> files;
    files.reserve(details.files->size());
    for (const auto& file : *details.files) files.push_back(encode_file(file));
    entries.emplace_back(Value::text_string("files"), Value::array(std::move(files)));
  }
  entries.emplace_back(Value::text_string("image"), encode_chunkable(details.image));
  if (details.media_type) {
    entries.emplace_back(Value::text_string("mediaType"), encode_string64(*details.media_type));
  }
  if (details.description) {
    entries.emplace_back(Value::text_string("description"), encode_chunkable(*details.description));
  }
  return Value::map(std::move(entries));
}

Result<Nfts> parse_v1(const Value& value) {
  auto policies = require_map(value, "CIP-25 V1 label");
  if (!policies) return std::unexpected(policies.error());
  Nfts output;
  for (const auto& [policy_key, assets_value] : (*policies)->entries) {
    const auto* policy_text = policy_key.as_text_string();
    if (policy_text == nullptr) continue;
    auto policy = PolicyId::from_hex(policy_text->value);
    if (!policy) return std::unexpected(policy.error());
    auto assets_map = require_map(assets_value, "CIP-25 V1 assets");
    if (!assets_map) return std::unexpected(assets_map.error());
    Assets assets;
    for (const auto& [asset_key, details_value] : (*assets_map)->entries) {
      const auto* asset_text = asset_key.as_text_string();
      if (asset_text == nullptr) continue;
      const auto bytes = core::ByteSpan(
          reinterpret_cast<const core::Byte*>(asset_text->value.data()), asset_text->value.size());
      auto asset = AssetName::from_bytes(bytes);
      auto details = parse_details(details_value);
      if (!asset) return std::unexpected(asset.error());
      if (!details) return std::unexpected(details.error());
      assets.emplace_back(std::move(*asset), std::move(*details));
    }
    output.emplace_back(std::move(*policy), std::move(assets));
  }
  return output;
}

Result<Nfts> parse_v2(const Value& value) {
  auto fields = named_map(value, "CIP-25 V2 label");
  if (!fields) return std::unexpected(fields.error());
  if (!fields->contains("data") || !fields->contains("version")) {
    return std::unexpected(structure_error("CIP-25 V2 label requires data and version"));
  }
  const auto* version = fields->at("version")->as_unsigned();
  if (version == nullptr || version->value != BigInteger(std::uint64_t{2})) {
    return std::unexpected(structure_error("CIP-25 V2 version must equal 2"));
  }
  auto policies = require_map(*fields->at("data"), "CIP-25 V2 data");
  if (!policies) return std::unexpected(policies.error());
  Nfts output;
  for (const auto& [policy_key, assets_value] : (*policies)->entries) {
    const auto* policy_bytes = policy_key.as_byte_string();
    if (policy_bytes == nullptr) continue;
    auto policy = PolicyId::from_bytes(policy_bytes->value);
    if (!policy) return std::unexpected(policy.error());
    auto assets_map = require_map(assets_value, "CIP-25 V2 assets");
    if (!assets_map) return std::unexpected(assets_map.error());
    Assets assets;
    for (const auto& [asset_key, details_value] : (*assets_map)->entries) {
      const auto* asset_bytes = asset_key.as_byte_string();
      if (asset_bytes == nullptr) continue;
      auto asset = AssetName::from_bytes(asset_bytes->value);
      auto details = parse_details(details_value);
      if (!asset) return std::unexpected(asset.error());
      if (!details) return std::unexpected(details.error());
      assets.emplace_back(std::move(*asset), std::move(*details));
    }
    output.emplace_back(std::move(*policy), std::move(assets));
  }
  return output;
}

Json chunkable_json(const ChunkableString& value) {
  if (value.kind() == ChunkableStringKind::single) {
    return Json{{"Single", value.chunks().front().value()}};
  }
  Json chunks = Json::array();
  for (const auto& chunk : value.chunks()) chunks.push_back(chunk.value());
  return Json{{"Chunked", std::move(chunks)}};
}

Result<ChunkableString> chunkable_from_json(const Json& json) {
  if (json.is_string()) return ChunkableString::single(json.get<std::string>());
  if (!json.is_object()) {
    return std::unexpected(structure_error("chunkable JSON must be a string or object"));
  }
  if (json.contains("Single") && json.at("Single").is_string()) {
    return ChunkableString::single(json.at("Single").get<std::string>());
  }
  if (json.contains("Chunked") && json.at("Chunked").is_array()) {
    std::vector<std::string> chunks;
    for (const auto& item : json.at("Chunked")) {
      if (!item.is_string()) {
        return std::unexpected(structure_error("Chunked entries must be strings"));
      }
      chunks.push_back(item.get<std::string>());
    }
    return ChunkableString::chunked(std::move(chunks));
  }
  return std::unexpected(structure_error("invalid chunkable JSON object"));
}

std::string json_string_coercion(const Json& value) {
  if (value.is_string()) return value.get<std::string>();
  if (value.is_null()) return "null";
  if (value.is_boolean()) return value.get<bool>() ? "true" : "false";
  if (value.is_number()) return value.dump();
  return value.dump();
}

Result<MetadataDetails> details_from_json(const Json& json) {
  if (!json.is_object() || !json.contains("name") || !json.contains("image")) {
    return std::unexpected(structure_error("CIP-25 details JSON requires name and image"));
  }
  auto name = String64::from_string(json_string_coercion(json.at("name")));
  auto image = chunkable_from_json(json.at("image"));
  if (!name) return std::unexpected(name.error());
  if (!image) return std::unexpected(image.error());
  MetadataDetails details{std::move(*name), std::move(*image), std::nullopt, std::nullopt,
                          std::nullopt};
  if (json.contains("media_type") && json.at("media_type").is_string()) {
    auto media = String64::from_string(json.at("media_type").get<std::string>());
    if (!media) return std::unexpected(media.error());
    details.media_type = std::move(*media);
  }
  if (json.contains("description") &&
      (json.at("description").is_string() || json.at("description").is_object())) {
    auto description = chunkable_from_json(json.at("description"));
    if (!description) return std::unexpected(description.error());
    details.description = std::move(*description);
  }
  if (json.contains("files") && json.at("files").is_array()) {
    std::vector<FileDetails> files;
    for (const auto& item : json.at("files")) {
      if (!item.is_object() || !item.contains("name") || !item.contains("src") ||
          (!item.contains("media_type") && !item.contains("mediaType"))) {
        return std::unexpected(structure_error("invalid CIP-25 file JSON"));
      }
      const auto& media_json =
          item.contains("media_type") ? item.at("media_type") : item.at("mediaType");
      auto file_name = String64::from_string(json_string_coercion(item.at("name")));
      auto media = String64::from_string(json_string_coercion(media_json));
      auto src = chunkable_from_json(item.at("src"));
      if (!file_name) return std::unexpected(file_name.error());
      if (!media) return std::unexpected(media.error());
      if (!src) return std::unexpected(src.error());
      files.push_back({std::move(*file_name), std::move(*media), std::move(*src)});
    }
    details.files = std::move(files);
  }
  return details;
}

Json details_json(const MetadataDetails& details) {
  Json output{
      {"name", details.name.value()},
      {"image", chunkable_json(details.image)},
  };
  if (details.media_type) output["media_type"] = details.media_type->value();
  if (details.description) output["description"] = chunkable_json(*details.description);
  if (details.files) {
    output["files"] = Json::array();
    for (const auto& file : *details.files) {
      output["files"].push_back({
          {"name", file.name.value()},
          {"media_type", file.media_type.value()},
          {"src", chunkable_json(file.src)},
      });
    }
  }
  return output;
}

}  // namespace

String64::String64(std::string value) : value_(std::move(value)) {}

Result<String64> String64::from_string(std::string value) {
  const auto bytes =
      core::ByteSpan(reinterpret_cast<const core::Byte*>(value.data()), value.size());
  if (!core::is_valid_utf8(bytes)) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_utf8, "CIP-25 string is not valid UTF-8"));
  }
  if (value.size() > 64) {
    return std::unexpected(
        CardanoError(ErrorCode::out_of_range, "CIP-25 string exceeds 64 UTF-8 bytes"));
  }
  return String64(std::move(value));
}

const std::string& String64::value() const noexcept { return value_; }

ChunkableString::ChunkableString(ChunkableStringKind kind, std::vector<String64> chunks)
    : kind_(kind), chunks_(std::move(chunks)) {}

Result<ChunkableString> ChunkableString::single(std::string value) {
  auto string = String64::from_string(std::move(value));
  if (!string) return std::unexpected(string.error());
  std::vector<String64> chunks;
  chunks.push_back(std::move(*string));
  return ChunkableString(ChunkableStringKind::single, std::move(chunks));
}

Result<ChunkableString> ChunkableString::chunked(std::vector<std::string> values) {
  std::vector<String64> chunks;
  chunks.reserve(values.size());
  for (auto& value : values) {
    auto chunk = String64::from_string(std::move(value));
    if (!chunk) return std::unexpected(chunk.error());
    chunks.push_back(std::move(*chunk));
  }
  return ChunkableString(ChunkableStringKind::chunked, std::move(chunks));
}

Result<ChunkableString> ChunkableString::from_string(std::string_view value) {
  if (value.size() <= 64) return single(std::string(value));
  std::vector<std::string> chunks;
  for (std::size_t offset = 0; offset < value.size(); offset += 64) {
    const auto length = std::min<std::size_t>(64, value.size() - offset);
    std::string chunk(value.substr(offset, length));
    const auto bytes =
        core::ByteSpan(reinterpret_cast<const core::Byte*>(chunk.data()), chunk.size());
    if (!core::is_valid_utf8(bytes)) {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_utf8, "CIP-25 chunk boundary splits a UTF-8 code point"));
    }
    chunks.push_back(std::move(chunk));
  }
  return chunked(std::move(chunks));
}

ChunkableStringKind ChunkableString::kind() const noexcept { return kind_; }
const std::vector<String64>& ChunkableString::chunks() const noexcept { return chunks_; }

std::string ChunkableString::joined() const {
  std::string output;
  for (const auto& chunk : chunks_) output += chunk.value();
  return output;
}

AssetName::AssetName(core::Bytes bytes) : bytes_(std::move(bytes)) {}

Result<AssetName> AssetName::from_bytes(core::ByteSpan bytes) {
  if (bytes.size() > 32) {
    return std::unexpected(CardanoError(ErrorCode::out_of_range, "asset name exceeds 32 bytes"));
  }
  return AssetName(core::copy_bytes(bytes));
}

Result<AssetName> AssetName::from_hex(std::string_view hex) {
  auto bytes = core::hex_to_bytes(hex);
  if (!bytes) return std::unexpected(bytes.error());
  return from_bytes(*bytes);
}

core::Bytes AssetName::to_bytes() const { return bytes_; }
std::string AssetName::to_hex() const { return core::bytes_to_hex(bytes_); }

LabelMetadata::LabelMetadata(Version version, Nfts nfts)
    : version_(version), nfts_(std::move(nfts)) {}

Result<LabelMetadata> LabelMetadata::from_cbor_value(const Value& value) {
  auto v1 = parse_v1(value);
  if (v1) return LabelMetadata(Version::v1, std::move(*v1));
  auto v2 = parse_v2(value);
  if (!v2) {
    return std::unexpected(CardanoError(ErrorCode::invalid_structure,
                                        "CIP-25 label is neither valid V1 nor V2", {}, std::nullopt,
                                        std::make_shared<const CardanoError>(v2.error())));
  }
  return LabelMetadata(Version::v2, std::move(*v2));
}

Result<LabelMetadata> LabelMetadata::from_json(std::string_view json_text) {
  Json json;
  try {
    json = Json::parse(json_text);
  } catch (const std::exception& error) {
    return std::unexpected(CardanoError(ErrorCode::invalid_encoding, error.what()));
  }
  if (!json.is_object()) {
    return std::unexpected(structure_error("CIP-25 label JSON must be an object"));
  }
  const auto version =
      (json.contains("version") &&
       ((json.at("version").is_string() && json.at("version").get<std::string>() == "V2") ||
        (json.at("version").is_number_integer() && json.at("version").get<std::int64_t>() == 1)))
          ? Version::v2
          : Version::v1;
  Nfts nfts;
  if (json.contains("nfts")) {
    if (!json.at("nfts").is_object()) {
      return std::unexpected(structure_error("CIP-25 nfts JSON must be an object"));
    }
    for (const auto& [policy_hex, assets_json] : json.at("nfts").items()) {
      auto policy = PolicyId::from_hex(policy_hex);
      if (!policy) return std::unexpected(policy.error());
      if (!assets_json.is_object()) {
        return std::unexpected(structure_error("CIP-25 assets JSON must be an object"));
      }
      Assets assets;
      for (const auto& [asset_hex, detail_json] : assets_json.items()) {
        auto asset = AssetName::from_hex(asset_hex);
        auto details = details_from_json(detail_json);
        if (!asset) return std::unexpected(asset.error());
        if (!details) return std::unexpected(details.error());
        assets.emplace_back(std::move(*asset), std::move(*details));
      }
      nfts.emplace_back(std::move(*policy), std::move(assets));
    }
  }
  return LabelMetadata(version, std::move(nfts));
}

Version LabelMetadata::version() const noexcept { return version_; }
const Nfts& LabelMetadata::nfts() const noexcept { return nfts_; }

Result<Value> LabelMetadata::to_cbor_value() const {
  auto sorted = nfts_;
  std::ranges::sort(sorted, {}, [](const auto& item) { return item.first.to_hex(); });
  std::vector<std::pair<Value, Value>> policies;
  for (auto& [policy, assets_value] : sorted) {
    std::ranges::sort(assets_value, {}, [](const auto& item) { return item.first.to_hex(); });
    std::vector<std::pair<Value, Value>> assets;
    for (const auto& [asset, details] : assets_value) {
      const auto asset_bytes = asset.to_bytes();
      Value key = version_ == Version::v1
                      ? Value::text_string(std::string(
                            reinterpret_cast<const char*>(asset_bytes.data()), asset_bytes.size()))
                      : Value::byte_string(asset_bytes);
      assets.emplace_back(std::move(key), encode_details(details));
    }
    Value key = version_ == Version::v1 ? Value::text_string(policy.to_hex())
                                        : Value::byte_string(policy.to_bytes());
    policies.emplace_back(std::move(key), Value::map(std::move(assets)));
  }
  if (version_ == Version::v1) return Value::map(std::move(policies));
  return Value::map({
      {Value::text_string("data"), Value::map(std::move(policies))},
      {Value::text_string("version"), Value::unsigned_integer(BigInteger(std::uint64_t{2}))},
  });
}

std::string LabelMetadata::to_json() const {
  Json nfts = Json::object();
  for (const auto& [policy, assets] : nfts_) {
    auto& policy_json = nfts[policy.to_hex()];
    policy_json = Json::object();
    for (const auto& [asset, details] : assets) {
      policy_json[asset.to_hex()] = details_json(details);
    }
  }
  return Json{
      {"nfts", std::move(nfts)},
      {"version", version_ == Version::v1 ? "V1" : "V2"},
  }
      .dump();
}

Metadata::Metadata(LabelMetadata label) : label_(std::move(label)) {}

Result<Metadata> Metadata::from_bytes(core::ByteSpan bytes) {
  auto value = core::cbor::decode_cbor(bytes);
  if (!value) return std::unexpected(value.error());
  return from_cbor_value(*value);
}

Result<Metadata> Metadata::from_cbor_value(const Value& value) {
  auto map = require_map(value, "CIP-25 metadata");
  if (!map) return std::unexpected(map.error());
  for (const auto& [key, item] : (*map)->entries) {
    const auto* label = key.as_unsigned();
    if (label != nullptr && label->value == BigInteger(std::uint64_t{METADATA_LABEL})) {
      auto parsed = LabelMetadata::from_cbor_value(item);
      if (!parsed) return std::unexpected(parsed.error());
      return Metadata(std::move(*parsed));
    }
  }
  return std::unexpected(structure_error("CIP-25 metadata has no label 721"));
}

Result<Metadata> Metadata::from_json(std::string_view json_text) {
  Json json;
  try {
    json = Json::parse(json_text);
  } catch (const std::exception& error) {
    return std::unexpected(CardanoError(ErrorCode::invalid_encoding, error.what()));
  }
  if (!json.is_object() || !json.contains("key_721")) {
    return std::unexpected(structure_error("CIP-25 metadata JSON requires key_721"));
  }
  auto label = LabelMetadata::from_json(json.at("key_721").dump());
  if (!label) return std::unexpected(label.error());
  return Metadata(std::move(*label));
}

const LabelMetadata& Metadata::label() const noexcept { return label_; }

Result<core::Bytes> Metadata::to_bytes() const {
  auto label = label_.to_cbor_value();
  if (!label) return std::unexpected(label.error());
  return core::cbor::encode_cbor(
      Value::map({{Value::unsigned_integer(BigInteger(std::uint64_t{METADATA_LABEL})),
                   std::move(*label)}}),
      {.mode = core::cbor::Mode::canonical});
}

Result<Value> Metadata::add_to_metadata(const Value& metadata) const {
  auto map = require_map(metadata, "metadata");
  if (!map) return std::unexpected(map.error());
  auto label = label_.to_cbor_value();
  if (!label) return std::unexpected(label.error());
  std::vector<std::pair<Value, Value>> entries;
  entries.reserve((*map)->entries.size() + 1);
  bool replaced = false;
  for (const auto& [key, item] : (*map)->entries) {
    const auto* candidate = key.as_unsigned();
    if (candidate != nullptr && candidate->value == BigInteger(std::uint64_t{METADATA_LABEL})) {
      if (!replaced) {
        entries.emplace_back(key, *label);
        replaced = true;
      }
    } else {
      entries.emplace_back(key, item);
    }
  }
  if (!replaced) {
    entries.emplace_back(Value::unsigned_integer(BigInteger(std::uint64_t{METADATA_LABEL})),
                         *label);
  }
  return Value::map(std::move(entries));
}

std::string Metadata::to_json() const {
  return Json{{"key_721", Json::parse(label_.to_json())}}.dump();
}

Result<MiniMetadataDetails> parse_mini_metadata(const Value& value) {
  auto fields = named_map(value, "CIP-25 mini metadata");
  if (!fields) return std::unexpected(fields.error());
  MiniMetadataDetails output;
  for (const auto candidate : {"name", "Name", "title", "id"}) {
    if (fields->contains(candidate)) {
      if (const auto* text = fields->at(candidate)->as_text_string();
          text != nullptr && text->value.size() <= 64) {
        output.name = text->value;
      }
      break;
    }
  }
  if (fields->contains("image")) {
    auto image = parse_chunkable(*fields->at("image"), "mini image");
    if (image) output.image = std::move(*image);
  }
  return output;
}

}  // namespace cardano::cip::cip25
