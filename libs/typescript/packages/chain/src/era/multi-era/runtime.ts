import {
  copyBytes,
  decodeCbor,
  encodeCbor,
  hexToBytes,
  type CborValue,
} from "@xray-network/cardano-core";
import {
  AuxiliaryDataHash,
  BlockBodyHash,
  BlockHeaderHash,
  ScriptDataHash,
  ScriptHash,
  TransactionHash,
  blake2b256,
} from "@xray-network/cardano-crypto";
import { Address } from "../../address/index.js";
import {
  AssetName,
  AuthCommitteeHotCert,
  AuxiliaryData,
  Block as ConwayBlock,
  Certificate as ConwayCertificate,
  MapAssetNameToNonZeroInt64,
  MapTransactionIndexToAuxiliaryData,
  Mint,
  NetworkId,
  PoolRegistration,
  PoolRetirement,
  RegCert,
  RegDrepCert,
  ResignCommitteeColdCert,
  StakeDelegation,
  StakeDeregistration,
  StakeRegDelegCert,
  StakeRegistration,
  StakeVoteDelegCert,
  StakeVoteRegDelegCert,
  TransactionBody as ConwayTransactionBody,
  TransactionOutput as ConwayTransactionOutput,
  TransactionWitnessSet,
  TransactionWitnessSetList,
  UnregCert,
  UnregDrepCert,
  UpdateDrepCert,
  Value,
  VoteDelegCert,
  VoteRegDelegCert,
} from "../conway/model.js";
import { TransactionInput, TransactionInputList } from "../shared/index.js";
import {
  arrayValue,
  HistoricalData,
  inputNode,
  mapValue,
  nodeJson,
  uint,
} from "../shared/codec.js";
import { AllegraBlock, AllegraTransactionBody } from "../allegra/runtime.js";
import { AlonzoBlock, AlonzoTransactionBody } from "../alonzo/runtime.js";
import { BabbageBlock, BabbageTransactionBody } from "../babbage/runtime.js";
import { ByronBlock, ByronBlockKind, ByronTx } from "../byron/runtime.js";
import { MaryBlock, MaryTransactionBody } from "../mary/runtime.js";
import {
  GenesisKeyDelegation,
  MoveInstantaneousRewardsCert,
  ShelleyBlock,
  ShelleyTransactionBody,
} from "../shelley/runtime.js";

export const MultiEraBlockKind = Object.freeze({ Byron: 0, Shelley: 1, Allegra: 2, Mary: 3, Alonzo: 4, Babbage: 5, Conway: 6 });
export const MultiEraCertificateKind = Object.freeze({ StakeRegistration: 0, StakeDeregistration: 1, StakeDelegation: 2, PoolRegistration: 3, PoolRetirement: 4, GenesisKeyDelegation: 5, MoveInstantaneousRewardsCert: 6, RegCert: 7, UnregCert: 8, VoteDelegCert: 9, StakeVoteDelegCert: 10, StakeRegDelegCert: 11, VoteRegDelegCert: 12, StakeVoteRegDelegCert: 13, AuthCommitteeHotCert: 14, ResignCommitteeColdCert: 15, RegDrepCert: 16, UnregDrepCert: 17, UpdateDrepCert: 18 });
export const MultiEraTransactionBodyKind = MultiEraBlockKind;

function eraClass(kind: number): typeof HistoricalData {
  return [ByronBlock, ShelleyBlock, AllegraBlock, MaryBlock, AlonzoBlock, BabbageBlock][kind] ?? HistoricalData;
}

function validateConwayPoolKeyHashes(node: CborValue): void {
  if (node.kind === "array") {
    if (
      node.values.length === 3 &&
      node.values[0]?.kind === "unsigned" &&
      node.values[0].value === 2n &&
      node.values[1]?.kind === "array" &&
      node.values[1].values.length === 2 &&
      node.values[1].values[1]?.kind === "bytes" &&
      node.values[1].values[1].value.length === 28 &&
      node.values[2]?.kind === "bytes" &&
      node.values[2].value.length !== 28
    ) {
      throw new TypeError(`Invalid pool key hash size ${node.values[2].value.length}; expected 28`);
    }
    for (const value of node.values) validateConwayPoolKeyHashes(value);
  } else if (node.kind === "map") {
    for (const [key, value] of node.entries) {
      validateConwayPoolKeyHashes(key);
      validateConwayPoolKeyHashes(value);
    }
  } else if (node.kind === "tag") validateConwayPoolKeyHashes(node.value);
}

