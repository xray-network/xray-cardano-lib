#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cardano/chain/address.hpp"
#include "cardano/chain/ledger.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/core/random.hpp"
#include "cardano/crypto/fixed_bytes.hpp"

namespace cardano::chain {

class AssetName {
 public:
  [[nodiscard]] static core::Result<AssetName> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<AssetName> from_hex(std::string_view hex);
  [[nodiscard]] const core::Bytes& bytes() const noexcept;
  [[nodiscard]] std::string hex() const;
  friend bool operator==(const AssetName&, const AssetName&) = default;
  friend auto operator<=>(const AssetName&, const AssetName&) = default;

 private:
  explicit AssetName(core::Bytes bytes);
  core::Bytes bytes_;
};

struct AssetQuantity {
  AssetName name;
  std::int64_t quantity{};
  friend bool operator==(const AssetQuantity&, const AssetQuantity&) = default;
};

struct PolicyAssets {
  crypto::ScriptHash policy;
  std::vector<AssetQuantity> assets;
  friend bool operator==(const PolicyAssets&, const PolicyAssets&) = default;
};

class Value {
 public:
  explicit Value(std::uint64_t coin = 0);
  Value(std::uint64_t coin, std::vector<PolicyAssets> multiasset);

  [[nodiscard]] std::uint64_t coin() const noexcept;
  void set_coin(std::uint64_t coin) noexcept;
  [[nodiscard]] const std::vector<PolicyAssets>& multiasset() const noexcept;
  [[nodiscard]] bool has_assets() const noexcept;
  [[nodiscard]] std::int64_t asset_quantity(const crypto::ScriptHash& policy,
                                            const AssetName& asset) const noexcept;

  [[nodiscard]] core::Result<Value> checked_add(const Value& other) const;
  [[nodiscard]] core::Result<Value> checked_sub(const Value& other) const;
  [[nodiscard]] bool covers(const Value& other) const noexcept;
  [[nodiscard]] core::cbor::Value to_cbor_value(bool signed_assets = false) const;
  [[nodiscard]] core::Result<core::Bytes> to_cbor() const;
  [[nodiscard]] std::string to_json() const;

  friend bool operator==(const Value&, const Value&) = default;

 private:
  std::uint64_t coin_{};
  std::vector<PolicyAssets> multiasset_;
};

class TransactionInput {
 public:
  TransactionInput(crypto::TransactionHash transaction_id, std::uint64_t index);
  [[nodiscard]] static core::Result<TransactionInput> from_json(std::string_view json);
  [[nodiscard]] const crypto::TransactionHash& transaction_id() const noexcept;
  [[nodiscard]] std::uint64_t index() const noexcept;
  [[nodiscard]] core::cbor::Value to_cbor_value() const;
  [[nodiscard]] std::string canonical_identity() const;
  [[nodiscard]] std::string to_json() const;
  friend bool operator==(const TransactionInput&, const TransactionInput&) = default;

 private:
  crypto::TransactionHash transaction_id_;
  std::uint64_t index_;
};

class TransactionOutput {
 public:
  TransactionOutput(Address address, Value amount);
  void set_datum_option(core::cbor::Value datum_option);
  [[nodiscard]] core::Result<std::monostate> set_communication_datum(
      const core::cbor::Value& datum);
  void set_script_reference(core::cbor::Value script_reference);

  [[nodiscard]] const Address& address() const noexcept;
  [[nodiscard]] const Value& amount() const noexcept;
  [[nodiscard]] Value& amount() noexcept;
  [[nodiscard]] const std::optional<core::cbor::Value>& datum_option() const noexcept;
  [[nodiscard]] const std::optional<core::cbor::Value>& script_reference() const noexcept;
  [[nodiscard]] const std::optional<core::cbor::Value>& communication_datum() const noexcept;
  [[nodiscard]] core::cbor::Value to_cbor_value() const;

