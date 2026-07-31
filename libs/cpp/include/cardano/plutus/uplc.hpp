#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cardano/core/big_integer.hpp"
#include "cardano/core/bytes.hpp"
#include "cardano/core/error.hpp"
#include "cardano/plutus/data.hpp"

namespace cardano::plutus {

struct UplcVersion {
  std::uint64_t major{1};
  std::uint64_t minor{0};
  std::uint64_t patch{0};

  [[nodiscard]] static constexpr UplcVersion v1_0_0() noexcept { return {1, 0, 0}; }
  [[nodiscard]] static constexpr UplcVersion v1_1_0() noexcept { return {1, 1, 0}; }

  friend auto operator<=>(const UplcVersion&, const UplcVersion&) = default;
};

enum class SemanticsVariant { a, b, c, d, e };

enum class PlutusLanguage : std::uint8_t { v1 = 0, v2 = 1, v3 = 2 };

enum class UplcTypeTag : std::uint8_t {
  integer = 0,
  byte_string = 1,
  string = 2,
  unit = 3,
  boolean = 4,
  proto_list = 5,
  proto_pair = 6,
  apply = 7,
  data = 8,
  bls12_381_g1 = 9,
  bls12_381_g2 = 10,
  bls12_381_ml_result = 11,
  proto_array = 12,
  value = 13
};

class UplcType {
 public:
  [[nodiscard]] static UplcType primitive(UplcTypeTag tag);
  [[nodiscard]] static UplcType list(UplcType item);
  [[nodiscard]] static UplcType array(UplcType item);
  [[nodiscard]] static UplcType pair(UplcType first, UplcType second);
  [[nodiscard]] static UplcType apply(UplcType function, UplcType argument);

  [[nodiscard]] UplcTypeTag tag() const noexcept;
  [[nodiscard]] const std::vector<UplcType>& arguments() const noexcept;
  [[nodiscard]] bool is_value_type() const noexcept;

  friend bool operator==(const UplcType&, const UplcType&) = default;

 private:
  UplcType(UplcTypeTag tag, std::vector<UplcType> arguments);
  UplcTypeTag tag_;
  std::vector<UplcType> arguments_;
};

class UplcConstant;

struct UplcPair {
  UplcConstant* first_ptr{};
  UplcConstant* second_ptr{};

  UplcPair(UplcConstant first, UplcConstant second);
  UplcPair(const UplcPair& other);
  UplcPair(UplcPair&& other) noexcept;
  UplcPair& operator=(const UplcPair& other);
  UplcPair& operator=(UplcPair&& other) noexcept;
  ~UplcPair();

  [[nodiscard]] const UplcConstant& first() const noexcept;
  [[nodiscard]] const UplcConstant& second() const noexcept;
  friend bool operator==(const UplcPair&, const UplcPair&);
};

using UplcData = Data;

class UplcConstant {
 public:
  using Items = std::vector<UplcConstant>;
  using Value = std::variant<core::BigInteger, core::Bytes, std::string, std::monostate, bool,
                             Items, UplcPair, Data>;

  [[nodiscard]] static UplcConstant integer(core::BigInteger value);
  [[nodiscard]] static UplcConstant bytes(core::Bytes value);
  [[nodiscard]] static core::Result<UplcConstant> string(std::string value);
  [[nodiscard]] static UplcConstant unit();
  [[nodiscard]] static UplcConstant boolean(bool value);
  [[nodiscard]] static core::Result<UplcConstant> list(UplcType item_type, Items values);
  [[nodiscard]] static core::Result<UplcConstant> array(UplcType item_type, Items values);
  [[nodiscard]] static core::Result<UplcConstant> pair(UplcConstant first, UplcConstant second);
  [[nodiscard]] static UplcConstant data(Data value);
  [[nodiscard]] static core::Result<UplcConstant> bls12_381_g1(core::Bytes compressed);
  [[nodiscard]] static core::Result<UplcConstant> bls12_381_g2(core::Bytes compressed);
  [[nodiscard]] static core::Result<UplcConstant> bls12_381_ml_result(core::Bytes value);
  [[nodiscard]] static core::Result<UplcConstant> value(Data value);

