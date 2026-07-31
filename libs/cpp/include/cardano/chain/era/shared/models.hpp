#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cardano/chain/address.hpp"
#include "cardano/chain/builder.hpp"
#include "cardano/chain/ledger.hpp"
#include "cardano/chain/plutus_data.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/core/error.hpp"

namespace cardano::chain {

enum class EraWireShape : std::uint8_t { any, array, map };

namespace detail {
[[nodiscard]] core::VoidResult validate_era_model(std::string_view name, EraWireShape shape,
                                                  const core::cbor::Value& value);
[[nodiscard]] core::Result<core::cbor::Value> era_model_value_from_json(std::string_view name,
                                                                        std::string_view json);
[[nodiscard]] core::Result<std::string> era_model_value_to_json(std::string_view name,
                                                                const core::cbor::Value& value);
}  // namespace detail

template <typename Derived, EraWireShape Shape = EraWireShape::any>
class EraCborModel {
 public:
  [[nodiscard]] static core::Result<Derived> from_cbor(core::ByteSpan bytes) {
    auto value = core::cbor::decode_cbor(bytes);
    return value ? from_value(std::move(*value)) : std::unexpected(value.error());
  }
  [[nodiscard]] static core::Result<Derived> from_cbor_hex(std::string_view hex) {
    auto bytes = core::hex_to_bytes(hex);
    return bytes ? from_cbor(*bytes) : std::unexpected(bytes.error());
  }
  [[nodiscard]] static core::Result<Derived> from_json(std::string_view json) {
    auto value = detail::era_model_value_from_json(Derived::model_name, json);
    return value ? from_value(std::move(*value)) : std::unexpected(value.error());
  }
  [[nodiscard]] static core::Result<Derived> from_value(core::cbor::Value value) {
    auto valid = detail::validate_era_model(Derived::model_name, Shape, value);
    if (!valid) return std::unexpected(valid.error());
    return Derived(std::move(value));
  }