 private:
  Address address_;
  Value amount_;
  std::optional<core::cbor::Value> datum_option_;
  std::optional<core::cbor::Value> script_reference_;
  std::optional<core::cbor::Value> communication_datum_;
};

struct TransactionUnspentOutput {
  TransactionInput input;
  TransactionOutput output;
};

struct TransactionBuilderConfig {
  LinearFee linear_fee;
  std::uint64_t reference_script_cost_per_byte{};
  std::uint64_t pool_deposit{};
  std::uint64_t key_deposit{};
  std::uint32_t max_value_size{};
  std::uint32_t max_transaction_size{};
  std::uint64_t coins_per_utxo_byte{};
  ExUnitPrices ex_unit_prices;
  std::uint32_t collateral_percentage{};
  std::uint32_t max_collateral_inputs{};
  core::cbor::Value cost_models{core::cbor::Value::map({})};
  bool prefer_pure_change{false};
};

class TransactionBuilderConfigBuilder {
 public:
  TransactionBuilderConfigBuilder& linear_fee(LinearFee value);
  TransactionBuilderConfigBuilder& reference_script_cost_per_byte(std::uint64_t value);
  TransactionBuilderConfigBuilder& pool_deposit(std::uint64_t value);
  TransactionBuilderConfigBuilder& key_deposit(std::uint64_t value);
  TransactionBuilderConfigBuilder& max_value_size(std::uint32_t value);
  TransactionBuilderConfigBuilder& max_transaction_size(std::uint32_t value);
  TransactionBuilderConfigBuilder& coins_per_utxo_byte(std::uint64_t value);
  TransactionBuilderConfigBuilder& ex_unit_prices(ExUnitPrices value);
  TransactionBuilderConfigBuilder& collateral_percentage(std::uint32_t value);
  TransactionBuilderConfigBuilder& max_collateral_inputs(std::uint32_t value);
  TransactionBuilderConfigBuilder& cost_models(core::cbor::Value value);
  TransactionBuilderConfigBuilder& prefer_pure_change(bool value);
  [[nodiscard]] core::Result<TransactionBuilderConfig> build() const;

 private:
  TransactionBuilderConfig config_;
  std::uint16_t required_mask_{};
};

enum class CoinSelectionStrategyCIP2 : std::uint8_t {
  largest_first = 0,
  random_improve = 1,
  largest_first_multi_asset = 2,
  random_improve_multi_asset = 3
};

enum class ChangeSelectionAlgo : std::uint8_t { default_selection = 0 };

enum class NativeScriptWitnessMode : std::uint8_t {
  signature_count,
  vkeys,
  assume_signature_count
};

class NativeScriptWitnessInfo {
 public:
  [[nodiscard]] static NativeScriptWitnessInfo signature_count(std::size_t count);
  [[nodiscard]] static NativeScriptWitnessInfo vkeys(std::vector<crypto::Ed25519KeyHash> hashes);
  [[nodiscard]] static NativeScriptWitnessInfo assume_signature_count();
  [[nodiscard]] NativeScriptWitnessMode mode() const noexcept;
  [[nodiscard]] std::size_t count() const noexcept;
  [[nodiscard]] const std::vector<crypto::Ed25519KeyHash>& hashes() const noexcept;

 private:
  NativeScriptWitnessInfo(NativeScriptWitnessMode mode, std::size_t count,
                          std::vector<crypto::Ed25519KeyHash> hashes);
  NativeScriptWitnessMode mode_;
  std::size_t count_{};
  std::vector<crypto::Ed25519KeyHash> hashes_;
};

class PlutusScript {
 public:
  [[nodiscard]] static core::Result<PlutusScript> create(std::uint8_t language,
                                                         core::ByteSpan bytes);
  [[nodiscard]] std::uint8_t language() const noexcept;
  [[nodiscard]] const core::Bytes& bytes() const noexcept;
  [[nodiscard]] crypto::ScriptHash hash() const;
  [[nodiscard]] core::cbor::Value to_script() const;