  [[nodiscard]] const UplcType& type() const noexcept;
  [[nodiscard]] const Value& value() const noexcept;
  [[nodiscard]] std::size_t memory_words(SemanticsVariant semantics = SemanticsVariant::e) const;

  friend bool operator==(const UplcConstant&, const UplcConstant&) = default;

 private:
  UplcConstant(UplcType type, Value value);
  UplcType type_;
  Value value_;
};

using FlatValue = UplcConstant::Value;

enum class Builtin : std::uint8_t {
  add_integer = 0,
  subtract_integer,
  multiply_integer,
  divide_integer,
  quotient_integer,
  remainder_integer,
  mod_integer,
  equals_integer,
  less_than_integer,
  less_than_equals_integer,
  append_byte_string,
  cons_byte_string,
  slice_byte_string,
  length_of_byte_string,
  index_byte_string,
  equals_byte_string,
  less_than_byte_string,
  less_than_equals_byte_string,
  sha2_256,
  sha3_256,
  blake2b_256,
  verify_ed25519_signature,
  append_string,
  equals_string,
  encode_utf8,
  decode_utf8,
  if_then_else,
  choose_unit,
  trace,
  fst_pair,
  snd_pair,
  choose_list,
  mk_cons,
  head_list,
  tail_list,
  null_list,
  choose_data,
  constr_data,
  map_data,
  list_data,
  i_data,
  b_data,
  un_constr_data,
  un_map_data,
  un_list_data,
  un_i_data,
  un_b_data,
  equals_data,
  mk_pair_data,
  mk_nil_data,
  mk_nil_pair_data,
  serialise_data,
  verify_ecdsa_secp256k1_signature,
  verify_schnorr_secp256k1_signature,
  bls12_381_g1_add,
  bls12_381_g1_neg,
  bls12_381_g1_scalar_mul,
  bls12_381_g1_equal,
  bls12_381_g1_hash_to_group,
  bls12_381_g1_compress,
  bls12_381_g1_uncompress,
  bls12_381_g2_add,
  bls12_381_g2_neg,
  bls12_381_g2_scalar_mul,
  bls12_381_g2_equal,
  bls12_381_g2_hash_to_group,
  bls12_381_g2_compress,
  bls12_381_g2_uncompress,
  bls12_381_miller_loop,
  bls12_381_mul_ml_result,
  bls12_381_final_verify,
  keccak_256,
  blake2b_224,
  integer_to_byte_string,
  byte_string_to_integer,
  and_byte_string,
  or_byte_string,
  xor_byte_string,
  complement_byte_string,
  read_bit,
  write_bits,
  replicate_byte,
  shift_byte_string,
  rotate_byte_string,
  count_set_bits,
  find_first_set_bit,
  ripemd_160,
  exp_mod_integer,
  drop_list,
  length_of_array,
  list_to_array,
  index_array,
  bls12_381_g1_multi_scalar_mul,
  bls12_381_g2_multi_scalar_mul,
  insert_coin,
  lookup_coin,
  union_value,
  value_contains,
  value_data,
  un_value_data,
  scale_value
};

[[nodiscard]] core::Result<Builtin> builtin_tag(std::uint8_t tag);
[[nodiscard]] std::uint8_t builtin_tag(Builtin builtin) noexcept;
[[nodiscard]] std::string_view builtin_name(Builtin builtin) noexcept;
[[nodiscard]] core::Result<Builtin> builtin_from_name(std::string_view name);

class UplcTerm;

struct UplcVariable {
  std::uint64_t index{};
  friend bool operator==(const UplcVariable&, const UplcVariable&) = default;
};
struct UplcDelay {
  std::shared_ptr<const UplcTerm> term;
  friend bool operator==(const UplcDelay&, const UplcDelay&);
};
struct UplcLambda {
  std::shared_ptr<const UplcTerm> body;
  friend bool operator==(const UplcLambda&, const UplcLambda&);
};
struct UplcApply {
  std::shared_ptr<const UplcTerm> function;
  std::shared_ptr<const UplcTerm> argument;
  friend bool operator==(const UplcApply&, const UplcApply&);
};
struct UplcForce {
  std::shared_ptr<const UplcTerm> term;
  friend bool operator==(const UplcForce&, const UplcForce&);
};
struct UplcError {
  friend bool operator==(const UplcError&, const UplcError&) = default;
};
struct UplcConstr {
  std::uint64_t tag{};
  std::vector<UplcTerm> fields;
  friend bool operator==(const UplcConstr&, const UplcConstr&) = default;
};
struct UplcCase {
  std::shared_ptr<const UplcTerm> scrutinee;
  std::vector<UplcTerm> branches;
  friend bool operator==(const UplcCase&, const UplcCase&);
};

class UplcTerm {
 public:
  using Node = std::variant<UplcVariable, UplcDelay, UplcLambda, UplcApply, UplcConstant, UplcForce,
                            UplcError, Builtin, UplcConstr, UplcCase>;