export class MultiEraBlock extends HistoricalData {
  readonly #kind: number;
  readonly #networkTag: number;
  readonly #explicit: Uint8Array | undefined;

  private constructor(node: CborValue, kind: number, networkTag: number, explicit?: Uint8Array) {
    super(node);
    this.#kind = kind;
    this.#networkTag = networkTag;
    this.#explicit = explicit === undefined ? undefined : copyBytes(explicit);
  }

  public static override from_cbor_bytes(bytes: Uint8Array): MultiEraBlock {
    const block = ConwayBlock.from_cbor_bytes(bytes);
    return new MultiEraBlock(decodeCbor(block.to_cbor_bytes()), MultiEraBlockKind.Conway, 7);
  }
  public static override from_cbor_hex(hex: string): MultiEraBlock {
    return MultiEraBlock.from_cbor_bytes(hexToBytes(hex));
  }
  public static from_explicit_network_cbor_bytes(bytes: Uint8Array): MultiEraBlock {
    const envelope = decodeCbor(bytes);
    if (envelope.kind !== "array" || envelope.values.length !== 2) {
      throw new TypeError("Explicit network block must be [era, block]");
    }
    const tag = envelope.values[0];
    const block = envelope.values[1];
    if (tag?.kind !== "unsigned" || block === undefined || tag.value > 7n) {
      throw new TypeError("Explicit network block has an unsupported era tag");
    }
    const networkTag = Number(tag.value);
    if (networkTag === 7) validateConwayPoolKeyHashes(block);
    const encodedBlock = encodeCbor(block);
    if (networkTag <= 1) ByronBlock.from_cbor_bytes(encodedBlock);
    else if (networkTag === 2) ShelleyBlock.from_cbor_bytes(encodedBlock);
    else if (networkTag === 3) AllegraBlock.from_cbor_bytes(encodedBlock);
    else if (networkTag === 4) MaryBlock.from_cbor_bytes(encodedBlock);
    else if (networkTag === 5) AlonzoBlock.from_cbor_bytes(encodedBlock);
    else if (networkTag === 6) BabbageBlock.from_cbor_bytes(encodedBlock);
    else ConwayBlock.from_cbor_bytes(encodedBlock);
    const kind = networkTag <= 1 ? MultiEraBlockKind.Byron : networkTag - 1;
    return new MultiEraBlock(block, kind, networkTag, bytes);
  }
  public static override from_json(json: string): MultiEraBlock {
    const value = JSON.parse(json) as Record<string, unknown>;
    const names = ["Byron", "Shelley", "Allegra", "Mary", "Alonzo", "Babbage", "Conway"];
    const kind = names.findIndex((name) => value[name] !== undefined);
    if (kind < 0) throw new TypeError("Multi-era block JSON requires an era variant");
    const node = inputNode(value[names[kind] as string]);
    const encoded = encodeCbor(node);
    if (kind === MultiEraBlockKind.Byron) ByronBlock.from_cbor_bytes(encoded);
    else if (kind === MultiEraBlockKind.Shelley) ShelleyBlock.from_cbor_bytes(encoded);
    else if (kind === MultiEraBlockKind.Allegra) AllegraBlock.from_cbor_bytes(encoded);
    else if (kind === MultiEraBlockKind.Mary) MaryBlock.from_cbor_bytes(encoded);
    else if (kind === MultiEraBlockKind.Alonzo) AlonzoBlock.from_cbor_bytes(encoded);
    else if (kind === MultiEraBlockKind.Babbage) BabbageBlock.from_cbor_bytes(encoded);
    else ConwayBlock.from_cbor_bytes(encoded);
    return new MultiEraBlock(node, kind, kind === 0 ? 1 : kind + 1);
  }
  private static create(kind: number, value: { to_cbor_bytes(): Uint8Array }): MultiEraBlock {
    return new MultiEraBlock(decodeCbor(value.to_cbor_bytes()), kind, kind === 0 ? 1 : kind + 1);
  }
  public static new_byron(value: ByronBlock): MultiEraBlock {
    const tag = value.kind() === ByronBlockKind.EpochBoundary ? 0 : 1;
    return new MultiEraBlock(value.cbor_node(), MultiEraBlockKind.Byron, tag);
  }
  public static new_shelley(value: ShelleyBlock): MultiEraBlock { return this.create(1, value); }
  public static new_allegra(value: AllegraBlock): MultiEraBlock { return this.create(2, value); }
  public static new_mary(value: MaryBlock): MultiEraBlock { return this.create(3, value); }
  public static new_alonzo(value: AlonzoBlock): MultiEraBlock { return this.create(4, value); }
  public static new_babbage(value: BabbageBlock): MultiEraBlock { return this.create(5, value); }
  public static new_conway(value: ConwayBlock): MultiEraBlock { return this.create(6, value); }
  public kind(): number { return this.#kind; }
  private historical(kind: number): HistoricalData | undefined {
    if (this.#kind !== kind) return undefined;
    if (kind === MultiEraBlockKind.Byron) return new ByronBlock(this.cbor_node(), this.#networkTag === 0 ? ByronBlockKind.EpochBoundary : ByronBlockKind.Main);
    return eraClass(kind).from_cbor_bytes(this.to_cbor_bytes());
  }
  public as_byron(): ByronBlock | undefined { return this.historical(0) as ByronBlock | undefined; }
  public as_shelley(): ShelleyBlock | undefined { return this.historical(1) as ShelleyBlock | undefined; }
  public as_allegra(): AllegraBlock | undefined { return this.historical(2) as AllegraBlock | undefined; }
  public as_mary(): MaryBlock | undefined { return this.historical(3) as MaryBlock | undefined; }
  public as_alonzo(): AlonzoBlock | undefined { return this.historical(4) as AlonzoBlock | undefined; }
  public as_babbage(): BabbageBlock | undefined { return this.historical(5) as BabbageBlock | undefined; }
  public as_conway(): ConwayBlock | undefined {
    return this.#kind === MultiEraBlockKind.Conway
      ? ConwayBlock.from_cbor_bytes(this.to_cbor_bytes())
      : undefined;
  }
  public header(): MultiEraBlockHeader {
    const header = arrayValue(this.cbor_node(), 0);
    if (header === undefined) throw new TypeError("Block has no header");
    return new MultiEraBlockHeader(header, this.#kind, this.#networkTag);
  }
  public transaction_bodies(): MultiEraTransactionBodyList {
    const output = MultiEraTransactionBodyList.new();
    if (this.#networkTag === 0) return output;
    if (this.#networkTag === 1) {
      const body = arrayValue(this.cbor_node(), 1);
      const payload = body === undefined ? undefined : arrayValue(body, 0);
      if (payload?.kind === "array") {
        for (const auxiliary of payload.values) {
          const tx = arrayValue(auxiliary, 0);
          if (tx !== undefined) output.add(new MultiEraTransactionBody(tx, 0));
        }
      }
      return output;
    }
    const bodies = arrayValue(this.cbor_node(), 1);
    if (bodies?.kind === "array") {
      for (const body of bodies.values) output.add(new MultiEraTransactionBody(body, this.#kind));
    }
    return output;
  }
  public transaction_witness_sets(): TransactionWitnessSetList {
    const output = TransactionWitnessSetList.new();
    if (this.#kind === MultiEraBlockKind.Byron) return output;
    const witnesses = arrayValue(this.cbor_node(), 2);
    if (witnesses?.kind === "array") {
      for (const witness of witnesses.values) {
        output.add(TransactionWitnessSet.from_cbor_bytes(encodeCbor(witness)));
      }
    }
    return output;
  }
  public auxiliary_data_set(): MapTransactionIndexToAuxiliaryData {
    const output = MapTransactionIndexToAuxiliaryData.new();
    if (this.#kind === MultiEraBlockKind.Byron) return output;
    const metadata = arrayValue(this.cbor_node(), 3);
    if (metadata?.kind === "map") {
      for (const [key, value] of metadata.entries) {
        if (key.kind === "unsigned") output.insert(key.value, AuxiliaryData.from_cbor_bytes(encodeCbor(value)));
      }
    }
    return output;
  }
  public invalid_transactions(): Uint16Array {
    if (this.#kind < MultiEraBlockKind.Alonzo) return new Uint16Array();
    const invalid = arrayValue(this.cbor_node(), 4);
    if (invalid?.kind !== "array") return new Uint16Array();
    return Uint16Array.from(invalid.values.map((value) => value.kind === "unsigned" ? Number(value.value) : 0));
  }
  public is_empty(): boolean { return this.transaction_bodies().len() === 0; }
  public hash(): Uint8Array {
    const header = this.header().to_cbor_bytes();
    const wrapped = this.#kind === MultiEraBlockKind.Byron
      ? Uint8Array.from([0x82, this.#networkTag, ...header])
      : header;
    return blake2b256(wrapped);
  }
  public to_explicit_network_cbor_bytes(): Uint8Array {
    if (this.#explicit !== undefined) return copyBytes(this.#explicit);
    return encodeCbor({
      kind: "array",
      values: [uint(BigInt(this.#networkTag)), this.cbor_node()],
      encoding: { kind: "definite", width: 0 },
    });
  }
  public override to_js_value(): unknown {
    const name = ["Byron", "Shelley", "Allegra", "Mary", "Alonzo", "Babbage", "Conway"][this.#kind];
    return { [name as string]: super.to_js_value() };
  }
}

export class MultiEraBlockHeader extends HistoricalData {
  readonly #kind: number;
  readonly #networkTag: number;
  public constructor(node: CborValue, kind: number = MultiEraBlockKind.Conway, networkTag = 7) {
    super(node);
    this.#kind = kind;
    this.#networkTag = networkTag;
  }
  private body(): CborValue {
    if (this.#kind === MultiEraBlockKind.Byron) return this.cbor_node();
    return arrayValue(this.cbor_node(), 0) ?? this.cbor_node();
  }
  public block_number(): bigint {
    if (this.#kind === MultiEraBlockKind.Byron) {
      return this.#networkTag === 0 ? this.uintAt([3, 1, 0], 0n) : this.uintAt([3, 2, 0], 0n);
    }
    return this.uintAt([0], 0n);
  }
  public slot(): bigint {
    if (this.#networkTag === 0) return this.uintAt([3, 0], 0n) * 21_600n;
    if (this.#networkTag === 1) {
      const epoch = this.uintAt([3, 0, 0], 0n);
      const relative = this.uintAt([3, 0, 1], 0n);
      return epoch * 21_600n + relative;
    }
    return this.uintAt([1], 0n);
  }
  public prev_hash(): BlockHeaderHash | undefined {
    return this.hashAt([this.#kind === MultiEraBlockKind.Byron ? 1 : 2], BlockHeaderHash);
  }
  public block_body_size(): bigint | undefined { return this.#kind === 0 ? undefined : this.uintAt([7]); }
  public block_body_hash(): BlockBodyHash | undefined { return this.#kind === 0 ? undefined : this.hashAt([8], BlockBodyHash); }
  public issuer_vkey(): undefined { return undefined; }
  public vrf_vkey(): undefined { return undefined; }
  public nonce_vrf(): undefined { return undefined; }
  public leader_vrf(): undefined { return undefined; }
  public vrf_result(): undefined { return undefined; }
  public operational_cert(): undefined { return undefined; }
  public protocol_version(): undefined { return undefined; }
  private uintAt(path: number[], fallback?: bigint): bigint {
    let node: CborValue | undefined = this.body();
    for (const index of path) node = node === undefined ? undefined : arrayValue(node, index);
    if (node?.kind === "unsigned") return node.value;
    if (fallback !== undefined) return fallback;
    throw new TypeError("Header field is not an unsigned integer");
  }
  private hashAt<T>(path: number[], owner: { from_raw_bytes(bytes: Uint8Array): T }): T | undefined {
    let node: CborValue | undefined = this.body();
    for (const index of path) node = node === undefined ? undefined : arrayValue(node, index);
    return node?.kind === "bytes" && node.value.length === 32 ? owner.from_raw_bytes(node.value) : undefined;
  }
}

export class MultiEraTransactionInput {
  readonly #node: CborValue;
  readonly #byron: boolean;
  public constructor(node: CborValue, byron = false) { this.#node = node; this.#byron = byron; }
  public hash(): TransactionHash | undefined {
    if (this.#byron) {
      const variant = arrayValue(this.#node, 1);
      if (arrayValue(this.#node, 0)?.kind !== "unsigned" || variant?.kind !== "bytes") return undefined;
      const embedded = decodeCbor(variant.value);
      const hash = arrayValue(embedded, 0);
      return hash?.kind === "bytes" && hash.value.length === 32 ? TransactionHash.from_raw_bytes(hash.value) : undefined;
    }
    const hash = arrayValue(this.#node, 0);
    return hash?.kind === "bytes" && hash.value.length === 32 ? TransactionHash.from_raw_bytes(hash.value) : undefined;
  }
  public index(): bigint | undefined {
    if (this.#byron) {
      const variant = arrayValue(this.#node, 1);
      if (variant?.kind !== "bytes") return undefined;
      const embedded = decodeCbor(variant.value);
      const index = arrayValue(embedded, 1);
      return index?.kind === "unsigned" ? index.value : undefined;
    }
    const index = arrayValue(this.#node, 1);
    return index?.kind === "unsigned" ? index.value : undefined;
  }
  public cbor_node(): CborValue { return this.#node; }
}

export class MultiEraTransactionInputList {
  readonly #values: MultiEraTransactionInput[] = [];
  public static new(): MultiEraTransactionInputList { return new MultiEraTransactionInputList(); }
  public len(): number { return this.#values.length; }
  public get(index: number): MultiEraTransactionInput {
    const value = this.#values[index];
    if (value === undefined) throw new RangeError(`Index ${index} is outside the list`);
    return value;
  }
  public add(value: MultiEraTransactionInput): void { this.#values.push(value); }
}

export class MultiEraTransactionOutput {
  readonly #node: CborValue;
  readonly #byron: boolean;
  public constructor(node: CborValue, byron = false) { this.#node = node; this.#byron = byron; }
  public address(): Address {
    const address = this.#node.kind === "map" ? mapValue(this.#node, 0n) : arrayValue(this.#node, 0);
    if (address?.kind === "bytes") return Address.from_raw_bytes(address.value);
    if (this.#byron && address !== undefined) return Address.from_raw_bytes(encodeCbor(address));
    throw new TypeError("Transaction output has no supported address");
  }
  public amount(): Value {
    const amount = this.#node.kind === "map" ? mapValue(this.#node, 1n) : arrayValue(this.#node, 1);
    if (amount === undefined) throw new TypeError("Transaction output has no amount");
    if (this.#byron) {
      if (amount.kind !== "unsigned") throw new TypeError("Byron output amount must be unsigned");
      return Value.from_coin(amount.value);
    }
    return Value.from_cbor_bytes(encodeCbor(amount));
  }
  public cbor_node(): CborValue { return this.#node; }
}

export class MultiEraTransactionOutputList {
  readonly #values: MultiEraTransactionOutput[] = [];
  public static new(): MultiEraTransactionOutputList { return new MultiEraTransactionOutputList(); }
  public len(): number { return this.#values.length; }
  public get(index: number): MultiEraTransactionOutput {
    const value = this.#values[index];
    if (value === undefined) throw new RangeError(`Index ${index} is outside the list`);
    return value;
  }
  public add(value: MultiEraTransactionOutput): void { this.#values.push(value); }
}

export class MultiEraCertificate {
  readonly #node: CborValue;
  public constructor(node: CborValue) { this.#node = node; }
  public kind(): number {
    const tag = arrayValue(this.#node, 0);
    if (tag?.kind !== "unsigned" || tag.value > 18n) throw new TypeError("Invalid certificate tag");
    return Number(tag.value);
  }
  public static from_json(json: string): MultiEraCertificate {
    return new MultiEraCertificate(inputNode(JSON.parse(json)));
  }
  public static from_cbor_bytes(bytes: Uint8Array): MultiEraCertificate {
    const certificate = ConwayCertificate.from_cbor_bytes(bytes);
    return new MultiEraCertificate(decodeCbor(certificate.to_cbor_bytes()));
  }
  public static from_cbor_hex(hex: string): MultiEraCertificate {
    return this.from_cbor_bytes(hexToBytes(hex));
  }
  private variant<T>(tag: number, owner: { from_cbor_bytes(bytes: Uint8Array): T }): T | undefined {
    return this.kind() === tag ? owner.from_cbor_bytes(encodeCbor(this.#node)) : undefined;
  }
  public as_stake_registration(): StakeRegistration | undefined { return this.variant(0, StakeRegistration); }
  public as_stake_deregistration(): StakeDeregistration | undefined { return this.variant(1, StakeDeregistration); }
  public as_stake_delegation(): StakeDelegation | undefined { return this.variant(2, StakeDelegation); }
  public as_pool_registration(): PoolRegistration | undefined { return this.variant(3, PoolRegistration); }
  public as_pool_retirement(): PoolRetirement | undefined { return this.variant(4, PoolRetirement); }
  public as_genesis_key_delegation(): GenesisKeyDelegation | undefined { return this.variant(5, GenesisKeyDelegation); }
  public as_move_instantaneous_rewards_cert(): MoveInstantaneousRewardsCert | undefined { return this.variant(6, MoveInstantaneousRewardsCert); }
  public as_reg_cert(): RegCert | undefined { return this.variant(7, RegCert); }
  public as_unreg_cert(): UnregCert | undefined { return this.variant(8, UnregCert); }
  public as_vote_deleg_cert(): VoteDelegCert | undefined { return this.variant(9, VoteDelegCert); }
  public as_stake_vote_deleg_cert(): StakeVoteDelegCert | undefined { return this.variant(10, StakeVoteDelegCert); }
  public as_stake_reg_deleg_cert(): StakeRegDelegCert | undefined { return this.variant(11, StakeRegDelegCert); }
  public as_vote_reg_deleg_cert(): VoteRegDelegCert | undefined { return this.variant(12, VoteRegDelegCert); }
  public as_stake_vote_reg_deleg_cert(): StakeVoteRegDelegCert | undefined { return this.variant(13, StakeVoteRegDelegCert); }
  public as_auth_committee_hot_cert(): AuthCommitteeHotCert | undefined { return this.variant(14, AuthCommitteeHotCert); }
  public as_resign_committee_cold_cert(): ResignCommitteeColdCert | undefined { return this.variant(15, ResignCommitteeColdCert); }
  public as_reg_drep_cert(): RegDrepCert | undefined { return this.variant(16, RegDrepCert); }
  public as_unreg_drep_cert(): UnregDrepCert | undefined { return this.variant(17, UnregDrepCert); }
  public as_update_drep_cert(): UpdateDrepCert | undefined { return this.variant(18, UpdateDrepCert); }
  public as_conway(): ConwayCertificate { return ConwayCertificate.from_cbor_bytes(encodeCbor(this.#node)); }
  public to_js_value(): unknown { return nodeJson(this.#node); }
  public to_json(): string { return JSON.stringify(this.to_js_value()); }
  public cbor_node(): CborValue { return this.#node; }
}

export class MultiEraCertificateList {
  readonly #values: MultiEraCertificate[] = [];
  public static new(): MultiEraCertificateList { return new MultiEraCertificateList(); }
  public len(): number { return this.#values.length; }
  public get(index: number): MultiEraCertificate {
    const value = this.#values[index];
    if (value === undefined) throw new RangeError(`Index ${index} is outside the list`);
    return value;
  }
  public add(value: MultiEraCertificate): void { this.#values.push(value); }
}

export class MultiEraUpdate {
  readonly #node: CborValue;
  public constructor(node: CborValue) { this.#node = node; }
  public epoch(): bigint {
    const epoch = arrayValue(this.#node, 1);
    if (epoch?.kind !== "unsigned") throw new TypeError("Update has no epoch");
    return epoch.value;
  }
  public proposed_protocol_parameter_updates(): HistoricalData {
    const updates = arrayValue(this.#node, 0);
    if (updates === undefined) throw new TypeError("Update has no proposed updates");
    return new HistoricalData(updates);
  }
}

export class MultiEraTransactionBody extends HistoricalData {
  readonly #kind: number;
  public constructor(node: CborValue, kind: number = MultiEraTransactionBodyKind.Conway) {
    super(node);
    this.#kind = kind;
  }
  public static override from_cbor_bytes(bytes: Uint8Array): MultiEraTransactionBody {
    return new MultiEraTransactionBody(decodeCbor(bytes));
  }
  private static create(kind: number, value: { to_cbor_bytes(): Uint8Array }): MultiEraTransactionBody {
    return new MultiEraTransactionBody(decodeCbor(value.to_cbor_bytes()), kind);
  }
  public static new_byron(value: ByronTx): MultiEraTransactionBody { return this.create(0, value); }
  public static new_shelley(value: ShelleyTransactionBody): MultiEraTransactionBody { return this.create(1, value); }
  public static new_allegra(value: AllegraTransactionBody): MultiEraTransactionBody { return this.create(2, value); }
  public static new_mary(value: MaryTransactionBody): MultiEraTransactionBody { return this.create(3, value); }
  public static new_alonzo(value: AlonzoTransactionBody): MultiEraTransactionBody { return this.create(4, value); }
  public static new_babbage(value: BabbageTransactionBody): MultiEraTransactionBody { return this.create(5, value); }
  public static new_conway(value: ConwayTransactionBody): MultiEraTransactionBody { return this.create(6, value); }
  public kind(): number { return this.#kind; }
  private asHistorical(kind: number, owner: typeof HistoricalData): HistoricalData | undefined {
    return this.#kind === kind ? owner.from_cbor_bytes(this.to_cbor_bytes()) : undefined;
  }
  public as_byron(): ByronTx | undefined { return this.asHistorical(0, ByronTx) as ByronTx | undefined; }
  public as_shelley(): ShelleyTransactionBody | undefined { return this.asHistorical(1, ShelleyTransactionBody) as ShelleyTransactionBody | undefined; }
  public as_allegra(): AllegraTransactionBody | undefined { return this.asHistorical(2, AllegraTransactionBody) as AllegraTransactionBody | undefined; }
  public as_mary(): MaryTransactionBody | undefined { return this.asHistorical(3, MaryTransactionBody) as MaryTransactionBody | undefined; }
  public as_alonzo(): AlonzoTransactionBody | undefined { return this.asHistorical(4, AlonzoTransactionBody) as AlonzoTransactionBody | undefined; }
  public as_babbage(): BabbageTransactionBody | undefined { return this.asHistorical(5, BabbageTransactionBody) as BabbageTransactionBody | undefined; }
  public as_conway(): ConwayTransactionBody | undefined {
    return this.#kind === 6 ? ConwayTransactionBody.from_cbor_bytes(this.to_cbor_bytes()) : undefined;
  }
  private field(key: bigint): CborValue | undefined {
    if (this.#kind === 0) return undefined;
    return mapValue(this.cbor_node(), key);
  }
  public inputs(): MultiEraTransactionInputList {
    const result = MultiEraTransactionInputList.new();
    const inputs = this.#kind === 0 ? arrayValue(this.cbor_node(), 0) : this.field(0n);
    if (inputs?.kind === "array") for (const input of inputs.values) result.add(new MultiEraTransactionInput(input, this.#kind === 0));
    return result;
  }
  public outputs(): MultiEraTransactionOutputList {
    const result = MultiEraTransactionOutputList.new();
    const outputs = this.#kind === 0 ? arrayValue(this.cbor_node(), 1) : this.field(1n);
    if (outputs?.kind === "array") for (const output of outputs.values) result.add(new MultiEraTransactionOutput(output, this.#kind === 0));
    return result;
  }
  private unsigned(key: bigint): bigint | undefined {
    const node = this.field(key);
    return node?.kind === "unsigned" ? node.value : undefined;
  }
  public fee(): bigint | undefined { return this.#kind === 0 ? undefined : this.unsigned(2n); }
  public ttl(): bigint | undefined { return this.unsigned(3n); }
  public validity_interval_start(): bigint | undefined { return this.unsigned(8n); }
  public current_treasury_value(): bigint | undefined { return this.unsigned(21n); }
  public donation(): bigint | undefined { return this.unsigned(22n); }
  public total_collateral(): bigint | undefined { return this.unsigned(17n); }
  public certs(): MultiEraCertificateList | undefined {
    const node = this.field(4n);
    if (node?.kind !== "array") return undefined;
    const result = MultiEraCertificateList.new();
    for (const certificate of node.values) result.add(new MultiEraCertificate(certificate));
    return result;
  }
  private transactionInputs(key: bigint): TransactionInputList | undefined {
    const node = this.field(key);
    if (node?.kind !== "array") return undefined;
    const result = TransactionInputList.new();
    for (const input of node.values) result.add(TransactionInput.from_cbor_bytes(encodeCbor(input)));
    return result;
  }
  public collateral_inputs(): TransactionInputList | undefined { return this.transactionInputs(13n); }
  public reference_inputs(): TransactionInputList | undefined { return this.transactionInputs(18n); }
  public collateral_return(): MultiEraTransactionOutput | undefined {
    const node = this.field(16n);
    return node === undefined ? undefined : new MultiEraTransactionOutput(node);
  }
  public auxiliary_data_hash(): AuxiliaryDataHash | undefined {
    const node = this.field(7n);
    return node?.kind === "bytes" && node.value.length === 32 ? AuxiliaryDataHash.from_raw_bytes(node.value) : undefined;
  }
  public script_data_hash(): ScriptDataHash | undefined {
    const node = this.field(11n);
    return node?.kind === "bytes" && node.value.length === 32 ? ScriptDataHash.from_raw_bytes(node.value) : undefined;
  }
  public mint(): Mint | undefined {
    const node = this.field(9n);
    if (node === undefined) return undefined;
    if (node.kind !== "map") throw new TypeError("Mint must be a CBOR map");
    const mint = Mint.new();
    for (const [policyNode, assetsNode] of node.entries) {
      if (policyNode.kind !== "bytes" || policyNode.value.length !== 28 || assetsNode.kind !== "map") {
        throw new TypeError("Invalid mint policy");
      }
      const assets = MapAssetNameToNonZeroInt64.new();
      for (const [assetNode, amountNode] of assetsNode.entries) {
        if (assetNode.kind !== "bytes" || (amountNode.kind !== "unsigned" && amountNode.kind !== "negative")) {
          throw new TypeError("Invalid mint asset");
        }
        assets.insert(AssetName.from_raw_bytes(assetNode.value), amountNode.value);
      }
      mint.insert(ScriptHash.from_raw_bytes(policyNode.value), assets);
    }
    return mint;
  }
  public network_id(): NetworkId | undefined {
    const value = this.unsigned(15n);
    return value === undefined ? undefined : NetworkId.new(value);
  }
  public update(): MultiEraUpdate | undefined {
    const node = this.field(6n);
    return node === undefined ? undefined : new MultiEraUpdate(node);
  }
  public withdrawals(): HistoricalData | undefined { return this.optional(5n); }
  public required_signers(): HistoricalData | undefined { return this.optional(14n); }
  public voting_procedures(): HistoricalData | undefined { return this.optional(19n); }
  public proposal_procedures(): HistoricalData | undefined { return this.optional(20n); }
  private optional(key: bigint): HistoricalData | undefined {
    const node = this.field(key);
    return node === undefined ? undefined : new HistoricalData(node);
  }
  public hash(): TransactionHash { return TransactionHash.from_raw_bytes(blake2b256(this.to_cbor_bytes())); }
}

export class MultiEraTransactionBodyList {
  readonly #values: MultiEraTransactionBody[] = [];
  public static new(): MultiEraTransactionBodyList { return new MultiEraTransactionBodyList(); }
  public len(): number { return this.#values.length; }
  public get(index: number): MultiEraTransactionBody {
    const value = this.#values[index];
    if (value === undefined) throw new RangeError(`Index ${index} is outside the list`);
    return value;
  }
  public add(value: MultiEraTransactionBody): void { this.#values.push(value); }
}

export class MultiEraProtocolParamUpdate extends HistoricalData {
  private numeric(key: bigint): bigint | undefined {
    const node = mapValue(this.cbor_node(), key);
    return node?.kind === "unsigned" ? node.value : undefined;
  }
  public minfee_a(): bigint | undefined { return this.numeric(0n); }
  public minfee_b(): bigint | undefined { return this.numeric(1n); }
  public max_block_body_size(): bigint | undefined { return this.numeric(2n); }
  public max_transaction_size(): bigint | undefined { return this.numeric(3n); }
  public max_block_header_size(): bigint | undefined { return this.numeric(4n); }
  public key_deposit(): bigint | undefined { return this.numeric(5n); }
  public pool_deposit(): bigint | undefined { return this.numeric(6n); }
  public maximum_epoch(): bigint | undefined { return this.numeric(7n); }
  public n_opt(): bigint | undefined { return this.numeric(8n); }
  public min_utxo_value(): bigint | undefined { return this.numeric(15n); }
  public min_pool_cost(): bigint | undefined { return this.numeric(16n); }
  public ada_per_utxo_byte(): bigint | undefined { return this.numeric(17n); }
  public max_value_size(): bigint | undefined { return this.numeric(22n); }
  public collateral_percentage(): bigint | undefined { return this.numeric(23n); }
  public max_collateral_inputs(): bigint | undefined { return this.numeric(24n); }
  public min_committee_size(): bigint | undefined { return this.numeric(27n); }
  public committee_term_limit(): bigint | undefined { return this.numeric(28n); }
  public governance_action_validity_period(): bigint | undefined { return this.numeric(29n); }
  public governance_action_deposit(): bigint | undefined { return this.numeric(30n); }
  public d_rep_deposit(): bigint | undefined { return this.numeric(31n); }
  public d_rep_inactivity_period(): bigint | undefined { return this.numeric(32n); }
}

export class MapGenesisHashToMultiEraProtocolParamUpdate extends HistoricalData {}