 private:
  PlutusScript(std::uint8_t language, core::Bytes bytes);
  std::uint8_t language_;
  core::Bytes bytes_;
};

class PlutusScriptWitness {
 public:
  [[nodiscard]] static PlutusScriptWitness reference(crypto::ScriptHash hash);
  [[nodiscard]] static PlutusScriptWitness inline_script(PlutusScript script);
  [[nodiscard]] const crypto::ScriptHash& hash() const noexcept;
  [[nodiscard]] const std::optional<PlutusScript>& script() const noexcept;

 private:
  PlutusScriptWitness(crypto::ScriptHash hash, std::optional<PlutusScript> script);
  crypto::ScriptHash hash_;
  std::optional<PlutusScript> script_;
};

class PartialPlutusWitness {
 public:
  PartialPlutusWitness(PlutusScriptWitness script, core::cbor::Value data);
  [[nodiscard]] const PlutusScriptWitness& script() const noexcept;
  [[nodiscard]] const core::cbor::Value& data() const noexcept;

 private:
  PlutusScriptWitness script_;
  core::cbor::Value data_;
};

class InputAggregateWitnessData {
 public:
  enum class Kind : std::uint8_t { native, plutus };

  [[nodiscard]] static InputAggregateWitnessData native(crypto::ScriptHash hash,
                                                        core::cbor::Value script,
                                                        NativeScriptWitnessInfo info);
  [[nodiscard]] static InputAggregateWitnessData plutus(
      PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers,
      std::optional<core::cbor::Value> datum = std::nullopt);
  [[nodiscard]] Kind kind() const noexcept;
  [[nodiscard]] const crypto::ScriptHash& script_hash() const noexcept;
  [[nodiscard]] const std::optional<core::cbor::Value>& native_script() const noexcept;
  [[nodiscard]] const std::optional<NativeScriptWitnessInfo>& native_info() const noexcept;
  [[nodiscard]] const std::optional<PartialPlutusWitness>& plutus_witness() const noexcept;
  [[nodiscard]] const std::vector<crypto::Ed25519KeyHash>& signers() const noexcept;
  [[nodiscard]] const std::optional<core::cbor::Value>& datum() const noexcept;

 private:
  InputAggregateWitnessData(Kind kind, crypto::ScriptHash script_hash,
                            std::optional<core::cbor::Value> native_script,
                            std::optional<NativeScriptWitnessInfo> native_info,
                            std::optional<PartialPlutusWitness> plutus_witness,
                            std::vector<crypto::Ed25519KeyHash> signers,
                            std::optional<core::cbor::Value> datum);
  Kind kind_;
  crypto::ScriptHash script_hash_;
  std::optional<core::cbor::Value> native_script_;
  std::optional<NativeScriptWitnessInfo> native_info_;
  std::optional<PartialPlutusWitness> plutus_witness_;
  std::vector<crypto::Ed25519KeyHash> signers_;
  std::optional<core::cbor::Value> datum_;
};

struct InputBuilderResult {
  TransactionUnspentOutput utxo;
  std::optional<crypto::Ed25519KeyHash> required_vkey;
  std::optional<InputAggregateWitnessData> aggregate;
};

enum class RedeemerPurpose : std::uint8_t {
  spend = 0,
  mint = 1,
  certificate = 2,
  reward = 3,
  voting = 4,
  proposing = 5
};

struct RedeemerWitnessKey {
  RedeemerPurpose purpose{};
  std::string sort_key;
  friend bool operator==(const RedeemerWitnessKey&, const RedeemerWitnessKey&) = default;
};

struct UntaggedRedeemer {
  core::cbor::Value data;
  std::optional<ExUnits> ex_units;
};

class TxRedeemerBuilder {
 public:
  TxRedeemerBuilder(RedeemerWitnessKey key, core::cbor::Value data);
  TxRedeemerBuilder& ex_units(ExUnits value);
  [[nodiscard]] RedeemerWitnessKey key() const;
  [[nodiscard]] UntaggedRedeemer build() const;

