#include "cardano/plutus/uplc.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "cardano/core/cbor.hpp"
#include "cardano/crypto/primitives.hpp"

namespace cardano::plutus {
namespace {

[[nodiscard]] core::CardanoError uplc_error(core::ErrorCode code, std::string message) {
  return core::CardanoError(code, std::move(message));
}

[[nodiscard]] core::CardanoError decode_error(std::string message) {
  return uplc_error(core::ErrorCode::invalid_encoding, std::move(message));
}

constexpr std::array<std::string_view, 101> BUILTIN_NAMES{"addInteger",
                                                          "subtractInteger",
                                                          "multiplyInteger",
                                                          "divideInteger",
                                                          "quotientInteger",
                                                          "remainderInteger",
                                                          "modInteger",
                                                          "equalsInteger",
                                                          "lessThanInteger",
                                                          "lessThanEqualsInteger",
                                                          "appendByteString",
                                                          "consByteString",
                                                          "sliceByteString",
                                                          "lengthOfByteString",
                                                          "indexByteString",
                                                          "equalsByteString",
                                                          "lessThanByteString",
                                                          "lessThanEqualsByteString",
                                                          "sha2_256",
                                                          "sha3_256",
                                                          "blake2b_256",
                                                          "verifyEd25519Signature",
                                                          "appendString",
                                                          "equalsString",
                                                          "encodeUtf8",
                                                          "decodeUtf8",
                                                          "ifThenElse",
                                                          "chooseUnit",
                                                          "trace",
                                                          "fstPair",
                                                          "sndPair",
                                                          "chooseList",
                                                          "mkCons",
                                                          "headList",
                                                          "tailList",
                                                          "nullList",
                                                          "chooseData",
                                                          "constrData",
                                                          "mapData",
                                                          "listData",
                                                          "iData",
                                                          "bData",
                                                          "unConstrData",
                                                          "unMapData",
                                                          "unListData",
                                                          "unIData",
                                                          "unBData",
                                                          "equalsData",
                                                          "mkPairData",
                                                          "mkNilData",
                                                          "mkNilPairData",
                                                          "serialiseData",
                                                          "verifyEcdsaSecp256k1Signature",
                                                          "verifySchnorrSecp256k1Signature",
                                                          "bls12_381_G1_add",
                                                          "bls12_381_G1_neg",
                                                          "bls12_381_G1_scalarMul",
                                                          "bls12_381_G1_equal",
                                                          "bls12_381_G1_hashToGroup",
                                                          "bls12_381_G1_compress",
                                                          "bls12_381_G1_uncompress",
                                                          "bls12_381_G2_add",
                                                          "bls12_381_G2_neg",
                                                          "bls12_381_G2_scalarMul",
                                                          "bls12_381_G2_equal",
                                                          "bls12_381_G2_hashToGroup",
                                                          "bls12_381_G2_compress",
                                                          "bls12_381_G2_uncompress",
                                                          "bls12_381_millerLoop",
                                                          "bls12_381_mulMlResult",
                                                          "bls12_381_finalVerify",
                                                          "keccak_256",
                                                          "blake2b_224",
                                                          "integerToByteString",
                                                          "byteStringToInteger",
                                                          "andByteString",
                                                          "orByteString",
                                                          "xorByteString",
                                                          "complementByteString",
                                                          "readBit",
                                                          "writeBits",
                                                          "replicateByte",
                                                          "shiftByteString",
                                                          "rotateByteString",
                                                          "countSetBits",
                                                          "findFirstSetBit",
                                                          "ripemd_160",
                                                          "expModInteger",
                                                          "dropList",
                                                          "lengthOfArray",
                                                          "listToArray",
                                                          "indexArray",
                                                          "bls12_381_G1_multiScalarMul",
                                                          "bls12_381_G2_multiScalarMul",
                                                          "insertCoin",
                                                          "lookupCoin",
                                                          "unionValue",
                                                          "valueContains",
                                                          "valueData",
                                                          "unValueData",
                                                          "scaleValue"};

[[nodiscard]] bool same_type(const UplcType& type, const UplcConstant& constant) {
  return type == constant.type();
}

[[nodiscard]] core::VoidResult validate_collection(const UplcType& item_type,
                                                   const UplcConstant::Items& values) {
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (!same_type(item_type, values[index])) {
      return std::unexpected(uplc_error(core::ErrorCode::invalid_structure,
                                        "UPLC collection contains a value of the wrong type")
                                 .at(index));
    }
  }
  return std::monostate{};
}

[[nodiscard]] std::size_t integer_memory(const core::BigInteger& integer) {
  const auto magnitude = integer.is_negative() ? -integer : integer;
  const auto bytes = magnitude.to_unsigned_bytes_be().size();
  return std::max<std::size_t>(1U, (bytes + 7U) / 8U);
}

[[nodiscard]] std::size_t data_memory(const chain::PlutusData& root) {
  std::size_t result = 0;
  std::vector<const chain::PlutusData*> stack{&root};
  while (!stack.empty()) {
    const auto* current = stack.back();
    stack.pop_back();
    std::visit(
        [&](const auto& node) {
          using Node = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<Node, core::BigInteger>) {
            result += 4U + integer_memory(node);
          } else if constexpr (std::is_same_v<Node, core::Bytes>) {
            result += 4U + std::max<std::size_t>(1U, (node.size() + 7U) / 8U);
          } else if constexpr (std::is_same_v<Node, std::shared_ptr<chain::PlutusData::List>>) {
            result += 4U;
            for (const auto& item : *node) {
              stack.push_back(&item);
            }
          } else if constexpr (std::is_same_v<Node, std::shared_ptr<chain::PlutusMap>>) {
            result += 4U;
            for (const auto& [key, value] : node->entries) {
              stack.push_back(&key);
              stack.push_back(&value);
            }
          } else {
            result += 4U;
            for (const auto& field : node->fields) {
              stack.push_back(&field);
            }
          }
        },
        current->node());
  }
  return result;
}

}  // namespace

UplcType::UplcType(UplcTypeTag tag, std::vector<UplcType> arguments)
    : tag_(tag), arguments_(std::move(arguments)) {}

UplcType UplcType::primitive(UplcTypeTag tag) { return UplcType(tag, {}); }

UplcType UplcType::list(UplcType item) {
  return apply(primitive(UplcTypeTag::proto_list), std::move(item));
}

UplcType UplcType::array(UplcType item) {
  return apply(primitive(UplcTypeTag::proto_array), std::move(item));
}

UplcType UplcType::pair(UplcType first, UplcType second) {
  return apply(apply(primitive(UplcTypeTag::proto_pair), std::move(first)), std::move(second));
}

UplcType UplcType::apply(UplcType function, UplcType argument) {
  return UplcType(UplcTypeTag::apply, {std::move(function), std::move(argument)});
}

UplcTypeTag UplcType::tag() const noexcept { return tag_; }

const std::vector<UplcType>& UplcType::arguments() const noexcept { return arguments_; }

bool UplcType::is_value_type() const noexcept {
  if (tag_ != UplcTypeTag::apply) {
    return tag_ != UplcTypeTag::proto_list && tag_ != UplcTypeTag::proto_pair &&
           tag_ != UplcTypeTag::proto_array;
  }
  if (arguments_.size() != 2U || !arguments_[0].is_value_type() || !arguments_[1].is_value_type()) {
    return false;
  }
  const auto& function = arguments_[0];
  if (function.tag() == UplcTypeTag::proto_list || function.tag() == UplcTypeTag::proto_array) {
    return true;
  }
  return function.tag() == UplcTypeTag::apply && function.arguments().size() == 2U &&
         function.arguments()[0].tag() == UplcTypeTag::proto_pair;
}

UplcPair::UplcPair(UplcConstant first, UplcConstant second)
    : first_ptr(new UplcConstant(std::move(first))),
      second_ptr(new UplcConstant(std::move(second))) {}

UplcPair::UplcPair(const UplcPair& other)
    : first_ptr(new UplcConstant(*other.first_ptr)),
      second_ptr(new UplcConstant(*other.second_ptr)) {}

UplcPair::UplcPair(UplcPair&& other) noexcept
    : first_ptr(std::exchange(other.first_ptr, nullptr)),
      second_ptr(std::exchange(other.second_ptr, nullptr)) {}

UplcPair& UplcPair::operator=(const UplcPair& other) {
  if (this != &other) {
    UplcPair replacement(other);
    *this = std::move(replacement);
  }
  return *this;
}

UplcPair& UplcPair::operator=(UplcPair&& other) noexcept {
  if (this != &other) {
    delete first_ptr;
    delete second_ptr;
    first_ptr = std::exchange(other.first_ptr, nullptr);
    second_ptr = std::exchange(other.second_ptr, nullptr);
  }
  return *this;
}

UplcPair::~UplcPair() {
  delete first_ptr;
  delete second_ptr;
}

const UplcConstant& UplcPair::first() const noexcept { return *first_ptr; }
const UplcConstant& UplcPair::second() const noexcept { return *second_ptr; }

bool operator==(const UplcPair& left, const UplcPair& right) {
  return left.first() == right.first() && left.second() == right.second();
}

UplcConstant::UplcConstant(UplcType type, Value value)
    : type_(std::move(type)), value_(std::move(value)) {}

UplcConstant UplcConstant::integer(core::BigInteger value) {
  return UplcConstant(UplcType::primitive(UplcTypeTag::integer), std::move(value));
}

UplcConstant UplcConstant::bytes(core::Bytes value) {
  return UplcConstant(UplcType::primitive(UplcTypeTag::byte_string), std::move(value));
}

core::Result<UplcConstant> UplcConstant::string(std::string value) {
  const auto bytes = std::as_bytes(std::span(value));
  if (!core::is_valid_utf8(bytes)) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_utf8, "UPLC string constant is not valid UTF-8"));
  }
  return UplcConstant(UplcType::primitive(UplcTypeTag::string), std::move(value));
}

UplcConstant UplcConstant::unit() {
  return UplcConstant(UplcType::primitive(UplcTypeTag::unit), std::monostate{});
}

UplcConstant UplcConstant::boolean(bool value) {
  return UplcConstant(UplcType::primitive(UplcTypeTag::boolean), value);
}

core::Result<UplcConstant> UplcConstant::list(UplcType item_type, Items values) {
  auto valid = validate_collection(item_type, values);
  if (!valid) {
    return std::unexpected(valid.error());
  }
  return UplcConstant(UplcType::list(std::move(item_type)), std::move(values));
}

core::Result<UplcConstant> UplcConstant::array(UplcType item_type, Items values) {
  auto valid = validate_collection(item_type, values);
  if (!valid) {
    return std::unexpected(valid.error());
  }
  return UplcConstant(UplcType::array(std::move(item_type)), std::move(values));
}

core::Result<UplcConstant> UplcConstant::pair(UplcConstant first, UplcConstant second) {
  auto type = UplcType::pair(first.type(), second.type());
  return UplcConstant(std::move(type), UplcPair(std::move(first), std::move(second)));
}

UplcConstant UplcConstant::data(Data value) {
  return UplcConstant(UplcType::primitive(UplcTypeTag::data), std::move(value));
}

core::Result<UplcConstant> UplcConstant::bls12_381_g1(core::Bytes compressed) {
  auto point = crypto::bls12_381_uncompress(compressed);
  if (!point || point->group() != crypto::BlsGroup::g1) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_argument, "invalid compressed BLS12-381 G1 element"));
  }
  return UplcConstant(UplcType::primitive(UplcTypeTag::bls12_381_g1), std::move(compressed));
}

core::Result<UplcConstant> UplcConstant::bls12_381_g2(core::Bytes compressed) {
  auto point = crypto::bls12_381_uncompress(compressed);
  if (!point || point->group() != crypto::BlsGroup::g2) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_argument, "invalid compressed BLS12-381 G2 element"));
  }
  return UplcConstant(UplcType::primitive(UplcTypeTag::bls12_381_g2), std::move(compressed));
}

core::Result<UplcConstant> UplcConstant::bls12_381_ml_result(core::Bytes value) {
  if (value.empty()) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_length, "BLS12-381 ML result cannot be empty"));
  }
  return UplcConstant(UplcType::primitive(UplcTypeTag::bls12_381_ml_result), std::move(value));
}

core::Result<UplcConstant> UplcConstant::value(Data value) {
  return UplcConstant(UplcType::primitive(UplcTypeTag::value), std::move(value));
}

const UplcType& UplcConstant::type() const noexcept { return type_; }
const UplcConstant::Value& UplcConstant::value() const noexcept { return value_; }

std::size_t UplcConstant::memory_words([[maybe_unused]] SemanticsVariant semantics) const {
  return std::visit(
      [&](const auto& value) -> std::size_t {
        using ValueType = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<ValueType, core::BigInteger>) {
          return integer_memory(value);
        } else if constexpr (std::is_same_v<ValueType, core::Bytes>) {
          return std::max<std::size_t>(1U, (value.size() + 7U) / 8U);
        } else if constexpr (std::is_same_v<ValueType, std::string>) {
          std::size_t code_points = 0U;
          for (const auto character : value) {
            if ((static_cast<unsigned char>(character) & 0xC0U) != 0x80U) {
              ++code_points;
            }
          }
          return code_points;
        } else if constexpr (std::is_same_v<ValueType, std::monostate> ||
                             std::is_same_v<ValueType, bool>) {
          return 1U;
        } else if constexpr (std::is_same_v<ValueType, Items>) {
          return value.size();
        } else if constexpr (std::is_same_v<ValueType, UplcPair>) {
          return static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max());
        } else {
          return data_memory(value.to_plutus_data());
        }
      },
      value_);
}

core::Result<Builtin> builtin_tag(std::uint8_t tag) {
  if (tag >= BUILTIN_NAMES.size()) {
    return std::unexpected(decode_error("unknown UPLC builtin tag"));
  }
  return static_cast<Builtin>(tag);
}

std::uint8_t builtin_tag(Builtin builtin) noexcept { return static_cast<std::uint8_t>(builtin); }

std::string_view builtin_name(Builtin builtin) noexcept {
  const auto tag = builtin_tag(builtin);
  return tag < BUILTIN_NAMES.size() ? BUILTIN_NAMES[tag] : std::string_view{};
}

core::Result<Builtin> builtin_from_name(std::string_view name) {
  const auto found = std::find(BUILTIN_NAMES.begin(), BUILTIN_NAMES.end(), name);
  if (found == BUILTIN_NAMES.end()) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_argument, "unknown UPLC builtin name"));
  }
  return static_cast<Builtin>(static_cast<std::uint8_t>(found - BUILTIN_NAMES.begin()));
}

UplcTerm::UplcTerm(Node node) : node_(std::move(node)) {}
UplcTerm UplcTerm::variable(std::uint64_t index) { return UplcTerm(UplcVariable{index}); }
UplcTerm UplcTerm::delay(UplcTerm term) {
  return UplcTerm(UplcDelay{std::make_shared<UplcTerm>(std::move(term))});
}
UplcTerm UplcTerm::lambda(UplcTerm body) {
  return UplcTerm(UplcLambda{std::make_shared<UplcTerm>(std::move(body))});
}
UplcTerm UplcTerm::apply(UplcTerm function, UplcTerm argument) {
  return UplcTerm(UplcApply{std::make_shared<UplcTerm>(std::move(function)),
                            std::make_shared<UplcTerm>(std::move(argument))});
}
UplcTerm UplcTerm::constant(UplcConstant value) { return UplcTerm(std::move(value)); }
UplcTerm UplcTerm::force(UplcTerm term) {
  return UplcTerm(UplcForce{std::make_shared<UplcTerm>(std::move(term))});
}
UplcTerm UplcTerm::error() { return UplcTerm(UplcError{}); }
UplcTerm UplcTerm::builtin(Builtin value) { return UplcTerm(value); }
UplcTerm UplcTerm::constr(std::uint64_t tag, std::vector<UplcTerm> fields) {
  return UplcTerm(UplcConstr{tag, std::move(fields)});
}
UplcTerm UplcTerm::case_of(UplcTerm scrutinee, std::vector<UplcTerm> branches) {
  return UplcTerm(UplcCase{std::make_shared<UplcTerm>(std::move(scrutinee)), std::move(branches)});
}
const UplcTerm::Node& UplcTerm::node() const noexcept { return node_; }

bool operator==(const UplcDelay& left, const UplcDelay& right) { return *left.term == *right.term; }
bool operator==(const UplcLambda& left, const UplcLambda& right) {
  return *left.body == *right.body;
}
bool operator==(const UplcApply& left, const UplcApply& right) {
  return *left.function == *right.function && *left.argument == *right.argument;
}
bool operator==(const UplcForce& left, const UplcForce& right) { return *left.term == *right.term; }
bool operator==(const UplcCase& left, const UplcCase& right) {
  return *left.scrutinee == *right.scrutinee && left.branches == right.branches;
}

MachineBudget CostStream::total() const noexcept {
  MachineBudget result{};
  for (const auto& cost : costs) {
    if (cost.cpu > 0 && result.cpu > std::numeric_limits<std::int64_t>::max() - cost.cpu) {
      result.cpu = std::numeric_limits<std::int64_t>::max();
    } else {
      result.cpu += cost.cpu;
    }
    if (cost.memory > 0 && result.memory > std::numeric_limits<std::int64_t>::max() - cost.memory) {
      result.memory = std::numeric_limits<std::int64_t>::max();
    } else {
      result.memory += cost.memory;
    }
  }
  return result;
}

