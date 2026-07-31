#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cardano/chain/era_models.hpp"

namespace cardano::chain {

using ConwayInput = core::cbor::Value;
enum class ConwayWireShape : std::uint8_t { historical, conway };

// The frozen TypeScript inventory distinguishes runtime ledger models from type-only JSON DTOs.
// C++ retains that distinction: these are owned data contracts and never expose the private JSON
// parser type used by the implementation.
struct JsonValue {
  using Array = std::vector<JsonValue>;
  using Object = std::vector<std::pair<std::string, JsonValue>>;
  std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;
};

template <typename Model>
struct EraModelJSON {
  JsonValue value;
};

template <typename Domain>
struct HexJSON {
  std::string value;
};

template <typename Domain>
struct IntegerJSON {
  std::variant<std::int64_t, std::uint64_t, std::string> value;
};

template <typename Enum>
struct EnumJSON {
  Enum value;
};

struct TransactionInputJSON {
  std::string transaction_id;
  std::uint64_t index{};
};

struct RationalJSON {
  IntegerJSON<struct RationalNumeratorJSONDomain> numerator;
  IntegerJSON<struct RationalDenominatorJSONDomain> denominator;
};

struct UnitIntervalJSON {
  IntegerJSON<struct UnitIntervalStartJSONDomain> start;
  IntegerJSON<struct UnitIntervalEndJSONDomain> end;
};

struct ExUnitsJSON {
  IntegerJSON<struct ExUnitsMemoryJSONDomain> mem;
  IntegerJSON<struct ExUnitsStepsJSONDomain> steps;
};

struct ExUnitPricesJSON {
  RationalJSON mem_price;
  RationalJSON step_price;
};

struct CredentialPubKeyJSON {
  std::string hash;
};
struct CredentialScriptJSON {
  std::string hash;
};
using CredentialJSON = std::variant<CredentialPubKeyJSON, CredentialScriptJSON>;

struct AnchorJSON {
  std::string anchor_url;
  std::string anchor_doc_hash;
};

struct ProtocolVersionJSON {
  std::uint64_t major{};
  std::uint64_t minor{};
};

struct VRFCertJSON {
  std::vector<std::uint8_t> output;
  std::vector<std::uint8_t> proof;
};

struct ValueJSON {
  double coin{};
  std::map<std::string, std::map<std::string, double>> multiasset;
};

struct ByronTxOutJSON {
  std::string address;
  double amount{};
};

struct AddrAttributesJSON {
  std::optional<std::vector<std::uint8_t>> derivation_path;
  std::optional<std::uint32_t> protocol_magic;
  std::optional<std::variant<std::string, std::map<std::string, std::string>>> stake_distribution;
};

using AddressJSON = std::string;
struct BootstrapWitnessJSON {
  AddrAttributesJSON attributes;
  std::vector<std::uint8_t> chain_code;
  std::string public_key;
  std::string signature;
};
using ByronAddrTypeJSON = std::string;
using ByronAddressJSON = std::string;
using NonEmptyVecBootstrapWitnessJSON = std::vector<BootstrapWitnessJSON>;
using NonEmptyVecVkeywitnessJSON = std::vector<EraModelJSON<ByronPkWitness>>;
using RewardAddressJSON = std::string;
using StakeDistributionJSON = std::variant<std::string, std::map<std::string, std::string>>;
using StakeholderIdJSON = std::string;
using VkeywitnessJSON = EraModelJSON<ByronPkWitness>;
using MultiEraBlockJSON = core::cbor::Value;
using MultiEraTransactionBodyJSON = core::cbor::Value;

struct AddressContentJSON {
  AddrAttributesJSON addr_attributes;
  std::string addr_type;
  std::string address_id;
};

struct SpendingDataPubKeyJSON {
  std::string value;
};
struct SpendingDataScriptJSON {
  std::string value;
};
struct SpendingDataRedeemJSON {
  std::string value;
};
using SpendingDataJSON =
    std::variant<SpendingDataPubKeyJSON, SpendingDataScriptJSON, SpendingDataRedeemJSON>;
using HDAddressPayloadJSON = std::vector<std::uint8_t>;
using Crc32JSON = std::uint32_t;
using ProtocolMagicJSON = std::uint32_t;

#define CARDANO_MODEL_JSON(name) using name##JSON = EraModelJSON<name>

CARDANO_MODEL_JSON(MIRAction);
using MIRPotJSON = EnumJSON<MIRPot>;
CARDANO_MODEL_JSON(MoveInstantaneousReward);
CARDANO_MODEL_JSON(MoveInstantaneousRewardsCert);
CARDANO_MODEL_JSON(AllegraAuxiliaryData);
CARDANO_MODEL_JSON(AllegraBlock);
CARDANO_MODEL_JSON(AllegraCertificate);
CARDANO_MODEL_JSON(AllegraTransactionBody);
CARDANO_MODEL_JSON(AllegraTransaction);
CARDANO_MODEL_JSON(AllegraTransactionWitnessSet);
CARDANO_MODEL_JSON(AlonzoAuxiliaryData);
CARDANO_MODEL_JSON(AlonzoBlock);
CARDANO_MODEL_JSON(AlonzoFormatAuxData);
CARDANO_MODEL_JSON(AlonzoProtocolParamUpdate);
CARDANO_MODEL_JSON(AlonzoRedeemer);
using AlonzoRedeemerTagJSON = EnumJSON<AlonzoRedeemerTag>;
CARDANO_MODEL_JSON(AlonzoTransactionBody);
CARDANO_MODEL_JSON(AlonzoTransaction);
CARDANO_MODEL_JSON(AlonzoTransactionWitnessSet);
CARDANO_MODEL_JSON(AlonzoUpdate);
CARDANO_MODEL_JSON(BabbageAuxiliaryData);
CARDANO_MODEL_JSON(BabbageBlock);
CARDANO_MODEL_JSON(BabbageFormatAuxData);
CARDANO_MODEL_JSON(BabbageFormatTxOut);
using BabbageMintJSON = EraModelJSON<Mint>;
CARDANO_MODEL_JSON(BabbageProtocolParamUpdate);
CARDANO_MODEL_JSON(BabbageScript);
CARDANO_MODEL_JSON(BabbageTransactionBody);
CARDANO_MODEL_JSON(BabbageTransaction);
CARDANO_MODEL_JSON(BabbageTransactionOutput);
CARDANO_MODEL_JSON(BabbageTransactionWitnessSet);
CARDANO_MODEL_JSON(BabbageUpdate);
CARDANO_MODEL_JSON(SoftForkRule);

using AnyJSON = EraModelJSON<ByronAny>;
using Blake2B256JSON = HexJSON<struct Blake2B256JSONDomain>;
CARDANO_MODEL_JSON(BlockHeaderExtraData);
CARDANO_MODEL_JSON(Bvermod);
CARDANO_MODEL_JSON(ByronBlockBody);
CARDANO_MODEL_JSON(ByronBlockConsensusData);
CARDANO_MODEL_JSON(ByronBlockHeader);
CARDANO_MODEL_JSON(ByronBlock);
CARDANO_MODEL_JSON(ByronBlockSignature);
CARDANO_MODEL_JSON(ByronBlockSignatureNormal);
CARDANO_MODEL_JSON(ByronBlockSignatureProxyHeavy);
CARDANO_MODEL_JSON(ByronBlockSignatureProxyLight);
CARDANO_MODEL_JSON(ByronBlockVersion);
CARDANO_MODEL_JSON(ByronBodyProof);
CARDANO_MODEL_JSON(ByronDelegation);
CARDANO_MODEL_JSON(ByronDelegationSignature);
CARDANO_MODEL_JSON(ByronDifficulty);
CARDANO_MODEL_JSON(ByronEbBlock);
CARDANO_MODEL_JSON(ByronMainBlock);
CARDANO_MODEL_JSON(ByronPkWitnessEntry);
CARDANO_MODEL_JSON(ByronPkWitness);
CARDANO_MODEL_JSON(ByronRedeemWitness);
CARDANO_MODEL_JSON(ByronRedeemerScript);
CARDANO_MODEL_JSON(ByronRedeemerWitnessEntry);
CARDANO_MODEL_JSON(ByronScriptWitnessEntry);
CARDANO_MODEL_JSON(ByronScriptWitness);
CARDANO_MODEL_JSON(ByronSlotId);
CARDANO_MODEL_JSON(ByronSoftwareVersion);
CARDANO_MODEL_JSON(ByronTxFeePolicy);
CARDANO_MODEL_JSON(ByronTxInGenesis);
CARDANO_MODEL_JSON(ByronTxIn);
CARDANO_MODEL_JSON(ByronTxInRegular);
CARDANO_MODEL_JSON(ByronTx);
CARDANO_MODEL_JSON(ByronTxOutPtr);
CARDANO_MODEL_JSON(ByronTxProof);
CARDANO_MODEL_JSON(ByronTxWitness);
CARDANO_MODEL_JSON(ByronUpdateData);
CARDANO_MODEL_JSON(ByronUpdate);
CARDANO_MODEL_JSON(ByronUpdateProposal);
CARDANO_MODEL_JSON(ByronUpdateVote);
CARDANO_MODEL_JSON(ByronValidatorScript);
CARDANO_MODEL_JSON(EbbConsensusData);
CARDANO_MODEL_JSON(EbbHead);
CARDANO_MODEL_JSON(EpochRange);
CARDANO_MODEL_JSON(LightWeightDelegationSignature);
CARDANO_MODEL_JSON(LightWeightDlg);
CARDANO_MODEL_JSON(SscCert);
CARDANO_MODEL_JSON(SscCertificatesPayload);
CARDANO_MODEL_JSON(SscCertificatesProof);
CARDANO_MODEL_JSON(SscCommitment);
CARDANO_MODEL_JSON(SscCommitmentsPayload);
CARDANO_MODEL_JSON(SscCommitmentsProof);
CARDANO_MODEL_JSON(Ssc);
CARDANO_MODEL_JSON(SscOpeningsPayload);
CARDANO_MODEL_JSON(SscOpeningsProof);
CARDANO_MODEL_JSON(SscProof);
CARDANO_MODEL_JSON(SscSharesPayload);
CARDANO_MODEL_JSON(SscSharesProof);
CARDANO_MODEL_JSON(SscSignedCommitment);
CARDANO_MODEL_JSON(StdFeePolicy);
CARDANO_MODEL_JSON(TxAux);
CARDANO_MODEL_JSON(VssEncryptedShare);
CARDANO_MODEL_JSON(VssProof);

CARDANO_MODEL_JSON(AlonzoFormatTxOut);
CARDANO_MODEL_JSON(AuthCommitteeHotCert);
CARDANO_MODEL_JSON(AuxiliaryData);
CARDANO_MODEL_JSON(Block);
CARDANO_MODEL_JSON(Certificate);
CARDANO_MODEL_JSON(Constitution);
CARDANO_MODEL_JSON(ConwayFormatAuxData);
CARDANO_MODEL_JSON(ConwayFormatTxOut);
CARDANO_MODEL_JSON(CostModels);
CARDANO_MODEL_JSON(DNSName);
CARDANO_MODEL_JSON(DRep);
CARDANO_MODEL_JSON(DRepVotingThresholds);
CARDANO_MODEL_JSON(DatumOption);
CARDANO_MODEL_JSON(GovAction);
CARDANO_MODEL_JSON(GovActionId);
CARDANO_MODEL_JSON(HardForkInitiationAction);
CARDANO_MODEL_JSON(HeaderBody);
CARDANO_MODEL_JSON(Header);
CARDANO_MODEL_JSON(Ipv4);
CARDANO_MODEL_JSON(Ipv6);
CARDANO_MODEL_JSON(KESSignature);
CARDANO_MODEL_JSON(LegacyRedeemer);
CARDANO_MODEL_JSON(Metadata);
CARDANO_MODEL_JSON(MultiHostName);
CARDANO_MODEL_JSON(NativeScript);
CARDANO_MODEL_JSON(NewConstitution);
CARDANO_MODEL_JSON(NoConfidence);
CARDANO_MODEL_JSON(Nonce);
CARDANO_MODEL_JSON(OperationalCert);
CARDANO_MODEL_JSON(ParameterChangeAction);
CARDANO_MODEL_JSON(PlutusV1Script);
CARDANO_MODEL_JSON(PlutusV2Script);
CARDANO_MODEL_JSON(PlutusV3Script);
CARDANO_MODEL_JSON(PoolMetadata);
CARDANO_MODEL_JSON(PoolParams);
CARDANO_MODEL_JSON(PoolRegistration);
CARDANO_MODEL_JSON(PoolRetirement);
CARDANO_MODEL_JSON(PoolVotingThresholds);
CARDANO_MODEL_JSON(ProposalProcedure);
CARDANO_MODEL_JSON(ProtocolParamUpdate);
CARDANO_MODEL_JSON(RedeemerKey);
CARDANO_MODEL_JSON(RedeemerVal);
CARDANO_MODEL_JSON(Redeemers);
CARDANO_MODEL_JSON(RegCert);
CARDANO_MODEL_JSON(RegDrepCert);
CARDANO_MODEL_JSON(Relay);
CARDANO_MODEL_JSON(ResignCommitteeColdCert);
CARDANO_MODEL_JSON(ScriptAll);
CARDANO_MODEL_JSON(ScriptAny);
CARDANO_MODEL_JSON(ScriptInvalidBefore);
CARDANO_MODEL_JSON(ScriptInvalidHereafter);
CARDANO_MODEL_JSON(Script);
CARDANO_MODEL_JSON(ScriptNOfK);
CARDANO_MODEL_JSON(ScriptPubkey);
CARDANO_MODEL_JSON(ScriptRef);
CARDANO_MODEL_JSON(ShelleyMAFormatAuxData);
CARDANO_MODEL_JSON(SingleHostAddr);
CARDANO_MODEL_JSON(SingleHostName);
CARDANO_MODEL_JSON(StakeDelegation);
CARDANO_MODEL_JSON(StakeDeregistration);
CARDANO_MODEL_JSON(StakeRegDelegCert);
CARDANO_MODEL_JSON(StakeRegistration);
CARDANO_MODEL_JSON(StakeVoteDelegCert);
CARDANO_MODEL_JSON(StakeVoteRegDelegCert);
CARDANO_MODEL_JSON(TransactionBody);
CARDANO_MODEL_JSON(Transaction);
CARDANO_MODEL_JSON(TransactionMetadatum);
CARDANO_MODEL_JSON(TransactionOutput);
CARDANO_MODEL_JSON(TransactionWitnessSet);
CARDANO_MODEL_JSON(TreasuryWithdrawalsAction);
CARDANO_MODEL_JSON(UnregCert);
CARDANO_MODEL_JSON(UnregDrepCert);
CARDANO_MODEL_JSON(UpdateCommittee);
CARDANO_MODEL_JSON(UpdateDrepCert);
CARDANO_MODEL_JSON(Url);
CARDANO_MODEL_JSON(VoteDelegCert);
CARDANO_MODEL_JSON(VoteRegDelegCert);
CARDANO_MODEL_JSON(Voter);
CARDANO_MODEL_JSON(VotingProcedure);

using AnchorDocHashJSON = HexJSON<struct AnchorDocHashJSONDomain>;
using ArrayOf_CredentialJSON = std::vector<CredentialJSON>;
using ArrayOf_Ed25519KeyHashJSON = std::vector<std::string>;
using ArrayOf_TransactionInputJSON = std::vector<TransactionInputJSON>;
using AssetNameJSON = HexJSON<struct AssetNameJSONDomain>;
using AuxiliaryDataHashJSON = HexJSON<struct AuxiliaryDataHashJSONDomain>;
using BigIntegerJSON = IntegerJSON<struct BigIntegerJSONDomain>;
using Bip32PublicKeyJSON = HexJSON<struct Bip32PublicKeyJSONDomain>;
using BlockBodyHashJSON = HexJSON<struct BlockBodyHashJSONDomain>;
using BlockHeaderHashJSON = HexJSON<struct BlockHeaderHashJSONDomain>;
using DatumHashJSON = HexJSON<struct DatumHashJSONDomain>;
using Ed25519KeyHashJSON = HexJSON<struct Ed25519KeyHashJSONDomain>;
using Ed25519SignatureJSON = HexJSON<struct Ed25519SignatureJSONDomain>;
using GenesisDelegateHashJSON = HexJSON<struct GenesisDelegateHashJSONDomain>;
using GenesisHashJSON = HexJSON<struct GenesisHashJSONDomain>;
using IntJSON = IntegerJSON<struct IntJSONDomain>;
using KESVkeyJSON = HexJSON<struct KESVkeyJSONDomain>;
using LanguageJSON = EnumJSON<Language>;
using NetworkIdJSON = std::uint8_t;
using NonEmptyVecCertificateJSON = std::vector<CertificateJSON>;
using NonEmptyVecEd25519KeyHashJSON = std::vector<Ed25519KeyHashJSON>;
using NonEmptyVecNativeScriptJSON = std::vector<NativeScriptJSON>;
using NonEmptyVecPlutusDataJSON = std::vector<core::cbor::Value>;
using NonEmptyVecPlutusV1ScriptJSON = std::vector<PlutusV1ScriptJSON>;
using NonEmptyVecPlutusV2ScriptJSON = std::vector<PlutusV2ScriptJSON>;
using NonEmptyVecPlutusV3ScriptJSON = std::vector<PlutusV3ScriptJSON>;
using NonEmptyVecProposalProcedureJSON = std::vector<ProposalProcedureJSON>;
using NonEmptyVecTransactionInputJSON = std::vector<TransactionInputJSON>;
using NonceHashJSON = HexJSON<struct NonceHashJSONDomain>;
using PlutusDataJSON = core::cbor::Value;
using PoolMetadataHashJSON = HexJSON<struct PoolMetadataHashJSONDomain>;
using PublicKeyJSON = HexJSON<struct PublicKeyJSONDomain>;
using RedeemerTagJSON = EnumJSON<RedeemerTag>;
using ScriptDataHashJSON = HexJSON<struct ScriptDataHashJSONDomain>;
using ScriptHashJSON = HexJSON<struct ScriptHashJSONDomain>;
using TransactionHashJSON = HexJSON<struct TransactionHashJSONDomain>;
using VRFKeyHashJSON = HexJSON<struct VRFKeyHashJSONDomain>;
using VRFVkeyJSON = HexJSON<struct VRFVkeyJSONDomain>;
using VoteJSON = EnumJSON<Vote>;

CARDANO_MODEL_JSON(MaryBlock);
CARDANO_MODEL_JSON(MaryTransactionBody);
CARDANO_MODEL_JSON(MaryTransaction);
CARDANO_MODEL_JSON(MaryTransactionOutput);
CARDANO_MODEL_JSON(GenesisKeyDelegation);
CARDANO_MODEL_JSON(MultisigAll);
CARDANO_MODEL_JSON(MultisigAny);
CARDANO_MODEL_JSON(MultisigNOfK);
CARDANO_MODEL_JSON(MultisigPubkey);
CARDANO_MODEL_JSON(MultisigScript);
CARDANO_MODEL_JSON(ProtocolVersionStruct);
CARDANO_MODEL_JSON(ShelleyBlock);
CARDANO_MODEL_JSON(ShelleyCertificate);
CARDANO_MODEL_JSON(ShelleyDNSName);
CARDANO_MODEL_JSON(ShelleyHeaderBody);
CARDANO_MODEL_JSON(ShelleyHeader);
CARDANO_MODEL_JSON(ShelleyMoveInstantaneousReward);
CARDANO_MODEL_JSON(ShelleyMoveInstantaneousRewardsCert);
CARDANO_MODEL_JSON(ShelleyMultiHostName);
CARDANO_MODEL_JSON(ShelleyPoolParams);
CARDANO_MODEL_JSON(ShelleyPoolRegistration);
CARDANO_MODEL_JSON(ShelleyProtocolParamUpdate);
CARDANO_MODEL_JSON(ShelleyRelay);
CARDANO_MODEL_JSON(ShelleySingleHostName);
CARDANO_MODEL_JSON(ShelleyTransactionBody);
CARDANO_MODEL_JSON(ShelleyTransaction);
CARDANO_MODEL_JSON(ShelleyTransactionOutput);
CARDANO_MODEL_JSON(ShelleyTransactionWitnessSet);
CARDANO_MODEL_JSON(ShelleyUpdate);

#undef CARDANO_MODEL_JSON

}  // namespace cardano::chain