 private:
  RedeemerWitnessKey key_;
  UntaggedRedeemer redeemer_;
};

class RedeemerSetBuilder {
 public:
  void add(RedeemerWitnessKey key, UntaggedRedeemer redeemer);
  void set_ex_units(RedeemerPurpose purpose, std::uint64_t final_index, ExUnits ex_units);
  [[nodiscard]] core::Result<core::cbor::Value> build(bool dummy_ex_units = false) const;
  [[nodiscard]] bool empty() const noexcept;

 private:
  std::vector<std::pair<RedeemerWitnessKey, UntaggedRedeemer>> redeemers_;
  std::map<std::string, ExUnits> overrides_;
};

struct RequiredWitnessSet {
  std::set<std::string> vkeys;
  std::set<std::string> bootstraps;
  std::set<std::string> native_scripts;
  std::set<std::string> plutus_v1_scripts;
  std::set<std::string> plutus_v2_scripts;
  std::set<std::string> plutus_v3_scripts;
  std::set<std::string> datums;
  std::set<std::string> redeemers;
  std::set<std::string> script_references;

  [[nodiscard]] bool empty() const noexcept;
  void merge(const RequiredWitnessSet& other);
};

class SingleInputBuilder {
 public:
  explicit SingleInputBuilder(TransactionUnspentOutput utxo);
  SingleInputBuilder(TransactionInput input, TransactionOutput output);
  [[nodiscard]] core::Result<InputBuilderResult> payment_key() const;
  [[nodiscard]] core::Result<InputBuilderResult> native_script(crypto::ScriptHash hash,
                                                               core::cbor::Value script,
                                                               NativeScriptWitnessInfo info) const;
  [[nodiscard]] core::Result<InputBuilderResult> plutus_script(
      PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers,
      std::optional<core::cbor::Value> datum = std::nullopt) const;

 private:
  TransactionUnspentOutput utxo_;
};

struct MintBuilderResult {
  PolicyAssets mint;
  InputAggregateWitnessData aggregate;
  RequiredWitnessSet required;
};

class SingleMintBuilder {
 public:
  [[nodiscard]] static core::Result<SingleMintBuilder> create(std::vector<AssetQuantity> assets);
  [[nodiscard]] core::Result<MintBuilderResult> native_script(crypto::ScriptHash hash,
                                                              core::cbor::Value script,
                                                              NativeScriptWitnessInfo info) const;
  [[nodiscard]] core::Result<MintBuilderResult> plutus_script(
      PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers) const;

 private:
  explicit SingleMintBuilder(std::vector<AssetQuantity> assets);
  std::vector<AssetQuantity> assets_;
};

struct WithdrawalBuilderResult {
  Address address;
  std::uint64_t amount{};
  std::optional<InputAggregateWitnessData> aggregate;
  RequiredWitnessSet required;
};

class SingleWithdrawalBuilder {
 public:
  SingleWithdrawalBuilder(Address reward_address, std::uint64_t amount);
  [[nodiscard]] core::Result<WithdrawalBuilderResult> payment_key() const;
  [[nodiscard]] core::Result<WithdrawalBuilderResult> native_script(
      crypto::ScriptHash hash, core::cbor::Value script, NativeScriptWitnessInfo info) const;
  [[nodiscard]] core::Result<WithdrawalBuilderResult> plutus_script(
      PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers) const;

 private:
  Address reward_address_;
  std::uint64_t amount_;
};

struct CertificateBuilderResult {
  core::cbor::Value certificate;
  std::optional<InputAggregateWitnessData> aggregate;
  RequiredWitnessSet required;
};

class SingleCertificateBuilder {
 public:
  explicit SingleCertificateBuilder(core::cbor::Value certificate);
  [[nodiscard]] core::Result<CertificateBuilderResult> skip_witness() const;
  [[nodiscard]] core::Result<CertificateBuilderResult> payment_key() const;
  [[nodiscard]] core::Result<CertificateBuilderResult> native_script(
      crypto::ScriptHash hash, core::cbor::Value script, NativeScriptWitnessInfo info) const;
  [[nodiscard]] core::Result<CertificateBuilderResult> plutus_script(
      PartialPlutusWitness witness, std::vector<crypto::Ed25519KeyHash> signers) const;