  [[nodiscard]] static UplcTerm variable(std::uint64_t index);
  [[nodiscard]] static UplcTerm delay(UplcTerm term);
  [[nodiscard]] static UplcTerm lambda(UplcTerm body);
  [[nodiscard]] static UplcTerm apply(UplcTerm function, UplcTerm argument);
  [[nodiscard]] static UplcTerm constant(UplcConstant value);
  [[nodiscard]] static UplcTerm force(UplcTerm term);
  [[nodiscard]] static UplcTerm error();
  [[nodiscard]] static UplcTerm builtin(Builtin value);
  [[nodiscard]] static UplcTerm constr(std::uint64_t tag, std::vector<UplcTerm> fields);
  [[nodiscard]] static UplcTerm case_of(UplcTerm scrutinee, std::vector<UplcTerm> branches);

  [[nodiscard]] const Node& node() const noexcept;
  friend bool operator==(const UplcTerm&, const UplcTerm&) = default;

 private:
  explicit UplcTerm(Node node);
  Node node_;
};

struct UplcProgram {
  UplcVersion version;
  UplcTerm term;
  friend bool operator==(const UplcProgram&, const UplcProgram&) = default;
};

struct ProgramDecodeOptions {
  std::size_t max_depth{512};
  std::size_t max_nodes{1'000'000};
  std::size_t max_constant_bytes{65'536};
  std::size_t max_universe_header{1'000'000};
  std::size_t max_constr_fields{1'000'000};
  bool enforce_data_wire_limit{true};
  bool require_complete_input{true};
  std::optional<std::uint64_t> protocol_major;
};

struct MachineBudget {
  std::int64_t cpu{};
  std::int64_t memory{};
  friend bool operator==(const MachineBudget&, const MachineBudget&) = default;
};
using UplcExBudget = MachineBudget;

struct MachineCosts {
  MachineBudget startup{100, 100};
  MachineBudget variable{16'000, 100};
  MachineBudget constant{16'000, 100};
  MachineBudget lambda{16'000, 100};
  MachineBudget delay{16'000, 100};
  MachineBudget force{16'000, 100};
  MachineBudget apply{16'000, 100};
  MachineBudget builtin{16'000, 100};
  MachineBudget constr{16'000, 100};
  MachineBudget case_cost{16'000, 100};
};

struct CostStream {
  std::vector<MachineBudget> costs;
  [[nodiscard]] MachineBudget total() const noexcept;
};

struct BuiltinCostModel {
  SemanticsVariant semantics{SemanticsVariant::a};
  std::optional<PlutusLanguage> language;
  std::vector<std::int64_t> ledger_parameters;
  std::map<Builtin, std::vector<std::int64_t>> parameters;
};

struct MachineResult {
  UplcTerm result;
  MachineBudget spent;
  std::vector<std::string> logs;
  CostStream stream;
};

struct PhaseTwoEvaluation {
  MachineBudget cost;
  std::vector<std::string> logs;
};

struct PhaseTwoRawEvaluation {
  core::Bytes redeemer;
  PhaseTwoEvaluation evaluation;
};

using PhaseTwoUtxo = std::pair<core::Bytes, core::Bytes>;

[[nodiscard]] MachineCosts default_machine_costs(SemanticsVariant semantics = SemanticsVariant::e);
[[nodiscard]] BuiltinCostModel default_builtin_cost_model(
    SemanticsVariant semantics = SemanticsVariant::e);
[[nodiscard]] BuiltinCostModel make_builtin_cost_model(SemanticsVariant semantics,
                                                       std::span<const std::int64_t> parameters);
[[nodiscard]] BuiltinCostModel make_builtin_cost_model(PlutusLanguage language,
                                                       std::span<const std::int64_t> parameters,
                                                       SemanticsVariant semantics);
[[nodiscard]] MachineBudget builtin_cost(Builtin builtin, std::span<const UplcConstant> arguments,
                                         const BuiltinCostModel& model);

[[nodiscard]] core::Result<UplcProgram> parse_uplc_text(std::string_view text,
                                                        ProgramDecodeOptions options = {});
[[nodiscard]] core::Result<core::Bytes> encode_flat_program(const UplcProgram& program,
                                                            ProgramDecodeOptions options = {});
[[nodiscard]] core::Result<UplcProgram> decode_flat_program(core::ByteSpan bytes,
                                                            ProgramDecodeOptions options = {});
[[nodiscard]] core::Result<core::Bytes> encode_program_envelope(const UplcProgram& program,
                                                                ProgramDecodeOptions options = {});
[[nodiscard]] core::Result<UplcProgram> decode_program_envelope(core::ByteSpan bytes,
                                                                ProgramDecodeOptions options = {});
[[nodiscard]] core::Result<UplcProgram> decode_program_envelope_compatible(
    core::ByteSpan bytes, std::uint64_t plutus_language, ProgramDecodeOptions options = {});
[[nodiscard]] core::Result<core::Bytes> encode_plutus_data(const UplcData& data);
[[nodiscard]] core::Result<UplcData> validate_plutus_data_node(const core::cbor::Value& value,
                                                               std::size_t max_depth = 512,
                                                               bool enforce_wire_limit = true);

[[nodiscard]] core::Result<MachineResult> evaluate_program(
    const UplcProgram& program, MachineBudget maximum,
    SemanticsVariant semantics = SemanticsVariant::e,
    MachineCosts machine_costs = default_machine_costs(),
    BuiltinCostModel builtin_costs = default_builtin_cost_model());

[[nodiscard]] core::Result<core::Bytes> apply_params_to_script(core::ByteSpan script_envelope,
                                                               std::span<const Data> parameters,
                                                               ProgramDecodeOptions options = {});
[[nodiscard]] core::Result<core::Bytes> apply_params_to_script(core::ByteSpan parameters_cbor,
                                                               core::ByteSpan script_envelope,
                                                               ProgramDecodeOptions options = {});

[[nodiscard]] core::Result<std::vector<PhaseTwoRawEvaluation>> eval_phase_two_raw(
    core::ByteSpan transaction_cbor, std::span<const PhaseTwoUtxo> utxos,
    core::ByteSpan cost_models_cbor, MachineBudget maximum, std::array<std::int64_t, 3> slot_config,
    std::uint64_t protocol_major, bool run_phase_one);

}  // namespace cardano::plutus