namespace {

enum class CostKind {
  constant,
  linear_x,
  linear_y,
  linear_z,
  linear_u,
  quadratic_x,
  quadratic_y,
  quadratic_z,
  added,
  multiplied,
  minimum,
  maximum,
  subtracted,
  linear_xy,
  linear_yz,
  linear_max_yz,
  diagonal,
  const_above,
  above_below,
  const_above_multiplied,
  above_below_multiplied,
  interaction,
  literal_y_or_linear_z,
  exp_mod
};

struct Costing {
  CostKind kind;
  std::array<std::int64_t, 8> p{};
};

struct CostEntry {
  Costing cpu;
  Costing memory;
};

constexpr std::array<std::array<std::uint16_t, 101>, 3> COST_PARAMETER_STARTS{{
    {0,   145, 115, 49,  121, 127, 109, 66,  94,  91,  4,   39,  139, 83,  81,  59,  85,
     88,  133, 136, 14,  163, 8,   69,  55,  45,  79,  37,  151, 73,  143, 35,  101, 75,
     149, 119, 33,  43,  99,  97,  77,  12,  155, 161, 159, 157, 153, 63,  107, 103, 105,
     166, 170, 172, 179, 188, 190, 183, 185, 181, 193, 195, 204, 206, 199, 201, 197, 209,
     213, 215, 211, 217, 220, 223, 228, 233, 238, 243, 248, 252, 254, 258, 262, 266, 270,
     273, 276, 279, 284, 287, 289, 293, 295, 298, 301, 305, 308, 314, 319, 323, 328},
    {0,   149, 115, 49,  121, 127, 109, 66,  94,  91,  4,   39,  143, 83,  81,  59,  85,
     88,  137, 140, 14,  169, 8,   69,  55,  45,  79,  37,  155, 73,  147, 35,  101, 75,
     153, 119, 33,  43,  99,  97,  77,  12,  159, 165, 163, 161, 157, 63,  107, 103, 105,
     133, 167, 172, 189, 198, 200, 193, 195, 191, 203, 205, 214, 216, 209, 211, 207, 219,
     223, 225, 221, 227, 230, 175, 180, 233, 238, 243, 248, 252, 254, 258, 262, 266, 270,
     273, 276, 279, 284, 287, 289, 293, 295, 298, 301, 305, 308, 314, 319, 323, 328},
    {0,   167, 124, 49,  130, 141, 114, 71,  99,  96,  4,   39,  161, 88,  86,  64,  90,
     93,  155, 158, 14,  187, 8,   74,  60,  45,  84,  37,  173, 78,  165, 35,  106, 80,
     171, 128, 33,  43,  104, 102, 82,  12,  177, 183, 181, 179, 175, 68,  112, 108, 110,
     151, 185, 190, 197, 206, 208, 201, 203, 199, 211, 213, 222, 224, 217, 219, 215, 227,
     231, 233, 229, 235, 238, 241, 246, 251, 256, 261, 266, 270, 272, 276, 280, 284, 288,
     291, 294, 297, 302, 305, 307, 311, 313, 316, 319, 323, 326, 332, 337, 341, 346},
}};

constexpr Costing fixed(std::int64_t value) { return {CostKind::constant, {value}}; }

constexpr Costing linear(CostKind kind, std::int64_t intercept, std::int64_t slope) {
  return {kind, {intercept, slope}};
}

constexpr Costing quadratic(CostKind kind, std::int64_t c0, std::int64_t c1, std::int64_t c2) {
  return {kind, {c0, c1, c2}};
}

constexpr Costing two_linear(CostKind kind, std::int64_t intercept, std::int64_t slope1,
                             std::int64_t slope2) {
  return {kind, {intercept, slope1, slope2}};
}

constexpr Costing division_model(CostKind kind) {
  return {kind, {85'848, 123'203, 1'716, 7'305, 57, 960, -900, 85'848}};
}

constexpr CostEntry default_cost_entry_e(Builtin builtin) {
  using enum CostKind;
  switch (builtin) {
    case Builtin::add_integer:
    case Builtin::subtract_integer:
      return {linear(maximum, 100'788, 420), linear(maximum, 1, 1)};
    case Builtin::multiply_integer:
      return {linear(multiplied, 90'434, 519), linear(added, 0, 1)};
    case Builtin::divide_integer:
      return {division_model(above_below), {subtracted, {0, 1, 1}}};
    case Builtin::quotient_integer:
      return {division_model(const_above), {subtracted, {0, 1, 1}}};
    case Builtin::remainder_integer:
      return {division_model(const_above), linear(linear_y, 0, 1)};
    case Builtin::mod_integer:
      return {division_model(above_below), linear(linear_y, 0, 1)};
    case Builtin::equals_integer:
      return {linear(minimum, 51'775, 558), fixed(1)};
    case Builtin::less_than_integer:
      return {linear(minimum, 44'749, 541), fixed(1)};
    case Builtin::less_than_equals_integer:
      return {linear(minimum, 43'285, 552), fixed(1)};
    case Builtin::append_byte_string:
      return {linear(added, 1'000, 173), linear(added, 0, 1)};
    case Builtin::cons_byte_string:
      return {linear(linear_y, 72'010, 178), linear(added, 0, 1)};
    case Builtin::slice_byte_string:
      return {linear(linear_z, 20'467, 1), linear(linear_z, 4, 0)};
    case Builtin::length_of_byte_string:
      return {fixed(22'100), fixed(10)};
    case Builtin::index_byte_string:
      return {fixed(13'169), fixed(4)};
    case Builtin::equals_byte_string:
      return {{diagonal, {30'623, 28'755, 75}}, fixed(1)};
    case Builtin::less_than_byte_string:
    case Builtin::less_than_equals_byte_string:
      return {linear(minimum, 28'999, 74), fixed(1)};
    case Builtin::sha2_256:
      return {linear(linear_x, 270'652, 22'588), fixed(4)};
    case Builtin::sha3_256:
      return {linear(linear_x, 1'457'325, 64'566), fixed(4)};
    case Builtin::blake2b_256:
      return {linear(linear_x, 201'305, 8'356), fixed(4)};
    case Builtin::verify_ed25519_signature:
      return {linear(linear_y, 53'384'111, 14'333), fixed(10)};
    case Builtin::append_string:
      return {linear(added, 1'000, 59'957), linear(added, 4, 1)};
    case Builtin::equals_string:
      return {{diagonal, {39'184, 1'000, 60'594}}, fixed(1)};
    case Builtin::encode_utf8:
      return {linear(linear_x, 1'000, 42'921), linear(linear_x, 4, 2)};
    case Builtin::decode_utf8:
      return {linear(linear_x, 91'189, 769), linear(linear_x, 4, 2)};
    case Builtin::if_then_else:
      return {fixed(76'049), fixed(1)};
    case Builtin::choose_unit:
      return {fixed(61'462), fixed(4)};
    case Builtin::trace:
      return {fixed(59'498), fixed(32)};
    case Builtin::fst_pair:
      return {fixed(141'895), fixed(32)};
    case Builtin::snd_pair:
      return {fixed(141'992), fixed(32)};
    case Builtin::choose_list:
      return {fixed(132'994), fixed(32)};
    case Builtin::mk_cons:
      return {fixed(72'362), fixed(32)};
    case Builtin::head_list:
      return {fixed(83'150), fixed(32)};
    case Builtin::tail_list:
      return {fixed(81'663), fixed(32)};
    case Builtin::null_list:
      return {fixed(74'433), fixed(32)};
    case Builtin::choose_data:
      return {fixed(94'375), fixed(32)};
    case Builtin::constr_data:
      return {fixed(22'151), fixed(32)};
    case Builtin::map_data:
      return {fixed(68'246), fixed(32)};
    case Builtin::list_data:
      return {fixed(33'852), fixed(32)};
    case Builtin::i_data:
      return {fixed(15'299), fixed(32)};
    case Builtin::b_data:
      return {fixed(11'183), fixed(32)};
    case Builtin::un_constr_data:
      return {fixed(24'588), fixed(32)};
    case Builtin::un_map_data:
      return {fixed(24'623), fixed(32)};
    case Builtin::un_list_data:
      return {fixed(25'933), fixed(32)};
    case Builtin::un_i_data:
      return {fixed(20'744), fixed(32)};
    case Builtin::un_b_data:
      return {fixed(20'142), fixed(32)};
    case Builtin::equals_data:
      return {linear(minimum, 898'148, 27'279), fixed(1)};
    case Builtin::mk_pair_data:
      return {fixed(11'546), fixed(32)};
    case Builtin::mk_nil_data:
      return {fixed(7'243), fixed(32)};
    case Builtin::mk_nil_pair_data:
      return {fixed(7'391), fixed(32)};
    case Builtin::serialise_data:
      return {linear(linear_x, 955'506, 213'312), linear(linear_x, 0, 2)};
    case Builtin::verify_ecdsa_secp256k1_signature:
      return {fixed(43'053'543), fixed(10)};
    case Builtin::verify_schnorr_secp256k1_signature:
      return {linear(linear_y, 43'574'283, 26'308), fixed(10)};
    case Builtin::bls12_381_g1_add:
      return {fixed(962'335), fixed(18)};
    case Builtin::bls12_381_g1_neg:
      return {fixed(267'929), fixed(18)};
    case Builtin::bls12_381_g1_scalar_mul:
      return {linear(linear_x, 76'433'006, 8'868), fixed(18)};
    case Builtin::bls12_381_g1_equal:
      return {fixed(442'008), fixed(1)};
    case Builtin::bls12_381_g1_hash_to_group:
      return {linear(linear_x, 52'538'055, 3'756), fixed(18)};
    case Builtin::bls12_381_g1_compress:
      return {fixed(2'780'678), fixed(6)};
    case Builtin::bls12_381_g1_uncompress:
      return {fixed(52'948'122), fixed(18)};
    case Builtin::bls12_381_g2_add:
      return {fixed(1'995'836), fixed(36)};
    case Builtin::bls12_381_g2_neg:
      return {fixed(284'546), fixed(36)};
    case Builtin::bls12_381_g2_scalar_mul:
      return {linear(linear_x, 158'221'314, 26'549), fixed(36)};
    case Builtin::bls12_381_g2_equal:
      return {fixed(901'022), fixed(1)};
    case Builtin::bls12_381_g2_hash_to_group:
      return {linear(linear_x, 166'917'843, 4'307), fixed(36)};
    case Builtin::bls12_381_g2_compress:
      return {fixed(3'227'919), fixed(12)};
    case Builtin::bls12_381_g2_uncompress:
      return {fixed(74'698'472), fixed(36)};
    case Builtin::bls12_381_miller_loop:
      return {fixed(254'006'273), fixed(72)};
    case Builtin::bls12_381_mul_ml_result:
      return {fixed(2'174'038), fixed(72)};
    case Builtin::bls12_381_final_verify:
      return {fixed(333'849'714), fixed(1)};
    case Builtin::keccak_256:
      return {linear(linear_x, 2'261'318, 64'571), fixed(4)};
    case Builtin::blake2b_224:
      return {linear(linear_x, 207'616, 8'310), fixed(4)};
    case Builtin::integer_to_byte_string:
      return {quadratic(quadratic_z, 1'293'828, 28'716, 63), linear(literal_y_or_linear_z, 0, 1)};
    case Builtin::byte_string_to_integer:
      return {quadratic(quadratic_y, 1'006'041, 43'623, 251), linear(linear_y, 0, 1)};
    case Builtin::and_byte_string:
    case Builtin::or_byte_string:
    case Builtin::xor_byte_string:
      return {two_linear(linear_yz, 100'181, 726, 719), linear(linear_max_yz, 0, 1)};
    case Builtin::complement_byte_string:
      return {linear(linear_x, 107'878, 680), linear(linear_x, 0, 1)};
    case Builtin::read_bit:
      return {fixed(95'336), fixed(1)};
    case Builtin::write_bits:
      return {linear(linear_y, 281'145, 18'848), linear(linear_x, 0, 1)};
    case Builtin::replicate_byte:
      return {linear(linear_x, 180'194, 159), linear(linear_x, 1, 1)};
    case Builtin::shift_byte_string:
      return {linear(linear_x, 158'519, 8'942), linear(linear_x, 0, 1)};
    case Builtin::rotate_byte_string:
      return {linear(linear_x, 159'378, 8'813), linear(linear_x, 0, 1)};
    case Builtin::count_set_bits:
      return {linear(linear_x, 107'490, 3'298), fixed(1)};
    case Builtin::find_first_set_bit:
      return {linear(linear_x, 106'057, 655), fixed(1)};
    case Builtin::ripemd_160:
      return {linear(linear_x, 1'964'219, 24'520), fixed(3)};
    case Builtin::exp_mod_integer:
      return {{exp_mod, {607'153, 231'697, 53'144}}, linear(linear_z, 0, 1)};
    case Builtin::drop_list:
      return {linear(linear_x, 116'711, 1'957), fixed(4)};
    case Builtin::length_of_array:
      return {fixed(231'883), fixed(10)};
    case Builtin::list_to_array:
      return {linear(linear_x, 1'000, 24'838), linear(linear_x, 7, 1)};
    case Builtin::index_array:
      return {fixed(232'010), fixed(32)};
    case Builtin::bls12_381_g1_multi_scalar_mul:
      return {linear(linear_x, 321'837'444, 25'087'669), fixed(18)};
    case Builtin::bls12_381_g2_multi_scalar_mul:
      return {linear(linear_x, 617'887'431, 67'302'824), fixed(36)};
    case Builtin::insert_coin:
      return {linear(linear_u, 356'924, 18'413), linear(linear_u, 45, 21)};
    case Builtin::lookup_coin:
      return {linear(linear_z, 219'951, 9'444), fixed(1)};
    case Builtin::union_value:
      return {{interaction, {1'000, 172'116, 183'150, 6}}, linear(added, 24, 21)};
    case Builtin::value_contains:
      return {{const_above, {213'283, 618'401, 1'998, 28'258}}, fixed(1)};
    case Builtin::value_data:
      return {linear(linear_x, 1'000, 38'159), linear(linear_x, 2, 22)};
    case Builtin::un_value_data:
      return {quadratic(quadratic_x, 1'000, 95'933, 1), linear(linear_x, 1, 11)};
    case Builtin::scale_value:
      return {linear(linear_y, 1'000, 277'577), linear(linear_y, 12, 21)};
  }
  return {fixed(0), fixed(0)};
}

constexpr CostEntry default_cost_entry(SemanticsVariant semantics, Builtin builtin) {
  using enum CostKind;
  if (semantics == SemanticsVariant::a) {
    switch (builtin) {
      case Builtin::add_integer:
      case Builtin::subtract_integer:
        return {linear(maximum, 205'665, 812), linear(maximum, 1, 1)};
      case Builtin::multiply_integer:
        return {linear(added, 69'522, 11'687), linear(added, 0, 1)};
      case Builtin::divide_integer:
      case Builtin::mod_integer:
      case Builtin::quotient_integer:
      case Builtin::remainder_integer:
        return {{const_above_multiplied, {196'500, 453'240, 220}}, {subtracted, {0, 1, 1}}};
      case Builtin::equals_integer:
        return {linear(minimum, 208'512, 421), fixed(1)};
      case Builtin::less_than_integer:
        return {linear(minimum, 208'896, 511), fixed(1)};
      case Builtin::less_than_equals_integer:
        return {linear(minimum, 204'924, 473), fixed(1)};
      case Builtin::append_byte_string:
        return {linear(added, 1'000, 571), linear(added, 0, 1)};
      case Builtin::cons_byte_string:
        return {linear(linear_y, 221'973, 511), linear(added, 0, 1)};
      case Builtin::slice_byte_string:
        return {linear(linear_z, 265'318, 0), linear(linear_z, 4, 0)};
      case Builtin::length_of_byte_string:
        return {fixed(1'000), fixed(10)};
      case Builtin::index_byte_string:
        return {fixed(57'667), fixed(4)};
      case Builtin::equals_byte_string:
        return {{diagonal, {245'000, 216'773, 62}}, fixed(1)};
      case Builtin::less_than_byte_string:
      case Builtin::less_than_equals_byte_string:
        return {linear(minimum, 197'145, 156), fixed(1)};
      case Builtin::sha2_256:
        return {linear(linear_x, 806'990, 30'482), fixed(4)};
      case Builtin::sha3_256:
        return {linear(linear_x, 1'927'926, 82'523), fixed(4)};
      case Builtin::blake2b_256:
        return {linear(linear_x, 117'366, 10'475), fixed(4)};
      case Builtin::verify_ed25519_signature:
        return {linear(linear_z, 57'996'947, 18'975), fixed(10)};
      case Builtin::append_string:
        return {linear(added, 1'000, 24'177), linear(added, 4, 1)};
      case Builtin::equals_string:
        return {{diagonal, {187'000, 1'000, 52'998}}, fixed(1)};
      case Builtin::encode_utf8:
        return {linear(linear_x, 1'000, 28'662), linear(linear_x, 4, 2)};
      case Builtin::decode_utf8:
        return {linear(linear_x, 497'525, 14'068), linear(linear_x, 4, 2)};
      case Builtin::if_then_else:
        return {fixed(80'556), fixed(1)};
      case Builtin::choose_unit:
        return {fixed(46'417), fixed(4)};
      case Builtin::trace:
        return {fixed(212'342), fixed(32)};
      case Builtin::fst_pair:
        return {fixed(80'436), fixed(32)};
      case Builtin::snd_pair:
        return {fixed(85'931), fixed(32)};
      case Builtin::choose_list:
        return {fixed(175'354), fixed(32)};
      case Builtin::mk_cons:
        return {fixed(65'493), fixed(32)};
      case Builtin::head_list:
        return {fixed(43'249), fixed(32)};
      case Builtin::tail_list:
        return {fixed(41'182), fixed(32)};
      case Builtin::null_list:
        return {fixed(60'091), fixed(32)};
      case Builtin::choose_data:
        return {fixed(19'537), fixed(32)};
      case Builtin::constr_data:
        return {fixed(89'141), fixed(32)};
      case Builtin::map_data:
        return {fixed(64'832), fixed(32)};
      case Builtin::list_data:
        return {fixed(52'467), fixed(32)};
      case Builtin::i_data:
      case Builtin::b_data:
        return {fixed(1'000), fixed(32)};
      case Builtin::un_constr_data:
        return {fixed(32'696), fixed(32)};
      case Builtin::un_map_data:
        return {fixed(38'314), fixed(32)};
      case Builtin::un_list_data:
        return {fixed(32'247), fixed(32)};
      case Builtin::un_i_data:
        return {fixed(43'357), fixed(32)};
      case Builtin::un_b_data:
        return {fixed(31'220), fixed(32)};
      case Builtin::equals_data:
        return {linear(minimum, 1'060'367, 12'586), fixed(1)};
      case Builtin::mk_pair_data:
        return {fixed(76'511), fixed(32)};
      case Builtin::mk_nil_data:
        return {fixed(22'558), fixed(32)};
      case Builtin::mk_nil_pair_data:
        return {fixed(16'563), fixed(32)};
      case Builtin::serialise_data:
        return {linear(linear_x, 1'159'724, 392'670), linear(linear_x, 0, 2)};
      case Builtin::verify_ecdsa_secp256k1_signature:
        return {fixed(35'190'005), fixed(10)};
      case Builtin::verify_schnorr_secp256k1_signature:
        return {linear(linear_y, 39'121'781, 32'260), fixed(10)};
      default:
        break;
    }
  } else if (semantics == SemanticsVariant::b) {
    switch (builtin) {
      case Builtin::divide_integer:
      case Builtin::mod_integer:
      case Builtin::quotient_integer:
      case Builtin::remainder_integer:
        return {{const_above_multiplied, {85'848, 228'465, 122}}, {subtracted, {0, 1, 1}}};
      case Builtin::equals_byte_string:
        return {{diagonal, {24'548, 29'498, 38}}, fixed(1)};
      default:
        break;
    }
  } else if (semantics == SemanticsVariant::c) {
    switch (builtin) {
      case Builtin::divide_integer:
      case Builtin::quotient_integer:
        return {{const_above, {85'848, 123'203, 1'716, 7'305, 57, 549, -900, 85'848}},
                {subtracted, {0, 1, 1}}};
      case Builtin::mod_integer:
      case Builtin::remainder_integer:
        return {{const_above, {85'848, 123'203, 1'716, 7'305, 57, 549, -900, 85'848}},
                linear(linear_y, 0, 1)};
      case Builtin::equals_byte_string:
        return {{diagonal, {24'548, 29'498, 38}}, fixed(1)};
      default:
        break;
    }
  } else if (semantics == SemanticsVariant::d) {
    switch (builtin) {
      case Builtin::divide_integer:
        return {{above_below_multiplied, {85'848, 228'465, 122}}, {subtracted, {0, 1, 1}}};
      case Builtin::mod_integer:
        return {{above_below_multiplied, {85'848, 228'465, 122}}, linear(linear_y, 0, 1)};
      case Builtin::quotient_integer:
        return {{const_above_multiplied, {85'848, 228'465, 122}}, {subtracted, {0, 1, 1}}};
      case Builtin::remainder_integer:
        return {{const_above_multiplied, {85'848, 228'465, 122}}, linear(linear_y, 0, 1)};
      default:
        break;
    }
  }
  return default_cost_entry_e(builtin);
}

[[nodiscard]] std::int64_t clamp_cost(__int128 value) {
  if (value <= 0) {
    return 0;
  }
  if (value >= std::numeric_limits<std::int64_t>::max()) {
    return std::numeric_limits<std::int64_t>::max();
  }
  return static_cast<std::int64_t>(value);
}

[[nodiscard]] std::int64_t evaluate_costing(const Costing& model,
                                            const std::array<std::int64_t, 4>& sizes) {
  const auto x = static_cast<__int128>(sizes[0]);
  const auto y = static_cast<__int128>(sizes[1]);
  const auto z = static_cast<__int128>(sizes[2]);
  const auto u = static_cast<__int128>(sizes[3]);
  const auto p = [&](std::size_t index) { return static_cast<__int128>(model.p[index]); };
  __int128 result = 0;
  switch (model.kind) {
    case CostKind::constant:
      result = p(0);
      break;
    case CostKind::linear_x:
      result = p(0) + p(1) * x;
      break;
    case CostKind::linear_y:
      result = p(0) + p(1) * y;
      break;
    case CostKind::linear_z:
      result = p(0) + p(1) * z;
      break;
    case CostKind::linear_u:
      result = p(0) + p(1) * u;
      break;
    case CostKind::quadratic_x:
      result = p(0) + p(1) * x + p(2) * x * x;
      break;
    case CostKind::quadratic_y:
      result = p(0) + p(1) * y + p(2) * y * y;
      break;
    case CostKind::quadratic_z:
      result = p(0) + p(1) * z + p(2) * z * z;
      break;
    case CostKind::added:
      result = p(0) + p(1) * (x + y);
      break;
    case CostKind::multiplied:
      result = p(0) + p(1) * x * y;
      break;
    case CostKind::minimum:
      result = p(0) + p(1) * std::min(x, y);
      break;
    case CostKind::maximum:
      result = p(0) + p(1) * std::max(x, y);
      break;
    case CostKind::subtracted:
      result = p(0) + p(1) * std::max(x - y, p(2));
      break;
    case CostKind::linear_xy:
      result = p(0) + p(1) * x + p(2) * y;
      break;
    case CostKind::linear_yz:
      result = p(0) + p(1) * y + p(2) * z;
      break;
    case CostKind::linear_max_yz:
      result = p(0) + p(1) * std::max(y, z);
      break;
    case CostKind::diagonal:
      result = x == y ? p(1) + p(2) * x : p(0);
      break;
    case CostKind::const_above:
      if (x < y) {
        result = p(0);
      } else if (model.p[4] == 0 && model.p[5] == 0 && model.p[6] == 0 && model.p[7] == 0) {
        result = p(1) + p(2) * x + p(3) * y;
      } else {
        result =
            std::max(p(1) + p(2) * x + p(3) * y + p(4) * x * x + p(5) * x * y + p(6) * y * y, p(7));
      }
      break;
    case CostKind::above_below: {
      const auto high = std::max(x, y);
      const auto low = std::min(x, y);
      result = std::max(p(1) + p(2) * high + p(3) * low + p(4) * high * high + p(5) * high * low +
                            p(6) * low * low,
                        p(7));
      break;
    }
    case CostKind::const_above_multiplied:
      result = x < y ? p(0) : p(1) + p(2) * x * y;
      break;
    case CostKind::above_below_multiplied:
      result = p(1) + p(2) * std::max(x, y) * std::min(x, y);
      break;
    case CostKind::interaction:
      result = p(0) + p(1) * x + p(2) * y + p(3) * x * y;
      break;
    case CostKind::literal_y_or_linear_z:
      result = y == 0 ? p(0) + p(1) * z : y;
      break;
    case CostKind::exp_mod: {
      const auto base = p(0) + p(1) * y * z + p(2) * y * z * z;
      result = x <= z ? base : base + base / 2;
      break;
    }
  }
  return clamp_cost(result);
}

[[nodiscard]] Costing with_ledger_parameters(Costing model,
                                             const std::vector<std::int64_t>& parameters,
                                             std::size_t& cursor) {
  const auto next = [&]() {
    return cursor < parameters.size() ? parameters[cursor++]
                                      : (cursor++, std::numeric_limits<std::int64_t>::max());
  };
  switch (model.kind) {
    case CostKind::constant:
      model.p[0] = next();
      break;
    case CostKind::linear_x:
    case CostKind::linear_y:
    case CostKind::linear_z:
    case CostKind::linear_u:
    case CostKind::added:
    case CostKind::multiplied:
    case CostKind::minimum:
    case CostKind::maximum:
    case CostKind::linear_max_yz:
    case CostKind::literal_y_or_linear_z:
      model.p[0] = next();
      model.p[1] = next();
      break;
    case CostKind::quadratic_x:
    case CostKind::quadratic_y:
    case CostKind::quadratic_z:
    case CostKind::linear_xy:
    case CostKind::linear_yz:
    case CostKind::diagonal:
    case CostKind::exp_mod:
      model.p[0] = next();
      model.p[1] = next();
      model.p[2] = next();
      break;
    case CostKind::subtracted: {
      const auto intercept = next();
      const auto minimum_value = next();
      const auto slope = next();
      model.p[0] = intercept;
      model.p[1] = slope;
      model.p[2] = minimum_value;
      break;
    }
    case CostKind::interaction:
      model.p[0] = next();
      model.p[1] = next();
      model.p[2] = next();
      model.p[3] = next();
      break;
    case CostKind::const_above:
      if (model.p[4] == 0 && model.p[5] == 0 && model.p[6] == 0 && model.p[7] == 0) {
        model.p[0] = next();
        model.p[1] = next();
        model.p[2] = next();
        model.p[3] = next();
      } else {
        const auto constant = next();
        const auto c00 = next();
        const auto c01 = next();
        const auto c02 = next();
        const auto c10 = next();
        const auto c11 = next();
        const auto c20 = next();
        const auto minimum_value = next();
        model.p = {constant, c00, c10, c01, c20, c11, c02, minimum_value};
      }
      break;
    case CostKind::above_below: {
      const auto constant = next();
      const auto c00 = next();
      const auto c01 = next();
      const auto c02 = next();
      const auto c10 = next();
      const auto c11 = next();
      const auto c20 = next();
      const auto minimum_value = next();
      model.p = {constant, c00, c10, c01, c20, c11, c02, minimum_value};
      break;
    }
    case CostKind::const_above_multiplied:
    case CostKind::above_below_multiplied:
      model.p[0] = next();
      model.p[1] = next();
      model.p[2] = next();
      break;
  }
  return model;
}

}  // namespace

MachineCosts default_machine_costs(SemanticsVariant semantics) {
  MachineCosts result;
  if (semantics == SemanticsVariant::d || semantics == SemanticsVariant::e) {
    result.constr = {16'000, 100};
    result.case_cost = {16'000, 100};
  }
  return result;
}

BuiltinCostModel default_builtin_cost_model(SemanticsVariant semantics) {
  BuiltinCostModel result;
  result.semantics = semantics;
  return result;
}

BuiltinCostModel make_builtin_cost_model(SemanticsVariant semantics,
                                         std::span<const std::int64_t> parameters) {
  BuiltinCostModel result;
  result.semantics = semantics;
  for (std::size_t tag = 0; tag < BUILTIN_NAMES.size(); ++tag) {
    const auto value =
        tag < parameters.size() ? parameters[tag] : std::numeric_limits<std::int64_t>::max();
    result.parameters.emplace(static_cast<Builtin>(tag), std::vector<std::int64_t>{value});
  }
  return result;
}

BuiltinCostModel make_builtin_cost_model(PlutusLanguage language,
                                         std::span<const std::int64_t> parameters,
                                         SemanticsVariant semantics) {
  BuiltinCostModel result;
  result.semantics = semantics;
  result.language = language;
  result.ledger_parameters.assign(parameters.begin(), parameters.end());
  return result;
}

namespace {

[[nodiscard]] std::int64_t saturated_size(std::size_t size) {
  const auto maximum = static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max());
  return static_cast<std::int64_t>(std::min(size, maximum));
}

[[nodiscard]] std::int64_t absolute_integer_metric(const core::BigInteger& integer) {
  const auto magnitude = integer.is_negative() ? -integer : integer;
  const auto converted = magnitude.to_uint64();
  if (!converted ||
      *converted > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::numeric_limits<std::int64_t>::max();
  }
  return static_cast<std::int64_t>(*converted);
}

[[nodiscard]] std::int64_t data_node_count(const Data& data) {
  std::int64_t count = 0;
  std::vector<const chain::PlutusData*> stack{&data.to_plutus_data()};
  while (!stack.empty()) {
    const auto* current = stack.back();
    stack.pop_back();
    if (count != std::numeric_limits<std::int64_t>::max()) {
      ++count;
    }
    std::visit(
        [&](const auto& node) {
          using Node = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<Node, std::shared_ptr<chain::PlutusData::List>>) {
            for (const auto& item : *node) {
              stack.push_back(&item);
            }
          } else if constexpr (std::is_same_v<Node, std::shared_ptr<chain::PlutusMap>>) {
            for (const auto& [key, value] : node->entries) {
              stack.push_back(&key);
              stack.push_back(&value);
            }
          } else if constexpr (!std::is_same_v<Node, core::BigInteger> &&
                               !std::is_same_v<Node, core::Bytes>) {
            for (const auto& field : node->fields) {
              stack.push_back(&field);
            }
          }
        },
        current->node());
  }
  return count;
}

struct ValueShape {
  std::int64_t total_size{};
  std::int64_t maximum_depth{};
};

[[nodiscard]] ValueShape value_shape(const Data& value) {
  const auto* outer =
      std::get_if<std::shared_ptr<chain::PlutusMap>>(&value.to_plutus_data().node());
  if (outer == nullptr) {
    return {};
  }
  std::size_t total = 0U;
  std::size_t maximum_inner = 0U;
  for (const auto& [policy, token_map] : (*outer)->entries) {
    static_cast<void>(policy);
    const auto* inner = std::get_if<std::shared_ptr<chain::PlutusMap>>(&token_map.node());
    if (inner == nullptr) {
      continue;
    }
    total += (*inner)->entries.size();
    maximum_inner = std::max(maximum_inner, (*inner)->entries.size());
  }
  const auto tree_depth = [](std::size_t size) {
    std::int64_t depth = 0;
    while (size != 0U) {
      ++depth;
      size >>= 1U;
    }
    return depth;
  };
  return {saturated_size(total), tree_depth((*outer)->entries.size()) + tree_depth(maximum_inner)};
}

[[nodiscard]] std::int64_t builtin_argument_size(Builtin builtin, std::size_t index,
                                                 const UplcConstant& argument,
                                                 SemanticsVariant semantics) {
  const auto tag = builtin_tag(builtin);
  if ((semantics == SemanticsVariant::d || semantics == SemanticsVariant::e) &&
      (tag == 22U || tag == 23U || tag == 24U) && argument.type().tag() == UplcTypeTag::string) {
    const auto* text = std::get_if<std::string>(&argument.value());
    return text == nullptr ? 0 : saturated_size(text->size() / 4U);
  }
  if (((tag == 73U && index == 1U) || (tag == 81U && index == 0U)) &&
      argument.type().tag() == UplcTypeTag::integer) {
    const auto* integer = std::get_if<core::BigInteger>(&argument.value());
    if (integer == nullptr) {
      return 0;
    }
    const auto absolute = absolute_integer_metric(*integer);
    return absolute == 0 ? 0 : (absolute - 1) / 8 + 1;
  }
  if ((((tag == 82U || tag == 83U) && index == 1U) || (tag == 88U && index == 0U)) &&
      argument.type().tag() == UplcTypeTag::integer) {
    const auto* integer = std::get_if<core::BigInteger>(&argument.value());
    return integer == nullptr ? 0 : absolute_integer_metric(*integer);
  }
  if (argument.type().tag() == UplcTypeTag::value) {
    const auto* value = std::get_if<Data>(&argument.value());
    if (value != nullptr) {
      const auto shape = value_shape(*value);
      if ((tag == 94U && index == 3U) || (tag == 95U && index == 2U)) {
        return shape.maximum_depth;
      }
      if (tag == 96U || tag == 97U || (tag == 98U && index == 0U) || (tag == 100U && index == 1U)) {
        return shape.total_size;
      }
    }
  }
  if (tag == 99U && index == 0U && argument.type().tag() == UplcTypeTag::data) {
    const auto* data = std::get_if<Data>(&argument.value());
    return data == nullptr ? 0 : data_node_count(*data);
  }
  return saturated_size(argument.memory_words(semantics));
}

}  // namespace

MachineBudget builtin_cost(Builtin builtin, std::span<const UplcConstant> arguments,
                           const BuiltinCostModel& model) {
  std::array<std::int64_t, 4> sizes{};
  for (std::size_t index = 0U; index < arguments.size() && index < sizes.size(); ++index) {
    sizes[index] = builtin_argument_size(builtin, index, arguments[index], model.semantics);
  }
  if (model.parameters.empty()) {
    auto entry = default_cost_entry(model.semantics, builtin);
    if (model.language) {
      const auto language = static_cast<std::size_t>(*model.language);
      auto cursor = static_cast<std::size_t>(COST_PARAMETER_STARTS[language][builtin_tag(builtin)]);
      entry.cpu = with_ledger_parameters(entry.cpu, model.ledger_parameters, cursor);
      entry.memory = with_ledger_parameters(entry.memory, model.ledger_parameters, cursor);
    }
    return {evaluate_costing(entry.cpu, sizes), evaluate_costing(entry.memory, sizes)};
  }
  const auto found = model.parameters.find(builtin);
  const auto cpu = found != model.parameters.end() && !found->second.empty()
                       ? std::max<std::int64_t>(0, found->second.front())
                       : 100'000 + static_cast<std::int64_t>(arguments.size()) * 1'000;
  return {cpu,
          clamp_cost(100 + std::accumulate(sizes.begin(), sizes.end(), static_cast<__int128>(0)))};
}

core::Result<core::Bytes> encode_plutus_data(const UplcData& data) { return data.to_cbor(); }

core::Result<UplcData> validate_plutus_data_node(const core::cbor::Value& value,
                                                 std::size_t max_depth, bool enforce_wire_limit) {
  auto result = chain::validate_plutus_data_node(value, max_depth, enforce_wire_limit);
  if (!result) {
    return std::unexpected(result.error());
  }
  return Data::from_plutus_data(std::move(*result));
}

namespace {

class BitWriter {
 public:
  void bits(std::uint8_t value, std::size_t count) {
    for (std::size_t remaining = count; remaining > 0U; --remaining) {
      bit((value >> (remaining - 1U)) & 1U);
    }
  }

  void bit(std::uint8_t value) {
    if (offset_ == 0U) {
      bytes_.push_back(std::byte{0});
    }
    if (value != 0U) {
      bytes_.back() |= std::byte{static_cast<std::uint8_t>(1U << (7U - offset_))};
    }
    offset_ = (offset_ + 1U) % 8U;
  }

  void var_uint(std::uint64_t value) {
    do {
      auto byte = static_cast<std::uint8_t>(value & 0x7fU);
      value >>= 7U;
      if (value != 0U) {
        byte |= 0x80U;
      }
      bits(byte, 8U);
    } while (value != 0U);
  }

  core::VoidResult var_big(core::BigInteger value) {
    if (value.is_negative()) {
      return std::unexpected(
          uplc_error(core::ErrorCode::invalid_argument, "Flat natural number cannot be negative"));
    }
    const core::BigInteger radix(std::uint64_t{128});
    do {
      const auto remainder = (value % radix).to_uint64();
      if (!remainder) {
        return std::unexpected(remainder.error());
      }
      value /= radix;
      auto byte = static_cast<std::uint8_t>(*remainder);
      if (!value.is_zero()) {
        byte |= 0x80U;
      }
      bits(byte, 8U);
    } while (!value.is_zero());
    return std::monostate{};
  }

  void byte_string(core::ByteSpan value) {
    pad();
    std::size_t offset = 0U;
    while (offset < value.size()) {
      const auto length = std::min<std::size_t>(255U, value.size() - offset);
      bits(static_cast<std::uint8_t>(length), 8U);
      for (std::size_t index = 0; index < length; ++index) {
        bits(std::to_integer<std::uint8_t>(value[offset + index]), 8U);
      }
      offset += length;
    }
    bits(0U, 8U);
  }

  void pad() {
    while (offset_ != 7U) {
      bit(0U);
    }
    bit(1U);
  }

  [[nodiscard]] core::Bytes finish() {
    pad();
    return std::move(bytes_);
  }

 private:
  core::Bytes bytes_;
  std::size_t offset_{};
};

class BitReader {
 public:
  BitReader(core::ByteSpan bytes, const ProgramDecodeOptions& options)
      : bytes_(bytes), options_(options) {}

  [[nodiscard]] core::Result<std::uint8_t> bits(std::size_t count) {
    if (count > 8U || position_ + count > bytes_.size() * 8U) {
      return std::unexpected(
          uplc_error(core::ErrorCode::truncated_input, "truncated Flat bit stream"));
    }
    std::uint8_t result = 0U;
    for (std::size_t index = 0; index < count; ++index) {
      const auto byte = std::to_integer<std::uint8_t>(bytes_[(position_ + index) / 8U]);
      result = static_cast<std::uint8_t>((result << 1U) |
                                         ((byte >> (7U - ((position_ + index) % 8U))) & 1U));
    }
    position_ += count;
    return result;
  }

  [[nodiscard]] core::Result<std::uint64_t> var_uint() {
    std::uint64_t result = 0U;
    unsigned shift = 0U;
    for (std::size_t count = 0U; count < 10U; ++count) {
      auto byte = bits(8U);
      if (!byte) {
        return std::unexpected(byte.error());
      }
      if (shift == 63U && (*byte & 0x7eU) != 0U) {
        return std::unexpected(decode_error("Flat natural number overflows uint64"));
      }
      result |= static_cast<std::uint64_t>(*byte & 0x7fU) << shift;
      if ((*byte & 0x80U) == 0U) {
        if (count != 0U && (*byte & 0x7fU) == 0U) {
          return std::unexpected(decode_error("Flat natural number is not minimally encoded"));
        }
        return result;
      }
      shift += 7U;
    }
    return std::unexpected(decode_error("Flat natural number is too long"));
  }

  [[nodiscard]] core::Result<core::BigInteger> var_big() {
    core::BigInteger result(std::uint64_t{0});
    core::BigInteger multiplier(std::uint64_t{1});
    const core::BigInteger radix(std::uint64_t{128});
    for (std::size_t count = 0U; count < 37'450U; ++count) {
      auto byte = bits(8U);
      if (!byte) {
        return std::unexpected(byte.error());
      }
      result += core::BigInteger(static_cast<std::uint64_t>(*byte & 0x7fU)) * multiplier;
      if ((*byte & 0x80U) == 0U) {
        if (count != 0U && (*byte & 0x7fU) == 0U) {
          return std::unexpected(decode_error("Flat integer is not minimally encoded"));
        }
        return result;
      }
      multiplier *= radix;
    }
    return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                      "Flat integer exceeds the Cardano integer bound"));
  }

  [[nodiscard]] core::Result<core::Bytes> byte_string() {
    auto aligned = padding();
    if (!aligned) {
      return std::unexpected(aligned.error());
    }
    core::Bytes result;
    while (true) {
      auto length = bits(8U);
      if (!length) {
        return std::unexpected(length.error());
      }
      if (*length == 0U) {
        return result;
      }
      if (result.size() + *length > options_.max_constant_bytes) {
        return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                          "Flat byte string exceeds the configured limit"));
      }
      for (std::size_t index = 0U; index < *length; ++index) {
        auto byte = bits(8U);
        if (!byte) {
          return std::unexpected(byte.error());
        }
        result.push_back(std::byte{*byte});
      }
    }
  }

  [[nodiscard]] core::VoidResult padding() {
    while ((position_ + 1U) % 8U != 0U) {
      auto fill = bits(1U);
      if (!fill) {
        return std::unexpected(fill.error());
      }
      if (*fill != 0U) {
        return std::unexpected(decode_error("nonzero Flat padding bit"));
      }
    }
    auto marker = bits(1U);
    if (!marker) {
      return std::unexpected(marker.error());
    }
    if (*marker != 1U) {
      return std::unexpected(decode_error("invalid Flat padding marker"));
    }
    return std::monostate{};
  }

  [[nodiscard]] std::size_t position() const noexcept { return position_; }
  [[nodiscard]] std::size_t size_bits() const noexcept { return bytes_.size() * 8U; }

 private:
  core::ByteSpan bytes_;
  const ProgramDecodeOptions& options_;
  std::size_t position_{};
};

void flatten_type(const UplcType& type, std::vector<UplcTypeTag>& tags) {
  tags.push_back(type.tag());
  for (const auto& argument : type.arguments()) {
    flatten_type(argument, tags);
  }
}

core::VoidResult encode_type(BitWriter& writer, const UplcType& type,
                             const ProgramDecodeOptions& options) {
  if (!type.is_value_type()) {
    return std::unexpected(uplc_error(core::ErrorCode::invalid_structure,
                                      "UPLC constant has a non-value universe type"));
  }
  std::vector<UplcTypeTag> tags;
  flatten_type(type, tags);
  if (tags.size() > options.max_universe_header) {
    return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                      "UPLC universe header exceeds the configured limit"));
  }
  for (const auto tag : tags) {
    writer.bit(1U);
    writer.bits(static_cast<std::uint8_t>(tag), 4U);
  }
  writer.bit(0U);
  return std::monostate{};
}

core::Result<UplcType> parse_type_tags(std::span<const UplcTypeTag> tags, std::size_t& cursor) {
  if (cursor >= tags.size()) {
    return std::unexpected(decode_error("incomplete UPLC universe type"));
  }
  const auto tag = tags[cursor++];
  if (tag == UplcTypeTag::apply) {
    auto function = parse_type_tags(tags, cursor);
    if (!function) {
      return std::unexpected(function.error());
    }
    auto argument = parse_type_tags(tags, cursor);
    if (!argument) {
      return std::unexpected(argument.error());
    }
    return UplcType::apply(std::move(*function), std::move(*argument));
  }
  return UplcType::primitive(tag);
}

core::Result<UplcType> decode_type(BitReader& reader, const ProgramDecodeOptions& options) {
  std::vector<UplcTypeTag> tags;
  while (true) {
    auto more = reader.bits(1U);
    if (!more) {
      return std::unexpected(more.error());
    }
    if (*more == 0U) {
      break;
    }
    auto tag = reader.bits(4U);
    if (!tag) {
      return std::unexpected(tag.error());
    }
    if (*tag > static_cast<std::uint8_t>(UplcTypeTag::value)) {
      return std::unexpected(decode_error("unknown UPLC universe tag"));
    }
    tags.push_back(static_cast<UplcTypeTag>(*tag));
    if (tags.size() > options.max_universe_header) {
      return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                        "UPLC universe header exceeds the configured limit"));
    }
  }
  std::size_t cursor = 0U;
  auto type = parse_type_tags(tags, cursor);
  if (!type) {
    return std::unexpected(type.error());
  }
  if (cursor != tags.size() || !type->is_value_type()) {
    return std::unexpected(decode_error("invalid UPLC universe type"));
  }
  return type;
}

core::VoidResult encode_constant_value(BitWriter& writer, const UplcConstant& constant);

core::VoidResult encode_collection(BitWriter& writer, const UplcConstant::Items& items) {
  for (const auto& item : items) {
    writer.bit(1U);
    auto encoded = encode_constant_value(writer, item);
    if (!encoded) {
      return encoded;
    }
  }
  writer.bit(0U);
  return std::monostate{};
}

core::VoidResult encode_constant_value(BitWriter& writer, const UplcConstant& constant) {
  return std::visit(
      [&](const auto& value) -> core::VoidResult {
        using Value = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Value, core::BigInteger>) {
          const auto natural = value.is_negative() ? (-value * core::BigInteger(std::uint64_t{2})) -
                                                         core::BigInteger(std::uint64_t{1})
                                                   : value * core::BigInteger(std::uint64_t{2});
          return writer.var_big(natural);
        } else if constexpr (std::is_same_v<Value, core::Bytes>) {
          writer.byte_string(value);
          return std::monostate{};
        } else if constexpr (std::is_same_v<Value, std::string>) {
          writer.byte_string(std::as_bytes(std::span(value)));
          return std::monostate{};
        } else if constexpr (std::is_same_v<Value, std::monostate>) {
          return std::monostate{};
        } else if constexpr (std::is_same_v<Value, bool>) {
          writer.bit(value ? 1U : 0U);
          return std::monostate{};
        } else if constexpr (std::is_same_v<Value, UplcConstant::Items>) {
          return encode_collection(writer, value);
        } else if constexpr (std::is_same_v<Value, UplcPair>) {
          auto first = encode_constant_value(writer, value.first());
          return first ? encode_constant_value(writer, value.second()) : first;
        } else {
          auto cbor = value.to_cbor();
          if (!cbor) {
            return std::unexpected(cbor.error());
          }
          writer.byte_string(*cbor);
          return std::monostate{};
        }
      },
      constant.value());
}

core::VoidResult encode_constant(BitWriter& writer, const UplcConstant& constant,
                                 const ProgramDecodeOptions& options) {
  auto type = encode_type(writer, constant.type(), options);
  return type ? encode_constant_value(writer, constant) : type;
}

core::Result<UplcConstant> decode_constant_value(BitReader& reader, const UplcType& type,
                                                 const ProgramDecodeOptions& options,
                                                 std::size_t depth);

core::VoidResult validate_data_wire(const core::cbor::Value& value, bool enforce_wire_limit,
                                    std::size_t depth = 0U) {
  if (depth > 128U) {
    return std::unexpected(
        uplc_error(core::ErrorCode::depth_limit_exceeded, "Plutus Data is too deeply nested"));
  }
  if (const auto* byte_string = value.as_byte_string()) {
    const auto valid =
        !enforce_wire_limit || (!byte_string->encoding.indefinite
                                    ? byte_string->value.size() <= 64U
                                    : std::ranges::all_of(byte_string->encoding.chunks,
                                                          [](const core::cbor::ByteChunk& chunk) {
                                                            return chunk.value.size() <= 64U;
                                                          }));
    return valid ? core::VoidResult(std::monostate{})
                 : std::unexpected(uplc_error(core::ErrorCode::out_of_range,
                                              "Plutus Data byte chunks are limited to 64 bytes"));
  }
  if (const auto* values = value.as_array()) {
    for (std::size_t index = 0U; index < values->values.size(); ++index) {
      auto valid = validate_data_wire(values->values[index], enforce_wire_limit, depth + 1U);
      if (!valid) {
        return std::unexpected(valid.error().at(index));
      }
    }
  } else if (const auto* entries = value.as_map()) {
    for (std::size_t index = 0U; index < entries->entries.size(); ++index) {
      auto key = validate_data_wire(entries->entries[index].first, enforce_wire_limit, depth + 1U);
      auto item =
          validate_data_wire(entries->entries[index].second, enforce_wire_limit, depth + 1U);
      if (!key || !item) {
        return std::unexpected((!key ? key.error() : item.error()).at(index));
      }
    }
  } else if (const auto* tag = value.as_tag()) {
    return validate_data_wire(*tag->value, enforce_wire_limit, depth + 1U);
  }
  return std::monostate{};
}

core::Result<UplcConstant::Items> decode_collection(BitReader& reader, const UplcType& item_type,
                                                    const ProgramDecodeOptions& options,
                                                    std::size_t depth) {
  UplcConstant::Items items;
  while (true) {
    auto more = reader.bits(1U);
    if (!more) {
      return std::unexpected(more.error());
    }
    if (*more == 0U) {
      return items;
    }
    if (items.size() >= options.max_nodes) {
      return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                        "UPLC collection exceeds the configured node limit"));
    }
    auto item = decode_constant_value(reader, item_type, options, depth + 1U);
    if (!item) {
      return std::unexpected(item.error().at(items.size()));
    }
    items.push_back(std::move(*item));
  }
}

core::Result<UplcConstant> decode_constant_value(BitReader& reader, const UplcType& type,
                                                 const ProgramDecodeOptions& options,
                                                 std::size_t depth) {
  if (depth > options.max_depth) {
    return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                      "UPLC constant exceeds the configured depth"));
  }
  switch (type.tag()) {
    case UplcTypeTag::integer: {
      auto natural = reader.var_big();
      if (!natural) {
        return std::unexpected(natural.error());
      }
      const auto odd = (*natural % core::BigInteger(std::uint64_t{2})).to_uint64();
      if (!odd) {
        return std::unexpected(odd.error());
      }
      auto value = *natural / core::BigInteger(std::uint64_t{2});
      if (*odd != 0U) {
        value = -value - core::BigInteger(std::uint64_t{1});
      }
      return UplcConstant::integer(std::move(value));
    }
    case UplcTypeTag::byte_string: {
      auto value = reader.byte_string();
      return value ? core::Result<UplcConstant>(UplcConstant::bytes(std::move(*value)))
                   : std::unexpected(value.error());
    }
    case UplcTypeTag::string: {
      auto bytes = reader.byte_string();
      if (!bytes) {
        return std::unexpected(bytes.error());
      }
      const std::string value(reinterpret_cast<const char*>(bytes->data()), bytes->size());
      return UplcConstant::string(value);
    }
    case UplcTypeTag::unit:
      return UplcConstant::unit();
    case UplcTypeTag::boolean: {
      auto value = reader.bits(1U);
      return value ? core::Result<UplcConstant>(UplcConstant::boolean(*value != 0U))
                   : std::unexpected(value.error());
    }
    case UplcTypeTag::data: {
      auto bytes = reader.byte_string();
      if (!bytes) {
        return std::unexpected(bytes.error());
      }
      auto decoded = core::cbor::decode_cbor(*bytes);
      if (!decoded) {
        return std::unexpected(decoded.error());
      }
      auto valid_wire = validate_data_wire(*decoded, options.enforce_data_wire_limit);
      if (!valid_wire) {
        return std::unexpected(valid_wire.error());
      }
      auto data = validate_plutus_data_node(*decoded, 128U, options.enforce_data_wire_limit);
      return data ? core::Result<UplcConstant>(UplcConstant::data(std::move(*data)))
                  : std::unexpected(data.error());
    }
    case UplcTypeTag::bls12_381_g1:
    case UplcTypeTag::bls12_381_g2:
    case UplcTypeTag::bls12_381_ml_result: {
      auto bytes = reader.byte_string();
      if (!bytes) {
        return std::unexpected(bytes.error());
      }
      if (type.tag() == UplcTypeTag::bls12_381_g1) {
        return UplcConstant::bls12_381_g1(std::move(*bytes));
      }
      if (type.tag() == UplcTypeTag::bls12_381_g2) {
        return UplcConstant::bls12_381_g2(std::move(*bytes));
      }
      return UplcConstant::bls12_381_ml_result(std::move(*bytes));
    }
    case UplcTypeTag::value: {
      auto bytes = reader.byte_string();
      if (!bytes) {
        return std::unexpected(bytes.error());
      }
      auto data = Data::from_cbor(*bytes);
      return data ? UplcConstant::value(std::move(*data))
                  : core::Result<UplcConstant>(std::unexpected(data.error()));
    }
    case UplcTypeTag::apply: {
      if (type.arguments().size() != 2U) {
        break;
      }
      const auto& function = type.arguments()[0];
      if (function.tag() == UplcTypeTag::proto_list || function.tag() == UplcTypeTag::proto_array) {
        auto items = decode_collection(reader, type.arguments()[1], options, depth);
        if (!items) {
          return std::unexpected(items.error());
        }
        return function.tag() == UplcTypeTag::proto_list
                   ? UplcConstant::list(type.arguments()[1], std::move(*items))
                   : UplcConstant::array(type.arguments()[1], std::move(*items));
      }
      if (function.tag() == UplcTypeTag::apply && function.arguments().size() == 2U &&
          function.arguments()[0].tag() == UplcTypeTag::proto_pair) {
        auto first = decode_constant_value(reader, function.arguments()[1], options, depth + 1U);
        if (!first) {
          return std::unexpected(first.error());
        }
        auto second = decode_constant_value(reader, type.arguments()[1], options, depth + 1U);
        if (!second) {
          return std::unexpected(second.error());
        }
        return UplcConstant::pair(std::move(*first), std::move(*second));
      }
      break;
    }
    case UplcTypeTag::proto_list:
    case UplcTypeTag::proto_pair:
    case UplcTypeTag::proto_array:
      break;
  }
  return std::unexpected(decode_error("invalid UPLC constant universe type"));
}

std::uint8_t maximum_builtin_tag(const UplcVersion& version,
                                 const std::optional<std::uint64_t>& protocol_major) {
  if (protocol_major) {
    if (*protocol_major <= 6U) {
      return 50U;
    }
    if (*protocol_major <= 8U) {
      return 53U;
    }
    if (*protocol_major == 9U) {
      return 70U;
    }
    if (*protocol_major == 10U) {
      return 87U;
    }
    return 100U;
  }
  static_cast<void>(version);
  return 100U;
}

core::VoidResult encode_term(BitWriter& writer, const UplcTerm& term, const UplcVersion& version,
                             const ProgramDecodeOptions& options, std::size_t depth,
                             std::size_t& nodes) {
  if (depth > options.max_depth) {
    return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                      "UPLC term exceeds the configured depth"));
  }
  if (++nodes > options.max_nodes) {
    return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                      "UPLC program exceeds the configured node limit"));
  }
  return std::visit(
      [&](const auto& node) -> core::VoidResult {
        using Node = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<Node, UplcVariable>) {
          if (node.index == 0U) {
            return std::unexpected(uplc_error(core::ErrorCode::invalid_structure,
                                              "UPLC De Bruijn indexes start at one"));
          }
          writer.bits(0U, 4U);
          writer.var_uint(node.index);
        } else if constexpr (std::is_same_v<Node, UplcDelay>) {
          writer.bits(1U, 4U);
          return encode_term(writer, *node.term, version, options, depth + 1U, nodes);
        } else if constexpr (std::is_same_v<Node, UplcLambda>) {
          writer.bits(2U, 4U);
          return encode_term(writer, *node.body, version, options, depth + 1U, nodes);
        } else if constexpr (std::is_same_v<Node, UplcApply>) {
          writer.bits(3U, 4U);
          auto function = encode_term(writer, *node.function, version, options, depth + 1U, nodes);
          return function ? encode_term(writer, *node.argument, version, options, depth + 1U, nodes)
                          : function;
        } else if constexpr (std::is_same_v<Node, UplcConstant>) {
          writer.bits(4U, 4U);
          return encode_constant(writer, node, options);
        } else if constexpr (std::is_same_v<Node, UplcForce>) {
          writer.bits(5U, 4U);
          return encode_term(writer, *node.term, version, options, depth + 1U, nodes);
        } else if constexpr (std::is_same_v<Node, UplcError>) {
          writer.bits(6U, 4U);
        } else if constexpr (std::is_same_v<Node, Builtin>) {
          if (builtin_tag(node) > maximum_builtin_tag(version, options.protocol_major)) {
            return std::unexpected(
                uplc_error(core::ErrorCode::unsupported,
                           "UPLC builtin is unavailable for this protocol version"));
          }
          writer.bits(7U, 4U);
          writer.bits(builtin_tag(node), 7U);
        } else if constexpr (std::is_same_v<Node, UplcConstr>) {
          if (version < UplcVersion::v1_1_0()) {
            return std::unexpected(uplc_error(core::ErrorCode::unsupported,
                                              "UPLC constr is unavailable before version 1.1.0"));
          }
          if (node.fields.size() > options.max_constr_fields) {
            return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                              "UPLC constr exceeds the configured field limit"));
          }
          writer.bits(8U, 4U);
          writer.var_uint(node.tag);
          for (const auto& field : node.fields) {
            writer.bit(1U);
            auto encoded = encode_term(writer, field, version, options, depth + 1U, nodes);
            if (!encoded) {
              return encoded;
            }
          }
          writer.bit(0U);
        } else {
          if (version < UplcVersion::v1_1_0()) {
            return std::unexpected(uplc_error(core::ErrorCode::unsupported,
                                              "UPLC case is unavailable before version 1.1.0"));
          }
          writer.bits(9U, 4U);
          auto scrutinee =
              encode_term(writer, *node.scrutinee, version, options, depth + 1U, nodes);
          if (!scrutinee) {
            return scrutinee;
          }
          for (const auto& branch : node.branches) {
            writer.bit(1U);
            auto encoded = encode_term(writer, branch, version, options, depth + 1U, nodes);
            if (!encoded) {
              return encoded;
            }
          }
          writer.bit(0U);
        }
        return std::monostate{};
      },
      term.node());
}

core::Result<UplcTerm> decode_term(BitReader& reader, const UplcVersion& version,
                                   const ProgramDecodeOptions& options, std::size_t depth,
                                   std::size_t& nodes) {
  if (depth > options.max_depth) {
    return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                      "UPLC term exceeds the configured depth"));
  }
  if (++nodes > options.max_nodes) {
    return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                      "UPLC program exceeds the configured node limit"));
  }
  auto tag = reader.bits(4U);
  if (!tag) {
    return std::unexpected(tag.error());
  }
  switch (*tag) {
    case 0: {
      auto index = reader.var_uint();
      if (!index) {
        return std::unexpected(index.error());
      }
      if (*index == 0U) {
        return std::unexpected(decode_error("UPLC De Bruijn indexes start at one"));
      }
      return UplcTerm::variable(*index);
    }
    case 1: {
      auto term = decode_term(reader, version, options, depth + 1U, nodes);
      return term ? core::Result<UplcTerm>(UplcTerm::delay(std::move(*term)))
                  : std::unexpected(term.error());
    }
    case 2: {
      auto body = decode_term(reader, version, options, depth + 1U, nodes);
      return body ? core::Result<UplcTerm>(UplcTerm::lambda(std::move(*body)))
                  : std::unexpected(body.error());
    }
    case 3: {
      auto function = decode_term(reader, version, options, depth + 1U, nodes);
      if (!function) {
        return std::unexpected(function.error());
      }
      auto argument = decode_term(reader, version, options, depth + 1U, nodes);
      return argument ? core::Result<UplcTerm>(
                            UplcTerm::apply(std::move(*function), std::move(*argument)))
                      : std::unexpected(argument.error());
    }
    case 4: {
      auto type = decode_type(reader, options);
      if (!type) {
        return std::unexpected(type.error());
      }
      auto constant = decode_constant_value(reader, *type, options, depth);
      return constant ? core::Result<UplcTerm>(UplcTerm::constant(std::move(*constant)))
                      : std::unexpected(constant.error());
    }
    case 5: {
      auto term = decode_term(reader, version, options, depth + 1U, nodes);
      return term ? core::Result<UplcTerm>(UplcTerm::force(std::move(*term)))
                  : std::unexpected(term.error());
    }
    case 6:
      return UplcTerm::error();
    case 7: {
      auto tag_value = reader.bits(7U);
      if (!tag_value) {
        return std::unexpected(tag_value.error());
      }
      if (*tag_value > maximum_builtin_tag(version, options.protocol_major)) {
        return std::unexpected(
            decode_error("UPLC builtin is unavailable for this protocol version"));
      }
      auto builtin = builtin_tag(*tag_value);
      return builtin ? core::Result<UplcTerm>(UplcTerm::builtin(*builtin))
                     : std::unexpected(builtin.error());
    }
    case 8: {
      if (version < UplcVersion::v1_1_0()) {
        return std::unexpected(decode_error("UPLC constr is unavailable before version 1.1.0"));
      }
      auto constructor = reader.var_uint();
      if (!constructor) {
        return std::unexpected(constructor.error());
      }
      std::vector<UplcTerm> fields;
      while (true) {
        auto more = reader.bits(1U);
        if (!more) {
          return std::unexpected(more.error());
        }
        if (*more == 0U) {
          break;
        }
        if (fields.size() >= options.max_constr_fields) {
          return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                            "UPLC constr exceeds the configured field limit"));
        }
        auto field = decode_term(reader, version, options, depth + 1U, nodes);
        if (!field) {
          return std::unexpected(field.error().at(fields.size()));
        }
        fields.push_back(std::move(*field));
      }
      return UplcTerm::constr(*constructor, std::move(fields));
    }
    case 9: {
      if (version < UplcVersion::v1_1_0()) {
        return std::unexpected(decode_error("UPLC case is unavailable before version 1.1.0"));
      }
      auto scrutinee = decode_term(reader, version, options, depth + 1U, nodes);
      if (!scrutinee) {
        return std::unexpected(scrutinee.error());
      }
      std::vector<UplcTerm> branches;
      while (true) {
        auto more = reader.bits(1U);
        if (!more) {
          return std::unexpected(more.error());
        }
        if (*more == 0U) {
          break;
        }
        auto branch = decode_term(reader, version, options, depth + 1U, nodes);
        if (!branch) {
          return std::unexpected(branch.error().at(branches.size()));
        }
        branches.push_back(std::move(*branch));
      }
      return UplcTerm::case_of(std::move(*scrutinee), std::move(branches));
    }
    default:
      return std::unexpected(decode_error("unknown UPLC term tag"));
  }
}

core::VoidResult validate_version(const UplcVersion& version) {
  if (version != UplcVersion::v1_0_0() && version != UplcVersion::v1_1_0()) {
    return std::unexpected(uplc_error(core::ErrorCode::unsupported,
                                      "only UPLC versions 1.0.0 and 1.1.0 are supported"));
  }
  return std::monostate{};
}

}  // namespace

core::Result<core::Bytes> encode_flat_program(const UplcProgram& program,
                                              ProgramDecodeOptions options) {
  auto version = validate_version(program.version);
  if (!version) {
    return std::unexpected(version.error());
  }
  BitWriter writer;
  writer.var_uint(program.version.major);
  writer.var_uint(program.version.minor);
  writer.var_uint(program.version.patch);
  std::size_t nodes = 0U;
  auto term = encode_term(writer, program.term, program.version, options, 0U, nodes);
  return term ? core::Result<core::Bytes>(writer.finish()) : std::unexpected(term.error());
}

core::Result<UplcProgram> decode_flat_program(core::ByteSpan bytes, ProgramDecodeOptions options) {
  if (bytes.empty()) {
    return std::unexpected(uplc_error(core::ErrorCode::truncated_input, "empty Flat program"));
  }
  BitReader reader(bytes, options);
  auto major = reader.var_uint();
  auto minor = reader.var_uint();
  auto patch = reader.var_uint();
  if (!major || !minor || !patch) {
    return std::unexpected(!major ? major.error() : (!minor ? minor.error() : patch.error()));
  }
  const UplcVersion version{*major, *minor, *patch};
  auto valid_version = validate_version(version);
  if (!valid_version) {
    return std::unexpected(valid_version.error());
  }
  std::size_t nodes = 0U;
  auto term = decode_term(reader, version, options, 0U, nodes);
  if (!term) {
    return std::unexpected(term.error());
  }
  auto padding = reader.padding();
  if (!padding) {
    return std::unexpected(padding.error());
  }
  if (options.require_complete_input && reader.position() != reader.size_bits()) {
    return std::unexpected(
        uplc_error(core::ErrorCode::trailing_data, "trailing bytes after Flat program"));
  }
  return UplcProgram{version, std::move(*term)};
}

core::Result<core::Bytes> encode_program_envelope(const UplcProgram& program,
                                                  ProgramDecodeOptions options) {
  auto flat = encode_flat_program(program, options);
  if (!flat) {
    return std::unexpected(flat.error());
  }
  return core::cbor::encode_cbor(core::cbor::Value::byte_string(std::move(*flat)),
                                 {.mode = core::cbor::Mode::canonical});
}

core::Result<UplcProgram> decode_program_envelope(core::ByteSpan bytes,
                                                  ProgramDecodeOptions options) {
  auto decoded = core::cbor::decode_cbor(bytes);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  const auto* byte_string = decoded->as_byte_string();
  if (byte_string == nullptr) {
    return std::unexpected(decode_error("serialized UPLC program must be one CBOR byte string"));
  }
  return decode_flat_program(byte_string->value, options);
}

core::Result<UplcProgram> decode_program_envelope_compatible(core::ByteSpan bytes,
                                                             std::uint64_t plutus_language,
                                                             ProgramDecodeOptions options) {
  if (plutus_language < 1U || plutus_language > 3U) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_argument, "Plutus language must be V1, V2, or V3"));
  }
  if (plutus_language == 3U) {
    return decode_program_envelope(bytes, options);
  }
  // Historical V1/V2 validation decodes the first definite byte string and
  // deliberately ignores trailing bytes after that CBOR object.
  if (bytes.empty()) {
    return std::unexpected(
        uplc_error(core::ErrorCode::truncated_input, "empty serialized UPLC program"));
  }
  const auto first = std::to_integer<std::uint8_t>(bytes.front());
  if ((first >> 5U) != 2U || (first & 0x1fU) == 31U) {
    return std::unexpected(
        decode_error("serialized UPLC program must start with a definite CBOR byte string"));
  }
  std::size_t header = 1U;
  std::uint64_t length = first & 0x1fU;
  const auto additional = first & 0x1fU;
  if (additional >= 24U) {
    const std::size_t length_bytes = additional == 24U   ? 1U
                                     : additional == 25U ? 2U
                                     : additional == 26U ? 4U
                                     : additional == 27U ? 8U
                                                         : 0U;
    if (length_bytes == 0U || bytes.size() < 1U + length_bytes) {
      return std::unexpected(uplc_error(core::ErrorCode::truncated_input,
                                        "truncated serialized UPLC byte-string header"));
    }
    header += length_bytes;
    length = 0U;
    for (std::size_t index = 0U; index < length_bytes; ++index) {
      length = (length << 8U) | std::to_integer<std::uint8_t>(bytes[1U + index]);
    }
  }
  if (length > bytes.size() - header) {
    return std::unexpected(
        uplc_error(core::ErrorCode::truncated_input, "truncated serialized UPLC byte string"));
  }
  return decode_flat_program(bytes.subspan(header, length), options);
}

namespace {

core::VoidResult require_closed_term(const UplcTerm& root) {
  struct Pending {
    const UplcTerm* term;
    std::uint64_t lambda_depth;
  };
  std::vector<Pending> pending{{&root, 0U}};
  while (!pending.empty()) {
    const auto [term, lambda_depth] = pending.back();
    pending.pop_back();
    const auto result = std::visit(
        [&pending, lambda_depth](const auto& node) -> core::VoidResult {
          using Node = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<Node, UplcVariable>) {
            if (node.index == 0U || node.index > lambda_depth) {
              return std::unexpected(
                  uplc_error(core::ErrorCode::invalid_structure,
                             "parameter application produced a program with a free variable"));
            }
          } else if constexpr (std::is_same_v<Node, UplcDelay>) {
            pending.push_back({node.term.get(), lambda_depth});
          } else if constexpr (std::is_same_v<Node, UplcLambda>) {
            if (lambda_depth == std::numeric_limits<std::uint64_t>::max()) {
              return std::unexpected(uplc_error(core::ErrorCode::out_of_range,
                                                "UPLC lambda nesting exceeds the supported range"));
            }
            pending.push_back({node.body.get(), lambda_depth + 1U});
          } else if constexpr (std::is_same_v<Node, UplcApply>) {
            pending.push_back({node.argument.get(), lambda_depth});
            pending.push_back({node.function.get(), lambda_depth});
          } else if constexpr (std::is_same_v<Node, UplcForce>) {
            pending.push_back({node.term.get(), lambda_depth});
          } else if constexpr (std::is_same_v<Node, UplcConstr>) {
            for (const auto& field : node.fields) {
              pending.push_back({&field, lambda_depth});
            }
          } else if constexpr (std::is_same_v<Node, UplcCase>) {
            pending.push_back({node.scrutinee.get(), lambda_depth});
            for (const auto& branch : node.branches) {
              pending.push_back({&branch, lambda_depth});
            }
          }
          return std::monostate{};
        },
        term->node());
    if (!result) {
      return result;
    }
  }
  return std::monostate{};
}

}  // namespace

core::Result<core::Bytes> apply_params_to_script(core::ByteSpan script_envelope,
                                                 std::span<const Data> parameters,
                                                 ProgramDecodeOptions options) {
  auto program = decode_program_envelope(script_envelope, options);
  if (!program) {
    return std::unexpected(program.error());
  }
  for (const auto& parameter : parameters) {
    program->term = UplcTerm::apply(std::move(program->term),
                                    UplcTerm::constant(UplcConstant::data(parameter)));
  }
  const auto closed = require_closed_term(program->term);
  if (!closed) {
    return std::unexpected(closed.error());
  }
  return encode_program_envelope(*program, options);
}

core::Result<core::Bytes> apply_params_to_script(core::ByteSpan parameters_cbor,
                                                 core::ByteSpan script_envelope,
                                                 ProgramDecodeOptions options) {
  constexpr std::size_t MAX_STANDALONE_INPUT_BYTES = 16U * 1024U * 1024U;
  if (parameters_cbor.size() > MAX_STANDALONE_INPUT_BYTES ||
      script_envelope.size() > MAX_STANDALONE_INPUT_BYTES) {
    return std::unexpected(
        uplc_error(core::ErrorCode::out_of_range, "parameter application input exceeds 16 MiB"));
  }

  auto decoded = core::cbor::decode_cbor(parameters_cbor);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  const auto* array = decoded->as_array();
  if (array == nullptr) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_structure, "script parameters must be one CBOR array"));
  }
  std::vector<Data> parameters;
  parameters.reserve(array->values.size());
  for (std::size_t index = 0U; index < array->values.size(); ++index) {
    auto valid_wire = validate_data_wire(array->values[index], options.enforce_data_wire_limit);
    if (!valid_wire) {
      return std::unexpected(valid_wire.error().at(index));
    }
    auto parameter = validate_plutus_data_node(array->values[index], options.max_depth);
    if (!parameter) {
      return std::unexpected(parameter.error().at(index));
    }
    parameters.push_back(std::move(*parameter));
  }
  return apply_params_to_script(script_envelope, std::span<const Data>(parameters), options);
}

namespace {

struct TextToken {
  std::string text;
  bool quoted{};
  std::size_t offset{};
};

core::VoidResult append_utf8_codepoint(std::string& output, std::uint32_t codepoint) {
  if (codepoint > 0x10ffffU || (codepoint >= 0xd800U && codepoint <= 0xdfffU)) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_utf8, "invalid Unicode code point in UPLC string"));
  }
  if (codepoint <= 0x7fU) {
    output.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7ffU) {
    output.push_back(static_cast<char>(0xc0U | (codepoint >> 6U)));
    output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
  } else if (codepoint <= 0xffffU) {
    output.push_back(static_cast<char>(0xe0U | (codepoint >> 12U)));
    output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
  } else {
    output.push_back(static_cast<char>(0xf0U | (codepoint >> 18U)));
    output.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3fU)));
    output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
  }
  return std::monostate{};
}

core::Result<std::vector<TextToken>> tokenize_uplc(std::string_view text) {
  std::vector<TextToken> tokens;
  std::size_t cursor = 0U;
  while (cursor < text.size()) {
    if (std::isspace(static_cast<unsigned char>(text[cursor])) != 0) {
      ++cursor;
      continue;
    }
    if (text[cursor] == '-' && cursor + 1U < text.size() && text[cursor + 1U] == '-') {
      cursor += 2U;
      while (cursor < text.size() && text[cursor] != '\n') {
        ++cursor;
      }
      continue;
    }
    if (text[cursor] == '(' || text[cursor] == ')' || text[cursor] == '[' || text[cursor] == ']' ||
        text[cursor] == ',') {
      tokens.push_back({std::string(1U, text[cursor]), false, cursor++});
      continue;
    }
    if (text[cursor] == '"') {
      const auto start = cursor++;
      std::string value;
      bool complete = false;
      while (cursor < text.size()) {
        const auto character = text[cursor++];
        if (character == '"') {
          complete = true;
          break;
        }
        if (character != '\\') {
          value.push_back(character);
          continue;
        }
        if (cursor >= text.size()) {
          break;
        }
        const auto escaped = text[cursor++];
        switch (escaped) {
          case '"':
          case '\\':
          case '/':
            value.push_back(escaped);
            break;
          case 'n':
            value.push_back('\n');
            break;
          case 'r':
            value.push_back('\r');
            break;
          case 't':
            value.push_back('\t');
            break;
          case 'b':
            value.push_back('\b');
            break;
          case 'f':
            value.push_back('\f');
            break;
          case 'a':
            value.push_back('\a');
            break;
          case '&':
            break;
          default: {
            unsigned base = 0U;
            std::size_t digits_start = cursor - 1U;
            if (escaped == 'x') {
              base = 16U;
              digits_start = cursor;
            } else if (escaped == 'o') {
              base = 8U;
              digits_start = cursor;
            } else if (std::isdigit(static_cast<unsigned char>(escaped)) != 0) {
              base = 10U;
            }
            if (base == 0U) {
              std::string name(1U, escaped);
              while (cursor < text.size() &&
                     std::isalpha(static_cast<unsigned char>(text[cursor])) != 0) {
                name.push_back(text[cursor++]);
              }
              constexpr std::array<std::string_view, 33> control_names{
                  "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS",  "HT",  "LF",
                  "VT",  "FF",  "CR",  "SO",  "SI",  "DLE", "DC1", "DC2", "DC3", "DC4", "NAK",
                  "SYN", "ETB", "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",  "SP"};
              const auto named = std::find(control_names.begin(), control_names.end(), name);
              if (named != control_names.end()) {
                value.push_back(static_cast<char>(named - control_names.begin()));
                break;
              }
              if (name == "DEL") {
                value.push_back(static_cast<char>(0x7f));
                break;
              }
              return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                                        "unsupported escape in UPLC string literal",
                                                        {}, cursor - name.size()));
            }
            auto digit_value = [base](char digit) -> std::optional<unsigned> {
              unsigned value = 0U;
              if (digit >= '0' && digit <= '9') {
                value = static_cast<unsigned>(digit - '0');
              } else if (digit >= 'a' && digit <= 'f') {
                value = static_cast<unsigned>(digit - 'a' + 10);
              } else if (digit >= 'A' && digit <= 'F') {
                value = static_cast<unsigned>(digit - 'A' + 10);
              } else {
                return std::nullopt;
              }
              return value < base ? std::optional<unsigned>(value) : std::nullopt;
            };
            std::uint32_t codepoint = 0U;
            std::size_t digit_cursor = digits_start;
            while (digit_cursor < text.size()) {
              auto digit = digit_value(text[digit_cursor]);
              if (!digit) break;
              if (codepoint > (0x10ffffU - *digit) / base) {
                return std::unexpected(core::CardanoError(
                    core::ErrorCode::invalid_utf8, "numeric UPLC string escape is out of range", {},
                    digits_start));
              }
              codepoint = codepoint * base + *digit;
              ++digit_cursor;
            }
            if (digit_cursor == digits_start) {
              return std::unexpected(core::CardanoError(core::ErrorCode::invalid_encoding,
                                                        "numeric UPLC string escape has no digits",
                                                        {}, digits_start));
            }
            cursor = digit_cursor;
            auto appended = append_utf8_codepoint(value, codepoint);
            if (!appended) return std::unexpected(appended.error());
            break;
          }
        }
      }
      if (!complete) {
        return std::unexpected(core::CardanoError(core::ErrorCode::truncated_input,
                                                  "unterminated UPLC string literal", {}, start));
      }
      tokens.push_back({std::move(value), true, start});
      continue;
    }
    const auto start = cursor;
    while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor])) == 0 &&
           text[cursor] != '(' && text[cursor] != ')' && text[cursor] != '[' &&
           text[cursor] != ']' && text[cursor] != ',') {
      ++cursor;
    }
    tokens.push_back({std::string(text.substr(start, cursor - start)), false, start});
  }
  return tokens;
}

class TextParser {
 public:
  TextParser(std::vector<TextToken> tokens, ProgramDecodeOptions options)
      : tokens_(std::move(tokens)), options_(std::move(options)) {}

  core::Result<UplcProgram> program() {
    auto open = consume("(");
    if (!open) {
      return std::unexpected(open.error());
    }
    auto keyword = consume("program");
    if (!keyword) {
      return std::unexpected(keyword.error());
    }
    auto version = parse_version();
    if (!version) {
      return std::unexpected(version.error());
    }
    auto valid = validate_version(*version);
    if (!valid) {
      return std::unexpected(valid.error());
    }
    std::vector<std::string> scope;
    auto term = parse_term(*version, scope, 0U);
    if (!term) {
      return std::unexpected(term.error());
    }
    auto close = consume(")");
    if (!close) {
      return std::unexpected(close.error());
    }
    if (cursor_ != tokens_.size()) {
      return std::unexpected(error("trailing tokens after UPLC program"));
    }
    return UplcProgram{*version, std::move(*term)};
  }

 private:
  [[nodiscard]] core::CardanoError error(std::string message) const {
    const auto offset = cursor_ < tokens_.size()
                            ? std::optional<std::size_t>(tokens_[cursor_].offset)
                            : std::nullopt;
    return core::CardanoError(core::ErrorCode::invalid_encoding, std::move(message), {}, offset);
  }

  core::VoidResult consume(std::string_view expected) {
    if (cursor_ >= tokens_.size() || tokens_[cursor_].quoted || tokens_[cursor_].text != expected) {
      return std::unexpected(error("expected '" + std::string(expected) + "' in UPLC text"));
    }
    ++cursor_;
    return std::monostate{};
  }

  core::Result<TextToken> take() {
    if (cursor_ >= tokens_.size()) {
      return std::unexpected(error("unexpected end of UPLC text"));
    }
    return tokens_[cursor_++];
  }

  [[nodiscard]] bool peek(std::string_view value) const noexcept {
    return cursor_ < tokens_.size() && !tokens_[cursor_].quoted && tokens_[cursor_].text == value;
  }

  core::Result<std::uint64_t> unsigned_number() {
    auto token = take();
    if (!token) {
      return std::unexpected(token.error());
    }
    std::uint64_t result = 0U;
    const auto parsed =
        std::from_chars(token->text.data(), token->text.data() + token->text.size(), result);
    if (token->quoted || parsed.ec != std::errc{} ||
        parsed.ptr != token->text.data() + token->text.size()) {
      return std::unexpected(error("expected an unsigned decimal number"));
    }
    return result;
  }

  core::Result<UplcVersion> parse_version() {
    auto token = take();
    if (!token || token->quoted) {
      return std::unexpected(token ? error("expected a UPLC version") : token.error());
    }
    UplcVersion result{};
    std::array<std::uint64_t*, 3> parts{&result.major, &result.minor, &result.patch};
    std::size_t start = 0U;
    for (std::size_t index = 0U; index < parts.size(); ++index) {
      const auto end =
          index + 1U == parts.size() ? token->text.size() : token->text.find('.', start);
      if (end == std::string::npos || end == start) {
        return std::unexpected(error("invalid UPLC version"));
      }
      const auto parsed =
          std::from_chars(token->text.data() + start, token->text.data() + end, *parts[index]);
      if (parsed.ec != std::errc{} || parsed.ptr != token->text.data() + end) {
        return std::unexpected(error("invalid UPLC version"));
      }
      start = end + 1U;
    }
    return result;
  }

  core::Result<UplcType> parse_type(std::size_t depth) {
    if (depth > std::min(options_.max_universe_header, options_.max_depth)) {
      return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                        "UPLC universe header exceeds the configured limit"));
    }
    if (peek("(")) {
      ++cursor_;
      auto constructor = take();
      if (!constructor || constructor->quoted) {
        return std::unexpected(constructor ? error("expected a UPLC type constructor")
                                           : constructor.error());
      }
      if (constructor->text == "list" || constructor->text == "array") {
        auto item = parse_type(depth + 1U);
        if (!item) {
          return std::unexpected(item.error());
        }
        auto close = consume(")");
        if (!close) {
          return std::unexpected(close.error());
        }
        return constructor->text == "list" ? UplcType::list(std::move(*item))
                                           : UplcType::array(std::move(*item));
      }
      if (constructor->text == "pair") {
        auto first = parse_type(depth + 1U);
        auto second = parse_type(depth + 1U);
        if (!first || !second) {
          return std::unexpected(!first ? first.error() : second.error());
        }
        auto close = consume(")");
        if (!close) {
          return std::unexpected(close.error());
        }
        return UplcType::pair(std::move(*first), std::move(*second));
      }
      return std::unexpected(error("unknown UPLC type constructor"));
    }
    auto token = take();
    if (!token || token->quoted) {
      return std::unexpected(token ? error("expected a UPLC type") : token.error());
    }
    constexpr std::array<std::pair<std::string_view, UplcTypeTag>, 11> primitive_types{{
        {"integer", UplcTypeTag::integer},
        {"bytestring", UplcTypeTag::byte_string},
        {"string", UplcTypeTag::string},
        {"unit", UplcTypeTag::unit},
        {"bool", UplcTypeTag::boolean},
        {"data", UplcTypeTag::data},
        {"bls12_381_G1_element", UplcTypeTag::bls12_381_g1},
        {"bls12_381_G2_element", UplcTypeTag::bls12_381_g2},
        {"bls12_381_mlresult", UplcTypeTag::bls12_381_ml_result},
        {"value", UplcTypeTag::value},
        {"byte_string", UplcTypeTag::byte_string},
    }};
    const auto found = std::find_if(primitive_types.begin(), primitive_types.end(),
                                    [&](const auto& entry) { return entry.first == token->text; });
    if (found == primitive_types.end()) {
      return std::unexpected(error("unknown UPLC constant type"));
    }
    return UplcType::primitive(found->second);
  }

  core::Result<Data> parse_data(std::size_t depth) {
    if (depth > options_.max_depth) {
      return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                        "UPLC Data constant exceeds the configured depth"));
    }
    const bool parenthesized = peek("(");
    if (parenthesized) ++cursor_;
    auto finish = [&]() -> core::VoidResult {
      return parenthesized ? consume(")") : core::VoidResult(std::monostate{});
    };
    auto constructor = take();
    if (!constructor || constructor->quoted) {
      return std::unexpected(constructor ? error("expected a Data constructor")
                                         : constructor.error());
    }
    if (constructor->text == "I") {
      auto token = take();
      if (!token || token->quoted) {
        return std::unexpected(token ? error("expected a Data integer") : token.error());
      }
      auto decimal = std::string_view(token->text);
      if (decimal.starts_with("+")) decimal.remove_prefix(1U);
      auto integer = core::BigInteger::from_decimal(decimal);
      auto close = integer ? finish() : core::VoidResult(std::unexpected(integer.error()));
      return close ? core::Result<Data>(Data::integer(std::move(*integer)))
                   : std::unexpected(close.error());
    }
    if (constructor->text == "B") {
      auto token = take();
      if (!token || token->quoted || token->text.empty() || token->text.front() != '#') {
        return std::unexpected(token ? error("expected a #hex Data byte string") : token.error());
      }
      auto bytes = core::hex_to_bytes(std::string_view(token->text).substr(1U));
      auto close = bytes ? finish() : core::VoidResult(std::unexpected(bytes.error()));
      return close ? core::Result<Data>(Data::bytes(std::move(*bytes)))
                   : std::unexpected(close.error());
    }
    if (constructor->text == "List") {
      auto bracket = consume("[");
      if (!bracket) return std::unexpected(bracket.error());
      std::vector<Data> values;
      while (!peek("]")) {
        auto value = parse_data(depth + 1U);
        if (!value) return std::unexpected(value.error().at(values.size()));
        values.push_back(std::move(*value));
        if (peek(","))
          ++cursor_;
        else if (!peek("]"))
          return std::unexpected(error("expected ',' or ']' in Data list"));
      }
      ++cursor_;
      auto close = finish();
      return close ? core::Result<Data>(Data::list(std::move(values)))
                   : std::unexpected(close.error());
    }
    if (constructor->text == "Map") {
      auto bracket = consume("[");
      if (!bracket) return std::unexpected(bracket.error());
      std::vector<std::pair<Data, Data>> entries;
      while (!peek("]")) {
        auto pair_open = consume("(");
        if (!pair_open) return std::unexpected(pair_open.error());
        auto key = parse_data(depth + 1U);
        auto comma = key ? consume(",") : core::VoidResult(std::unexpected(key.error()));
        auto value =
            comma ? parse_data(depth + 1U) : core::Result<Data>(std::unexpected(comma.error()));
        auto pair_close = value ? consume(")") : core::VoidResult(std::unexpected(value.error()));
        if (!pair_close) return std::unexpected(pair_close.error());
        entries.emplace_back(std::move(*key), std::move(*value));
        if (peek(","))
          ++cursor_;
        else if (!peek("]"))
          return std::unexpected(error("expected ',' or ']' in Data map"));
      }
      ++cursor_;
      auto close = finish();
      return close ? core::Result<Data>(Data::map(std::move(entries)))
                   : std::unexpected(close.error());
    }
    if (constructor->text == "Constr") {
      auto alternative = take();
      if (!alternative || alternative->quoted) {
        return std::unexpected(alternative ? error("expected a Data constructor alternative")
                                           : alternative.error());
      }
      auto decimal = std::string_view(alternative->text);
      if (decimal.starts_with("+")) decimal.remove_prefix(1U);
      auto integer = core::BigInteger::from_decimal(decimal);
      if (!integer || integer->is_negative()) {
        return std::unexpected(integer ? error("Data constructor alternative must be nonnegative")
                                       : integer.error());
      }
      auto bracket = consume("[");
      if (!bracket) return std::unexpected(bracket.error());
      std::vector<Data> fields;
      while (!peek("]")) {
        auto field = parse_data(depth + 1U);
        if (!field) return std::unexpected(field.error().at(fields.size()));
        fields.push_back(std::move(*field));
        if (peek(","))
          ++cursor_;
        else if (!peek("]"))
          return std::unexpected(error("expected ',' or ']' in Data constructor"));
      }
      ++cursor_;
      auto close = finish();
      return close ? core::Result<Data>(Data::constr(std::move(*integer), std::move(fields)))
                   : std::unexpected(close.error());
    }
    return std::unexpected(error("unknown Data constructor"));
  }

  core::Result<core::Bytes> parse_hash_bytes() {
    auto token = take();
    if (!token || token->quoted || token->text.empty() || token->text.front() != '#') {
      return std::unexpected(token ? error("expected a #hex byte string") : token.error());
    }
    return core::hex_to_bytes(std::string_view(token->text).substr(1U));
  }

  core::Result<Data> parse_value_constant(std::size_t depth) {
    if (depth > options_.max_depth) {
      return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                        "UPLC Value constant exceeds the configured depth"));
    }
    auto open = consume("[");
    if (!open) return std::unexpected(open.error());
    std::vector<std::pair<Data, Data>> policies;
    std::optional<core::Bytes> previous_policy;
    const auto minimum_quantity =
        core::BigInteger::from_decimal("-170141183460469231731687303715884105728");
    const auto maximum_quantity =
        core::BigInteger::from_decimal("170141183460469231731687303715884105727");
    while (!peek("]")) {
      auto pair_open = consume("(");
      if (!pair_open) return std::unexpected(pair_open.error());
      auto policy = parse_hash_bytes();
      if (policy && (policy->size() > 32U || (previous_policy && !(*previous_policy < *policy)))) {
        return std::unexpected(
            error("Value policy keys must be distinct, ascending, and at most 32 bytes"));
      }
      auto comma = policy ? consume(",") : core::VoidResult(std::unexpected(policy.error()));
      auto tokens_open = comma ? consume("[") : comma;
      if (!tokens_open) return std::unexpected(tokens_open.error());
      std::vector<std::pair<Data, Data>> tokens;
      std::optional<core::Bytes> previous_asset;
      while (!peek("]")) {
        auto token_open = consume("(");
        if (!token_open) return std::unexpected(token_open.error());
        auto asset = parse_hash_bytes();
        if (asset && (asset->size() > 32U || (previous_asset && !(*previous_asset < *asset)))) {
          return std::unexpected(
              error("Value asset keys must be distinct, ascending, and at most 32 bytes"));
        }
        auto asset_comma = asset ? consume(",") : core::VoidResult(std::unexpected(asset.error()));
        auto quantity_token =
            asset_comma ? take() : core::Result<TextToken>(std::unexpected(asset_comma.error()));
        if (!quantity_token || quantity_token->quoted) {
          return std::unexpected(quantity_token ? error("expected a Value quantity")
                                                : quantity_token.error());
        }
        auto decimal = std::string_view(quantity_token->text);
        if (decimal.starts_with("+")) decimal.remove_prefix(1U);
        auto quantity = core::BigInteger::from_decimal(decimal);
        if (quantity && (quantity->is_zero() || *quantity < *minimum_quantity ||
                         *quantity > *maximum_quantity)) {
          return std::unexpected(error("Value quantity must be nonzero signed 128-bit"));
        }
        auto token_close =
            quantity ? consume(")") : core::VoidResult(std::unexpected(quantity.error()));
        if (!token_close) return std::unexpected(token_close.error());
        tokens.emplace_back(Data::bytes(std::move(*asset)), Data::integer(std::move(*quantity)));
        previous_asset = std::get<core::Bytes>(tokens.back().first.to_plutus_data().node());
        if (peek(","))
          ++cursor_;
        else if (!peek("]"))
          return std::unexpected(error("expected ',' or ']' in Value token map"));
      }
      ++cursor_;
      if (tokens.empty()) {
        return std::unexpected(error("Value cannot contain an empty token map"));
      }
      auto pair_close = consume(")");
      if (!pair_close) return std::unexpected(pair_close.error());
      policies.emplace_back(Data::bytes(std::move(*policy)), Data::map(std::move(tokens)));
      previous_policy = std::get<core::Bytes>(policies.back().first.to_plutus_data().node());
      if (peek(","))
        ++cursor_;
      else if (!peek("]"))
        return std::unexpected(error("expected ',' or ']' in Value policy map"));
    }
    ++cursor_;
    return Data::map(std::move(policies));
  }

  core::Result<UplcConstant> parse_constant_value(const UplcType& type, std::size_t depth) {
    if (depth > options_.max_depth) {
      return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                        "UPLC constant exceeds the configured depth"));
    }
    switch (type.tag()) {
      case UplcTypeTag::integer: {
        auto token = take();
        if (!token || token->quoted) {
          return std::unexpected(token ? error("expected a UPLC integer") : token.error());
        }
        auto decimal = std::string_view(token->text);
        if (decimal.starts_with("+")) decimal.remove_prefix(1U);
        auto integer = core::BigInteger::from_decimal(decimal);
        return integer ? core::Result<UplcConstant>(UplcConstant::integer(std::move(*integer)))
                       : std::unexpected(integer.error());
      }
      case UplcTypeTag::byte_string: {
        auto token = take();
        if (!token || token->quoted || token->text.empty() || token->text.front() != '#') {
          return std::unexpected(token ? error("expected a #hex UPLC byte string") : token.error());
        }
        auto bytes = core::hex_to_bytes(std::string_view(token->text).substr(1U));
        if (!bytes) {
          return std::unexpected(bytes.error());
        }
        if (bytes->size() > options_.max_constant_bytes) {
          return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                            "UPLC byte string exceeds the configured limit"));
        }
        return UplcConstant::bytes(std::move(*bytes));
      }
      case UplcTypeTag::string: {
        auto token = take();
        if (!token || !token->quoted) {
          return std::unexpected(token ? error("expected a quoted UPLC string") : token.error());
        }
        return UplcConstant::string(std::move(token->text));
      }
      case UplcTypeTag::unit: {
        auto open = consume("(");
        auto close = open ? consume(")") : open;
        return close ? core::Result<UplcConstant>(UplcConstant::unit())
                     : std::unexpected(close.error());
      }
      case UplcTypeTag::boolean: {
        auto token = take();
        if (!token || token->quoted || (token->text != "True" && token->text != "False")) {
          return std::unexpected(token ? error("expected True or False") : token.error());
        }
        return UplcConstant::boolean(token->text == "True");
      }
      case UplcTypeTag::bls12_381_g1:
      case UplcTypeTag::bls12_381_g2:
      case UplcTypeTag::bls12_381_ml_result: {
        auto token = take();
        if (!token || token->quoted || !std::string_view(token->text).starts_with("0x")) {
          return std::unexpected(token ? error("expected a 0x-prefixed BLS constant")
                                       : token.error());
        }
        auto bytes = core::hex_to_bytes(std::string_view(token->text).substr(2U));
        if (!bytes) {
          return std::unexpected(bytes.error());
        }
        if (type.tag() == UplcTypeTag::bls12_381_g1) {
          return UplcConstant::bls12_381_g1(std::move(*bytes));
        }
        if (type.tag() == UplcTypeTag::bls12_381_g2) {
          return UplcConstant::bls12_381_g2(std::move(*bytes));
        }
        return UplcConstant::bls12_381_ml_result(std::move(*bytes));
      }
      case UplcTypeTag::apply: {
        if (type.arguments().size() != 2U) {
          break;
        }
        const auto& function = type.arguments()[0];
        if (function.tag() == UplcTypeTag::proto_list ||
            function.tag() == UplcTypeTag::proto_array) {
          auto open = consume("[");
          if (!open) {
            return std::unexpected(open.error());
          }
          UplcConstant::Items values;
          while (!peek("]")) {
            auto value = parse_constant_value(type.arguments()[1], depth + 1U);
            if (!value) {
              return std::unexpected(value.error().at(values.size()));
            }
            values.push_back(std::move(*value));
            if (peek(",")) {
              ++cursor_;
            } else if (!peek("]")) {
              return std::unexpected(error("expected ',' or ']' in UPLC constant collection"));
            }
          }
          ++cursor_;
          return function.tag() == UplcTypeTag::proto_list
                     ? UplcConstant::list(type.arguments()[1], std::move(values))
                     : UplcConstant::array(type.arguments()[1], std::move(values));
        }
        if (function.tag() == UplcTypeTag::apply && function.arguments().size() == 2U &&
            function.arguments()[0].tag() == UplcTypeTag::proto_pair) {
          auto open = consume("(");
          if (!open) {
            return std::unexpected(open.error());
          }
          auto first = parse_constant_value(function.arguments()[1], depth + 1U);
          auto comma = first ? consume(",") : core::VoidResult(std::unexpected(first.error()));
          auto second = comma ? parse_constant_value(type.arguments()[1], depth + 1U)
                              : core::Result<UplcConstant>(std::unexpected(comma.error()));
          auto close = second ? consume(")") : core::VoidResult(std::unexpected(second.error()));
          return close ? UplcConstant::pair(std::move(*first), std::move(*second))
                       : core::Result<UplcConstant>(std::unexpected(close.error()));
        }
        break;
      }
      case UplcTypeTag::data: {
        auto data = parse_data(depth + 1U);
        return data ? core::Result<UplcConstant>(UplcConstant::data(std::move(*data)))
                    : std::unexpected(data.error());
      }
      case UplcTypeTag::value: {
        auto value = parse_value_constant(depth + 1U);
        return value ? UplcConstant::value(std::move(*value))
                     : core::Result<UplcConstant>(std::unexpected(value.error()));
      }
      case UplcTypeTag::proto_list:
      case UplcTypeTag::proto_pair:
      case UplcTypeTag::proto_array:
        break;
    }
    return std::unexpected(error("unsupported or malformed UPLC text constant"));
  }

  core::Result<UplcTerm> parse_term(const UplcVersion& version, std::vector<std::string>& scope,
                                    std::size_t depth) {
    if (depth > options_.max_depth) {
      return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                        "UPLC text term exceeds the configured depth"));
    }
    if (++nodes_ > options_.max_nodes) {
      return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                        "UPLC text exceeds the configured node limit"));
    }
    if (peek("[")) {
      ++cursor_;
      std::vector<UplcTerm> terms;
      while (!peek("]")) {
        auto term = parse_term(version, scope, depth + 1U);
        if (!term) {
          return std::unexpected(term.error().at(terms.size()));
        }
        terms.push_back(std::move(*term));
      }
      ++cursor_;
      if (terms.size() < 2U) {
        return std::unexpected(error("UPLC application requires at least two terms"));
      }
      UplcTerm result = std::move(terms.front());
      for (std::size_t index = 1U; index < terms.size(); ++index) {
        result = UplcTerm::apply(std::move(result), std::move(terms[index]));
      }
      return result;
    }
    if (peek("(")) {
      ++cursor_;
      auto constructor = take();
      if (!constructor || constructor->quoted) {
        return std::unexpected(constructor ? error("expected a UPLC term constructor")
                                           : constructor.error());
      }
      if (constructor->text == "delay") {
        auto term = parse_term(version, scope, depth + 1U);
        auto close = term ? consume(")") : core::VoidResult(std::unexpected(term.error()));
        return close ? core::Result<UplcTerm>(UplcTerm::delay(std::move(*term)))
                     : std::unexpected(close.error());
      }
      if (constructor->text == "force") {
        auto term = parse_term(version, scope, depth + 1U);
        auto close = term ? consume(")") : core::VoidResult(std::unexpected(term.error()));
        return close ? core::Result<UplcTerm>(UplcTerm::force(std::move(*term)))
                     : std::unexpected(close.error());
      }
      if (constructor->text == "lam") {
        auto binder = take();
        if (!binder || binder->quoted) {
          return std::unexpected(binder ? error("expected a lambda binder") : binder.error());
        }
        scope.push_back(binder->text);
        auto body = parse_term(version, scope, depth + 1U);
        scope.pop_back();
        auto close = body ? consume(")") : core::VoidResult(std::unexpected(body.error()));
        return close ? core::Result<UplcTerm>(UplcTerm::lambda(std::move(*body)))
                     : std::unexpected(close.error());
      }
      if (constructor->text == "con") {
        auto type = parse_type(0U);
        if (!type) {
          return std::unexpected(type.error());
        }
        auto value = parse_constant_value(*type, depth + 1U);
        auto close = value ? consume(")") : core::VoidResult(std::unexpected(value.error()));
        return close ? core::Result<UplcTerm>(UplcTerm::constant(std::move(*value)))
                     : std::unexpected(close.error());
      }
      if (constructor->text == "builtin") {
        auto name = take();
        if (!name || name->quoted) {
          return std::unexpected(name ? error("expected a builtin name") : name.error());
        }
        auto builtin = builtin_from_name(name->text);
        if (!builtin) {
          return std::unexpected(builtin.error());
        }
        if (builtin_tag(*builtin) > maximum_builtin_tag(version, options_.protocol_major)) {
          return std::unexpected(error("builtin is unavailable for this protocol version"));
        }
        auto close = consume(")");
        return close ? core::Result<UplcTerm>(UplcTerm::builtin(*builtin))
                     : std::unexpected(close.error());
      }
      if (constructor->text == "error") {
        auto close = consume(")");
        return close ? core::Result<UplcTerm>(UplcTerm::error()) : std::unexpected(close.error());
      }
      if (constructor->text == "constr") {
        if (version < UplcVersion::v1_1_0()) {
          return std::unexpected(error("constr is unavailable before UPLC 1.1.0"));
        }
        auto tag = unsigned_number();
        if (!tag) {
          return std::unexpected(tag.error());
        }
        std::vector<UplcTerm> fields;
        while (!peek(")")) {
          if (fields.size() >= options_.max_constr_fields) {
            return std::unexpected(uplc_error(core::ErrorCode::resource_limit_exceeded,
                                              "UPLC constr exceeds the configured field limit"));
          }
          auto field = parse_term(version, scope, depth + 1U);
          if (!field) {
            return std::unexpected(field.error().at(fields.size()));
          }
          fields.push_back(std::move(*field));
        }
        ++cursor_;
        return UplcTerm::constr(*tag, std::move(fields));
      }
      if (constructor->text == "case") {
        if (version < UplcVersion::v1_1_0()) {
          return std::unexpected(error("case is unavailable before UPLC 1.1.0"));
        }
        auto scrutinee = parse_term(version, scope, depth + 1U);
        if (!scrutinee) {
          return std::unexpected(scrutinee.error());
        }
        std::vector<UplcTerm> branches;
        while (!peek(")")) {
          auto branch = parse_term(version, scope, depth + 1U);
          if (!branch) {
            return std::unexpected(branch.error().at(branches.size()));
          }
          branches.push_back(std::move(*branch));
        }
        ++cursor_;
        return UplcTerm::case_of(std::move(*scrutinee), std::move(branches));
      }
      return std::unexpected(error("unknown UPLC term constructor"));
    }
    auto variable = take();
    if (!variable || variable->quoted) {
      return std::unexpected(variable ? error("expected a UPLC variable") : variable.error());
    }
    const auto found = std::find(scope.rbegin(), scope.rend(), variable->text);
    if (found == scope.rend()) {
      return UplcTerm::variable(static_cast<std::uint64_t>(scope.size() + 1U));
    }
    return UplcTerm::variable(static_cast<std::uint64_t>(std::distance(scope.rbegin(), found) + 1));
  }

  std::vector<TextToken> tokens_;
  ProgramDecodeOptions options_;
  std::size_t cursor_{};
  std::size_t nodes_{};
};

}  // namespace

core::Result<UplcProgram> parse_uplc_text(std::string_view text, ProgramDecodeOptions options) {
  auto tokens = tokenize_uplc(text);
  if (!tokens) {
    return std::unexpected(tokens.error());
  }
  return TextParser(std::move(*tokens), std::move(options)).program();
}

namespace {

struct RuntimeValue {
  struct Closure {
    UplcTerm body;
    std::vector<RuntimeValue> environment;
  };
  struct Delayed {
    UplcTerm term;
    std::vector<RuntimeValue> environment;
  };
  struct PartialBuiltin {
    Builtin builtin;
    std::size_t forces{};
    std::vector<RuntimeValue> arguments;
  };
  struct Constructor {
    std::uint64_t tag{};
    std::vector<RuntimeValue> fields;
  };

  using Node = std::variant<UplcConstant, std::shared_ptr<Closure>, std::shared_ptr<Delayed>,
                            std::shared_ptr<PartialBuiltin>, std::shared_ptr<Constructor>,
                            std::shared_ptr<crypto::BlsMlResult>>;
  Node node;
};

using TokenMap = std::map<core::Bytes, core::BigInteger>;
using ValueMap = std::map<core::Bytes, TokenMap>;

bool signed_128(const core::BigInteger& value);

core::Result<ValueMap> decode_value_map(const Data& data) {
  const auto* outer = std::get_if<std::shared_ptr<chain::PlutusMap>>(&data.to_plutus_data().node());
  if (outer == nullptr) {
    return std::unexpected(
        uplc_error(core::ErrorCode::evaluation, "Value must be represented by map Data"));
  }
  ValueMap result;
  std::optional<core::Bytes> previous_policy;
  for (const auto& [policy_data, tokens_data] : (*outer)->entries) {
    const auto* policy = std::get_if<core::Bytes>(&policy_data.node());
    const auto* tokens = std::get_if<std::shared_ptr<chain::PlutusMap>>(&tokens_data.node());
    if (policy == nullptr || policy->size() > 32U || tokens == nullptr ||
        (*tokens)->entries.empty() || (previous_policy && !(*previous_policy < *policy))) {
      return std::unexpected(uplc_error(core::ErrorCode::evaluation, "invalid Value policy entry"));
    }
    TokenMap inner;
    std::optional<core::Bytes> previous_asset;
    for (const auto& [asset_data, quantity_data] : (*tokens)->entries) {
      const auto* asset = std::get_if<core::Bytes>(&asset_data.node());
      const auto* quantity = std::get_if<core::BigInteger>(&quantity_data.node());
      if (asset == nullptr || asset->size() > 32U || quantity == nullptr || quantity->is_zero() ||
          !signed_128(*quantity) || (previous_asset && !(*previous_asset < *asset))) {
        return std::unexpected(
            uplc_error(core::ErrorCode::evaluation, "invalid Value token entry"));
      }
      if (!inner.emplace(*asset, *quantity).second) {
        return std::unexpected(
            uplc_error(core::ErrorCode::evaluation, "duplicate Value token key"));
      }
      previous_asset = *asset;
    }
    if (!result.emplace(*policy, std::move(inner)).second) {
      return std::unexpected(uplc_error(core::ErrorCode::evaluation, "duplicate Value policy key"));
    }
    previous_policy = *policy;
  }
  return result;
}

Data encode_value_map(const ValueMap& value) {
  std::vector<std::pair<Data, Data>> policies;
  for (const auto& [policy, tokens] : value) {
    if (tokens.empty()) continue;
    std::vector<std::pair<Data, Data>> entries;
    for (const auto& [asset, quantity] : tokens) {
      if (!quantity.is_zero()) {
        entries.emplace_back(Data::bytes(asset), Data::integer(quantity));
      }
    }
    if (!entries.empty()) {
      policies.emplace_back(Data::bytes(policy), Data::map(std::move(entries)));
    }
  }
  return Data::map(std::move(policies));
}

bool signed_128(const core::BigInteger& value) {
  static const auto minimum =
      *core::BigInteger::from_decimal("-170141183460469231731687303715884105728");
  static const auto maximum =
      *core::BigInteger::from_decimal("170141183460469231731687303715884105727");
  return value >= minimum && value <= maximum;
}

bool bls_scalar_in_bounds(const core::BigInteger& scalar) {
  auto magnitude = scalar.is_negative() ? -scalar : scalar;
  const auto bytes = magnitude.to_unsigned_bytes_be();
  if (bytes.size() < 512U) return true;
  if (bytes.size() > 512U) return false;
  const auto first = std::to_integer<std::uint8_t>(bytes.front());
  if (!scalar.is_negative()) return first < 0x80U;
  if (first < 0x80U) return true;
  if (first > 0x80U) return false;
  return std::all_of(bytes.begin() + 1, bytes.end(),
                     [](std::byte byte) { return byte == std::byte{0}; });
}

core::Result<core::BigInteger> modular_inverse(core::BigInteger value,
                                               const core::BigInteger& modulus) {
  core::BigInteger old_r = value % modulus;
  if (old_r.is_negative()) old_r += modulus;
  core::BigInteger r = modulus;
  core::BigInteger old_s(std::int64_t{1});
  core::BigInteger s(std::int64_t{0});
  while (!r.is_zero()) {
    const auto quotient = old_r / r;
    auto next_r = old_r - quotient * r;
    old_r = std::move(r);
    r = std::move(next_r);
    auto next_s = old_s - quotient * s;
    old_s = std::move(s);
    s = std::move(next_s);
  }
  if (old_r != core::BigInteger(std::int64_t{1})) {
    return std::unexpected(
        uplc_error(core::ErrorCode::evaluation, "base is not invertible modulo the modulus"));
  }
  old_s %= modulus;
  if (old_s.is_negative()) old_s += modulus;
  return old_s;
}

core::Result<core::BigInteger> modular_power(core::BigInteger base, core::BigInteger exponent,
                                             const core::BigInteger& modulus) {
  if (modulus <= core::BigInteger(std::int64_t{0})) {
    return std::unexpected(
        uplc_error(core::ErrorCode::evaluation, "expModInteger modulus must be positive"));
  }
  if (modulus == core::BigInteger(std::int64_t{1})) {
    return core::BigInteger(std::int64_t{0});
  }
  if (exponent.is_negative()) {
    auto inverse = modular_inverse(base, modulus);
    if (!inverse) return std::unexpected(inverse.error());
    base = std::move(*inverse);
    exponent = -exponent;
  }
  base %= modulus;
  if (base.is_negative()) base += modulus;
  core::BigInteger result(std::int64_t{1});
  const core::BigInteger two(std::int64_t{2});
  while (!exponent.is_zero()) {
    if (!(exponent % two).is_zero()) {
      result = (result * base) % modulus;
    }
    exponent /= two;
    if (!exponent.is_zero()) base = (base * base) % modulus;
  }
  return result;
}

[[nodiscard]] std::pair<std::size_t, std::size_t> builtin_shape(Builtin builtin) {
  switch (builtin) {
    case Builtin::add_integer:
    case Builtin::subtract_integer:
    case Builtin::multiply_integer:
    case Builtin::divide_integer:
    case Builtin::quotient_integer:
    case Builtin::remainder_integer:
    case Builtin::mod_integer:
    case Builtin::equals_integer:
    case Builtin::less_than_integer:
    case Builtin::less_than_equals_integer:
      return {0U, 2U};
    case Builtin::append_byte_string:
      return {0U, 2U};
    case Builtin::cons_byte_string:
      return {0U, 2U};
    case Builtin::slice_byte_string:
      return {0U, 3U};
    case Builtin::length_of_byte_string:
      return {0U, 1U};
    case Builtin::index_byte_string:
      return {0U, 2U};
    case Builtin::equals_byte_string:
    case Builtin::less_than_byte_string:
    case Builtin::less_than_equals_byte_string:
      return {0U, 2U};
    case Builtin::sha2_256:
    case Builtin::sha3_256:
    case Builtin::blake2b_256:
      return {0U, 1U};
    case Builtin::verify_ed25519_signature:
    case Builtin::verify_ecdsa_secp256k1_signature:
    case Builtin::verify_schnorr_secp256k1_signature:
      return {0U, 3U};
    case Builtin::append_string:
    case Builtin::equals_string:
      return {0U, 2U};
    case Builtin::encode_utf8:
    case Builtin::decode_utf8:
      return {0U, 1U};
    case Builtin::if_then_else:
      return {1U, 3U};
    case Builtin::choose_unit:
      return {1U, 2U};
    case Builtin::trace:
      return {1U, 2U};
    case Builtin::fst_pair:
    case Builtin::snd_pair:
      return {2U, 1U};
    case Builtin::choose_list:
      return {2U, 3U};
    case Builtin::mk_cons:
      return {1U, 2U};
    case Builtin::head_list:
    case Builtin::tail_list:
    case Builtin::null_list:
      return {1U, 1U};
    case Builtin::choose_data:
      return {1U, 6U};
    case Builtin::constr_data:
      return {0U, 2U};
    case Builtin::map_data:
    case Builtin::list_data:
    case Builtin::i_data:
    case Builtin::b_data:
    case Builtin::un_constr_data:
    case Builtin::un_map_data:
    case Builtin::un_list_data:
    case Builtin::un_i_data:
    case Builtin::un_b_data:
    case Builtin::equals_data:
    case Builtin::serialise_data:
      return {0U, builtin == Builtin::equals_data ? 2U : 1U};
    case Builtin::mk_pair_data:
      return {0U, 2U};
    case Builtin::mk_nil_data:
    case Builtin::mk_nil_pair_data:
      return {0U, 1U};
    case Builtin::bls12_381_g1_add:
    case Builtin::bls12_381_g1_equal:
    case Builtin::bls12_381_g2_add:
    case Builtin::bls12_381_g2_equal:
    case Builtin::bls12_381_miller_loop:
    case Builtin::bls12_381_mul_ml_result:
    case Builtin::bls12_381_final_verify:
      return {0U, 2U};
    case Builtin::bls12_381_g1_neg:
    case Builtin::bls12_381_g1_compress:
    case Builtin::bls12_381_g1_uncompress:
    case Builtin::bls12_381_g2_neg:
    case Builtin::bls12_381_g2_compress:
    case Builtin::bls12_381_g2_uncompress:
      return {0U, 1U};
    case Builtin::bls12_381_g1_scalar_mul:
    case Builtin::bls12_381_g1_hash_to_group:
    case Builtin::bls12_381_g2_scalar_mul:
    case Builtin::bls12_381_g2_hash_to_group:
      return {0U, 2U};
    case Builtin::keccak_256:
    case Builtin::blake2b_224:
    case Builtin::complement_byte_string:
    case Builtin::count_set_bits:
    case Builtin::find_first_set_bit:
    case Builtin::ripemd_160:
      return {0U, 1U};
    case Builtin::length_of_array:
    case Builtin::list_to_array:
      return {1U, 1U};
    case Builtin::bls12_381_g1_multi_scalar_mul:
    case Builtin::bls12_381_g2_multi_scalar_mul:
      return {0U, 2U};
    case Builtin::integer_to_byte_string:
      return {0U, 3U};
    case Builtin::byte_string_to_integer:
      return {0U, 2U};
    case Builtin::and_byte_string:
    case Builtin::or_byte_string:
    case Builtin::xor_byte_string:
    case Builtin::write_bits:
    case Builtin::exp_mod_integer:
      return {0U, 3U};
    case Builtin::read_bit:
    case Builtin::replicate_byte:
    case Builtin::shift_byte_string:
    case Builtin::rotate_byte_string:
      return {0U, 2U};
    case Builtin::drop_list:
    case Builtin::index_array:
      return {1U, 2U};
    case Builtin::insert_coin:
      return {0U, 4U};
    case Builtin::lookup_coin:
      return {0U, 3U};
    case Builtin::union_value:
    case Builtin::value_contains:
      return {0U, 2U};
    case Builtin::value_data:
    case Builtin::un_value_data:
      return {0U, 1U};
    case Builtin::scale_value:
      return {0U, 2U};
  }
  return {0U, 0U};
}

class Evaluator {
 public:
  Evaluator(MachineBudget maximum, SemanticsVariant semantics, MachineCosts machine_costs,
            BuiltinCostModel builtin_costs)
      : maximum_(maximum),
        machine_costs_(std::move(machine_costs)),
        builtin_costs_(std::move(builtin_costs)) {
    builtin_costs_.semantics = semantics;
  }

  core::Result<MachineResult> run(const UplcProgram& program) {
    auto startup = charge(machine_costs_.startup);
    if (!startup) {
      return std::unexpected(startup.error());
    }
    std::vector<RuntimeValue> environment;
    auto result = evaluate(program.term, environment, 0U);
    if (!result) {
      return std::unexpected(result.error());
    }
    auto quoted = quote(*result);
    if (!quoted) {
      return std::unexpected(quoted.error());
    }
    return MachineResult{std::move(*quoted), spent_, std::move(logs_), std::move(stream_)};
  }

 private:
  core::VoidResult charge(MachineBudget cost) {
    if (cost.cpu < 0 || cost.memory < 0) {
      return std::unexpected(
          uplc_error(core::ErrorCode::evaluation, "UPLC machine cost must be nonnegative"));
    }
    const auto add = [](std::int64_t left, std::int64_t right) {
      return left > std::numeric_limits<std::int64_t>::max() - right
                 ? std::numeric_limits<std::int64_t>::max()
                 : left + right;
    };
    const auto next_cpu = add(spent_.cpu, cost.cpu);
    const auto next_memory = add(spent_.memory, cost.memory);
    if (next_cpu > maximum_.cpu || next_memory > maximum_.memory) {
      return std::unexpected(
          uplc_error(core::ErrorCode::evaluation, "UPLC evaluation budget exhausted"));
    }
    spent_.cpu = next_cpu;
    spent_.memory = next_memory;
    stream_.costs.push_back(cost);
    return std::monostate{};
  }

  core::Result<RuntimeValue> evaluate(const UplcTerm& term,
                                      const std::vector<RuntimeValue>& environment,
                                      std::size_t depth) {
    if (depth > 10'000U) {
      return std::unexpected(uplc_error(core::ErrorCode::depth_limit_exceeded,
                                        "UPLC evaluation continuation is too deep"));
    }
    return std::visit(
        [&](const auto& node) -> core::Result<RuntimeValue> {
          using Node = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<Node, UplcVariable>) {
            auto cost = charge(machine_costs_.variable);
            if (!cost) return std::unexpected(cost.error());
            if (node.index == 0U || node.index > environment.size()) {
              return std::unexpected(
                  uplc_error(core::ErrorCode::evaluation, "open UPLC variable during evaluation"));
            }
            return environment[environment.size() - node.index];
          } else if constexpr (std::is_same_v<Node, UplcDelay>) {
            auto cost = charge(machine_costs_.delay);
            if (!cost) return std::unexpected(cost.error());
            return RuntimeValue{std::make_shared<RuntimeValue::Delayed>(
                RuntimeValue::Delayed{*node.term, environment})};
          } else if constexpr (std::is_same_v<Node, UplcLambda>) {
            auto cost = charge(machine_costs_.lambda);
            if (!cost) return std::unexpected(cost.error());
            return RuntimeValue{std::make_shared<RuntimeValue::Closure>(
                RuntimeValue::Closure{*node.body, environment})};
          } else if constexpr (std::is_same_v<Node, UplcApply>) {
            auto cost = charge(machine_costs_.apply);
            if (!cost) return std::unexpected(cost.error());
            auto function = evaluate(*node.function, environment, depth + 1U);
            if (!function) return std::unexpected(function.error());
            auto argument = evaluate(*node.argument, environment, depth + 1U);
            return argument ? apply(std::move(*function), std::move(*argument), depth)
                            : core::Result<RuntimeValue>(std::unexpected(argument.error()));
          } else if constexpr (std::is_same_v<Node, UplcConstant>) {
            auto cost = charge(machine_costs_.constant);
            return cost ? core::Result<RuntimeValue>(RuntimeValue{node})
                        : std::unexpected(cost.error());
          } else if constexpr (std::is_same_v<Node, UplcForce>) {
            auto cost = charge(machine_costs_.force);
            if (!cost) return std::unexpected(cost.error());
            auto value = evaluate(*node.term, environment, depth + 1U);
            return value ? force(std::move(*value), depth)
                         : core::Result<RuntimeValue>(std::unexpected(value.error()));
          } else if constexpr (std::is_same_v<Node, UplcError>) {
            return std::unexpected(
                uplc_error(core::ErrorCode::evaluation, "explicit UPLC error term"));
          } else if constexpr (std::is_same_v<Node, Builtin>) {
            auto cost = charge(machine_costs_.builtin);
            if (!cost) return std::unexpected(cost.error());
            return RuntimeValue{std::make_shared<RuntimeValue::PartialBuiltin>(
                RuntimeValue::PartialBuiltin{node, 0U, {}})};
          } else if constexpr (std::is_same_v<Node, UplcConstr>) {
            auto cost = charge(machine_costs_.constr);
            if (!cost) return std::unexpected(cost.error());
            std::vector<RuntimeValue> fields;
            fields.reserve(node.fields.size());
            for (const auto& field : node.fields) {
              auto value = evaluate(field, environment, depth + 1U);
              if (!value) return std::unexpected(value.error());
              fields.push_back(std::move(*value));
            }
            return RuntimeValue{std::make_shared<RuntimeValue::Constructor>(
                RuntimeValue::Constructor{node.tag, std::move(fields)})};
          } else {
            auto cost = charge(machine_costs_.case_cost);
            if (!cost) return std::unexpected(cost.error());
            auto scrutinee = evaluate(*node.scrutinee, environment, depth + 1U);
            if (!scrutinee) return std::unexpected(scrutinee.error());
            const auto* constructor =
                std::get_if<std::shared_ptr<RuntimeValue::Constructor>>(&scrutinee->node);
            if (constructor != nullptr) {
              if ((*constructor)->tag >= node.branches.size()) {
                return std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                                  "UPLC case constructor tag has no branch"));
              }
              auto branch = evaluate(node.branches[(*constructor)->tag], environment, depth + 1U);
              if (!branch) return std::unexpected(branch.error());
              for (const auto& field : (*constructor)->fields) {
                branch = apply(std::move(*branch), field, depth + 1U);
                if (!branch) return std::unexpected(branch.error());
              }
              return branch;
            }
            const auto* constant = std::get_if<UplcConstant>(&scrutinee->node);
            if (constant == nullptr) {
              return std::unexpected(
                  uplc_error(core::ErrorCode::evaluation,
                             "UPLC case expected a constructor or supported constant"));
            }
            auto branch_at = [&](std::size_t index) -> core::Result<RuntimeValue> {
              if (index >= node.branches.size()) {
                return std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                                  "UPLC constant case has no matching branch"));
              }
              return evaluate(node.branches[index], environment, depth + 1U);
            };
            if (std::holds_alternative<std::monostate>(constant->value())) {
              return node.branches.size() == 1U
                         ? branch_at(0U)
                         : core::Result<RuntimeValue>(std::unexpected(
                               uplc_error(core::ErrorCode::evaluation,
                                          "unit case requires exactly one branch")));
            }
            if (const auto* boolean = std::get_if<bool>(&constant->value())) {
              if (node.branches.size() < 1U || node.branches.size() > 2U ||
                  (*boolean && node.branches.size() != 2U)) {
                return std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                                  "boolean case has an invalid branch count"));
              }
              return branch_at(*boolean ? 1U : 0U);
            }
            if (const auto* integer = std::get_if<core::BigInteger>(&constant->value())) {
              auto index = integer->to_uint64();
              return index
                         ? branch_at(static_cast<std::size_t>(*index))
                         : core::Result<RuntimeValue>(std::unexpected(uplc_error(
                               core::ErrorCode::evaluation, "integer case index is out of range")));
            }
            if (const auto* list = std::get_if<UplcConstant::Items>(&constant->value())) {
              if (node.branches.size() != 1U && node.branches.size() != 2U) {
                return std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                                  "list case requires one or two branches"));
              }
              if (list->empty()) {
                return node.branches.size() == 2U
                           ? branch_at(1U)
                           : core::Result<RuntimeValue>(std::unexpected(
                                 uplc_error(core::ErrorCode::evaluation,
                                            "one-branch list case requires a nonempty list")));
              }
              auto branch = branch_at(0U);
              if (!branch) return std::unexpected(branch.error());
              branch = apply(std::move(*branch), RuntimeValue{list->front()}, depth + 1U);
              if (!branch) return std::unexpected(branch.error());
              auto tail = *list;
              tail.erase(tail.begin());
              auto tail_constant =
                  UplcConstant::list(constant->type().arguments()[1], std::move(tail));
              if (!tail_constant) return std::unexpected(tail_constant.error());
              return apply(std::move(*branch), RuntimeValue{std::move(*tail_constant)}, depth + 1U);
            }
            if (const auto* pair = std::get_if<UplcPair>(&constant->value())) {
              if (node.branches.size() != 1U) {
                return std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                                  "pair case requires exactly one branch"));
              }
              auto branch = branch_at(0U);
              if (!branch) return std::unexpected(branch.error());
              branch = apply(std::move(*branch), RuntimeValue{pair->first()}, depth + 1U);
              return branch ? apply(std::move(*branch), RuntimeValue{pair->second()}, depth + 1U)
                            : branch;
            }
            return std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                              "casing this UPLC constant type is unsupported"));
          }
        },
        term.node());
  }

  core::Result<RuntimeValue> force(RuntimeValue value, std::size_t depth) {
    if (const auto* delayed = std::get_if<std::shared_ptr<RuntimeValue::Delayed>>(&value.node)) {
      return evaluate((*delayed)->term, (*delayed)->environment, depth + 1U);
    }
    if (const auto* partial =
            std::get_if<std::shared_ptr<RuntimeValue::PartialBuiltin>>(&value.node)) {
      const auto [forces, arguments] = builtin_shape((*partial)->builtin);
      static_cast<void>(arguments);
      if ((*partial)->forces >= forces) {
        return std::unexpected(
            uplc_error(core::ErrorCode::evaluation, "UPLC builtin received too many forces"));
      }
      auto copy = std::make_shared<RuntimeValue::PartialBuiltin>(**partial);
      ++copy->forces;
      return RuntimeValue{std::move(copy)};
    }
    return std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                      "UPLC force expected a delay or polymorphic builtin"));
  }

  core::Result<RuntimeValue> apply(RuntimeValue function, RuntimeValue argument,
                                   std::size_t depth) {
    if (const auto* closure = std::get_if<std::shared_ptr<RuntimeValue::Closure>>(&function.node)) {
      auto environment = (*closure)->environment;
      environment.push_back(std::move(argument));
      return evaluate((*closure)->body, environment, depth + 1U);
    }
    if (const auto* partial =
            std::get_if<std::shared_ptr<RuntimeValue::PartialBuiltin>>(&function.node)) {
      const auto [forces, arguments] = builtin_shape((*partial)->builtin);
      if ((*partial)->forces != forces) {
        return std::unexpected(
            uplc_error(core::ErrorCode::evaluation, "UPLC builtin is missing a force"));
      }
      if ((*partial)->arguments.size() >= arguments) {
        return std::unexpected(
            uplc_error(core::ErrorCode::evaluation, "UPLC builtin received too many arguments"));
      }
      auto copy = std::make_shared<RuntimeValue::PartialBuiltin>(**partial);
      copy->arguments.push_back(std::move(argument));
      if (copy->arguments.size() == arguments) {
        return execute(*copy);
      }
      return RuntimeValue{std::move(copy)};
    }
    return std::unexpected(
        uplc_error(core::ErrorCode::evaluation, "UPLC application expected a function"));
  }

  core::Result<UplcConstant> constant(const RuntimeValue& value, std::string_view expected) const {
    const auto* result = std::get_if<UplcConstant>(&value.node);
    return result != nullptr
               ? core::Result<UplcConstant>(*result)
               : std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                            "UPLC builtin expected " + std::string(expected)));
  }

  template <typename Type>
  core::Result<Type> constant_value(const RuntimeValue& value, std::string_view expected) const {
    auto item = constant(value, expected);
    if (!item) return std::unexpected(item.error());
    const auto* result = std::get_if<Type>(&item->value());
    return result != nullptr
               ? core::Result<Type>(*result)
               : std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                            "UPLC builtin expected " + std::string(expected)));
  }

  core::Result<RuntimeValue> execute(const RuntimeValue::PartialBuiltin& partial) {
    std::vector<UplcConstant> constants;
    constants.reserve(partial.arguments.size());
    for (const auto& argument : partial.arguments) {
      if (const auto* item = std::get_if<UplcConstant>(&argument.node)) {
        constants.push_back(*item);
      }
    }
    auto cost = charge(builtin_cost(partial.builtin, constants, builtin_costs_));
    if (!cost) return std::unexpected(cost.error());

    const auto& args = partial.arguments;
    const auto boolean = [](bool value) { return RuntimeValue{UplcConstant::boolean(value)}; };
    const auto integer = [](core::BigInteger value) {
      return RuntimeValue{UplcConstant::integer(std::move(value))};
    };
    const auto bytes = [](core::Bytes value) {
      return RuntimeValue{UplcConstant::bytes(std::move(value))};
    };
    const auto fail = [](std::string message) -> core::Result<RuntimeValue> {
      return std::unexpected(uplc_error(core::ErrorCode::evaluation, std::move(message)));
    };

    if (builtin_tag(partial.builtin) <= 9U) {
      auto left = constant_value<core::BigInteger>(args[0], "integer");
      auto right = constant_value<core::BigInteger>(args[1], "integer");
      if (!left || !right) {
        return std::unexpected(!left ? left.error() : right.error());
      }
      switch (partial.builtin) {
        case Builtin::add_integer:
          return integer(*left + *right);
        case Builtin::subtract_integer:
          return integer(*left - *right);
        case Builtin::multiply_integer:
          return integer(*left * *right);
        case Builtin::divide_integer:
        case Builtin::mod_integer:
        case Builtin::quotient_integer:
        case Builtin::remainder_integer: {
          if (right->is_zero()) return fail("integer division by zero");
          auto quotient = *left / *right;
          auto remainder = *left % *right;
          if ((partial.builtin == Builtin::divide_integer ||
               partial.builtin == Builtin::mod_integer) &&
              !remainder.is_zero() && (left->is_negative() != right->is_negative())) {
            quotient -= core::BigInteger(std::int64_t{1});
            remainder += *right;
          }
          return partial.builtin == Builtin::divide_integer ||
                         partial.builtin == Builtin::quotient_integer
                     ? core::Result<RuntimeValue>(integer(std::move(quotient)))
                     : core::Result<RuntimeValue>(integer(std::move(remainder)));
        }
        case Builtin::equals_integer:
          return boolean(*left == *right);
        case Builtin::less_than_integer:
          return boolean(*left < *right);
        case Builtin::less_than_equals_integer:
          return boolean(*left <= *right);
        default:
          break;
      }
    }

    switch (partial.builtin) {
      case Builtin::append_byte_string: {
        auto left = constant_value<core::Bytes>(args[0], "bytestring");
        auto right = constant_value<core::Bytes>(args[1], "bytestring");
        if (!left || !right) return std::unexpected(!left ? left.error() : right.error());
        left->insert(left->end(), right->begin(), right->end());
        return bytes(std::move(*left));
      }
      case Builtin::cons_byte_string: {
        auto value = constant_value<core::BigInteger>(args[0], "integer");
        auto tail = constant_value<core::Bytes>(args[1], "bytestring");
        if (!value || !tail) return std::unexpected(!value ? value.error() : tail.error());
        auto byte = value->to_uint64();
        if (!byte || *byte > 255U) return fail("byte value is out of range");
        tail->insert(tail->begin(), std::byte{static_cast<std::uint8_t>(*byte)});
        return bytes(std::move(*tail));
      }
      case Builtin::slice_byte_string: {
        auto start = constant_value<core::BigInteger>(args[0], "integer");
        auto length = constant_value<core::BigInteger>(args[1], "integer");
        auto input = constant_value<core::Bytes>(args[2], "bytestring");
        if (!start || !length || !input)
          return std::unexpected(!start ? start.error()
                                        : (!length ? length.error() : input.error()));
        const auto start_i =
            start->fits_int64() ? *start->to_int64() : std::numeric_limits<std::int64_t>::max();
        const auto length_i =
            length->fits_int64() ? *length->to_int64() : std::numeric_limits<std::int64_t>::max();
        const auto begin = static_cast<std::size_t>(
            std::clamp<std::int64_t>(start_i, 0, static_cast<std::int64_t>(input->size())));
        const auto count = length_i <= 0 ? 0U
                                         : std::min<std::size_t>(static_cast<std::size_t>(length_i),
                                                                 input->size() - begin);
        return bytes(core::Bytes(input->begin() + static_cast<std::ptrdiff_t>(begin),
                                 input->begin() + static_cast<std::ptrdiff_t>(begin + count)));
      }
      case Builtin::length_of_byte_string: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        return input ? core::Result<RuntimeValue>(
                           integer(core::BigInteger(static_cast<std::uint64_t>(input->size()))))
                     : std::unexpected(input.error());
      }
      case Builtin::index_byte_string: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        auto index = constant_value<core::BigInteger>(args[1], "integer");
        if (!input || !index) return std::unexpected(!input ? input.error() : index.error());
        auto position = index->to_uint64();
        if (!position || *position >= input->size())
          return fail("bytestring index is out of range");
        return integer(core::BigInteger(
            static_cast<std::uint64_t>(std::to_integer<std::uint8_t>((*input)[*position]))));
      }
      case Builtin::equals_byte_string:
      case Builtin::less_than_byte_string:
      case Builtin::less_than_equals_byte_string: {
        auto left = constant_value<core::Bytes>(args[0], "bytestring");
        auto right = constant_value<core::Bytes>(args[1], "bytestring");
        if (!left || !right) return std::unexpected(!left ? left.error() : right.error());
        if (partial.builtin == Builtin::equals_byte_string) return boolean(*left == *right);
        const auto less =
            std::lexicographical_compare(left->begin(), left->end(), right->begin(), right->end());
        return boolean(
            less || (partial.builtin == Builtin::less_than_equals_byte_string && *left == *right));
      }
      case Builtin::sha2_256:
      case Builtin::sha3_256:
      case Builtin::blake2b_256:
      case Builtin::keccak_256:
      case Builtin::blake2b_224:
      case Builtin::ripemd_160: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        if (!input) return std::unexpected(input.error());
        if (partial.builtin == Builtin::sha2_256) return bytes(crypto::sha2_256(*input));
        if (partial.builtin == Builtin::sha3_256) return bytes(crypto::sha3_256(*input));
        if (partial.builtin == Builtin::blake2b_256) return bytes(crypto::blake2b256(*input));
        if (partial.builtin == Builtin::keccak_256) return bytes(crypto::keccak_256(*input));
        if (partial.builtin == Builtin::blake2b_224) return bytes(crypto::blake2b224(*input));
        return bytes(crypto::ripemd_160(*input));
      }
      case Builtin::verify_ed25519_signature:
      case Builtin::verify_ecdsa_secp256k1_signature:
      case Builtin::verify_schnorr_secp256k1_signature: {
        auto key = constant_value<core::Bytes>(args[0], "public key bytes");
        auto message = constant_value<core::Bytes>(args[1], "message bytes");
        auto signature = constant_value<core::Bytes>(args[2], "signature bytes");
        if (!key || !message || !signature)
          return std::unexpected(!key ? key.error()
                                      : (!message ? message.error() : signature.error()));
        core::Result<bool> verified =
            partial.builtin == Builtin::verify_ed25519_signature
                ? crypto::verify_ed25519_strict(*key, *message, *signature)
            : partial.builtin == Builtin::verify_ecdsa_secp256k1_signature
                ? crypto::verify_secp256k1_ecdsa_strict(*key, *message, *signature)
                : crypto::verify_secp256k1_schnorr_strict(*key, *message, *signature);
        return verified ? core::Result<RuntimeValue>(boolean(*verified))
                        : std::unexpected(verified.error());
      }
      case Builtin::append_string:
      case Builtin::equals_string: {
        auto left = constant_value<std::string>(args[0], "string");
        auto right = constant_value<std::string>(args[1], "string");
        if (!left || !right) return std::unexpected(!left ? left.error() : right.error());
        if (partial.builtin == Builtin::equals_string) return boolean(*left == *right);
        auto result = UplcConstant::string(*left + *right);
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::encode_utf8: {
        auto text = constant_value<std::string>(args[0], "string");
        return text ? core::Result<RuntimeValue>(
                          bytes(core::Bytes(std::as_bytes(std::span(*text)).begin(),
                                            std::as_bytes(std::span(*text)).end())))
                    : std::unexpected(text.error());
      }
      case Builtin::decode_utf8: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        if (!input) return std::unexpected(input.error());
        if (!core::is_valid_utf8(*input)) return fail("invalid UTF-8 byte string");
        auto text = UplcConstant::string(
            std::string(reinterpret_cast<const char*>(input->data()), input->size()));
        return text ? core::Result<RuntimeValue>(RuntimeValue{std::move(*text)})
                    : std::unexpected(text.error());
      }
      case Builtin::if_then_else: {
        auto condition = constant_value<bool>(args[0], "boolean");
        return condition ? core::Result<RuntimeValue>(*condition ? args[1] : args[2])
                         : std::unexpected(condition.error());
      }
      case Builtin::choose_unit: {
        auto unit = constant_value<std::monostate>(args[0], "unit");
        return unit ? core::Result<RuntimeValue>(args[1]) : std::unexpected(unit.error());
      }
      case Builtin::trace: {
        auto text = constant_value<std::string>(args[0], "string");
        if (!text) return std::unexpected(text.error());
        logs_.push_back(std::move(*text));
        return args[1];
      }
      case Builtin::fst_pair:
      case Builtin::snd_pair: {
        auto pair = constant_value<UplcPair>(args[0], "pair");
        if (!pair) return std::unexpected(pair.error());
        return RuntimeValue{partial.builtin == Builtin::fst_pair ? pair->first() : pair->second()};
      }
      case Builtin::choose_list:
      case Builtin::head_list:
      case Builtin::tail_list:
      case Builtin::null_list:
      case Builtin::mk_cons: {
        if (partial.builtin == Builtin::mk_cons) {
          auto head = constant(args[0], "constant");
          auto tail = constant_value<UplcConstant::Items>(args[1], "list");
          auto tail_constant = constant(args[1], "list");
          if (!head || !tail || !tail_constant)
            return std::unexpected(!head ? head.error()
                                         : (!tail ? tail.error() : tail_constant.error()));
          if (head->type() != tail_constant->type().arguments()[1])
            return fail("mkCons element type mismatch");
          tail->insert(tail->begin(), std::move(*head));
          auto list = UplcConstant::list(tail_constant->type().arguments()[1], std::move(*tail));
          return list ? core::Result<RuntimeValue>(RuntimeValue{std::move(*list)})
                      : std::unexpected(list.error());
        }
        auto list = constant_value<UplcConstant::Items>(args[0], "list");
        if (!list) return std::unexpected(list.error());
        if (partial.builtin == Builtin::choose_list) return list->empty() ? args[1] : args[2];
        if (partial.builtin == Builtin::null_list) return boolean(list->empty());
        if (list->empty()) return fail("empty list");
        if (partial.builtin == Builtin::head_list) return RuntimeValue{list->front()};
        auto source = constant(args[0], "list");
        list->erase(list->begin());
        auto result = UplcConstant::list(source->type().arguments()[1], std::move(*list));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::choose_data: {
        auto data = constant_value<Data>(args[0], "data");
        if (!data) return std::unexpected(data.error());
        return std::visit(
            [&](const auto& node) -> core::Result<RuntimeValue> {
              using Node = std::decay_t<decltype(node)>;
              if constexpr (std::is_same_v<Node, std::shared_ptr<chain::ConstrPlutusData>>) {
                return args[1];
              } else if constexpr (std::is_same_v<Node, std::shared_ptr<chain::PlutusMap>>) {
                return args[2];
              } else if constexpr (std::is_same_v<Node, std::shared_ptr<chain::PlutusData::List>>) {
                return args[3];
              } else if constexpr (std::is_same_v<Node, core::BigInteger>) {
                return args[4];
              } else {
                return args[5];
              }
            },
            data->to_plutus_data().node());
      }
      case Builtin::i_data: {
        auto value = constant_value<core::BigInteger>(args[0], "integer");
        return value ? core::Result<RuntimeValue>(
                           RuntimeValue{UplcConstant::data(Data::integer(std::move(*value)))})
                     : std::unexpected(value.error());
      }
      case Builtin::b_data: {
        auto value = constant_value<core::Bytes>(args[0], "bytestring");
        return value ? core::Result<RuntimeValue>(
                           RuntimeValue{UplcConstant::data(Data::bytes(std::move(*value)))})
                     : std::unexpected(value.error());
      }
      case Builtin::list_data: {
        auto list = constant_value<UplcConstant::Items>(args[0], "data list");
        if (!list) return std::unexpected(list.error());
        std::vector<Data> values;
        for (const auto& item : *list) {
          const auto* data = std::get_if<Data>(&item.value());
          if (data == nullptr) return fail("listData expected data values");
          values.push_back(*data);
        }
        return RuntimeValue{UplcConstant::data(Data::list(std::move(values)))};
      }
      case Builtin::map_data: {
        auto list = constant_value<UplcConstant::Items>(args[0], "data pair list");
        if (!list) return std::unexpected(list.error());
        std::vector<std::pair<Data, Data>> entries;
        for (const auto& item : *list) {
          const auto* pair = std::get_if<UplcPair>(&item.value());
          if (pair == nullptr) return fail("mapData expected a list of pairs");
          const auto* key = std::get_if<Data>(&pair->first().value());
          const auto* value = std::get_if<Data>(&pair->second().value());
          if (key == nullptr || value == nullptr)
            return fail("mapData expected Data pair elements");
          entries.emplace_back(*key, *value);
        }
        return RuntimeValue{UplcConstant::data(Data::map(std::move(entries)))};
      }
      case Builtin::constr_data: {
        auto alternative = constant_value<core::BigInteger>(args[0], "integer");
        auto fields = constant_value<UplcConstant::Items>(args[1], "data list");
        if (!alternative || !fields)
          return std::unexpected(!alternative ? alternative.error() : fields.error());
        if (alternative->is_negative()) return fail("negative Data constructor alternative");
        std::vector<Data> values;
        for (const auto& item : *fields) {
          const auto* data = std::get_if<Data>(&item.value());
          if (data == nullptr) return fail("constrData expected data values");
          values.push_back(*data);
        }
        return RuntimeValue{
            UplcConstant::data(Data::constr(std::move(*alternative), std::move(values)))};
      }
      case Builtin::un_i_data:
      case Builtin::un_b_data:
      case Builtin::un_list_data:
      case Builtin::un_map_data:
      case Builtin::un_constr_data:
      case Builtin::equals_data:
      case Builtin::serialise_data: {
        auto data = constant_value<Data>(args[0], "data");
        if (!data) return std::unexpected(data.error());
        if (partial.builtin == Builtin::equals_data) {
          auto right = constant_value<Data>(args[1], "data");
          return right ? core::Result<RuntimeValue>(boolean(*data == *right))
                       : std::unexpected(right.error());
        }
        if (partial.builtin == Builtin::serialise_data) {
          auto encoded = data->to_cbor();
          return encoded ? core::Result<RuntimeValue>(bytes(std::move(*encoded)))
                         : std::unexpected(encoded.error());
        }
        const auto& node = data->to_plutus_data().node();
        if (partial.builtin == Builtin::un_i_data) {
          const auto* value = std::get_if<core::BigInteger>(&node);
          return value != nullptr ? core::Result<RuntimeValue>(integer(*value))
                                  : fail("unIData expected integer data");
        }
        if (partial.builtin == Builtin::un_b_data) {
          const auto* value = std::get_if<core::Bytes>(&node);
          return value != nullptr ? core::Result<RuntimeValue>(bytes(*value))
                                  : fail("unBData expected bytes data");
        }
        if (partial.builtin == Builtin::un_list_data) {
          const auto* value = std::get_if<std::shared_ptr<chain::PlutusData::List>>(&node);
          if (value == nullptr) return fail("unListData expected list data");
          UplcConstant::Items items;
          for (const auto& item : **value)
            items.push_back(UplcConstant::data(Data::from_plutus_data(item)));
          auto list = UplcConstant::list(UplcType::primitive(UplcTypeTag::data), std::move(items));
          return list ? core::Result<RuntimeValue>(RuntimeValue{std::move(*list)})
                      : std::unexpected(list.error());
        }
        if (partial.builtin == Builtin::un_map_data) {
          const auto* value = std::get_if<std::shared_ptr<chain::PlutusMap>>(&node);
          if (value == nullptr) return fail("unMapData expected map data");
          UplcConstant::Items items;
          for (const auto& [key, mapped] : (*value)->entries) {
            auto pair = UplcConstant::pair(UplcConstant::data(Data::from_plutus_data(key)),
                                           UplcConstant::data(Data::from_plutus_data(mapped)));
            if (!pair) return std::unexpected(pair.error());
            items.push_back(std::move(*pair));
          }
          const auto data_type = UplcType::primitive(UplcTypeTag::data);
          auto list = UplcConstant::list(UplcType::pair(data_type, data_type), std::move(items));
          return list ? core::Result<RuntimeValue>(RuntimeValue{std::move(*list)})
                      : std::unexpected(list.error());
        }
        const auto* value = std::get_if<std::shared_ptr<chain::ConstrPlutusData>>(&node);
        if (value == nullptr) return fail("unConstrData expected constructor data");
        UplcConstant::Items fields;
        for (const auto& field : (*value)->fields)
          fields.push_back(UplcConstant::data(Data::from_plutus_data(field)));
        auto list = UplcConstant::list(UplcType::primitive(UplcTypeTag::data), std::move(fields));
        if (!list) return std::unexpected(list.error());
        auto pair =
            UplcConstant::pair(UplcConstant::integer((*value)->alternative), std::move(*list));
        return pair ? core::Result<RuntimeValue>(RuntimeValue{std::move(*pair)})
                    : std::unexpected(pair.error());
      }
      case Builtin::mk_pair_data: {
        auto first = constant(args[0], "data");
        auto second = constant(args[1], "data");
        if (!first || !second) return std::unexpected(!first ? first.error() : second.error());
        auto pair = UplcConstant::pair(std::move(*first), std::move(*second));
        return pair ? core::Result<RuntimeValue>(RuntimeValue{std::move(*pair)})
                    : std::unexpected(pair.error());
      }
      case Builtin::mk_nil_data:
      case Builtin::mk_nil_pair_data: {
        auto unit = constant_value<std::monostate>(args[0], "unit");
        if (!unit) return std::unexpected(unit.error());
        const auto data_type = UplcType::primitive(UplcTypeTag::data);
        auto list = UplcConstant::list(partial.builtin == Builtin::mk_nil_data
                                           ? data_type
                                           : UplcType::pair(data_type, data_type),
                                       {});
        return list ? core::Result<RuntimeValue>(RuntimeValue{std::move(*list)})
                    : std::unexpected(list.error());
      }
      case Builtin::bls12_381_g1_add:
      case Builtin::bls12_381_g2_add:
      case Builtin::bls12_381_g1_equal:
      case Builtin::bls12_381_g2_equal: {
        auto left = constant_value<core::Bytes>(args[0], "BLS point");
        auto right = constant_value<core::Bytes>(args[1], "BLS point");
        if (!left || !right) return std::unexpected(!left ? left.error() : right.error());
        auto left_point = crypto::bls12_381_uncompress(*left);
        auto right_point = crypto::bls12_381_uncompress(*right);
        if (!left_point || !right_point) return fail("invalid compressed BLS point");
        if (partial.builtin == Builtin::bls12_381_g1_equal ||
            partial.builtin == Builtin::bls12_381_g2_equal) {
          return boolean(crypto::bls12_381_equal(*left_point, *right_point));
        }
        auto sum = crypto::bls12_381_add(*left_point, *right_point);
        if (!sum) return std::unexpected(sum.error());
        auto compressed = crypto::bls12_381_compress(*sum);
        auto result = partial.builtin == Builtin::bls12_381_g1_add
                          ? UplcConstant::bls12_381_g1(std::move(compressed))
                          : UplcConstant::bls12_381_g2(std::move(compressed));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::bls12_381_g1_neg:
      case Builtin::bls12_381_g2_neg: {
        auto value = constant_value<core::Bytes>(args[0], "BLS point");
        if (!value) return std::unexpected(value.error());
        auto point = crypto::bls12_381_uncompress(*value);
        if (!point) return std::unexpected(point.error());
        auto compressed = crypto::bls12_381_compress(crypto::bls12_381_neg(*point));
        auto result = partial.builtin == Builtin::bls12_381_g1_neg
                          ? UplcConstant::bls12_381_g1(std::move(compressed))
                          : UplcConstant::bls12_381_g2(std::move(compressed));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::bls12_381_g1_scalar_mul:
      case Builtin::bls12_381_g2_scalar_mul: {
        auto scalar = constant_value<core::BigInteger>(args[0], "integer");
        auto value = constant_value<core::Bytes>(args[1], "BLS point");
        if (!scalar || !value) return std::unexpected(!scalar ? scalar.error() : value.error());
        if (!bls_scalar_in_bounds(*scalar))
          return fail("BLS scalar exceeds the signed 512-byte bound");
        auto point = crypto::bls12_381_uncompress(*value);
        if (!point) return std::unexpected(point.error());
        auto product = crypto::bls12_381_scalar_mul(*scalar, *point);
        if (!product) return std::unexpected(product.error());
        auto compressed = crypto::bls12_381_compress(*product);
        auto result = partial.builtin == Builtin::bls12_381_g1_scalar_mul
                          ? UplcConstant::bls12_381_g1(std::move(compressed))
                          : UplcConstant::bls12_381_g2(std::move(compressed));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::bls12_381_g1_hash_to_group:
      case Builtin::bls12_381_g2_hash_to_group: {
        auto message = constant_value<core::Bytes>(args[0], "bytestring");
        auto domain = constant_value<core::Bytes>(args[1], "bytestring");
        if (!message || !domain)
          return std::unexpected(!message ? message.error() : domain.error());
        const auto group = partial.builtin == Builtin::bls12_381_g1_hash_to_group
                               ? crypto::BlsGroup::g1
                               : crypto::BlsGroup::g2;
        auto point = crypto::bls12_381_hash_to_group(group, *message, *domain);
        if (!point) return std::unexpected(point.error());
        auto compressed = crypto::bls12_381_compress(*point);
        auto result = group == crypto::BlsGroup::g1
                          ? UplcConstant::bls12_381_g1(std::move(compressed))
                          : UplcConstant::bls12_381_g2(std::move(compressed));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::bls12_381_g1_compress:
      case Builtin::bls12_381_g2_compress: {
        auto value = constant_value<core::Bytes>(args[0], "BLS point");
        return value ? core::Result<RuntimeValue>(bytes(std::move(*value)))
                     : std::unexpected(value.error());
      }
      case Builtin::bls12_381_g1_uncompress:
      case Builtin::bls12_381_g2_uncompress: {
        auto value = constant_value<core::Bytes>(args[0], "compressed BLS point");
        if (!value) return std::unexpected(value.error());
        auto result = partial.builtin == Builtin::bls12_381_g1_uncompress
                          ? UplcConstant::bls12_381_g1(std::move(*value))
                          : UplcConstant::bls12_381_g2(std::move(*value));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::bls12_381_miller_loop: {
        auto g1 = constant_value<core::Bytes>(args[0], "BLS G1 point");
        auto g2 = constant_value<core::Bytes>(args[1], "BLS G2 point");
        if (!g1 || !g2) return std::unexpected(!g1 ? g1.error() : g2.error());
        auto g1_point = crypto::bls12_381_uncompress(*g1);
        auto g2_point = crypto::bls12_381_uncompress(*g2);
        if (!g1_point || !g2_point) return fail("invalid compressed BLS point");
        auto result = crypto::bls12_381_miller_loop(*g1_point, *g2_point);
        return result ? core::Result<RuntimeValue>(
                            RuntimeValue{std::make_shared<crypto::BlsMlResult>(std::move(*result))})
                      : std::unexpected(result.error());
      }
      case Builtin::bls12_381_mul_ml_result:
      case Builtin::bls12_381_final_verify: {
        const auto* left = std::get_if<std::shared_ptr<crypto::BlsMlResult>>(&args[0].node);
        const auto* right = std::get_if<std::shared_ptr<crypto::BlsMlResult>>(&args[1].node);
        if (left == nullptr || right == nullptr) return fail("builtin expected BLS ML results");
        if (partial.builtin == Builtin::bls12_381_final_verify)
          return boolean(crypto::bls12_381_final_verify(**left, **right));
        return RuntimeValue{std::make_shared<crypto::BlsMlResult>(
            crypto::bls12_381_mul_ml_result(**left, **right))};
      }
      case Builtin::bls12_381_g1_multi_scalar_mul:
      case Builtin::bls12_381_g2_multi_scalar_mul: {
        auto scalars = constant_value<UplcConstant::Items>(args[0], "integer list");
        auto points = constant_value<UplcConstant::Items>(args[1], "BLS point list");
        if (!scalars || !points)
          return std::unexpected(!scalars ? scalars.error() : points.error());
        const auto group = partial.builtin == Builtin::bls12_381_g1_multi_scalar_mul
                               ? crypto::BlsGroup::g1
                               : crypto::BlsGroup::g2;
        core::Bytes infinity(group == crypto::BlsGroup::g1 ? 48U : 96U, std::byte{0});
        infinity.front() = std::byte{0xc0};
        auto accumulator = crypto::bls12_381_uncompress(infinity);
        if (!accumulator) return std::unexpected(accumulator.error());
        const auto count = std::min(scalars->size(), points->size());
        for (std::size_t index = 0U; index < count; ++index) {
          const auto* scalar = std::get_if<core::BigInteger>(&(*scalars)[index].value());
          const auto* point_bytes = std::get_if<core::Bytes>(&(*points)[index].value());
          if (scalar == nullptr || point_bytes == nullptr)
            return fail("multiScalarMul list element type mismatch");
          if (!bls_scalar_in_bounds(*scalar))
            return fail("multiScalarMul scalar exceeds 512 bytes");
          auto point = crypto::bls12_381_uncompress(*point_bytes);
          if (!point) return std::unexpected(point.error());
          auto product = crypto::bls12_381_scalar_mul(*scalar, *point);
          if (!product) return std::unexpected(product.error());
          auto sum = crypto::bls12_381_add(*accumulator, *product);
          if (!sum) return std::unexpected(sum.error());
          accumulator = std::move(sum);
        }
        auto compressed = crypto::bls12_381_compress(*accumulator);
        auto result = group == crypto::BlsGroup::g1
                          ? UplcConstant::bls12_381_g1(std::move(compressed))
                          : UplcConstant::bls12_381_g2(std::move(compressed));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::integer_to_byte_string: {
        auto big_endian = constant_value<bool>(args[0], "boolean");
        auto requested = constant_value<core::BigInteger>(args[1], "integer");
        auto input = constant_value<core::BigInteger>(args[2], "integer");
        if (!big_endian || !requested || !input)
          return std::unexpected(!big_endian ? big_endian.error()
                                             : (!requested ? requested.error() : input.error()));
        auto length = requested->to_uint64();
        if (!length || *length > 8192U || input->is_negative())
          return fail("integerToByteString arguments are out of range");
        auto output = input->is_zero() ? core::Bytes{} : input->to_unsigned_bytes_be();
        if (*length == 0U && output.size() > 8192U)
          return fail("integerToByteString input exceeds 8192 bytes");
        if (*length != 0U && output.size() > *length)
          return fail("integer does not fit requested byte length");
        const auto desired = *length == 0U ? output.size() : static_cast<std::size_t>(*length);
        if (output.size() < desired) {
          output.insert(output.begin(), desired - output.size(), std::byte{0});
        }
        if (!*big_endian) std::reverse(output.begin(), output.end());
        return bytes(std::move(output));
      }
      case Builtin::byte_string_to_integer: {
        auto big_endian = constant_value<bool>(args[0], "boolean");
        auto input = constant_value<core::Bytes>(args[1], "bytestring");
        if (!big_endian || !input)
          return std::unexpected(!big_endian ? big_endian.error() : input.error());
        if (!*big_endian) std::reverse(input->begin(), input->end());
        return integer(core::BigInteger::from_unsigned_bytes_be(*input));
      }
      case Builtin::and_byte_string:
      case Builtin::or_byte_string:
      case Builtin::xor_byte_string: {
        auto pad = constant_value<bool>(args[0], "boolean");
        auto left = constant_value<core::Bytes>(args[1], "bytestring");
        auto right = constant_value<core::Bytes>(args[2], "bytestring");
        if (!pad || !left || !right)
          return std::unexpected(!pad ? pad.error() : (!left ? left.error() : right.error()));
        const auto length =
            *pad ? std::max(left->size(), right->size()) : std::min(left->size(), right->size());
        core::Bytes output = *pad ? (left->size() >= right->size() ? *left : *right)
                                  : (left->size() <= right->size() ? *left : *right);
        output.resize(length);
        const auto overlap = std::min(left->size(), right->size());
        for (std::size_t index = 0U; index < overlap; ++index) {
          const auto l = std::to_integer<std::uint8_t>((*left)[index]);
          const auto r = std::to_integer<std::uint8_t>((*right)[index]);
          const auto result = partial.builtin == Builtin::and_byte_string  ? (l & r)
                              : partial.builtin == Builtin::or_byte_string ? (l | r)
                                                                           : (l ^ r);
          output[index] = std::byte{static_cast<std::uint8_t>(result)};
        }
        return bytes(std::move(output));
      }
      case Builtin::complement_byte_string: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        if (!input) return std::unexpected(input.error());
        for (auto& byte : *input) byte = ~byte;
        return bytes(std::move(*input));
      }
      case Builtin::read_bit: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        auto index = constant_value<core::BigInteger>(args[1], "integer");
        if (!input || !index) return std::unexpected(!input ? input.error() : index.error());
        auto bit_index = index->to_uint64();
        if (!bit_index || *bit_index >= input->size() * 8U)
          return fail("readBit index is out of range");
        const auto byte_index = input->size() - static_cast<std::size_t>(*bit_index / 8U) - 1U;
        const auto mask = static_cast<std::uint8_t>(1U << (*bit_index % 8U));
        return boolean((std::to_integer<std::uint8_t>((*input)[byte_index]) & mask) != 0U);
      }
      case Builtin::write_bits: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        auto indexes = constant_value<UplcConstant::Items>(args[1], "integer list");
        auto bit = constant_value<bool>(args[2], "boolean");
        if (!input || !indexes || !bit)
          return std::unexpected(!input ? input.error()
                                        : (!indexes ? indexes.error() : bit.error()));
        if (input->size() > 4096U) return fail("writeBits input exceeds 4096 bytes");
        for (const auto& item : *indexes) {
          const auto* integer_value = std::get_if<core::BigInteger>(&item.value());
          if (integer_value == nullptr) return fail("writeBits expected an integer list");
          auto index = integer_value->to_uint64();
          if (!index || *index >= input->size() * 8U)
            return fail("writeBits index is out of range");
          const auto byte_index = input->size() - static_cast<std::size_t>(*index / 8U) - 1U;
          const auto mask = std::byte{static_cast<std::uint8_t>(1U << (*index % 8U))};
          if (*bit)
            (*input)[byte_index] |= mask;
          else
            (*input)[byte_index] &= ~mask;
        }
        return bytes(std::move(*input));
      }
      case Builtin::replicate_byte: {
        auto length = constant_value<core::BigInteger>(args[0], "integer");
        auto byte = constant_value<core::BigInteger>(args[1], "integer");
        if (!length || !byte) return std::unexpected(!length ? length.error() : byte.error());
        auto length_value = length->to_uint64();
        auto byte_value = byte->to_uint64();
        if (!length_value || *length_value > 8192U || !byte_value || *byte_value > 255U)
          return fail("replicateByte arguments are out of range");
        return bytes(core::Bytes(static_cast<std::size_t>(*length_value),
                                 std::byte{static_cast<std::uint8_t>(*byte_value)}));
      }
      case Builtin::shift_byte_string:
      case Builtin::rotate_byte_string: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        auto amount = constant_value<core::BigInteger>(args[1], "integer");
        if (!input || !amount) return std::unexpected(!input ? input.error() : amount.error());
        if (input->empty() || amount->is_zero()) return bytes(std::move(*input));
        if (!amount->fits_int64())
          return fail("bit movement does not fit the semantics integer bound");
        const auto bit_length = static_cast<std::int64_t>(input->size() * 8U);
        std::int64_t move = *amount->to_int64();
        if (partial.builtin == Builtin::rotate_byte_string) {
          move %= bit_length;
          if (move < 0) move += bit_length;
        } else if (move >= bit_length || move <= -bit_length) {
          return bytes(core::Bytes(input->size(), std::byte{0}));
        }
        core::Bytes output(input->size(), std::byte{0});
        auto read = [&](std::int64_t index) {
          const auto byte_index = input->size() - static_cast<std::size_t>(index / 8) - 1U;
          return (std::to_integer<std::uint8_t>((*input)[byte_index]) & (1U << (index % 8))) != 0U;
        };
        auto write = [&](std::int64_t index) {
          const auto byte_index = output.size() - static_cast<std::size_t>(index / 8) - 1U;
          output[byte_index] |= std::byte{static_cast<std::uint8_t>(1U << (index % 8))};
        };
        for (std::int64_t index = 0; index < bit_length; ++index) {
          if (!read(index)) continue;
          auto destination = index + move;
          if (partial.builtin == Builtin::rotate_byte_string) {
            destination %= bit_length;
          }
          if (destination >= 0 && destination < bit_length) write(destination);
        }
        return bytes(std::move(output));
      }
      case Builtin::count_set_bits:
      case Builtin::find_first_set_bit: {
        auto input = constant_value<core::Bytes>(args[0], "bytestring");
        if (!input) return std::unexpected(input.error());
        if (partial.builtin == Builtin::count_set_bits) {
          std::uint64_t count = 0U;
          for (const auto byte : *input)
            count += std::popcount(std::to_integer<std::uint8_t>(byte));
          return integer(core::BigInteger(count));
        }
        std::int64_t found = -1;
        for (std::size_t index = 0U; index < input->size() * 8U; ++index) {
          const auto byte_index = input->size() - index / 8U - 1U;
          if ((std::to_integer<std::uint8_t>((*input)[byte_index]) & (1U << (index % 8U))) != 0U) {
            found = static_cast<std::int64_t>(index);
            break;
          }
        }
        return integer(core::BigInteger(found));
      }
      case Builtin::exp_mod_integer: {
        auto base = constant_value<core::BigInteger>(args[0], "integer");
        auto exponent = constant_value<core::BigInteger>(args[1], "integer");
        auto modulus = constant_value<core::BigInteger>(args[2], "integer");
        if (!base || !exponent || !modulus)
          return std::unexpected(!base ? base.error()
                                       : (!exponent ? exponent.error() : modulus.error()));
        auto result = modular_power(std::move(*base), std::move(*exponent), *modulus);
        return result ? core::Result<RuntimeValue>(integer(std::move(*result)))
                      : std::unexpected(result.error());
      }
      case Builtin::drop_list: {
        auto count = constant_value<core::BigInteger>(args[0], "integer");
        auto list = constant_value<UplcConstant::Items>(args[1], "list");
        auto source = constant(args[1], "list");
        if (!count || !list || !source)
          return std::unexpected(!count ? count.error() : (!list ? list.error() : source.error()));
        std::size_t drop = 0U;
        if (!count->is_negative()) {
          auto converted = count->to_uint64();
          drop = converted ? std::min<std::size_t>(*converted, list->size()) : list->size();
        }
        list->erase(list->begin(), list->begin() + static_cast<std::ptrdiff_t>(drop));
        auto result = UplcConstant::list(source->type().arguments()[1], std::move(*list));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::length_of_array:
      case Builtin::list_to_array:
      case Builtin::index_array: {
        auto input = constant_value<UplcConstant::Items>(args[0], "array or list");
        auto source = constant(args[0], "array or list");
        if (!input || !source) return std::unexpected(!input ? input.error() : source.error());
        if (partial.builtin == Builtin::length_of_array)
          return integer(core::BigInteger(static_cast<std::uint64_t>(input->size())));
        if (partial.builtin == Builtin::list_to_array) {
          auto result = UplcConstant::array(source->type().arguments()[1], std::move(*input));
          return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                        : std::unexpected(result.error());
        }
        auto index = constant_value<core::BigInteger>(args[1], "integer");
        if (!index) return std::unexpected(index.error());
        auto converted = index->to_uint64();
        if (!converted || *converted >= input->size()) return fail("array index is out of range");
        return RuntimeValue{(*input)[*converted]};
      }
      case Builtin::insert_coin: {
        auto policy = constant_value<core::Bytes>(args[0], "bytestring");
        auto asset = constant_value<core::Bytes>(args[1], "bytestring");
        auto quantity = constant_value<core::BigInteger>(args[2], "integer");
        auto value = constant_value<Data>(args[3], "value");
        if (!policy || !asset || !quantity || !value)
          return std::unexpected(
              !policy ? policy.error()
                      : (!asset ? asset.error() : (!quantity ? quantity.error() : value.error())));
        auto map = decode_value_map(*value);
        if (!map) return std::unexpected(map.error());
        if (quantity->is_zero()) {
          const auto policy_it = map->find(*policy);
          if (policy_it != map->end()) {
            policy_it->second.erase(*asset);
            if (policy_it->second.empty()) map->erase(policy_it);
          }
        } else {
          if (policy->size() > 32U || asset->size() > 32U || !signed_128(*quantity))
            return fail("insertCoin key or quantity is out of range");
          (*map)[*policy][*asset] = *quantity;
        }
        auto result = UplcConstant::value(encode_value_map(*map));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::lookup_coin: {
        auto policy = constant_value<core::Bytes>(args[0], "bytestring");
        auto asset = constant_value<core::Bytes>(args[1], "bytestring");
        auto value = constant_value<Data>(args[2], "value");
        if (!policy || !asset || !value)
          return std::unexpected(!policy ? policy.error()
                                         : (!asset ? asset.error() : value.error()));
        auto map = decode_value_map(*value);
        if (!map) return std::unexpected(map.error());
        const auto policy_it = map->find(*policy);
        if (policy_it == map->end()) return integer(core::BigInteger(std::int64_t{0}));
        const auto asset_it = policy_it->second.find(*asset);
        return integer(asset_it == policy_it->second.end() ? core::BigInteger(std::int64_t{0})
                                                           : asset_it->second);
      }
      case Builtin::union_value:
      case Builtin::value_contains: {
        auto left_data = constant_value<Data>(args[0], "value");
        auto right_data = constant_value<Data>(args[1], "value");
        if (!left_data || !right_data)
          return std::unexpected(!left_data ? left_data.error() : right_data.error());
        auto left = decode_value_map(*left_data);
        auto right = decode_value_map(*right_data);
        if (!left || !right) return std::unexpected(!left ? left.error() : right.error());
        if (partial.builtin == Builtin::value_contains) {
          for (const auto& [policy, tokens] : *left)
            for (const auto& [asset, quantity] : tokens) {
              static_cast<void>(policy);
              static_cast<void>(asset);
              if (quantity.is_negative()) return fail("valueContains rejects negative quantities");
            }
          for (const auto& [policy, tokens] : *right) {
            for (const auto& [asset, quantity] : tokens) {
              if (quantity.is_negative()) return fail("valueContains rejects negative quantities");
              core::BigInteger available(std::int64_t{0});
              const auto policy_it = left->find(policy);
              if (policy_it != left->end()) {
                const auto asset_it = policy_it->second.find(asset);
                if (asset_it != policy_it->second.end()) available = asset_it->second;
              }
              if (available < quantity) return boolean(false);
            }
          }
          return boolean(true);
        }
        for (const auto& [policy, tokens] : *right) {
          for (const auto& [asset, quantity] : tokens) {
            auto& destination = (*left)[policy][asset];
            destination += quantity;
            if (!signed_128(destination)) return fail("unionValue quantity overflow");
          }
        }
        for (auto policy = left->begin(); policy != left->end();) {
          for (auto asset = policy->second.begin(); asset != policy->second.end();) {
            if (asset->second.is_zero())
              asset = policy->second.erase(asset);
            else
              ++asset;
          }
          if (policy->second.empty())
            policy = left->erase(policy);
          else
            ++policy;
        }
        auto result = UplcConstant::value(encode_value_map(*left));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      case Builtin::value_data: {
        auto value = constant_value<Data>(args[0], "value");
        if (!value) return std::unexpected(value.error());
        auto map = decode_value_map(*value);
        if (!map) return std::unexpected(map.error());
        std::size_t size = 0U;
        for (const auto& [policy, tokens] : *map) {
          static_cast<void>(policy);
          size += tokens.size();
        }
        if (size > 40'000U) return fail("valueData input exceeds 40000 entries");
        return RuntimeValue{UplcConstant::data(encode_value_map(*map))};
      }
      case Builtin::un_value_data: {
        auto data = constant_value<Data>(args[0], "data");
        if (!data) return std::unexpected(data.error());
        auto map = decode_value_map(*data);
        if (!map) return std::unexpected(map.error());
        auto value = UplcConstant::value(encode_value_map(*map));
        return value ? core::Result<RuntimeValue>(RuntimeValue{std::move(*value)})
                     : std::unexpected(value.error());
      }
      case Builtin::scale_value: {
        auto scalar = constant_value<core::BigInteger>(args[0], "integer");
        auto value = constant_value<Data>(args[1], "value");
        if (!scalar || !value) return std::unexpected(!scalar ? scalar.error() : value.error());
        auto map = decode_value_map(*value);
        if (!map) return std::unexpected(map.error());
        if (scalar->is_zero())
          map->clear();
        else {
          for (auto& [policy, tokens] : *map) {
            static_cast<void>(policy);
            for (auto& [asset, quantity] : tokens) {
              static_cast<void>(asset);
              quantity *= *scalar;
              if (!signed_128(quantity)) return fail("scaleValue quantity overflow");
            }
          }
        }
        auto result = UplcConstant::value(encode_value_map(*map));
        return result ? core::Result<RuntimeValue>(RuntimeValue{std::move(*result)})
                      : std::unexpected(result.error());
      }
      default:
        return fail("UPLC builtin '" + std::string(builtin_name(partial.builtin)) +
                    "' is not implemented by this evaluator");
    }
  }

  core::Result<UplcTerm> close_term(const UplcTerm& term,
                                    const std::vector<RuntimeValue>& environment,
                                    std::size_t binders) const {
    return std::visit(
        [&](const auto& node) -> core::Result<UplcTerm> {
          using Node = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<Node, UplcVariable>) {
            if (node.index <= binders) return term;
            const auto environment_index = node.index - binders;
            if (environment_index == 0U || environment_index > environment.size()) {
              return std::unexpected(uplc_error(core::ErrorCode::evaluation,
                                                "closure contains an unresolved variable"));
            }
            return quote(environment[environment.size() - environment_index]);
          } else if constexpr (std::is_same_v<Node, UplcDelay>) {
            auto closed = close_term(*node.term, environment, binders);
            return closed ? core::Result<UplcTerm>(UplcTerm::delay(std::move(*closed)))
                          : std::unexpected(closed.error());
          } else if constexpr (std::is_same_v<Node, UplcLambda>) {
            auto closed = close_term(*node.body, environment, binders + 1U);
            return closed ? core::Result<UplcTerm>(UplcTerm::lambda(std::move(*closed)))
                          : std::unexpected(closed.error());
          } else if constexpr (std::is_same_v<Node, UplcApply>) {
            auto function = close_term(*node.function, environment, binders);
            auto argument = close_term(*node.argument, environment, binders);
            return function && argument
                       ? core::Result<UplcTerm>(
                             UplcTerm::apply(std::move(*function), std::move(*argument)))
                       : std::unexpected(!function ? function.error() : argument.error());
          } else if constexpr (std::is_same_v<Node, UplcForce>) {
            auto closed = close_term(*node.term, environment, binders);
            return closed ? core::Result<UplcTerm>(UplcTerm::force(std::move(*closed)))
                          : std::unexpected(closed.error());
          } else if constexpr (std::is_same_v<Node, UplcConstr>) {
            std::vector<UplcTerm> fields;
            for (const auto& field : node.fields) {
              auto closed = close_term(field, environment, binders);
              if (!closed) return std::unexpected(closed.error());
              fields.push_back(std::move(*closed));
            }
            return UplcTerm::constr(node.tag, std::move(fields));
          } else if constexpr (std::is_same_v<Node, UplcCase>) {
            auto scrutinee = close_term(*node.scrutinee, environment, binders);
            if (!scrutinee) return std::unexpected(scrutinee.error());
            std::vector<UplcTerm> branches;
            for (const auto& branch : node.branches) {
              auto closed = close_term(branch, environment, binders);
              if (!closed) return std::unexpected(closed.error());
              branches.push_back(std::move(*closed));
            }
            return UplcTerm::case_of(std::move(*scrutinee), std::move(branches));
          } else {
            return term;
          }
        },
        term.node());
  }

  core::Result<UplcTerm> quote(const RuntimeValue& value) const {
    if (const auto* constant = std::get_if<UplcConstant>(&value.node))
      return UplcTerm::constant(*constant);
    if (const auto* closure = std::get_if<std::shared_ptr<RuntimeValue::Closure>>(&value.node)) {
      auto body = close_term((*closure)->body, (*closure)->environment, 1U);
      return body ? core::Result<UplcTerm>(UplcTerm::lambda(std::move(*body)))
                  : std::unexpected(body.error());
    }
    if (const auto* delayed = std::get_if<std::shared_ptr<RuntimeValue::Delayed>>(&value.node)) {
      auto term = close_term((*delayed)->term, (*delayed)->environment, 0U);
      return term ? core::Result<UplcTerm>(UplcTerm::delay(std::move(*term)))
                  : std::unexpected(term.error());
    }
    if (const auto* constructor =
            std::get_if<std::shared_ptr<RuntimeValue::Constructor>>(&value.node)) {
      std::vector<UplcTerm> fields;
      for (const auto& field : (*constructor)->fields) {
        auto term = quote(field);
        if (!term) return std::unexpected(term.error());
        fields.push_back(std::move(*term));
      }
      return UplcTerm::constr((*constructor)->tag, std::move(fields));
    }
    if (std::holds_alternative<std::shared_ptr<crypto::BlsMlResult>>(value.node)) {
      return std::unexpected(
          uplc_error(core::ErrorCode::unsupported,
                     "BLS12-381 ML results cannot be exported from the CEK runtime"));
    }
    const auto& partial = **std::get_if<std::shared_ptr<RuntimeValue::PartialBuiltin>>(&value.node);
    UplcTerm result = UplcTerm::builtin(partial.builtin);
    for (std::size_t index = 0U; index < partial.forces; ++index)
      result = UplcTerm::force(std::move(result));
    for (const auto& argument : partial.arguments) {
      auto term = quote(argument);
      if (!term) return std::unexpected(term.error());
      result = UplcTerm::apply(std::move(result), std::move(*term));
    }
    return result;
  }

  MachineBudget maximum_;
  MachineCosts machine_costs_;
  BuiltinCostModel builtin_costs_;
  MachineBudget spent_{};
  std::vector<std::string> logs_;
  CostStream stream_;
};

}  // namespace

core::Result<MachineResult> evaluate_program(const UplcProgram& program, MachineBudget maximum,
                                             SemanticsVariant semantics, MachineCosts machine_costs,
                                             BuiltinCostModel builtin_costs) {
  auto version = validate_version(program.version);
  if (!version) return std::unexpected(version.error());
  if (maximum.cpu < 0 || maximum.memory < 0) {
    return std::unexpected(
        uplc_error(core::ErrorCode::invalid_argument, "UPLC maximum budget cannot be negative"));
  }
  builtin_costs.semantics = semantics;
  return Evaluator(maximum, semantics, std::move(machine_costs), std::move(builtin_costs))
      .run(program);
}

}  // namespace cardano::plutus