 private:
  core::cbor::Value certificate_;
};

struct ProposalBuilderEntry {
  core::cbor::Value proposal;
  std::optional<InputAggregateWitnessData> aggregate;
};
struct ProposalBuilderResult {
  std::vector<ProposalBuilderEntry> entries;
};
class ProposalBuilder {
 public:
  ProposalBuilder& add(core::cbor::Value proposal);
  ProposalBuilder& add_native(core::cbor::Value proposal, crypto::ScriptHash hash,
                              core::cbor::Value script, NativeScriptWitnessInfo info);
  ProposalBuilder& add_plutus(core::cbor::Value proposal, PartialPlutusWitness witness,
                              std::vector<crypto::Ed25519KeyHash> signers,
                              std::optional<core::cbor::Value> datum = std::nullopt);
  [[nodiscard]] ProposalBuilderResult build() const;

 private:
  std::vector<ProposalBuilderEntry> entries_;
};

struct VoteBuilderEntry {
  core::cbor::Value voter;
  core::cbor::Value action;
  core::cbor::Value procedure;
  std::optional<InputAggregateWitnessData> aggregate;
};
struct VoteBuilderResult {
  std::vector<VoteBuilderEntry> entries;
};
class VoteBuilder {
 public:
  VoteBuilder& add(core::cbor::Value voter, core::cbor::Value action, core::cbor::Value procedure);
  VoteBuilder& add_native(core::cbor::Value voter, core::cbor::Value action,
                          core::cbor::Value procedure, crypto::ScriptHash hash,
                          core::cbor::Value script, NativeScriptWitnessInfo info);
  VoteBuilder& add_plutus(core::cbor::Value voter, core::cbor::Value action,
                          core::cbor::Value procedure, PartialPlutusWitness witness,
                          std::vector<crypto::Ed25519KeyHash> signers,
                          std::optional<core::cbor::Value> datum = std::nullopt);
  [[nodiscard]] VoteBuilderResult build() const;

 private:
  void insert(VoteBuilderEntry entry);
  std::vector<VoteBuilderEntry> entries_;
};

class TransactionWitnessSetBuilder {
 public:
  void require_vkey(crypto::Ed25519KeyHash hash);
  void require_bootstrap(core::ByteSpan public_key);
  void require_native_script(crypto::ScriptHash hash);
  void require_plutus_script(std::uint8_t language, crypto::ScriptHash hash);
  void require_datum(crypto::DatumHash hash);
  void require_redeemer(RedeemerWitnessKey key);
  [[nodiscard]] core::VoidResult satisfy_script_reference(std::uint8_t language,
                                                          crypto::ScriptHash hash);

  [[nodiscard]] core::VoidResult add_vkey_witness(core::cbor::Value witness);
  void add_bootstrap_witness(core::ByteSpan public_key, core::cbor::Value witness);
  [[nodiscard]] core::VoidResult add_native_script(crypto::ScriptHash hash,
                                                   core::cbor::Value script);
  [[nodiscard]] core::VoidResult add_plutus_script(std::uint8_t language, crypto::ScriptHash hash,
                                                   core::Bytes script);
  void add_datum(crypto::DatumHash hash, core::cbor::Value datum);
  void add_redeemer(RedeemerWitnessKey key, UntaggedRedeemer redeemer);
  void set_redeemer_ex_units(RedeemerPurpose purpose, std::uint64_t final_index, ExUnits ex_units);

  [[nodiscard]] core::Result<core::cbor::Value> build() const;
  [[nodiscard]] core::Result<core::cbor::Value> build_unchecked(bool fake_vkeys = false,
                                                                bool dummy_ex_units = false) const;
  [[nodiscard]] const RequiredWitnessSet& missing() const noexcept;

