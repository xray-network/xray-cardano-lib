#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "cardano/core/big_integer.hpp"
#include "cardano/core/bytes.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/core/error.hpp"

namespace cardano::chain {

class PlutusData;
struct ConstrPlutusData;
struct PlutusMap;

class PlutusData {
 public:
  using List = std::vector<PlutusData>;
  using Node = std::variant<std::shared_ptr<ConstrPlutusData>, std::shared_ptr<PlutusMap>,
                            std::shared_ptr<List>, core::BigInteger, core::Bytes>;

  [[nodiscard]] static PlutusData constr(core::BigInteger alternative, List fields);
  [[nodiscard]] static PlutusData map(std::vector<std::pair<PlutusData, PlutusData>> entries);
  [[nodiscard]] static PlutusData list(List values);
  [[nodiscard]] static PlutusData integer(core::BigInteger value);
  [[nodiscard]] static PlutusData bytes(core::Bytes value);

  [[nodiscard]] static core::Result<PlutusData> from_cbor(core::ByteSpan bytes,
                                                          core::cbor::DecodeOptions options = {});
  [[nodiscard]] static core::Result<PlutusData> from_cbor_value(const core::cbor::Value& value,
                                                                std::size_t max_depth = 512,
                                                                bool enforce_wire_limit = true);

  [[nodiscard]] core::Result<core::cbor::Value> to_cbor_value() const;
  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::canonical) const;
  [[nodiscard]] const Node& node() const noexcept;
  friend bool operator==(const PlutusData& left, const PlutusData& right);

 private:
  explicit PlutusData(Node node, std::shared_ptr<const core::cbor::Value> preserved = nullptr);
  Node node_;
  std::shared_ptr<const core::cbor::Value> preserved_;
};

struct ConstrPlutusData {
  core::BigInteger alternative;
  std::vector<PlutusData> fields;
  friend bool operator==(const ConstrPlutusData&, const ConstrPlutusData&) = default;
};

struct PlutusMap {
  std::vector<std::pair<PlutusData, PlutusData>> entries;
  friend bool operator==(const PlutusMap&, const PlutusMap&) = default;
};

[[nodiscard]] core::Result<PlutusData> validate_plutus_data_node(const core::cbor::Value& value,
                                                                 std::size_t max_depth = 512,
                                                                 bool enforce_wire_limit = true);

}  // namespace cardano::chain