  [[nodiscard]] const core::cbor::Value& cbor() const noexcept { return value_; }
  [[nodiscard]] core::Result<core::Bytes> to_cbor(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const {
    return core::cbor::encode_cbor(value_, {.mode = mode});
  }
  [[nodiscard]] core::Result<std::string> to_json() const {
    return detail::era_model_value_to_json(Derived::model_name, value_);
  }
  [[nodiscard]] std::size_t len() const noexcept {
    if (const auto* array = value_.as_array()) return array->values.size();
    if (const auto* map = value_.as_map()) return map->entries.size();
    return 0;
  }
  [[nodiscard]] core::Result<core::cbor::Value> get(std::size_t index) const {
    if (const auto* array = value_.as_array()) {
      if (index < array->values.size()) return array->values[index];
    }
    return std::unexpected(core::CardanoError(core::ErrorCode::out_of_range,
                                              "era model index is outside the collection"));
  }
  [[nodiscard]] std::optional<core::cbor::Value> field(std::uint64_t key) const {
    const auto* map = value_.as_map();
    if (map == nullptr) return std::nullopt;
    for (const auto& [candidate, item] : map->entries) {
      const auto* number = candidate.as_unsigned();
      if (number == nullptr) continue;
      auto converted = number->value.to_uint64();
      if (converted && *converted == key) return item;
    }
    return std::nullopt;
  }
  [[nodiscard]] core::Result<std::uint64_t> discriminator() const {
    const auto* array = value_.as_array();
    if (array == nullptr || array->values.empty() ||
        array->values.front().as_unsigned() == nullptr) {
      return std::unexpected(core::CardanoError(core::ErrorCode::invalid_structure,
                                                "era model has no unsigned discriminator"));
    }
    return array->values.front().as_unsigned()->value.to_uint64();
  }

 protected:
  explicit EraCborModel(core::cbor::Value value) : value_(std::move(value)) {}
  core::cbor::Value value_;
};

#define CARDANO_ERA_MODEL(name)                                                      \
  class name final : public EraCborModel<name> {                                     \
   public:                                                                           \
    static constexpr std::string_view model_name = #name;                            \
    explicit name(core::cbor::Value value) : EraCborModel<name>(std::move(value)) {} \
  }

#define CARDANO_ERA_ARRAY_MODEL(name)                                  \
  class name final : public EraCborModel<name, EraWireShape::array> {  \
   public:                                                             \
    static constexpr std::string_view model_name = #name;              \
    explicit name(core::cbor::Value value)                             \
        : EraCborModel<name, EraWireShape::array>(std::move(value)) {} \
  }

#define CARDANO_ERA_MAP_MODEL(name)                                  \
  class name final : public EraCborModel<name, EraWireShape::map> {  \
   public:                                                           \
    static constexpr std::string_view model_name = #name;            \
    explicit name(core::cbor::Value value)                           \
        : EraCborModel<name, EraWireShape::map>(std::move(value)) {} \
  }

enum class MIRActionKind : std::uint8_t { to_stake_credentials = 0, to_other_pot = 1 };
enum class MIRPot : std::uint8_t { reserve = 0, treasury = 1 };
enum class AllegraAuxiliaryDataKind : std::uint8_t { shelley = 0, shelley_ma = 1 };
enum class AllegraCertificateKind : std::uint8_t {
  stake_registration = 0,
  stake_deregistration = 1,
  stake_delegation = 2,
  pool_registration = 3,
  pool_retirement = 4,
  genesis_key_delegation = 5,
  move_instantaneous_rewards = 6
};
enum class AlonzoAuxiliaryDataKind : std::uint8_t { shelley = 0, shelley_ma = 1, alonzo = 2 };
enum class AlonzoRedeemerTag : std::uint8_t { spend = 0, mint = 1, certificate = 2, reward = 3 };
enum class BabbageAuxiliaryDataKind : std::uint8_t { shelley = 0, shelley_ma = 1, babbage = 2 };
enum class BabbageScriptKind : std::uint8_t { native = 0, plutus_v1 = 1, plutus_v2 = 2 };
enum class BabbageTransactionOutputKind : std::uint8_t { alonzo = 0, babbage = 1 };
enum class ByronBlockKind : std::uint8_t { epoch_boundary = 0, main = 1 };
enum class ByronBlockSignatureKind : std::uint8_t {
  signature = 0,
  proxy_light = 1,
  proxy_heavy = 2
};
enum class ByronTxInKind : std::uint8_t { regular = 0, genesis = 1 };
enum class ByronTxWitnessKind : std::uint8_t { public_key = 0, script = 1, redeem = 2 };
enum class SscKind : std::uint8_t { commitments = 0, openings = 1, shares = 2, certificates = 3 };
enum class SscProofKind : std::uint8_t {
  commitments = 0,
  openings = 1,
  shares = 2,
  certificates = 3
};
enum class AuxiliaryDataKind : std::uint8_t { shelley = 0, shelley_ma = 1, conway = 2 };
enum class CertificateKind : std::uint8_t {
  stake_registration = 0,
  stake_deregistration = 1,
  stake_delegation = 2,
  pool_registration = 3,
  pool_retirement = 4,
  registration = 7,
  unregistration = 8,
  vote_delegation = 9,
  stake_vote_delegation = 10,
  stake_registration_delegation = 11,
  vote_registration_delegation = 12,
  stake_vote_registration_delegation = 13,
  authorize_committee_hot = 14,
  resign_committee_cold = 15,
  register_drep = 16,
  unregister_drep = 17,
  update_drep = 18
};
enum class DRepKind : std::uint8_t {
  key = 0,
  script = 1,
  always_abstain = 2,
  always_no_confidence = 3
};
enum class DatumOptionKind : std::uint8_t { hash = 0, datum = 1 };
enum class GovActionKind : std::uint8_t {
  parameter_change = 0,
  hard_fork = 1,
  treasury_withdrawals = 2,
  no_confidence = 3,
  update_committee = 4,
  new_constitution = 5,
  information = 6
};
enum class Language : std::uint8_t { plutus_v1 = 0, plutus_v2 = 1, plutus_v3 = 2 };
enum class NativeScriptKind : std::uint8_t {
  public_key = 0,
  all = 1,
  any = 2,
  n_of_k = 3,
  invalid_before = 4,
  invalid_hereafter = 5
};
enum class NonceKind : std::uint8_t { identity = 0, hash = 1 };
enum class PlutusDataKind : std::uint8_t {
  constructor = 0,
  map = 1,
  list = 2,
  integer = 3,
  bytes = 4
};
enum class RedeemerTag : std::uint8_t {
  spend = 0,
  mint = 1,
  certificate = 2,
  reward = 3,
  voting = 4,
  proposing = 5
};
enum class RedeemersKind : std::uint8_t { legacy_array = 0, map = 1 };
enum class RelayKind : std::uint8_t {
  single_host_address = 0,
  single_host_name = 1,
  multi_host_name = 2
};
enum class ScriptKind : std::uint8_t { native = 0, plutus_v1 = 1, plutus_v2 = 2, plutus_v3 = 3 };
enum class TransactionMetadatumKind : std::uint8_t {
  map = 0,
  list = 1,
  integer = 2,
  bytes = 3,
  text = 4
};
enum class TransactionOutputKind : std::uint8_t { alonzo = 0, conway = 1 };
enum class Vote : std::uint8_t { no = 0, yes = 1, abstain = 2 };
enum class VoterKind : std::uint8_t {
  committee_hot_key = 0,
  committee_hot_script = 1,
  drep_key = 2,
  drep_script = 3,
  staking_pool_key = 4
};
enum class MultisigScriptKind : std::uint8_t { public_key = 0, all = 1, any = 2, n_of_k = 3 };
enum class ShelleyCertificateKind : std::uint8_t {
  stake_registration = 0,
  stake_deregistration = 1,
  stake_delegation = 2,
  pool_registration = 3,
  pool_retirement = 4,
  genesis_key_delegation = 5,
  move_instantaneous_rewards = 6
};
enum class ShelleyRelayKind : std::uint8_t {
  single_host_address = 0,
  single_host_name = 1,
  multi_host_name = 2
};

CARDANO_ERA_ARRAY_MODEL(MIRAction);
CARDANO_ERA_ARRAY_MODEL(MoveInstantaneousReward);
CARDANO_ERA_ARRAY_MODEL(MoveInstantaneousRewardsCert);
CARDANO_ERA_MODEL(AllegraAuxiliaryData);
CARDANO_ERA_ARRAY_MODEL(AllegraBlock);
CARDANO_ERA_ARRAY_MODEL(AllegraCertificate);
CARDANO_ERA_ARRAY_MODEL(AllegraTransaction);
CARDANO_ERA_MAP_MODEL(AllegraTransactionBody);
CARDANO_ERA_MAP_MODEL(AllegraTransactionWitnessSet);
CARDANO_ERA_MODEL(AlonzoAuxiliaryData);
CARDANO_ERA_ARRAY_MODEL(AlonzoBlock);
CARDANO_ERA_MODEL(AlonzoFormatAuxData);
CARDANO_ERA_MAP_MODEL(AlonzoProposedProtocolParameterUpdates);
CARDANO_ERA_MAP_MODEL(AlonzoProtocolParamUpdate);
CARDANO_ERA_ARRAY_MODEL(AlonzoRedeemer);
CARDANO_ERA_ARRAY_MODEL(AlonzoTransaction);
CARDANO_ERA_MAP_MODEL(AlonzoTransactionBody);
CARDANO_ERA_MAP_MODEL(AlonzoTransactionWitnessSet);
CARDANO_ERA_ARRAY_MODEL(AlonzoUpdate);
CARDANO_ERA_MODEL(BabbageAuxiliaryData);
CARDANO_ERA_ARRAY_MODEL(BabbageBlock);
CARDANO_ERA_MODEL(BabbageFormatAuxData);
CARDANO_ERA_MAP_MODEL(BabbageFormatTxOut);
CARDANO_ERA_MAP_MODEL(BabbageProposedProtocolParameterUpdates);
CARDANO_ERA_MAP_MODEL(BabbageProtocolParamUpdate);
CARDANO_ERA_ARRAY_MODEL(BabbageScript);
CARDANO_ERA_MODEL(BabbageScriptRef);
CARDANO_ERA_ARRAY_MODEL(BabbageTransaction);
CARDANO_ERA_MAP_MODEL(BabbageTransactionBody);
CARDANO_ERA_MODEL(BabbageTransactionOutput);
CARDANO_ERA_MAP_MODEL(BabbageTransactionWitnessSet);
CARDANO_ERA_ARRAY_MODEL(BabbageUpdate);
CARDANO_ERA_ARRAY_MODEL(SoftForkRule);
CARDANO_ERA_MODEL(BlockHeaderExtraData);
CARDANO_ERA_MODEL(Bvermod);
CARDANO_ERA_MODEL(ByronAny);
CARDANO_ERA_ARRAY_MODEL(ByronAttributes);
CARDANO_ERA_MODEL(ByronBlock);
CARDANO_ERA_ARRAY_MODEL(ByronBlockBody);
CARDANO_ERA_ARRAY_MODEL(ByronBlockConsensusData);
CARDANO_ERA_ARRAY_MODEL(ByronBlockHeader);
CARDANO_ERA_MODEL(ByronBlockSignature);
CARDANO_ERA_ARRAY_MODEL(ByronBlockSignatureNormal);
CARDANO_ERA_ARRAY_MODEL(ByronBlockSignatureProxyHeavy);
CARDANO_ERA_ARRAY_MODEL(ByronBlockSignatureProxyLight);
CARDANO_ERA_ARRAY_MODEL(ByronBlockVersion);
CARDANO_ERA_ARRAY_MODEL(ByronBodyProof);
CARDANO_ERA_ARRAY_MODEL(ByronDelegation);
CARDANO_ERA_MODEL(ByronDelegationSignature);
CARDANO_ERA_ARRAY_MODEL(ByronDifficulty);
CARDANO_ERA_ARRAY_MODEL(ByronEbBlock);
CARDANO_ERA_ARRAY_MODEL(ByronMainBlock);
CARDANO_ERA_ARRAY_MODEL(ByronPkWitness);
CARDANO_ERA_ARRAY_MODEL(ByronPkWitnessEntry);
CARDANO_ERA_ARRAY_MODEL(ByronRedeemWitness);
CARDANO_ERA_MODEL(ByronRedeemerScript);
CARDANO_ERA_ARRAY_MODEL(ByronRedeemerWitnessEntry);
CARDANO_ERA_ARRAY_MODEL(ByronScriptWitness);
CARDANO_ERA_ARRAY_MODEL(ByronScriptWitnessEntry);
CARDANO_ERA_ARRAY_MODEL(ByronSlotId);
CARDANO_ERA_ARRAY_MODEL(ByronSoftwareVersion);
CARDANO_ERA_ARRAY_MODEL(ByronTx);
CARDANO_ERA_ARRAY_MODEL(ByronTxFeePolicy);
CARDANO_ERA_MODEL(ByronTxIn);
CARDANO_ERA_ARRAY_MODEL(ByronTxInGenesis);
CARDANO_ERA_ARRAY_MODEL(ByronTxInRegular);
CARDANO_ERA_ARRAY_MODEL(ByronTxOut);
CARDANO_ERA_ARRAY_MODEL(ByronTxOutPtr);
CARDANO_ERA_ARRAY_MODEL(ByronTxProof);
CARDANO_ERA_MODEL(ByronTxWitness);
CARDANO_ERA_ARRAY_MODEL(ByronUpdate);
CARDANO_ERA_ARRAY_MODEL(ByronUpdateData);
CARDANO_ERA_ARRAY_MODEL(ByronUpdateProposal);
CARDANO_ERA_ARRAY_MODEL(ByronUpdateVote);
CARDANO_ERA_MODEL(ByronValidatorScript);
CARDANO_ERA_ARRAY_MODEL(EbbConsensusData);
CARDANO_ERA_ARRAY_MODEL(EbbHead);
CARDANO_ERA_ARRAY_MODEL(EpochRange);
CARDANO_ERA_MODEL(LightWeightDelegationSignature);
CARDANO_ERA_ARRAY_MODEL(LightWeightDlg);
CARDANO_ERA_MODEL(Ssc);
CARDANO_ERA_ARRAY_MODEL(SscCert);
CARDANO_ERA_ARRAY_MODEL(SscCertificatesPayload);
CARDANO_ERA_ARRAY_MODEL(SscCertificatesProof);
CARDANO_ERA_ARRAY_MODEL(SscCommitment);
CARDANO_ERA_ARRAY_MODEL(SscCommitmentsPayload);
CARDANO_ERA_ARRAY_MODEL(SscCommitmentsProof);
CARDANO_ERA_ARRAY_MODEL(SscOpeningsPayload);
CARDANO_ERA_ARRAY_MODEL(SscOpeningsProof);
CARDANO_ERA_ARRAY_MODEL(SscProof);
CARDANO_ERA_ARRAY_MODEL(SscSharesPayload);
CARDANO_ERA_ARRAY_MODEL(SscSharesProof);
CARDANO_ERA_MAP_MODEL(SscSharesSubmap);
CARDANO_ERA_ARRAY_MODEL(SscSignedCommitment);
CARDANO_ERA_ARRAY_MODEL(StdFeePolicy);
CARDANO_ERA_ARRAY_MODEL(TxAux);
CARDANO_ERA_ARRAY_MODEL(TxPayload);
CARDANO_ERA_MODEL(VssEncryptedShare);
CARDANO_ERA_MODEL(VssProof);
CARDANO_ERA_MODEL(TransactionMetadatum);
CARDANO_ERA_MODEL(AlonzoFormatTxOut);
CARDANO_ERA_ARRAY_MODEL(Anchor);
CARDANO_ERA_ARRAY_MODEL(AuthCommitteeHotCert);
CARDANO_ERA_MODEL(AuxiliaryData);
CARDANO_ERA_ARRAY_MODEL(Block);
CARDANO_ERA_ARRAY_MODEL(Certificate);
CARDANO_ERA_ARRAY_MODEL(Constitution);
CARDANO_ERA_MODEL(ConwayFormatAuxData);
CARDANO_ERA_MAP_MODEL(ConwayFormatTxOut);
CARDANO_ERA_MAP_MODEL(CostModels);
CARDANO_ERA_MODEL(DNSName);
CARDANO_ERA_MODEL(DRep);
CARDANO_ERA_ARRAY_MODEL(DRepVotingThresholds);
CARDANO_ERA_ARRAY_MODEL(DatumOption);
CARDANO_ERA_ARRAY_MODEL(GovAction);
CARDANO_ERA_ARRAY_MODEL(GovActionId);
CARDANO_ERA_ARRAY_MODEL(HardForkInitiationAction);
CARDANO_ERA_ARRAY_MODEL(Header);
CARDANO_ERA_ARRAY_MODEL(HeaderBody);
CARDANO_ERA_MODEL(Ipv4);
CARDANO_ERA_MODEL(Ipv6);
CARDANO_ERA_MODEL(KESSignature);
CARDANO_ERA_ARRAY_MODEL(LegacyRedeemer);
CARDANO_ERA_MODEL(Metadata);
CARDANO_ERA_MAP_MODEL(MetadatumMap);
CARDANO_ERA_MAP_MODEL(Mint);
CARDANO_ERA_MAP_MODEL(MultiAsset);
CARDANO_ERA_ARRAY_MODEL(MultiHostName);
CARDANO_ERA_ARRAY_MODEL(NativeScript);
CARDANO_ERA_ARRAY_MODEL(NewConstitution);
CARDANO_ERA_ARRAY_MODEL(NoConfidence);
CARDANO_ERA_ARRAY_MODEL(Nonce);
CARDANO_ERA_ARRAY_MODEL(OperationalCert);
CARDANO_ERA_ARRAY_MODEL(ParameterChangeAction);
CARDANO_ERA_MODEL(PlutusV1Script);
CARDANO_ERA_MODEL(PlutusV2Script);
CARDANO_ERA_MODEL(PlutusV3Script);
CARDANO_ERA_ARRAY_MODEL(PoolMetadata);
CARDANO_ERA_ARRAY_MODEL(PoolParams);
CARDANO_ERA_ARRAY_MODEL(PoolRegistration);
CARDANO_ERA_ARRAY_MODEL(PoolRetirement);
CARDANO_ERA_ARRAY_MODEL(PoolVotingThresholds);
CARDANO_ERA_ARRAY_MODEL(ProposalProcedure);
CARDANO_ERA_MAP_MODEL(ProtocolParamUpdate);
CARDANO_ERA_ARRAY_MODEL(ProtocolVersion);
CARDANO_ERA_MODEL(Rational);
CARDANO_ERA_ARRAY_MODEL(RedeemerKey);
CARDANO_ERA_ARRAY_MODEL(RedeemerVal);
CARDANO_ERA_MODEL(Redeemers);
CARDANO_ERA_ARRAY_MODEL(RegCert);
CARDANO_ERA_ARRAY_MODEL(RegDrepCert);
CARDANO_ERA_ARRAY_MODEL(Relay);
CARDANO_ERA_ARRAY_MODEL(ResignCommitteeColdCert);
CARDANO_ERA_ARRAY_MODEL(Script);
CARDANO_ERA_ARRAY_MODEL(ScriptAll);
CARDANO_ERA_ARRAY_MODEL(ScriptAny);
CARDANO_ERA_ARRAY_MODEL(ScriptInvalidBefore);
CARDANO_ERA_ARRAY_MODEL(ScriptInvalidHereafter);
CARDANO_ERA_ARRAY_MODEL(ScriptNOfK);
CARDANO_ERA_ARRAY_MODEL(ScriptPubkey);
CARDANO_ERA_MODEL(ScriptRef);
CARDANO_ERA_MODEL(ShelleyMAFormatAuxData);
CARDANO_ERA_ARRAY_MODEL(SingleHostAddr);
CARDANO_ERA_ARRAY_MODEL(SingleHostName);
CARDANO_ERA_ARRAY_MODEL(StakeDelegation);
CARDANO_ERA_ARRAY_MODEL(StakeDeregistration);
CARDANO_ERA_ARRAY_MODEL(StakeRegDelegCert);
CARDANO_ERA_ARRAY_MODEL(StakeRegistration);
CARDANO_ERA_ARRAY_MODEL(StakeVoteDelegCert);
CARDANO_ERA_ARRAY_MODEL(StakeVoteRegDelegCert);
CARDANO_ERA_ARRAY_MODEL(Transaction);
CARDANO_ERA_MAP_MODEL(TransactionBody);
CARDANO_ERA_MAP_MODEL(TransactionWitnessSet);
CARDANO_ERA_ARRAY_MODEL(TreasuryWithdrawalsAction);
CARDANO_ERA_MODEL(UnitInterval);
CARDANO_ERA_ARRAY_MODEL(UnregCert);
CARDANO_ERA_ARRAY_MODEL(UnregDrepCert);
CARDANO_ERA_ARRAY_MODEL(UpdateCommittee);
CARDANO_ERA_ARRAY_MODEL(UpdateDrepCert);
CARDANO_ERA_MODEL(Url);
CARDANO_ERA_ARRAY_MODEL(VRFCert);
CARDANO_ERA_ARRAY_MODEL(VoteDelegCert);
CARDANO_ERA_ARRAY_MODEL(VoteRegDelegCert);
CARDANO_ERA_ARRAY_MODEL(Voter);
CARDANO_ERA_ARRAY_MODEL(VotingProcedure);
CARDANO_ERA_MAP_MODEL(VotingProcedures);
CARDANO_ERA_MAP_MODEL(Withdrawals);
CARDANO_ERA_ARRAY_MODEL(MaryBlock);
CARDANO_ERA_ARRAY_MODEL(MaryTransaction);
CARDANO_ERA_MAP_MODEL(MaryTransactionBody);
CARDANO_ERA_ARRAY_MODEL(MaryTransactionOutput);
CARDANO_ERA_ARRAY_MODEL(GenesisKeyDelegation);
CARDANO_ERA_ARRAY_MODEL(MultisigAll);
CARDANO_ERA_ARRAY_MODEL(MultisigAny);
CARDANO_ERA_ARRAY_MODEL(MultisigNOfK);
CARDANO_ERA_ARRAY_MODEL(MultisigPubkey);
CARDANO_ERA_MODEL(MultisigScript);
CARDANO_ERA_ARRAY_MODEL(ProtocolVersionStruct);
CARDANO_ERA_ARRAY_MODEL(ShelleyBlock);
CARDANO_ERA_MODEL(ShelleyCertificate);
CARDANO_ERA_MODEL(ShelleyDNSName);
CARDANO_ERA_ARRAY_MODEL(ShelleyHeader);
CARDANO_ERA_ARRAY_MODEL(ShelleyHeaderBody);
CARDANO_ERA_ARRAY_MODEL(ShelleyMoveInstantaneousReward);
CARDANO_ERA_ARRAY_MODEL(ShelleyMoveInstantaneousRewardsCert);
CARDANO_ERA_ARRAY_MODEL(ShelleyMultiHostName);
CARDANO_ERA_ARRAY_MODEL(ShelleyPoolParams);
CARDANO_ERA_ARRAY_MODEL(ShelleyPoolRegistration);
CARDANO_ERA_MAP_MODEL(ShelleyProposedProtocolParameterUpdates);
CARDANO_ERA_MAP_MODEL(ShelleyProtocolParamUpdate);
CARDANO_ERA_MODEL(ShelleyRelay);
CARDANO_ERA_ARRAY_MODEL(ShelleySingleHostName);
CARDANO_ERA_ARRAY_MODEL(ShelleyTransaction);
CARDANO_ERA_MAP_MODEL(ShelleyTransactionBody);
CARDANO_ERA_ARRAY_MODEL(ShelleyTransactionOutput);
CARDANO_ERA_MAP_MODEL(ShelleyTransactionWitnessSet);
CARDANO_ERA_ARRAY_MODEL(ShelleyUpdate);
CARDANO_ERA_MODEL(ConwayData);
CARDANO_ERA_ARRAY_MODEL(ConwayList);
CARDANO_ERA_MAP_MODEL(ConwayMap);

#define CARDANO_ERA_LIST_ALIAS(name, element) using name = std::vector<element>
CARDANO_ERA_LIST_ALIAS(AddressIdList, AddressId);
CARDANO_ERA_LIST_ALIAS(BigIntegerList, core::BigInteger);
CARDANO_ERA_LIST_ALIAS(ByronAnyList, ByronAny);
CARDANO_ERA_LIST_ALIAS(ByronAttributesList, ByronAttributes);
CARDANO_ERA_LIST_ALIAS(ByronDelegationList, ByronDelegation);
CARDANO_ERA_LIST_ALIAS(ByronTxFeePolicyList, ByronTxFeePolicy);
CARDANO_ERA_LIST_ALIAS(ByronTxInList, ByronTxIn);
CARDANO_ERA_LIST_ALIAS(ByronTxOutList, ByronTxOut);
CARDANO_ERA_LIST_ALIAS(ByronTxWitnessList, ByronTxWitness);
CARDANO_ERA_LIST_ALIAS(ByronUpdateProposalList, ByronUpdateProposal);
CARDANO_ERA_LIST_ALIAS(ByronUpdateVoteList, ByronUpdateVote);
CARDANO_ERA_LIST_ALIAS(BytesList, core::Bytes);
CARDANO_ERA_LIST_ALIAS(SoftForkRuleList, SoftForkRule);
CARDANO_ERA_LIST_ALIAS(StakeholderIdList, StakeholderId);
CARDANO_ERA_LIST_ALIAS(SystemTagList, std::string);
CARDANO_ERA_LIST_ALIAS(AllegraCertificateList, AllegraCertificate);
CARDANO_ERA_LIST_ALIAS(AllegraTransactionBodyList, AllegraTransactionBody);
CARDANO_ERA_LIST_ALIAS(AllegraTransactionWitnessSetList, AllegraTransactionWitnessSet);
CARDANO_ERA_LIST_ALIAS(AlonzoRedeemerList, AlonzoRedeemer);
CARDANO_ERA_LIST_ALIAS(AlonzoTransactionBodyList, AlonzoTransactionBody);
CARDANO_ERA_LIST_ALIAS(AlonzoTransactionWitnessSetList, AlonzoTransactionWitnessSet);
CARDANO_ERA_LIST_ALIAS(BabbageTransactionBodyList, BabbageTransactionBody);
CARDANO_ERA_LIST_ALIAS(BabbageTransactionOutputList, BabbageTransactionOutput);
CARDANO_ERA_LIST_ALIAS(BabbageTransactionWitnessSetList, BabbageTransactionWitnessSet);
CARDANO_ERA_LIST_ALIAS(AlonzoFormatTxOutList, AlonzoFormatTxOut);
CARDANO_ERA_LIST_ALIAS(AssetNameList, AssetName);
CARDANO_ERA_LIST_ALIAS(CertificateList, Certificate);
CARDANO_ERA_LIST_ALIAS(CommitteeColdCredentialList, Credential);
CARDANO_ERA_LIST_ALIAS(Ed25519KeyHashList, crypto::Ed25519KeyHash);
CARDANO_ERA_LIST_ALIAS(GenesisHashList, crypto::GenesisHash);
CARDANO_ERA_LIST_ALIAS(GovActionIdList, GovActionId);
CARDANO_ERA_LIST_ALIAS(LanguageList, Language);
CARDANO_ERA_LIST_ALIAS(LegacyRedeemerList, LegacyRedeemer);
CARDANO_ERA_LIST_ALIAS(MetadatumList, TransactionMetadatum);
CARDANO_ERA_LIST_ALIAS(NativeScriptList, NativeScript);
CARDANO_ERA_LIST_ALIAS(NonEmptyCertificateList, Certificate);
CARDANO_ERA_LIST_ALIAS(NonEmptyLegacyRedeemerList, LegacyRedeemer);
CARDANO_ERA_LIST_ALIAS(NonEmptyNativeScriptList, NativeScript);
CARDANO_ERA_LIST_ALIAS(NonEmptyPlutusDataList, PlutusData);
CARDANO_ERA_LIST_ALIAS(NonEmptyPlutusV1ScriptList, PlutusV1Script);
CARDANO_ERA_LIST_ALIAS(NonEmptyPlutusV2ScriptList, PlutusV2Script);
CARDANO_ERA_LIST_ALIAS(NonEmptyPlutusV3ScriptList, PlutusV3Script);
CARDANO_ERA_LIST_ALIAS(NonEmptyProposalProcedureList, ProposalProcedure);
CARDANO_ERA_LIST_ALIAS(NonEmptyTransactionInputList, TransactionInput);
CARDANO_ERA_LIST_ALIAS(PlutusDataList, PlutusData);
CARDANO_ERA_LIST_ALIAS(PlutusV1ScriptList, PlutusV1Script);
CARDANO_ERA_LIST_ALIAS(PlutusV2ScriptList, PlutusV2Script);
CARDANO_ERA_LIST_ALIAS(PlutusV3ScriptList, PlutusV3Script);
CARDANO_ERA_LIST_ALIAS(PolicyIdList, crypto::ScriptHash);
CARDANO_ERA_LIST_ALIAS(ProposalProcedureList, ProposalProcedure);
CARDANO_ERA_LIST_ALIAS(RedeemerKeyList, RedeemerKey);
CARDANO_ERA_LIST_ALIAS(RelayList, Relay);
CARDANO_ERA_LIST_ALIAS(RequiredSigners, crypto::Ed25519KeyHash);
CARDANO_ERA_LIST_ALIAS(RewardAccountList, Address);
CARDANO_ERA_LIST_ALIAS(StakeCredentialList, Credential);
CARDANO_ERA_LIST_ALIAS(TransactionBodyList, TransactionBody);
CARDANO_ERA_LIST_ALIAS(TransactionInputList, TransactionInput);
CARDANO_ERA_LIST_ALIAS(TransactionMetadatumList, TransactionMetadatum);
CARDANO_ERA_LIST_ALIAS(TransactionOutputList, TransactionOutput);
CARDANO_ERA_LIST_ALIAS(TransactionWitnessSetList, TransactionWitnessSet);
CARDANO_ERA_LIST_ALIAS(VoterList, Voter);
CARDANO_ERA_LIST_ALIAS(MaryTransactionBodyList, MaryTransactionBody);
CARDANO_ERA_LIST_ALIAS(MaryTransactionOutputList, MaryTransactionOutput);
CARDANO_ERA_LIST_ALIAS(MultisigScriptList, MultisigScript);
CARDANO_ERA_LIST_ALIAS(ShelleyCertificateList, ShelleyCertificate);
CARDANO_ERA_LIST_ALIAS(ShelleyRelayList, ShelleyRelay);
CARDANO_ERA_LIST_ALIAS(ShelleyTransactionBodyList, ShelleyTransactionBody);
CARDANO_ERA_LIST_ALIAS(ShelleyTransactionOutputList, ShelleyTransactionOutput);
CARDANO_ERA_LIST_ALIAS(ShelleyTransactionWitnessSetList, ShelleyTransactionWitnessSet);

using MapAssetNameToCoin = std::vector<std::pair<AssetName, std::uint64_t>>;
using MapAssetNameToNonZeroInt64 = std::vector<std::pair<AssetName, std::int64_t>>;
using MapAssetNameToU64 = std::vector<std::pair<AssetName, std::uint64_t>>;
using MapCommitteeColdCredentialToEpoch = std::vector<std::pair<Credential, std::uint64_t>>;
using MapGovActionIdToVotingProcedure = std::vector<std::pair<GovActionId, VotingProcedure>>;
using NonEmptyMapGovActionIdToVotingProcedure = MapGovActionIdToVotingProcedure;
using MapRedeemerKeyToRedeemerVal = std::vector<std::pair<RedeemerKey, RedeemerVal>>;
using NonEmptyMapRedeemerKeyToRedeemerVal = MapRedeemerKeyToRedeemerVal;
using MapStakeCredentialToCoin = std::vector<std::pair<Credential, std::uint64_t>>;
using MapStakeCredentialToDeltaCoin = std::vector<std::pair<Credential, std::int64_t>>;
using MapTransactionIndexToAuxiliaryData = std::vector<std::pair<std::uint64_t, AuxiliaryData>>;
using MapTransactionIndexToMetadata = std::vector<std::pair<std::uint64_t, Metadata>>;
using MapU64ToArrI64 = std::vector<std::pair<std::uint64_t, std::vector<std::int64_t>>>;
using MapVoterToMapGovActionIdToVotingProcedure =
    std::vector<std::pair<Voter, MapGovActionIdToVotingProcedure>>;
using MapTransactionIndexToAllegraAuxiliaryData =
    std::vector<std::pair<std::uint64_t, AllegraAuxiliaryData>>;
using MapTransactionIndexToAlonzoAuxiliaryData =
    std::vector<std::pair<std::uint64_t, AlonzoAuxiliaryData>>;
using MapTransactionIndexToBabbageAuxiliaryData =
    std::vector<std::pair<std::uint64_t, BabbageAuxiliaryData>>;
using MapSystemTagToByronUpdateData = std::vector<std::pair<std::string, ByronUpdateData>>;
using TransactionMetadatumLabels = std::vector<std::pair<std::uint64_t, TransactionMetadatum>>;

using SscCerts = std::vector<SscCert>;
using SscOpens = std::vector<ByronAny>;
using SscShares = std::vector<ByronAny>;
using SscSignedCommitments = std::vector<SscSignedCommitment>;
using VssShares = std::vector<VssEncryptedShare>;

inline constexpr std::string_view NATIVE_SCRIPT_SCHEMA = "native_script";
inline constexpr std::string_view PLUTUS_DATA_SCHEMA = "plutus_data";
inline constexpr std::string_view TRANSACTION_INPUT_SCHEMA = "transaction_input";

#undef CARDANO_ERA_LIST_ALIAS
#undef CARDANO_ERA_MODEL
#undef CARDANO_ERA_ARRAY_MODEL
#undef CARDANO_ERA_MAP_MODEL

}  // namespace cardano::chain