 private:
  RequiredWitnessSet missing_;
  std::vector<std::pair<std::string, core::cbor::Value>> vkeys_;
  std::vector<std::pair<std::string, core::cbor::Value>> bootstraps_;
  std::vector<std::pair<std::string, core::cbor::Value>> native_scripts_;
  std::vector<std::pair<std::string, core::Bytes>> plutus_v1_scripts_;
  std::vector<std::pair<std::string, core::Bytes>> plutus_v2_scripts_;
  std::vector<std::pair<std::string, core::Bytes>> plutus_v3_scripts_;
  std::vector<std::pair<std::string, core::cbor::Value>> datums_;
  RedeemerSetBuilder redeemers_;
};

class SignedTxBuilder {
 public:
  SignedTxBuilder(core::cbor::Value body, TransactionWitnessSetBuilder witnesses,
                  std::optional<core::cbor::Value> auxiliary_data = std::nullopt);
  [[nodiscard]] core::VoidResult sign(const crypto::PrivateKey& private_key);
  [[nodiscard]] core::Result<core::cbor::Value> build() const;
  [[nodiscard]] core::Result<core::cbor::Value> build_unchecked() const;

 private:
  core::cbor::Value body_;
  TransactionWitnessSetBuilder witnesses_;
  std::optional<core::cbor::Value> auxiliary_data_;
};

struct SingleOutputBuilderResult {
  TransactionOutput output;
};

class TransactionOutputAmountBuilder {
 public:
  TransactionOutputAmountBuilder(Address address, std::optional<core::cbor::Value> datum,
                                 std::optional<core::cbor::Value> script_reference,
                                 std::optional<core::cbor::Value> communication_datum);
  TransactionOutputAmountBuilder& with_value(Value value);
  [[nodiscard]] core::VoidResult with_asset_and_min_required_coin(
      std::vector<PolicyAssets> assets, std::uint64_t coins_per_utxo_byte);
  [[nodiscard]] core::Result<SingleOutputBuilderResult> build() const;

 private:
  Address address_;
  std::optional<core::cbor::Value> datum_;
  std::optional<core::cbor::Value> script_reference_;
  std::optional<core::cbor::Value> communication_datum_;
  std::optional<Value> amount_;
};

class TransactionOutputBuilder {
 public:
  TransactionOutputBuilder& with_address(Address address);
  TransactionOutputBuilder& with_data(core::cbor::Value datum);
  TransactionOutputBuilder& with_communication_data(core::cbor::Value datum);
  TransactionOutputBuilder& with_reference_script(core::cbor::Value script);
  [[nodiscard]] core::Result<TransactionOutputAmountBuilder> next() const;
  [[nodiscard]] core::Result<SingleOutputBuilderResult> next(Value amount) const;

 private:
  std::optional<Address> address_;
  std::optional<core::cbor::Value> datum_;
  std::optional<core::cbor::Value> script_reference_;
  std::optional<core::cbor::Value> communication_datum_;
};

class TransactionBuilder {
 public:
  explicit TransactionBuilder(TransactionBuilderConfig config);

  [[nodiscard]] core::VoidResult add_input(TransactionUnspentOutput input);
  [[nodiscard]] core::VoidResult add_input(InputBuilderResult input);
  void add_utxo(TransactionUnspentOutput candidate);
  [[nodiscard]] core::VoidResult add_output(TransactionOutput output);
  void add_reference_input(TransactionInput input);
  [[nodiscard]] core::VoidResult add_reference_input(TransactionUnspentOutput input);
  [[nodiscard]] core::VoidResult set_mint(std::vector<PolicyAssets> mint);
  [[nodiscard]] core::VoidResult add_mint(MintBuilderResult mint);
  [[nodiscard]] core::VoidResult add_withdrawal(Address reward_address, std::uint64_t coin);
  [[nodiscard]] core::VoidResult add_withdrawal(WithdrawalBuilderResult withdrawal);
  void add_certificate(core::cbor::Value certificate);
  [[nodiscard]] core::VoidResult add_certificate(CertificateBuilderResult certificate);
  void add_proposal(core::cbor::Value proposal);
  [[nodiscard]] core::VoidResult add_proposals(ProposalBuilderResult proposals);
  [[nodiscard]] core::VoidResult add_votes(VoteBuilderResult votes);
  [[nodiscard]] core::VoidResult add_collateral(InputBuilderResult collateral);
  void set_collateral_return(TransactionOutput output);
  void add_required_signer(crypto::Ed25519KeyHash signer);
  void set_current_treasury_value(std::uint64_t value);
  void set_fee(std::uint64_t fee);
  void set_redeemer_ex_units(RedeemerPurpose purpose, std::uint64_t final_index, ExUnits ex_units);
  void set_donation(std::uint64_t donation);
  void set_ttl(std::uint64_t ttl);
  void set_validity_start(std::uint64_t validity_start);
  void set_network_id(std::uint8_t network_id);
  void set_auxiliary_data(core::cbor::Value auxiliary_data);
  void merge_auxiliary_data(core::cbor::Value auxiliary_data);

