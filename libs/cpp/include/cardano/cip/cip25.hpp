#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cardano/core/bytes.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/core/error.hpp"
#include "cardano/crypto/fixed_bytes.hpp"

namespace cardano::cip::cip25 {

inline constexpr std::uint64_t METADATA_LABEL = 721;

enum class Version { v1, v2 };

class String64 {
 public:
  [[nodiscard]] static core::Result<String64> from_string(std::string value);
  [[nodiscard]] const std::string& value() const noexcept;
  friend bool operator==(const String64&, const String64&) = default;

 private:
  explicit String64(std::string value);
  std::string value_;
};

enum class ChunkableStringKind { single, chunked };

class ChunkableString {
 public:
  [[nodiscard]] static core::Result<ChunkableString> single(std::string value);
  [[nodiscard]] static core::Result<ChunkableString> chunked(std::vector<std::string> chunks);
  [[nodiscard]] static core::Result<ChunkableString> from_string(std::string_view value);

  [[nodiscard]] ChunkableStringKind kind() const noexcept;
  [[nodiscard]] const std::vector<String64>& chunks() const noexcept;
  [[nodiscard]] std::string joined() const;
  friend bool operator==(const ChunkableString&, const ChunkableString&) = default;

 private:
  ChunkableString(ChunkableStringKind kind, std::vector<String64> chunks);
  ChunkableStringKind kind_;
  std::vector<String64> chunks_;
};

struct FileDetails {
  String64 name;
  String64 media_type;
  ChunkableString src;
  friend bool operator==(const FileDetails&, const FileDetails&) = default;
};

struct MetadataDetails {
  String64 name;
  ChunkableString image;
  std::optional<std::vector<FileDetails>> files;
  std::optional<String64> media_type;
  std::optional<ChunkableString> description;
  friend bool operator==(const MetadataDetails&, const MetadataDetails&) = default;
};

using PolicyId = crypto::ScriptHash;

class AssetName {
 public:
  [[nodiscard]] static core::Result<AssetName> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<AssetName> from_hex(std::string_view hex);
  [[nodiscard]] core::Bytes to_bytes() const;
  [[nodiscard]] std::string to_hex() const;
  friend bool operator==(const AssetName&, const AssetName&) = default;
  friend auto operator<=>(const AssetName&, const AssetName&) = default;

 private:
  explicit AssetName(core::Bytes bytes);
  core::Bytes bytes_;
};

using Assets = std::vector<std::pair<AssetName, MetadataDetails>>;
using Nfts = std::vector<std::pair<PolicyId, Assets>>;

struct MiniMetadataDetails {
  std::optional<std::string> name;
  std::optional<ChunkableString> image;
  friend bool operator==(const MiniMetadataDetails&, const MiniMetadataDetails&) = default;
};

class LabelMetadata {
 public:
  LabelMetadata(Version version, Nfts nfts);

  [[nodiscard]] static core::Result<LabelMetadata> from_cbor_value(const core::cbor::Value& value);
  [[nodiscard]] static core::Result<LabelMetadata> from_json(std::string_view json);

  [[nodiscard]] Version version() const noexcept;
  [[nodiscard]] const Nfts& nfts() const noexcept;
  [[nodiscard]] core::Result<core::cbor::Value> to_cbor_value() const;
  [[nodiscard]] std::string to_json() const;

  friend bool operator==(const LabelMetadata&, const LabelMetadata&) = default;

 private:
  Version version_;
  Nfts nfts_;
};

class Metadata {
 public:
  explicit Metadata(LabelMetadata label);

  [[nodiscard]] static core::Result<Metadata> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<Metadata> from_cbor_value(const core::cbor::Value& value);
  [[nodiscard]] static core::Result<Metadata> from_json(std::string_view json);

  [[nodiscard]] const LabelMetadata& label() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_bytes() const;
  [[nodiscard]] core::Result<core::cbor::Value> add_to_metadata(
      const core::cbor::Value& metadata) const;
  [[nodiscard]] std::string to_json() const;

 private:
  LabelMetadata label_;
};

[[nodiscard]] core::Result<MiniMetadataDetails> parse_mini_metadata(const core::cbor::Value& value);

using CIP25ChunkableString = ChunkableString;
using CIP25ChunkableStringKind = ChunkableStringKind;
using CIP25FilesDetails = FileDetails;
using CIP25FilesDetailsList = std::vector<FileDetails>;
using CIP25LabelMetadata = LabelMetadata;
using CIP25Metadata = Metadata;
using CIP25MetadataDetails = MetadataDetails;
using CIP25MiniMetadataDetails = MiniMetadataDetails;
using CIP25String64 = String64;
using CIP25String64List = std::vector<String64>;
using CIP25Version = Version;

}  // namespace cardano::cip::cip25
