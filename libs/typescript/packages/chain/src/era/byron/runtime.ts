import { decodeCbor, type CborValue } from "@xray-network/cardano-core";
import {
  HistoricalData as HistoricalCodecData,
  HistoricalList as HistoricalCodecList,
} from "../shared/codec.js";
import { validateByron } from "./validation.js";

class HistoricalData extends HistoricalCodecData {
  public static override validateNode(node: CborValue): void { validateByron(this.name, node); }
}
class HistoricalList<T> extends HistoricalCodecList<T> {
  public static override validateNode(node: CborValue): void { validateByron(this.name, node); }
}

export const ByronBlockKind = Object.freeze({ EpochBoundary: 0, Main: 1 });
export const ByronBlockSignatureKind = Object.freeze({ Signature: 0, ProxyLight: 1, ProxyHeavy: 2 });
export const ByronTxInKind = Object.freeze({ ByronTxInRegular: 0, ByronTxInGenesis: 1 });
export const ByronTxWitnessKind = Object.freeze({ ByronPkWitness: 0, ByronScriptWitness: 1, ByronRedeemWitness: 2 });
export const SscKind = Object.freeze({ SscCommitmentsPayload: 0, SscOpeningsPayload: 1, SscSharesPayload: 2, SscCertificatesPayload: 3 });
export const SscProofKind = Object.freeze({ SscCommitmentsProof: 0, SscOpeningsProof: 1, SscSharesProof: 2, SscCertificatesProof: 3 });

export class ByronBlock extends HistoricalData {
  readonly #kind: number;
  public constructor(node: CborValue, kind: number = ByronBlockKind.Main) { super(node); this.#kind = kind; }
  public static override from_cbor_bytes(bytes: Uint8Array): ByronBlock {
    const node = decodeCbor(bytes);
    validateByron("ByronBlock", node);
    return new ByronBlock(node);
  }
  public static new_epoch_boundary(value: HistoricalData): ByronBlock {
    return new ByronBlock(value.cbor_node(), ByronBlockKind.EpochBoundary);
  }
  public static new_main(value: HistoricalData): ByronBlock {
    return new ByronBlock(value.cbor_node(), ByronBlockKind.Main);
  }
  public kind(): number { return this.#kind; }
  public as_epoch_boundary(): HistoricalData | undefined {
    return this.#kind === ByronBlockKind.EpochBoundary ? new HistoricalData(this.cbor_node()) : undefined;
  }
  public as_main(): HistoricalData | undefined {
    return this.#kind === ByronBlockKind.Main ? new HistoricalData(this.cbor_node()) : undefined;
  }
}

export class AddressIdList extends HistoricalList<HistoricalData> {}
export class BigIntegerList extends HistoricalList<HistoricalData> {}
export class BlockHeaderExtraData extends HistoricalData {}
export class Bvermod extends HistoricalData {}
export class ByronAny extends HistoricalData {}
export class ByronAnyList extends HistoricalList<HistoricalData> {}
export class ByronAttributes extends HistoricalData {}
export class ByronAttributesList extends HistoricalList<HistoricalData> {}
export class ByronBlockBody extends HistoricalData {}
export class ByronBlockConsensusData extends HistoricalData {}
export class ByronBlockHeader extends HistoricalData {}
export class ByronBlockSignature extends HistoricalData {}
export class ByronBlockSignatureNormal extends HistoricalData {}
export class ByronBlockSignatureProxyHeavy extends HistoricalData {}
export class ByronBlockSignatureProxyLight extends HistoricalData {}
export class ByronBlockVersion extends HistoricalData {}
export class ByronBodyProof extends HistoricalData {}
export class ByronDelegation extends HistoricalData {}
export class ByronDelegationList extends HistoricalList<HistoricalData> {}
export class ByronDelegationSignature extends HistoricalData {}
export class ByronDifficulty extends HistoricalData {}
export class ByronEbBlock extends HistoricalData {}
export class ByronMainBlock extends HistoricalData {}
export class ByronPkWitness extends HistoricalData {}
export class ByronPkWitnessEntry extends HistoricalData {}
export class ByronRedeemWitness extends HistoricalData {}
export class ByronRedeemerScript extends HistoricalData {}
export class ByronRedeemerWitnessEntry extends HistoricalData {}
export class ByronScriptWitness extends HistoricalData {}
export class ByronScriptWitnessEntry extends HistoricalData {}
export class ByronSlotId extends HistoricalData {}
export class ByronSoftwareVersion extends HistoricalData {}
export class ByronTx extends HistoricalData {}
export class ByronTxFeePolicy extends HistoricalData {}
export class ByronTxFeePolicyList extends HistoricalList<HistoricalData> {}
export class ByronTxIn extends HistoricalData {}
export class ByronTxInGenesis extends HistoricalData {}
export class ByronTxInList extends HistoricalList<HistoricalData> {}
export class ByronTxInRegular extends HistoricalData {}
export class ByronTxOutList extends HistoricalList<HistoricalData> {}
export class ByronTxOutPtr extends HistoricalData {}
export class ByronTxProof extends HistoricalData {}
export class ByronTxWitness extends HistoricalData {}
export class ByronTxWitnessList extends HistoricalList<HistoricalData> {}
export class ByronUpdate extends HistoricalData {}
export class ByronUpdateData extends HistoricalData {}
export class ByronUpdateProposal extends HistoricalData {}
export class ByronUpdateProposalList extends HistoricalList<HistoricalData> {}
export class ByronUpdateVote extends HistoricalData {}
export class ByronUpdateVoteList extends HistoricalList<HistoricalData> {}
export class ByronValidatorScript extends HistoricalData {}
export class BytesList extends HistoricalList<HistoricalData> {}
export class EbbConsensusData extends HistoricalData {}
export class EbbHead extends HistoricalData {}
export class EpochRange extends HistoricalData {}
export class LightWeightDelegationSignature extends HistoricalData {}
export class LightWeightDlg extends HistoricalData {}
export class MapSystemTagToByronUpdateData extends HistoricalData {}
export class SoftForkRule extends HistoricalData {}
export class SoftForkRuleList extends HistoricalList<HistoricalData> {}
export class Ssc extends HistoricalData {}
export class SscCert extends HistoricalData {}
export class SscCertificatesPayload extends HistoricalData {}
export class SscCertificatesProof extends HistoricalData {}
export class SscCerts extends HistoricalData {}
export class SscCommitment extends HistoricalData {}
export class SscCommitmentsPayload extends HistoricalData {}
export class SscCommitmentsProof extends HistoricalData {}
export class SscOpeningsPayload extends HistoricalData {}
export class SscOpeningsProof extends HistoricalData {}
export class SscOpens extends HistoricalData {}
export class SscProof extends HistoricalData {}
export class SscShares extends HistoricalData {}
export class SscSharesPayload extends HistoricalData {}
export class SscSharesProof extends HistoricalData {}
export class SscSharesSubmap extends HistoricalData {}
export class SscSignedCommitment extends HistoricalData {}
export class SscSignedCommitments extends HistoricalData {}
export class StakeholderIdList extends HistoricalList<HistoricalData> {}
export class StdFeePolicy extends HistoricalData {}
export class SystemTagList extends HistoricalList<HistoricalData> {}
export class TxAux extends HistoricalData {}
export class TxPayload extends HistoricalList<HistoricalData> {}
export class VssEncryptedShare extends HistoricalData {}
export class VssProof extends HistoricalData {}
export class VssShares extends HistoricalData {}