  [[nodiscard]] core::VoidResult select_utxos(CoinSelectionStrategyCIP2 strategy,
                                              core::SecureRandomSource* random = nullptr);
  [[nodiscard]] core::Result<bool> add_change_if_needed(
      const Address& change_address,
      ChangeSelectionAlgo algorithm = ChangeSelectionAlgo::default_selection);

  [[nodiscard]] core::Result<std::uint64_t> min_fee(bool include_exunits = true) const;
  [[nodiscard]] core::Result<core::cbor::Value> build_body() const;
  [[nodiscard]] core::Result<core::cbor::Value> build_for_evaluation() const;
  [[nodiscard]] core::Result<core::cbor::Value> build_transaction() const;
  [[nodiscard]] core::Result<SignedTxBuilder> build_signed() const;
  [[nodiscard]] core::Result<Value> total_input() const;
  [[nodiscard]] core::Result<Value> total_output() const;
  [[nodiscard]] const std::vector<TransactionUnspentOutput>& inputs() const noexcept;
  [[nodiscard]] const std::vector<TransactionOutput>& outputs() const noexcept;
  [[nodiscard]] std::optional<std::uint64_t> fee() const noexcept;

 private:
  [[nodiscard]] bool contains_input(const TransactionInput& input) const;
  [[nodiscard]] core::Result<bool> covered() const;
  [[nodiscard]] core::VoidResult select_candidate(std::size_t index);
  [[nodiscard]] core::Result<std::vector<TransactionOutput>> make_change(const Address& address,
                                                                         const Value& change) const;
  [[nodiscard]] core::Result<core::cbor::Value> build_body_with_fee(std::uint64_t fee,
                                                                    bool enforce_size) const;

  TransactionBuilderConfig config_;
  std::vector<TransactionUnspentOutput> inputs_;
  std::vector<TransactionUnspentOutput> candidates_;
  std::vector<TransactionOutput> outputs_;
  std::vector<TransactionInput> reference_inputs_;
  std::vector<TransactionUnspentOutput> reference_utxos_;
  std::vector<InputBuilderResult> collateral_;
  std::optional<TransactionOutput> collateral_return_;
  std::vector<VoteBuilderEntry> votes_;
  std::vector<crypto::Ed25519KeyHash> required_signers_;
  std::vector<std::pair<Address, std::uint64_t>> withdrawals_;
  std::vector<core::cbor::Value> certificates_;
  std::vector<core::cbor::Value> proposals_;
  std::vector<PolicyAssets> mint_;
  std::optional<std::uint64_t> donation_;
  std::optional<std::uint64_t> ttl_;
  std::optional<std::uint64_t> validity_start_;
  std::optional<std::uint8_t> network_id_;
  std::optional<std::uint64_t> current_treasury_value_;
  std::optional<core::cbor::Value> auxiliary_data_;
  std::optional<std::uint64_t> fee_;
  TransactionWitnessSetBuilder witnesses_;
  RedeemerSetBuilder redeemers_;
};

}  // namespace cardano::chain
