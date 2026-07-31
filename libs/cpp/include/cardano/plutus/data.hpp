#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cardano/chain/plutus_data.hpp"

namespace cardano::plutus {

class Data;

struct Constr {
  core::BigInteger alternative;
  std::vector<Data> fields;
};

class Data {
 public:
  [[nodiscard]] static Data constr(core::BigInteger alternative, std::vector<Data> fields);
  [[nodiscard]] static Data integer(core::BigInteger value);
  [[nodiscard]] static Data bytes(core::Bytes value);
  [[nodiscard]] static Data list(std::vector<Data> values);
  [[nodiscard]] static Data map(std::vector<std::pair<Data, Data>> entries);

  [[nodiscard]] static core::Result<Data> from_cbor(core::ByteSpan bytes,
                                                    core::cbor::DecodeOptions options = {});
  [[nodiscard]] static core::Result<Data> from_cbor_hex(std::string_view hex,
                                                        core::cbor::DecodeOptions options = {});
  [[nodiscard]] static core::Result<Data> from_json(std::string_view json,
                                                    std::size_t max_depth = 128U,
                                                    std::size_t max_values = 100'000U);
  [[nodiscard]] static Data from_plutus_data(chain::PlutusData value);

  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::canonical) const;
  [[nodiscard]] core::Result<std::string> to_cbor_hex(
      core::cbor::Mode mode = core::cbor::Mode::canonical) const;
  [[nodiscard]] core::Result<std::string> to_json(std::size_t max_depth = 128U,
                                                  std::size_t max_values = 100'000U) const;
  [[nodiscard]] const chain::PlutusData& to_plutus_data() const noexcept;

  friend bool operator==(const Data&, const Data&) = default;

 private:
  explicit Data(chain::PlutusData value);
  chain::PlutusData value_;
};

enum class SchemaKind { any, integer, bytes, boolean, list, tuple, map, nullable, constructor };

class DataSchema {
 public:
  [[nodiscard]] static DataSchema any();
  [[nodiscard]] static DataSchema integer(std::optional<core::BigInteger> minimum = std::nullopt,
                                          std::optional<core::BigInteger> maximum = std::nullopt);
  [[nodiscard]] static DataSchema bytes(std::size_t minimum = 0U,
                                        std::optional<std::size_t> maximum = std::nullopt);
  [[nodiscard]] static DataSchema boolean();
  [[nodiscard]] static DataSchema list(DataSchema item, std::size_t minimum = 0U,
                                       std::optional<std::size_t> maximum = std::nullopt);
  [[nodiscard]] static DataSchema tuple(std::vector<DataSchema> items);
  [[nodiscard]] static DataSchema map(DataSchema key, DataSchema value);
  [[nodiscard]] static DataSchema nullable(DataSchema item);
  [[nodiscard]] static DataSchema constructor(core::BigInteger alternative,
                                              std::vector<DataSchema> fields);

  [[nodiscard]] SchemaKind kind() const noexcept;
  [[nodiscard]] core::VoidResult validate(const Data& data, std::size_t max_depth = 128U,
                                          std::size_t max_values = 100'000U) const;

 private:
  struct State;
  explicit DataSchema(std::shared_ptr<const State> state);
  std::shared_ptr<const State> state_;
};

using AnySchema = DataSchema;
using ArraySchema = DataSchema;
using BooleanSchema = DataSchema;
using BytesSchema = DataSchema;
using DataSchemaType = DataSchema;
using EnumItemSchema = DataSchema;
using EnumSchema = DataSchema;
using IntegerSchema = DataSchema;
using LiteralSchema = DataSchema;
using MapSchema = DataSchema;
using NullableSchema = DataSchema;
using ObjectSchema = DataSchema;
using TupleSchema = DataSchema;
using Exact = DataSchema;
using Json = std::string;
using PlutusDataValue = chain::PlutusData;
using Datum = Data;
using Redeemer = Data;
using SchemaProperties = std::vector<std::pair<std::string, DataSchema>>;
using StaticProperties = SchemaProperties;
using StaticSchema = DataSchema;

}  // namespace cardano::plutus
