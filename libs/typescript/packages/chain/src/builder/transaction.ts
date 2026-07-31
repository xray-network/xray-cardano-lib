import {
  UINT64_MAX,
  bytesToHex,
  copyBytes,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/cardano-core";
import type { CborValue } from "@xray-network/cardano-core";
import {
  DatumHash,
  Ed25519KeyHash,
  PublicKey,
  ScriptHash,
  blake2b224,
  secureRandomBytes,
} from "@xray-network/cardano-crypto";
import { Address, AddressKind, RewardAddress } from "../address/index.js";
import { ByronAddress } from "../era/byron/address.js";
import { BootstrapWitness } from "../era/byron/transaction.js";
import {
  AuxiliaryData,
  AssetName,
  Certificate,
  CostModels,
  DatumOption,
  ExUnitPrices,
  ExUnits,
  GovActionId,
  Language,
  LegacyRedeemer,
  LegacyRedeemerList,
  MapAssetNameToNonZeroInt64,
  Mint,
  MultiAsset,
  NetworkId,
  PlutusV1Script,
  PlutusV1ScriptList,
  PlutusV2Script,
  PlutusV2ScriptList,
  PlutusV3Script,
  PlutusV3ScriptList,
  ProposalProcedure,
  RedeemerTag,
  RequiredSigners,
  Script,
  ScriptRef,
  Transaction,
  TransactionBody,
  TransactionOutput,
  TransactionWitnessSet,
  Value,
  Vkeywitness,
  Voter,
  VotingProcedure,
  Withdrawals,
} from "../era/conway/model.js";
import {
  NativeScript,
  NativeScriptList,
  PlutusData,
  PlutusDataList,
  TransactionInput,
} from "../era/shared/models.js";
import {
  LinearFee,
  calc_script_data_hash,
  get_deposit,
  get_implicit_input,
  hash_auxiliary_data,
  hash_plutus_data,
  min_ada_required,
  min_fee as transactionMinFee,
} from "../ledger/operations.js";

interface CborSerializable { to_cbor_bytes(): Uint8Array }
interface CloneConstructor<T = unknown> { from_cbor_bytes(bytes: Uint8Array): T }

function clone<T extends CborSerializable>(value: T, constructor: CloneConstructor): T {
  return constructor.from_cbor_bytes(value.to_cbor_bytes()) as T;
}

function uint(value: bigint, name = "value"): CborValue {
  if (value < 0n || value > UINT64_MAX) throw new RangeError(`${name} must fit uint64`);
  return { kind: "unsigned", value, encoding: { width: 0 } };
}

function bytes(value: Uint8Array): CborValue {
  return { kind: "bytes", value: copyBytes(value), encoding: { kind: "definite", width: 0 } };
}

function array(values: readonly CborValue[]): CborValue {
  return { kind: "array", values, encoding: { kind: "definite", width: 0 } };
}

function map(entries: ReadonlyArray<readonly [CborValue, CborValue]>): CborValue {
  return { kind: "map", entries, encoding: { kind: "definite", width: 0 } };
}

function node(value: CborSerializable): CborValue { return decodeCbor(value.to_cbor_bytes()); }
function canonicalHex(value: CborSerializable): string { return bytesToHex(encodeCbor(node(value), { mode: "canonical" })); }
function checkedCoin(value: bigint, name = "coin"): bigint {
  if (value < 0n || value > UINT64_MAX) throw new RangeError(`${name} must fit uint64`);
  return value;
}

function collectionNode(values: readonly CborSerializable[], tagged = true): CborValue {
  const inner = array(values.map(node));
  return tagged ? { kind: "tag", tag: 258n, value: inner, encoding: { width: 2 } } : inner;
}

function compareBytes(left: Uint8Array, right: Uint8Array): number {
  if (left.length !== right.length) return left.length - right.length;
  for (let index = 0; index < left.length; index += 1) { const difference = (left[index] ?? 0) - (right[index] ?? 0); if (difference !== 0) return difference; }
  return 0;
}

function collectionValues(value: CborValue | undefined): CborValue[] {
  if (value === undefined) return [];
  const inner = value.kind === "tag" && value.tag === 258n ? value.value : value;
  if (inner.kind !== "array") throw new TypeError("expected a CBOR collection");
  return [...inner.values];
}

function mapField(value: CborSerializable, key: bigint): CborValue | undefined {
  const decoded = node(value);
  if (decoded.kind !== "map") throw new TypeError("expected a CBOR map");
  return decoded.entries.find(([field]) => field.kind === "unsigned" && field.value === key)?.[1];
}

function outputParts(output: TransactionOutput): {
  address: Address;
  amount: Value;
  datum?: DatumOption;
  scriptRef?: ScriptRef;
} {
  const decoded = node(output);
  if (decoded.kind === "array") {
    if (decoded.values.length < 2 || decoded.values[0]?.kind !== "bytes" || decoded.values[1] === undefined) throw new TypeError("invalid legacy transaction output");
    const result: { address: Address; amount: Value; datum?: DatumOption } = {
      address: Address.from_raw_bytes(decoded.values[0].value),
      amount: Value.from_cbor_bytes(encodeCbor(decoded.values[1])),
    };
    if (decoded.values[2] !== undefined) result.datum = DatumOption.new(0n, encodeCbor(decoded.values[2]));
    return result;
  }
  if (decoded.kind !== "map") throw new TypeError("invalid transaction output");
  const get = (key: bigint) => decoded.entries.find(([field]) => field.kind === "unsigned" && field.value === key)?.[1];
  const address = get(0n), amount = get(1n), datum = get(2n), scriptRef = get(3n);
  if (address?.kind !== "bytes" || amount === undefined) throw new TypeError("invalid Conway transaction output");
  const result: { address: Address; amount: Value; datum?: DatumOption; scriptRef?: ScriptRef } = {
    address: Address.from_raw_bytes(address.value),
    amount: Value.from_cbor_bytes(encodeCbor(amount)),
  };
  if (datum !== undefined) result.datum = DatumOption.from_cbor_bytes(encodeCbor(datum));
  if (scriptRef !== undefined) result.scriptRef = ScriptRef.from_cbor_bytes(encodeCbor(scriptRef));
  return result;
}

function makeOutput(address: Address, amount: Value, datum?: DatumOption, scriptRef?: ScriptRef): TransactionOutput {
  const entries: Array<readonly [CborValue, CborValue]> = [
    [uint(0n), bytes(address.to_raw_bytes())],
    [uint(1n), node(amount)],
  ];
  if (datum !== undefined) entries.push([uint(2n), node(datum)]);
  if (scriptRef !== undefined) entries.push([uint(3n), node(scriptRef)]);
  return TransactionOutput.from_cbor_bytes(encodeCbor(map(entries)));
}

function requiredSignerHashes(value: RequiredSigners): Ed25519KeyHash[] {
  return collectionValues(node(value)).map((item) => {
    if (item.kind !== "bytes") throw new TypeError("required signer must be a key hash");
    return Ed25519KeyHash.from_raw_bytes(item.value);
  });
}

function scriptFromNative(value: NativeScript): Script { return Script.from_cbor_bytes(encodeCbor(array([uint(0n), node(value)]))); }

function scriptParts(value: Script): { kind: number; payload: CborValue } {
  const decoded = node(value);
  if (decoded.kind !== "array" || decoded.values[0]?.kind !== "unsigned" || decoded.values[1] === undefined) throw new TypeError("invalid script");
  return { kind: Number(decoded.values[0].value), payload: decoded.values[1] };
}

export enum ChangeSelectionAlgo { Default = 0 }
export enum CoinSelectionStrategyCIP2 { LargestFirst = 0, RandomImprove = 1, LargestFirstMultiAsset = 2, RandomImproveMultiAsset = 3 }

export class PlutusScript {
  readonly #version: Language;
  readonly #bytes: Uint8Array;
  private constructor(version: Language, value: Uint8Array) { this.#version = version; this.#bytes = copyBytes(value); }
  public static from_v1(script: PlutusV1Script): PlutusScript { return new PlutusScript(Language.PlutusV1, script.to_raw_bytes()); }
  public static from_v2(script: PlutusV2Script): PlutusScript { return new PlutusScript(Language.PlutusV2, script.to_raw_bytes()); }
  public static from_v3(script: PlutusV3Script): PlutusScript { return new PlutusScript(Language.PlutusV3, script.to_raw_bytes()); }
  public as_v1(): PlutusV1Script | undefined { return this.#version === Language.PlutusV1 ? PlutusV1Script.from_raw_bytes(this.#bytes) : undefined; }
  public as_v2(): PlutusV2Script | undefined { return this.#version === Language.PlutusV2 ? PlutusV2Script.from_raw_bytes(this.#bytes) : undefined; }
  public as_v3(): PlutusV3Script | undefined { return this.#version === Language.PlutusV3 ? PlutusV3Script.from_raw_bytes(this.#bytes) : undefined; }
  public version(): Language { return this.#version; }
  public hash(): ScriptHash { return ScriptHash.from_raw_bytes(blake2b224(Uint8Array.from([this.#version + 1, ...this.#bytes]))); }
  public to_script(): Script { return Script.from_cbor_bytes(encodeCbor(array([uint(BigInt(this.#version + 1)), bytes(this.#bytes)]))); }
}

export class PlutusScriptWitness {
  readonly #script: PlutusScript | undefined;
  readonly #hash: ScriptHash;
  private constructor(hash: ScriptHash, script?: PlutusScript) { this.#hash = ScriptHash.from_raw_bytes(hash.to_raw_bytes()); this.#script = script; }
  public static new_ref(hash: ScriptHash): PlutusScriptWitness { return new PlutusScriptWitness(hash); }
  public static new_script(script: PlutusScript): PlutusScriptWitness { return new PlutusScriptWitness(script.hash(), script); }
  public hash(): ScriptHash { return ScriptHash.from_raw_bytes(this.#hash.to_raw_bytes()); }
  public script(): PlutusScript | undefined { return this.#script; }
}

export class PartialPlutusWitness {
  readonly #script: PlutusScriptWitness;
  readonly #data: PlutusData;
  private constructor(script: PlutusScriptWitness, data: PlutusData) { this.#script = script; this.#data = clone(data, PlutusData); }
  public static new(script: PlutusScriptWitness, data: PlutusData): PartialPlutusWitness { return new PartialPlutusWitness(script, data); }
  public script(): PlutusScriptWitness { return this.#script; }
  public data(): PlutusData { return clone(this.#data, PlutusData); }
}

export class NativeScriptWitnessInfo {
  readonly mode: "count" | "vkeys" | "assume";
  readonly count: number;
  readonly vkeyHashes: readonly Ed25519KeyHash[];
  private constructor(mode: "count" | "vkeys" | "assume", count = 0, hashes: readonly Ed25519KeyHash[] = []) {
    if (!Number.isSafeInteger(count) || count < 0) throw new RangeError("signature count must be non-negative");
    this.mode = mode; this.count = count; this.vkeyHashes = hashes.map((hash) => Ed25519KeyHash.from_raw_bytes(hash.to_raw_bytes()));
  }
  public static num_signatures(num: number): NativeScriptWitnessInfo { return new NativeScriptWitnessInfo("count", num); }
  public static vkeys(vkeys: { len(): number; get(index: number): Ed25519KeyHash }): NativeScriptWitnessInfo {
    return new NativeScriptWitnessInfo("vkeys", 0, Array.from({ length: vkeys.len() }, (_, index) => vkeys.get(index)));
  }
  public static assume_signature_count(): NativeScriptWitnessInfo { return new NativeScriptWitnessInfo("assume"); }
}

export class RedeemerWitnessKey {
  readonly #tag: RedeemerTag;
  readonly #index: bigint;
  private constructor(tag: RedeemerTag, index: bigint) { this.#tag = tag; this.#index = checkedCoin(index, "redeemer index"); }
  public static new(tag: RedeemerTag, index: bigint): RedeemerWitnessKey { return new RedeemerWitnessKey(tag, index); }
  public static from_redeemer(redeemer: LegacyRedeemer): RedeemerWitnessKey {
    const decoded = node(redeemer);
    if (decoded.kind !== "array" || decoded.values[0]?.kind !== "unsigned" || decoded.values[1]?.kind !== "unsigned") throw new TypeError("invalid legacy redeemer");
    return new RedeemerWitnessKey(Number(decoded.values[0].value), decoded.values[1].value);
  }
  public tag(): RedeemerTag { return this.#tag; }
  public index(): bigint { return this.#index; }
  public key(): string { return `${this.#tag}:${this.#index}`; }
}

export class UntaggedRedeemer {
  readonly #data: PlutusData;
  #exUnits: ExUnits;
  private constructor(data: PlutusData, exUnits: ExUnits) { this.#data = clone(data, PlutusData); this.#exUnits = clone(exUnits, ExUnits); }
  public static new(data: PlutusData, ex_units: ExUnits): UntaggedRedeemer { return new UntaggedRedeemer(data, ex_units); }
  public data(): PlutusData { return clone(this.#data, PlutusData); }
  public ex_units(): ExUnits { return clone(this.#exUnits, ExUnits); }
  public set_ex_units(value: ExUnits): void { this.#exUnits = clone(value, ExUnits); }
}

type AggregateState =
  | { kind: "native"; script: NativeScript; info: NativeScriptWitnessInfo }
  | { kind: "plutus"; partial: PartialPlutusWitness; signers: Ed25519KeyHash[]; datum?: PlutusData };

export class InputAggregateWitnessData {
  readonly state: AggregateState;
  public constructor(state: AggregateState) { this.state = state; }
  public plutus_data(): PlutusData | undefined { return this.state.kind === "plutus" ? this.state.partial.data() : undefined; }
}

export class RequiredWitnessSet {
  readonly vkeys = new Map<string, Ed25519KeyHash>();
  readonly bootstraps = new Map<string, ByronAddress>();
  readonly scripts = new Map<string, ScriptHash>();
  readonly plutusData = new Map<string, DatumHash>();
  readonly redeemers = new Map<string, RedeemerWitnessKey>();
  readonly scriptRefs = new Map<string, ScriptHash>();
  public static new(): RequiredWitnessSet { return new RequiredWitnessSet(); }
  public add_vkey_key_hash(hash: Ed25519KeyHash): void { this.vkeys.set(hash.to_hex(), Ed25519KeyHash.from_raw_bytes(hash.to_raw_bytes())); }
  public add_bootstrap(address: ByronAddress): void { this.bootstraps.set(address.to_cbor_hex(), ByronAddress.from_cbor_bytes(address.to_cbor_bytes())); }
  public add_script_ref(hash: ScriptHash): void { this.scripts.delete(hash.to_hex()); this.scriptRefs.set(hash.to_hex(), ScriptHash.from_raw_bytes(hash.to_raw_bytes())); }
  public add_script_hash(hash: ScriptHash): void { if (!this.scriptRefs.has(hash.to_hex())) this.scripts.set(hash.to_hex(), ScriptHash.from_raw_bytes(hash.to_raw_bytes())); }
  public add_plutus_datum_hash(hash: DatumHash): void { this.plutusData.set(hash.to_hex(), DatumHash.from_raw_bytes(hash.to_raw_bytes())); }
  public add_redeemer_tag(key: RedeemerWitnessKey): void { this.redeemers.set(key.key(), key); }
  public add_all(other: RequiredWitnessSet): void {
    for (const value of other.vkeys.values()) this.add_vkey_key_hash(value);
    for (const value of other.bootstraps.values()) this.add_bootstrap(value);
    for (const value of other.scripts.values()) this.add_script_hash(value);
    for (const value of other.plutusData.values()) this.add_plutus_datum_hash(value);
    for (const value of other.redeemers.values()) this.add_redeemer_tag(value);
    for (const value of other.scriptRefs.values()) this.add_script_ref(value);
  }
  public withdrawal_required_wits(address: RewardAddress): void { this.addCredential(address.payment()); }
  public addCredential(credential: { as_pub_key(): Ed25519KeyHash | undefined; as_script(): ScriptHash | undefined }): void {
    const key = credential.as_pub_key(), script = credential.as_script();
    if (key !== undefined) this.add_vkey_key_hash(key); else if (script !== undefined) this.add_script_hash(script);
  }
  public clone(): RequiredWitnessSet { const value = RequiredWitnessSet.new(); value.add_all(this); return value; }
  public len(): number { return this.vkeys.size + this.bootstraps.size + this.scripts.size + this.plutusData.size + this.redeemers.size; }
}

function addAggregateRequirements(required: RequiredWitnessSet, aggregate: InputAggregateWitnessData | undefined): void {
  if (aggregate === undefined) return;
  if (aggregate.state.kind === "native") {
    const signers = aggregate.state.script.get_required_signers();
    if (aggregate.state.info.mode === "vkeys") for (const hash of aggregate.state.info.vkeyHashes) required.add_vkey_key_hash(hash);
    else {
      const count = aggregate.state.info.mode === "count" ? aggregate.state.info.count : signers.len();
      for (let index = 0; index < Math.min(count, signers.len()); index += 1) required.add_vkey_key_hash(signers.get(index));
    }
    return;
  }
  for (const hash of aggregate.state.signers) required.add_vkey_key_hash(hash);
}

function scriptHashFromAddress(address: Address): ScriptHash | undefined { return address.payment_cred()?.as_script(); }
function keyHashFromAddress(address: Address): Ed25519KeyHash | undefined { return address.payment_cred()?.as_pub_key(); }

function requirementsForOutput(output: TransactionOutput): RequiredWitnessSet {
  const requirements = RequiredWitnessSet.new();
  const { address, datum } = outputParts(output);
  if (address.kind() === AddressKind.Byron) {
    const byron = ByronAddress.from_address(address);
    if (byron !== undefined) requirements.add_bootstrap(byron);
  } else {
    const key = keyHashFromAddress(address), script = scriptHashFromAddress(address);
    if (key !== undefined) requirements.add_vkey_key_hash(key);
    if (script !== undefined) requirements.add_script_hash(script);
  }
  if (datum !== undefined) {
    const decoded = node(datum);
    if (decoded.kind === "array" && decoded.values[0]?.kind === "unsigned" && decoded.values[0].value === 0n && decoded.values[1]?.kind === "bytes") requirements.add_plutus_datum_hash(DatumHash.from_raw_bytes(decoded.values[1].value));
  }
  return requirements;
}

export class InputBuilderResult {
  public constructor(
    readonly inputValue: TransactionInput,
    readonly outputValue: TransactionOutput,
    readonly aggregate: InputAggregateWitnessData | undefined,
    readonly required: RequiredWitnessSet,
  ) {}
}

export class TransactionUnspentOutput {
  readonly #input: TransactionInput;
  readonly #output: TransactionOutput;
  private constructor(input: TransactionInput, output: TransactionOutput) { this.#input = clone(input, TransactionInput); this.#output = clone(output, TransactionOutput); }
  public static new(input: TransactionInput, output: TransactionOutput): TransactionUnspentOutput { return new TransactionUnspentOutput(input, output); }
  public static from_cbor_bytes(value: Uint8Array): TransactionUnspentOutput {
    const decoded = decodeCbor(value);
    if (decoded.kind !== "array" || decoded.values.length !== 2 || decoded.values[0] === undefined || decoded.values[1] === undefined) throw new TypeError("invalid transaction unspent output");
    return new TransactionUnspentOutput(TransactionInput.from_cbor_bytes(encodeCbor(decoded.values[0])), TransactionOutput.from_cbor_bytes(encodeCbor(decoded.values[1])));
  }
  public static from_cbor_hex(value: string): TransactionUnspentOutput { return TransactionUnspentOutput.from_cbor_bytes(hexToBytes(value)); }
  public input(): TransactionInput { return clone(this.#input, TransactionInput); }
  public output(): TransactionOutput { return clone(this.#output, TransactionOutput); }
  public to_cbor_bytes(): Uint8Array { return encodeCbor(array([node(this.#input), node(this.#output)])); }
  public to_cbor_hex(): string { return bytesToHex(this.to_cbor_bytes()); }
}

export class SingleInputBuilder {
  readonly #input: TransactionInput;
  readonly #output: TransactionOutput;
  private constructor(input: TransactionInput, output: TransactionOutput) { this.#input = clone(input, TransactionInput); this.#output = clone(output, TransactionOutput); }
  public static new(input: TransactionInput, utxo_info: TransactionOutput): SingleInputBuilder { return new SingleInputBuilder(input, utxo_info); }
  public static from_transaction_unspent_output(utxo: TransactionUnspentOutput): SingleInputBuilder { return new SingleInputBuilder(utxo.input(), utxo.output()); }
  public payment_key(): InputBuilderResult {
    const required = requirementsForOutput(this.#output);
    if (required.scripts.size > 0) throw new TypeError("UTXO address is not a payment key");
    return new InputBuilderResult(this.#input, this.#output, undefined, required);
  }
  public native_script(script: NativeScript, info: NativeScriptWitnessInfo): InputBuilderResult {
    const required = requirementsForOutput(this.#output);
    if (!required.scripts.delete(script.hash().to_hex()) || required.scripts.size > 0) throw new TypeError("native script does not satisfy the input");
    required.add_script_hash(script.hash());
    return new InputBuilderResult(this.#input, this.#output, new InputAggregateWitnessData({ kind: "native", script, info }), required);
  }
  public plutus_script(partial: PartialPlutusWitness, signers: RequiredSigners, datum: PlutusData): InputBuilderResult { return this.plutus(partial, signers, datum); }
  public plutus_script_inline_datum(partial: PartialPlutusWitness, signers: RequiredSigners): InputBuilderResult { return this.plutus(partial, signers); }
  private plutus(partial: PartialPlutusWitness, signers: RequiredSigners, datum?: PlutusData): InputBuilderResult {
    const required = requirementsForOutput(this.#output), hash = partial.script().hash();
    if (!required.scripts.delete(hash.to_hex()) || required.scripts.size > 0) throw new TypeError("Plutus script does not satisfy the input");
    let datumHash: DatumHash | undefined;
    if (datum !== undefined) {
      datumHash = hash_plutus_data(datum);
      if (!required.plutusData.delete(datumHash.to_hex())) throw new TypeError("Plutus datum does not satisfy the input");
    }
    if (required.plutusData.size > 0) throw new TypeError("Plutus datum does not satisfy the input");
    required.add_script_hash(hash);
    if (datumHash !== undefined) required.add_plutus_datum_hash(datumHash);
    const hashes = requiredSignerHashes(signers); for (const value of hashes) required.add_vkey_key_hash(value);
    const state: AggregateState = datum === undefined ? { kind: "plutus", partial, signers: hashes } : { kind: "plutus", partial, signers: hashes, datum };
    return new InputBuilderResult(this.#input, this.#output, new InputAggregateWitnessData(state), required);
  }
}

export class SingleOutputBuilderResult {
  readonly #output: TransactionOutput;
  readonly #communicationDatum: PlutusData | undefined;
  public constructor(output: TransactionOutput, communicationDatum?: PlutusData) { this.#output = clone(output, TransactionOutput); this.#communicationDatum = communicationDatum === undefined ? undefined : clone(communicationDatum, PlutusData); }
  public static new(output: TransactionOutput): SingleOutputBuilderResult { return new SingleOutputBuilderResult(output); }
  public output(): TransactionOutput { return clone(this.#output, TransactionOutput); }
  public communication_datum(): PlutusData | undefined { return this.#communicationDatum === undefined ? undefined : clone(this.#communicationDatum, PlutusData); }
}

export class TransactionOutputBuilder {
  #address: Address | undefined;
  #datum: DatumOption | undefined;
  #communicationDatum: PlutusData | undefined;
  #scriptRef: ScriptRef | undefined;
  public static new(): TransactionOutputBuilder { return new TransactionOutputBuilder(); }
  public with_address(value: Address): TransactionOutputBuilder { this.#address = Address.from_raw_bytes(value.to_raw_bytes()); return this; }
  public with_communication_data(value: PlutusData): TransactionOutputBuilder {
    this.#communicationDatum = clone(value, PlutusData);
    this.#datum = DatumOption.from_cbor_bytes(encodeCbor(array([uint(0n), bytes(hash_plutus_data(value).to_raw_bytes())])));
    return this;
  }
  public with_data(value: DatumOption): TransactionOutputBuilder { this.#datum = clone(value, DatumOption); this.#communicationDatum = undefined; return this; }
  public with_reference_script(value: ScriptRef): TransactionOutputBuilder { this.#scriptRef = clone(value, ScriptRef); return this; }
  public next(): TransactionOutputAmountBuilder {
    if (this.#address === undefined) throw new TypeError("output address is missing");
    return new TransactionOutputAmountBuilder(this.#address, this.#datum, this.#scriptRef, this.#communicationDatum);
  }
}

export class TransactionOutputAmountBuilder {
  #amount: Value | undefined;
  public constructor(
    readonly addressValue: Address,
    readonly datumValue?: DatumOption,
    readonly scriptRefValue?: ScriptRef,
    readonly communicationDatumValue?: PlutusData,
  ) {}
  public with_value(value: Value): TransactionOutputAmountBuilder { this.#amount = clone(value, Value); return this; }
  public with_asset_and_min_required_coin(assets: MultiAsset, coinsPerByte: bigint): TransactionOutputAmountBuilder {
    const zero = makeOutput(this.addressValue, Value.new(0n, assets), this.datumValue, this.scriptRefValue);
    const coin = min_ada_required(zero, coinsPerByte);
    this.#amount = Value.new(min_ada_required(makeOutput(this.addressValue, Value.new(coin, assets), this.datumValue, this.scriptRefValue), coinsPerByte), assets);
    return this;
  }
  public build(): SingleOutputBuilderResult {
    if (this.#amount === undefined) throw new TypeError("output amount is missing");
    return new SingleOutputBuilderResult(makeOutput(this.addressValue, this.#amount, this.datumValue, this.scriptRefValue), this.communicationDatumValue);
  }
}

export class MintBuilderResult {
  public constructor(
    readonly policyId: ScriptHash,
    readonly assets: MapAssetNameToNonZeroInt64,
    readonly aggregate: InputAggregateWitnessData,
    readonly required: RequiredWitnessSet,
  ) {}
}

function copyMintAssets(value: MapAssetNameToNonZeroInt64): MapAssetNameToNonZeroInt64 {
  const output = MapAssetNameToNonZeroInt64.new();
  for (const asset of value.keys()) output.insert(asset, value.get(asset) ?? 0n);
  return output;
}

export class SingleMintBuilder {
  readonly #assets: MapAssetNameToNonZeroInt64;
  private constructor(assets: MapAssetNameToNonZeroInt64) {
    this.#assets = copyMintAssets(assets);
    if (this.#assets.len() === 0) throw new TypeError("mint assets must not be empty");
    for (const asset of this.#assets.keys()) if ((this.#assets.get(asset) ?? 0n) === 0n) throw new RangeError("mint quantity must be non-zero");
  }
  public static new(assets: MapAssetNameToNonZeroInt64): SingleMintBuilder { return new SingleMintBuilder(assets); }
  public static new_single_asset(asset: AssetName, amount: bigint): SingleMintBuilder {
    const values = MapAssetNameToNonZeroInt64.new(); values.insert(asset, amount); return new SingleMintBuilder(values);
  }
  public native_script(script: NativeScript, info: NativeScriptWitnessInfo): MintBuilderResult {
    const required = RequiredWitnessSet.new(); required.add_script_hash(script.hash());
    return new MintBuilderResult(script.hash(), copyMintAssets(this.#assets), new InputAggregateWitnessData({ kind: "native", script, info }), required);
  }
  public plutus_script(partial: PartialPlutusWitness, signers: RequiredSigners): MintBuilderResult {
    const required = RequiredWitnessSet.new(), hashes = requiredSignerHashes(signers), hash = partial.script().hash();
    required.add_script_hash(hash); for (const signer of hashes) required.add_vkey_key_hash(signer);
    return new MintBuilderResult(hash, copyMintAssets(this.#assets), new InputAggregateWitnessData({ kind: "plutus", partial, signers: hashes }), required);
  }
}

export class WithdrawalBuilderResult {
  public constructor(
    readonly address: RewardAddress,
    readonly amount: bigint,
    readonly aggregate: InputAggregateWitnessData | undefined,
    readonly required: RequiredWitnessSet,
  ) {}
}

export class SingleWithdrawalBuilder {
  readonly #address: RewardAddress;
  readonly #amount: bigint;
  private constructor(address: RewardAddress, amount: bigint) { this.#address = RewardAddress.from_address(address.to_address()) as RewardAddress; this.#amount = checkedCoin(amount); }
  public static new(address: RewardAddress, amount: bigint): SingleWithdrawalBuilder { return new SingleWithdrawalBuilder(address, amount); }
  private requirements(): RequiredWitnessSet { const value = RequiredWitnessSet.new(); value.withdrawal_required_wits(this.#address); return value; }
  public payment_key(): WithdrawalBuilderResult {
    const required = this.requirements(); if (required.scripts.size > 0) throw new TypeError("withdrawal requires a script");
    return new WithdrawalBuilderResult(this.#address, this.#amount, undefined, required);
  }
  public native_script(script: NativeScript, info: NativeScriptWitnessInfo): WithdrawalBuilderResult {
    const required = this.requirements(); if (!required.scripts.has(script.hash().to_hex())) throw new TypeError("native script does not satisfy withdrawal");
    return new WithdrawalBuilderResult(this.#address, this.#amount, new InputAggregateWitnessData({ kind: "native", script, info }), required);
  }
  public plutus_script(partial: PartialPlutusWitness, signers: RequiredSigners): WithdrawalBuilderResult {
    const required = this.requirements(), hashes = requiredSignerHashes(signers);
    if (!required.scripts.has(partial.script().hash().to_hex())) throw new TypeError("Plutus script does not satisfy withdrawal");
    for (const hash of hashes) required.add_vkey_key_hash(hash);
    return new WithdrawalBuilderResult(this.#address, this.#amount, new InputAggregateWitnessData({ kind: "plutus", partial, signers: hashes }), required);
  }
}

function certificateCredential(value: Certificate): { as_pub_key(): Ed25519KeyHash | undefined; as_script(): ScriptHash | undefined } | undefined {
  const decoded = node(value);
  if (decoded.kind !== "array" || decoded.values[0]?.kind !== "unsigned") throw new TypeError("invalid certificate");
  const tag = Number(decoded.values[0].value);
  if (tag === 0) return undefined;
  const position = new Map<number, number>([[1,1],[2,1],[5,1],[6,1],[7,1],[8,1],[9,1],[10,1],[11,1],[12,1],[13,1],[14,1],[15,1],[16,1]]).get(tag);
  const credential = position === undefined ? undefined : decoded.values[position];
  if (credential?.kind !== "array" || credential.values[0]?.kind !== "unsigned" || credential.values[1]?.kind !== "bytes") return undefined;
  const hashBytes = copyBytes(credential.values[1].value);
  return credential.values[0].value === 0n
    ? { as_pub_key: () => Ed25519KeyHash.from_raw_bytes(hashBytes), as_script: () => undefined }
    : { as_pub_key: () => undefined, as_script: () => ScriptHash.from_raw_bytes(hashBytes) };
}

function certificateRequirements(value: Certificate): RequiredWitnessSet {
  const required = RequiredWitnessSet.new(), credential = certificateCredential(value);
  if (credential !== undefined) required.addCredential(credential);
  const decoded = node(value);
  if (decoded.kind === "array" && decoded.values[0]?.kind === "unsigned") {
    if (decoded.values[0].value === 4n && decoded.values[1]?.kind === "bytes") required.add_vkey_key_hash(Ed25519KeyHash.from_raw_bytes(decoded.values[1].value));
    if (decoded.values[0].value === 3n && decoded.values[1]?.kind === "array") {
      const pool = decoded.values[1];
      if (pool.values[0]?.kind === "bytes") required.add_vkey_key_hash(Ed25519KeyHash.from_raw_bytes(pool.values[0].value));
      const owners = pool.values[6];
      for (const owner of collectionValues(owners)) if (owner.kind === "bytes") required.add_vkey_key_hash(Ed25519KeyHash.from_raw_bytes(owner.value));
    }
  }
  return required;
}

export class CertificateBuilderResult {
  public constructor(readonly certificate: Certificate, readonly aggregate: InputAggregateWitnessData | undefined, readonly required: RequiredWitnessSet) {}
}

export class SingleCertificateBuilder {
  readonly #certificate: Certificate;
  private constructor(value: Certificate) { this.#certificate = clone(value, Certificate); }
  public static new(value: Certificate): SingleCertificateBuilder { return new SingleCertificateBuilder(value); }
  public skip_witness(): CertificateBuilderResult { return new CertificateBuilderResult(this.#certificate, undefined, certificateRequirements(this.#certificate)); }
  public payment_key(): CertificateBuilderResult {
    const required = certificateRequirements(this.#certificate); if (required.scripts.size > 0) throw new TypeError("certificate requires a script");
    return new CertificateBuilderResult(this.#certificate, undefined, required);
  }
  public native_script(script: NativeScript, info: NativeScriptWitnessInfo): CertificateBuilderResult {
    const required = certificateRequirements(this.#certificate), expected = required.scripts.has(script.hash().to_hex());
    if (required.scripts.size > 0 && !expected) throw new TypeError("native script does not satisfy certificate");
    return new CertificateBuilderResult(this.#certificate, expected ? new InputAggregateWitnessData({ kind: "native", script, info }) : undefined, required);
  }
  public plutus_script(partial: PartialPlutusWitness, signers: RequiredSigners): CertificateBuilderResult {
    const required = certificateRequirements(this.#certificate), expected = required.scripts.has(partial.script().hash().to_hex()), hashes = requiredSignerHashes(signers);
    if (required.scripts.size > 0 && !expected) throw new TypeError("Plutus script does not satisfy certificate");
    for (const hash of hashes) required.add_vkey_key_hash(hash);
    return new CertificateBuilderResult(this.#certificate, expected ? new InputAggregateWitnessData({ kind: "plutus", partial, signers: hashes }) : undefined, required);
  }
}

type ProposalEntry = { proposal: ProposalProcedure; aggregate?: InputAggregateWitnessData };
export class ProposalBuilderResult {
  public constructor(readonly entries: readonly ProposalEntry[]) {}
}

export class ProposalBuilder {
  readonly #entries: ProposalEntry[] = [];
  public static new(): ProposalBuilder { return new ProposalBuilder(); }
  public with_proposal(proposal: ProposalProcedure): ProposalBuilder { this.#entries.push({ proposal: clone(proposal, ProposalProcedure) }); return this; }
  public with_native_script_proposal(proposal: ProposalProcedure, script: NativeScript, info: NativeScriptWitnessInfo): ProposalBuilder {
    this.#entries.push({ proposal: clone(proposal, ProposalProcedure), aggregate: new InputAggregateWitnessData({ kind: "native", script, info }) }); return this;
  }
  public with_plutus_proposal(proposal: ProposalProcedure, partial: PartialPlutusWitness, signers: RequiredSigners, datum: PlutusData): ProposalBuilder {
    this.#entries.push({ proposal: clone(proposal, ProposalProcedure), aggregate: new InputAggregateWitnessData({ kind: "plutus", partial, signers: requiredSignerHashes(signers), datum }) }); return this;
  }
  public with_plutus_proposal_inline_datum(proposal: ProposalProcedure, partial: PartialPlutusWitness, signers: RequiredSigners): ProposalBuilder {
    this.#entries.push({ proposal: clone(proposal, ProposalProcedure), aggregate: new InputAggregateWitnessData({ kind: "plutus", partial, signers: requiredSignerHashes(signers) }) }); return this;
  }
  public build(): ProposalBuilderResult { return new ProposalBuilderResult([...this.#entries]); }
}

type VoteEntry = { voter: Voter; action: GovActionId; procedure: VotingProcedure; aggregate?: InputAggregateWitnessData };
export class VoteBuilderResult {
  public constructor(readonly entries: readonly VoteEntry[]) {}
}

export class VoteBuilder {
  readonly #entries = new Map<string, VoteEntry>();
  public static new(): VoteBuilder { return new VoteBuilder(); }
  private add(entry: VoteEntry): VoteBuilder { this.#entries.set(`${canonicalHex(entry.voter)}:${canonicalHex(entry.action)}`, entry); return this; }
  public with_vote(voter: Voter, action: GovActionId, procedure: VotingProcedure): VoteBuilder {
    return this.add({ voter: clone(voter, Voter), action: clone(action, GovActionId), procedure: clone(procedure, VotingProcedure) });
  }
  public with_native_script_vote(voter: Voter, action: GovActionId, procedure: VotingProcedure, script: NativeScript, info: NativeScriptWitnessInfo): VoteBuilder {
    return this.add({ voter, action, procedure, aggregate: new InputAggregateWitnessData({ kind: "native", script, info }) });
  }
  public with_plutus_vote(voter: Voter, action: GovActionId, procedure: VotingProcedure, partial: PartialPlutusWitness, signers: RequiredSigners, datum: PlutusData): VoteBuilder {
    return this.add({ voter, action, procedure, aggregate: new InputAggregateWitnessData({ kind: "plutus", partial, signers: requiredSignerHashes(signers), datum }) });
  }
  public with_plutus_vote_inline_datum(voter: Voter, action: GovActionId, procedure: VotingProcedure, partial: PartialPlutusWitness, signers: RequiredSigners): VoteBuilder {
    return this.add({ voter, action, procedure, aggregate: new InputAggregateWitnessData({ kind: "plutus", partial, signers: requiredSignerHashes(signers) }) });
  }
  public build(): VoteBuilderResult { return new VoteBuilderResult([...this.#entries.values()]); }
}

interface RedeemerSource { readonly tag: RedeemerTag; readonly sortKey: string; readonly aggregate: InputAggregateWitnessData }

export class RedeemerSetBuilder {
  readonly #sources: RedeemerSource[] = [];
  readonly #overrides = new Map<string, ExUnits>();
  public static new(): RedeemerSetBuilder { return new RedeemerSetBuilder(); }
  private add(tag: RedeemerTag, sortKey: string, aggregate: InputAggregateWitnessData | undefined): void { if (aggregate?.state.kind === "plutus") this.#sources.push({ tag, sortKey, aggregate }); }
  public add_spend(result: InputBuilderResult): void { this.add(RedeemerTag.Spend, canonicalHex(result.inputValue), result.aggregate); }
  public add_mint(result: MintBuilderResult): void { this.add(RedeemerTag.Mint, result.policyId.to_hex(), result.aggregate); }
  public add_reward(result: WithdrawalBuilderResult): void { this.add(RedeemerTag.Reward, bytesToHex(result.address.to_address().to_raw_bytes()), result.aggregate); }
  public add_cert(result: CertificateBuilderResult): void { this.add(RedeemerTag.Cert, String(this.#sources.filter((value) => value.tag === RedeemerTag.Cert).length).padStart(10, "0"), result.aggregate); }
  public add_proposal(result: ProposalBuilderResult): void { for (const [index, entry] of result.entries.entries()) this.add(RedeemerTag.Proposing, String(index).padStart(10, "0"), entry.aggregate); }
  public add_vote(result: VoteBuilderResult): void { for (const entry of result.entries) this.add(RedeemerTag.Voting, `${canonicalHex(entry.voter)}:${canonicalHex(entry.action)}`, entry.aggregate); }
  public is_empty(): boolean { return this.#sources.length === 0; }
  public update_ex_units(key: RedeemerWitnessKey, exUnits: ExUnits): void { this.#overrides.set(key.key(), clone(exUnits, ExUnits)); }
  public build(defaultToDummyExUnits: boolean): LegacyRedeemerList {
    const output = LegacyRedeemerList.new();
    for (const tag of [RedeemerTag.Spend, RedeemerTag.Mint, RedeemerTag.Cert, RedeemerTag.Reward, RedeemerTag.Voting, RedeemerTag.Proposing]) {
      const values = this.#sources.filter((source) => source.tag === tag).sort((left, right) => left.sortKey.localeCompare(right.sortKey));
      for (const [index, source] of values.entries()) {
        if (source.aggregate.state.kind !== "plutus") throw new TypeError("redeemer source must be Plutus");
        const key = RedeemerWitnessKey.new(tag, BigInt(index));
        const exUnits = this.#overrides.get(key.key()) ?? (defaultToDummyExUnits ? ExUnits.new(0n, 0n) : undefined);
        if (exUnits === undefined) throw new TypeError(`missing execution units for redeemer ${key.key()}`);
        output.add(LegacyRedeemer.from_cbor_bytes(encodeCbor(array([uint(BigInt(tag)), uint(BigInt(index)), node(source.aggregate.state.partial.data()), node(exUnits)]))));
      }
    }
    return output;
  }
}

function listFromField(decoded: CborValue, key: bigint): CborValue[] {
  if (decoded.kind !== "map") throw new TypeError("witness set must be a map");
  return collectionValues(decoded.entries.find(([field]) => field.kind === "unsigned" && field.value === key)?.[1]);
}

function vkeyHash(value: Vkeywitness): Ed25519KeyHash {
  const decoded = node(value);
  if (decoded.kind !== "array" || decoded.values[0]?.kind !== "bytes") throw new TypeError("invalid vkey witness");
  return PublicKey.from_bytes(decoded.values[0].value).hash();
}

function redeemerKey(value: LegacyRedeemer): string { return RedeemerWitnessKey.from_redeemer(value).key(); }

export class TransactionWitnessSetBuilder {
  readonly #vkeys = new Map<string, Vkeywitness>();
  readonly #bootstraps = new Map<string, BootstrapWitness>();
  readonly #native = new Map<string, NativeScript>();
  readonly #plutusV1 = new Map<string, PlutusV1Script>();
  readonly #plutusV2 = new Map<string, PlutusV2Script>();
  readonly #plutusV3 = new Map<string, PlutusV3Script>();
  readonly #datums = new Map<string, PlutusData>();
  readonly #redeemers = new Map<string, LegacyRedeemer>();
  readonly #required = RequiredWitnessSet.new();
  public static new(): TransactionWitnessSetBuilder { return new TransactionWitnessSetBuilder(); }
  public add_vkey(value: Vkeywitness): void { this.#vkeys.set(vkeyHash(value).to_hex(), clone(value, Vkeywitness)); }
  public add_bootstrap(value: BootstrapWitness): void { this.#bootstraps.set(bytesToHex(value.public_key().to_raw_bytes()), clone(value, BootstrapWitness)); }
  public add_script(value: Script): void {
    const { kind, payload } = scriptParts(value);
    if (kind === 0) { const script = NativeScript.from_cbor_bytes(encodeCbor(payload)); this.#native.set(script.hash().to_hex(), script); return; }
    if (payload.kind !== "bytes") throw new TypeError("invalid Plutus script payload");
    if (kind === 1) { const script = PlutusV1Script.from_raw_bytes(payload.value); this.#plutusV1.set(PlutusScript.from_v1(script).hash().to_hex(), script); return; }
    if (kind === 2) { const script = PlutusV2Script.from_raw_bytes(payload.value); this.#plutusV2.set(PlutusScript.from_v2(script).hash().to_hex(), script); return; }
    if (kind === 3) { const script = PlutusV3Script.from_raw_bytes(payload.value); this.#plutusV3.set(PlutusScript.from_v3(script).hash().to_hex(), script); return; }
    throw new RangeError("unknown script language");
  }
  public add_plutus_datum(value: PlutusData): void { this.#datums.set(hash_plutus_data(value).to_hex(), clone(value, PlutusData)); }
  public add_redeemer(value: LegacyRedeemer): void { this.#redeemers.set(redeemerKey(value), clone(value, LegacyRedeemer)); }
  public add_required_wits(value: RequiredWitnessSet): void { this.#required.add_all(value); }
  public get_native_script(): NativeScriptList {
    const output = NativeScriptList.new(); for (const value of this.#native.values()) output.add(value); return output;
  }
  public get_plutus_v1_script(): PlutusV1ScriptList {
    const output = PlutusV1ScriptList.new(); for (const value of this.#plutusV1.values()) output.add(value); return output;
  }
  public get_plutus_v2_script(): PlutusV2ScriptList {
    const output = PlutusV2ScriptList.new(); for (const value of this.#plutusV2.values()) output.add(value); return output;
  }
  public get_plutus_v3_script(): PlutusV3ScriptList {
    const output = PlutusV3ScriptList.new(); for (const value of this.#plutusV3.values()) output.add(value); return output;
  }
  public get_plutus_datum(): PlutusDataList {
    const output = PlutusDataList.new(); for (const value of this.#datums.values()) output.add(value); return output;
  }
  public get_redeemer(): LegacyRedeemerList { const output = LegacyRedeemerList.new(); for (const value of this.#redeemers.values()) output.add(value); return output; }
  public add_existing(value: TransactionWitnessSet): void {
    const decoded = node(value);
    for (const item of listFromField(decoded, 0n)) this.add_vkey(Vkeywitness.from_cbor_bytes(encodeCbor(item)));
    for (const item of listFromField(decoded, 1n)) this.add_script(scriptFromNative(NativeScript.from_cbor_bytes(encodeCbor(item))));
    for (const item of listFromField(decoded, 2n)) this.add_bootstrap(BootstrapWitness.from_cbor_bytes(encodeCbor(item)));
    for (const item of listFromField(decoded, 3n)) this.add_script(PlutusScript.from_v1(PlutusV1Script.from_cbor_bytes(encodeCbor(item))).to_script());
    for (const item of listFromField(decoded, 4n)) this.add_plutus_datum(PlutusData.from_cbor_bytes(encodeCbor(item)));
    const redeemers = decoded.kind === "map" ? decoded.entries.find(([field]) => field.kind === "unsigned" && field.value === 5n)?.[1] : undefined;
    if (redeemers?.kind === "array") for (const item of redeemers.values) this.add_redeemer(LegacyRedeemer.from_cbor_bytes(encodeCbor(item)));
    for (const item of listFromField(decoded, 6n)) this.add_script(PlutusScript.from_v2(PlutusV2Script.from_cbor_bytes(encodeCbor(item))).to_script());
    for (const item of listFromField(decoded, 7n)) this.add_script(PlutusScript.from_v3(PlutusV3Script.from_cbor_bytes(encodeCbor(item))).to_script());
  }
  public remaining_wits(): RequiredWitnessSet {
    const remaining = this.#required.clone();
    for (const key of this.#vkeys.keys()) remaining.vkeys.delete(key);
    for (const witness of this.#bootstraps.values()) remaining.bootstraps.delete(witness.to_address().to_address().to_cbor_hex());
    for (const key of this.#native.keys()) remaining.scripts.delete(key);
    for (const key of this.#plutusV1.keys()) remaining.scripts.delete(key);
    for (const key of this.#plutusV2.keys()) remaining.scripts.delete(key);
    for (const key of this.#plutusV3.keys()) remaining.scripts.delete(key);
    for (const key of this.#datums.keys()) remaining.plutusData.delete(key);
    for (const key of this.#redeemers.keys()) remaining.redeemers.delete(key);
    return remaining;
  }
  public merge_fake_witness(value: RequiredWitnessSet): void {
    this.add_required_wits(value);
    let id = 0;
    for (const key of value.vkeys.keys()) {
      if (this.#vkeys.has(key)) continue;
      const publicKey = new Uint8Array(32); publicKey[0] = id++ & 255;
      this.#vkeys.set(key, Vkeywitness.from_cbor_bytes(encodeCbor(array([bytes(publicKey), bytes(new Uint8Array(64))]))));
    }
  }
  public build(): TransactionWitnessSet {
    const entries: Array<readonly [CborValue, CborValue]> = [];
    const add = (key: bigint, values: readonly CborSerializable[]) => { if (values.length > 0) entries.push([uint(key), collectionNode(values)]); };
    add(0n, [...this.#vkeys.values()]); add(1n, [...this.#native.values()]); add(2n, [...this.#bootstraps.values()]);
    add(3n, [...this.#plutusV1.values()]); add(4n, [...this.#datums.values()]);
    if (this.#redeemers.size > 0) entries.push([uint(5n), array([...this.#redeemers.values()].map(node))]);
    add(6n, [...this.#plutusV2.values()]); add(7n, [...this.#plutusV3.values()]);
    return TransactionWitnessSet.from_cbor_bytes(encodeCbor(map(entries)));
  }
  public try_build(): TransactionWitnessSet {
    const remaining = this.remaining_wits(); if (remaining.len() > 0) throw new TypeError(`missing ${remaining.len()} required witnesses`); return this.build();
  }
  public copy(): TransactionWitnessSetBuilder { const output = TransactionWitnessSetBuilder.new(); output.add_existing(this.build()); output.add_required_wits(this.#required); return output; }
}

export class TransactionBuilderConfig {
  public constructor(
    readonly feeAlgo: LinearFee,
    readonly poolDeposit: bigint,
    readonly keyDeposit: bigint,
    readonly maxValueSize: number,
    readonly maxTxSize: number,
    readonly coinsPerUtxoByte: bigint,
    readonly exUnitPrices: ExUnitPrices,
    readonly costModels: CostModels,
    readonly collateralPercentage: number,
    readonly maxCollateralInputs: number,
    readonly preferPureChange: boolean,
  ) {}
}

export class TransactionBuilderConfigBuilder {
  #feeAlgo: LinearFee | undefined;
  #poolDeposit: bigint | undefined;
  #keyDeposit: bigint | undefined;
  #maxValueSize: number | undefined;
  #maxTxSize: number | undefined;
  #coinsPerUtxoByte: bigint | undefined;
  #exUnitPrices: ExUnitPrices | undefined;
  #costModels: CostModels | undefined;
  #collateralPercentage: number | undefined;
  #maxCollateralInputs: number | undefined;
  #preferPureChange = false;
  public static new(): TransactionBuilderConfigBuilder { return new TransactionBuilderConfigBuilder(); }
  public fee_algo(value: LinearFee): TransactionBuilderConfigBuilder { this.#feeAlgo = value; return this; }
  public pool_deposit(value: bigint): TransactionBuilderConfigBuilder { this.#poolDeposit = checkedCoin(value); return this; }
  public key_deposit(value: bigint): TransactionBuilderConfigBuilder { this.#keyDeposit = checkedCoin(value); return this; }
  public max_value_size(value: number): TransactionBuilderConfigBuilder { this.#maxValueSize = checkedUint32(value, "maximum value size"); return this; }
  public max_tx_size(value: number): TransactionBuilderConfigBuilder { this.#maxTxSize = checkedUint32(value, "maximum transaction size"); return this; }
  public coins_per_utxo_byte(value: bigint): TransactionBuilderConfigBuilder { this.#coinsPerUtxoByte = checkedCoin(value); return this; }
  public ex_unit_prices(value: ExUnitPrices): TransactionBuilderConfigBuilder { this.#exUnitPrices = clone(value, ExUnitPrices); return this; }
  public cost_models(value: CostModels): TransactionBuilderConfigBuilder { this.#costModels = clone(value, CostModels); return this; }
  public collateral_percentage(value: number): TransactionBuilderConfigBuilder { this.#collateralPercentage = checkedUint32(value, "collateral percentage"); return this; }
  public max_collateral_inputs(value: number): TransactionBuilderConfigBuilder { this.#maxCollateralInputs = checkedUint32(value, "maximum collateral inputs"); return this; }
  public prefer_pure_change(value: boolean): TransactionBuilderConfigBuilder { this.#preferPureChange = value; return this; }
  public build(): TransactionBuilderConfig {
    const missing: string[] = [];
    if (this.#feeAlgo === undefined) missing.push("fee_algo");
    if (this.#poolDeposit === undefined) missing.push("pool_deposit");
    if (this.#keyDeposit === undefined) missing.push("key_deposit");
    if (this.#maxValueSize === undefined) missing.push("max_value_size");
    if (this.#maxTxSize === undefined) missing.push("max_tx_size");
    if (this.#coinsPerUtxoByte === undefined) missing.push("coins_per_utxo_byte");
    if (this.#exUnitPrices === undefined) missing.push("ex_unit_prices");
    if (this.#collateralPercentage === undefined) missing.push("collateral_percentage");
    if (this.#maxCollateralInputs === undefined) missing.push("max_collateral_inputs");
    if (missing.length > 0) throw new TypeError(`uninitialized transaction builder field: ${missing[0]}`);
    const emptyModels = CostModels.from_cbor_bytes(encodeCbor(map([])));
    return new TransactionBuilderConfig(this.#feeAlgo!, this.#poolDeposit!, this.#keyDeposit!, this.#maxValueSize!, this.#maxTxSize!, this.#coinsPerUtxoByte!, this.#exUnitPrices!, this.#costModels ?? emptyModels, this.#collateralPercentage!, this.#maxCollateralInputs!, this.#preferPureChange);
  }
}

function checkedUint32(value: number, name: string): number {
  if (!Number.isSafeInteger(value) || value < 0 || value > 0xffff_ffff) throw new RangeError(`${name} must fit uint32`);
  return value;
}

function cloneMint(value: Mint): Mint {
  const output = Mint.new();
  for (const policy of value.keys()) {
    const assets = MapAssetNameToNonZeroInt64.new();
    for (const asset of value.get(policy)?.keys() ?? []) assets.insert(asset, value.get_value(policy, asset));
    output.insert(policy, assets);
  }
  return output;
}

function mintNode(value: Mint): CborValue {
  return map(value.keys().sort((left, right) => left.to_hex().localeCompare(right.to_hex())).map((policy) => [
    bytes(policy.to_raw_bytes()),
    map((value.get(policy)?.keys() ?? []).sort((left, right) => left.to_hex().localeCompare(right.to_hex())).map((asset) => {
      const quantity = value.get_value(policy, asset);
      return [bytes(asset.to_raw_bytes()), quantity >= 0n ? uint(quantity) : { kind: "negative", value: quantity, encoding: { width: 0 } }];
    })),
  ]));
}

function cleanMultiAsset(value: MultiAsset): MultiAsset | undefined {
  const output = MultiAsset.new();
  for (const policy of value.keys()) for (const asset of value.get_assets(policy)?.keys() ?? []) {
    const amount = value.get_value(policy, asset); if (amount > 0n) output.insert(policy, asset, amount);
  }
  return output.len() === 0 ? undefined : output;
}

function addValues(left: Value, right: Value, label: string): Value {
  const value = left.checked_add(right); if (value === undefined) throw new RangeError(`${label} overflow`); return value;
}

function subtractValues(left: Value, right: Value, label: string): Value {
  const value = left.checked_sub(right); if (value === undefined) throw new RangeError(`${label} is insufficient`); return value;
}

function valueFromMint(value: Mint | undefined, positive: boolean): Value {
  if (value === undefined) return Value.zero();
  return Value.new(0n, positive ? value.as_positive_multiasset() : value.as_negative_multiasset());
}

function scriptRefSize(output: TransactionOutput): bigint {
  const reference = outputParts(output).scriptRef;
  if (reference === undefined) return 0n;
  const decoded = node(reference);
  return BigInt(decoded.kind === "tag" && decoded.value.kind === "bytes" ? decoded.value.value.length : reference.to_cbor_bytes().length);
}

function auxiliaryMerge(left: AuxiliaryData, right: AuxiliaryData): AuxiliaryData {
  const a = node(left), b = node(right);
  if (a.kind !== "map" || b.kind !== "map") return clone(right, AuxiliaryData);
  const entries = [...a.entries];
  for (const [key, value] of b.entries) {
    const encoded = bytesToHex(encodeCbor(key, { mode: "canonical" }));
    const index = entries.findIndex(([item]) => bytesToHex(encodeCbor(item, { mode: "canonical" })) === encoded);
    if (index < 0) entries.push([key, value]); else entries[index] = [key, value];
  }
  return AuxiliaryData.from_cbor_bytes(encodeCbor(map(entries)));
}

function systemCoinSelectionRandom(): number {
  const value = secureRandomBytes(4);
  return ((value[0] ?? 0) | ((value[1] ?? 0) << 8) | ((value[2] ?? 0) << 16) | ((value[3] ?? 0) << 24)) >>> 0;
}

let coinSelectionRandom = systemCoinSelectionRandom;

/** Internal deterministic-seed hook for the recorded-seed builder tests. */
export function __setCoinSelectionRandomSourceForTests(source?: () => number): void {
  coinSelectionRandom = source ?? systemCoinSelectionRandom;
}

function randomOrder<T>(values: readonly T[]): T[] {
  const output = [...values];
  for (let index = output.length - 1; index > 0; index -= 1) {
    const target = coinSelectionRandom() % (index + 1); [output[index], output[target]] = [output[target] as T, output[index] as T];
  }
  return output;
}

function votingNode(entries: readonly VoteEntry[]): CborValue {
  const voters = new Map<string, { voter: Voter; entries: Array<readonly [CborValue, CborValue]> }>();
  for (const entry of entries) {
    const key = canonicalHex(entry.voter), current = voters.get(key) ?? { voter: entry.voter, entries: [] };
    current.entries.push([node(entry.action), node(entry.procedure)]); current.entries.sort((left, right) => compareBytes(encodeCbor(left[0], { mode: "canonical" }), encodeCbor(right[0], { mode: "canonical" }))); voters.set(key, current);
  }
  return map([...voters.values()].sort((left, right) => canonicalHex(left.voter).localeCompare(canonicalHex(right.voter))).map((entry) => [node(entry.voter), map(entry.entries)]));
}

function transactionNode(body: TransactionBody, witness: TransactionWitnessSet, auxiliary: AuxiliaryData | undefined, valid = true): CborValue {
  return array([node(body), node(witness), { kind: "boolean", value: valid }, auxiliary === undefined ? { kind: "null" } : node(auxiliary)]);
}

export class TransactionBuilder {
  readonly #config: TransactionBuilderConfig;
  readonly #inputs: InputBuilderResult[] = [];
  readonly #utxos: InputBuilderResult[] = [];
  readonly #outputs: SingleOutputBuilderResult[] = [];
  readonly #certificates: CertificateBuilderResult[] = [];
  readonly #withdrawals: WithdrawalBuilderResult[] = [];
  readonly #proposals: ProposalEntry[] = [];
  readonly #votes: VoteEntry[] = [];
  readonly #collateral: InputBuilderResult[] = [];
  readonly #referenceInputs: TransactionUnspentOutput[] = [];
  readonly #requiredSigners = new Map<string, Ed25519KeyHash>();
  readonly #witnesses = TransactionWitnessSetBuilder.new();
  readonly #redeemers = RedeemerSetBuilder.new();
  #mint: Mint | undefined;
  #fee: bigint | undefined;
  #ttl: bigint | undefined;
  #validityStart: bigint | undefined;
  #networkId: NetworkId | undefined;
  #collateralReturn: TransactionOutput | undefined;
  #auxiliaryData: AuxiliaryData | undefined;
  #donation: bigint | undefined;
  #currentTreasuryValue: bigint | undefined;
  private constructor(config: TransactionBuilderConfig) { this.#config = config; }
  public static new(config: TransactionBuilderConfig): TransactionBuilder { return new TransactionBuilder(config); }
  public add_input(result: InputBuilderResult): void {
    const id = canonicalHex(result.inputValue); if (this.#inputs.some((entry) => canonicalHex(entry.inputValue) === id)) throw new TypeError("duplicate transaction input");
    this.#inputs.push(result); this.#witnesses.add_required_wits(result.required); this.#redeemers.add_spend(result); this.addAggregate(result.aggregate);
  }
  public add_utxo(result: InputBuilderResult): void {
    const id = canonicalHex(result.inputValue); if (!this.#utxos.some((entry) => canonicalHex(entry.inputValue) === id) && !this.#inputs.some((entry) => canonicalHex(entry.inputValue) === id)) this.#utxos.push(result);
  }
  public add_reference_input(value: TransactionUnspentOutput): void {
    const id = canonicalHex(value.input()); if (!this.#referenceInputs.some((entry) => canonicalHex(entry.input()) === id)) this.#referenceInputs.push(value);
    const reference = outputParts(value.output()).scriptRef;
    if (reference !== undefined) {
      const decoded = node(reference);
      if (decoded.kind === "tag" && decoded.value.kind === "bytes") {
        const script = Script.from_cbor_bytes(decoded.value.value); const { kind, payload } = scriptParts(script);
        const hash = kind === 0
          ? NativeScript.from_cbor_bytes(encodeCbor(payload)).hash()
          : payload.kind === "bytes" && kind === 1
            ? PlutusScript.from_v1(PlutusV1Script.from_raw_bytes(payload.value)).hash()
            : payload.kind === "bytes" && kind === 2
              ? PlutusScript.from_v2(PlutusV2Script.from_raw_bytes(payload.value)).hash()
              : payload.kind === "bytes" && kind === 3
                ? PlutusScript.from_v3(PlutusV3Script.from_raw_bytes(payload.value)).hash()
                : undefined;
        if (hash !== undefined) this.#witnesses.add_required_wits(scriptReferenceRequirement(hash));
      }
    }
  }
  public add_output(value: SingleOutputBuilderResult): void {
    const output = value.output(), amount = outputParts(output).amount;
    if (amount.to_cbor_bytes().length > this.#config.maxValueSize) throw new RangeError("transaction output value exceeds maximum size");
    const minimum = min_ada_required(output, this.#config.coinsPerUtxoByte);
    if (amount.coin() < minimum) throw new RangeError(`transaction output coin is below minimum ADA ${minimum}`);
    this.#outputs.push(value); const datum = value.communication_datum(); if (datum !== undefined) this.#witnesses.add_plutus_datum(datum);
  }
  public add_mint(result: MintBuilderResult): void {
    this.#mint ??= Mint.new();
    const existing = this.#mint.get(result.policyId);
    if (existing === undefined) this.#mint.insert(result.policyId, copyMintAssets(result.assets));
    else for (const asset of result.assets.keys()) {
      const total = existing.get(asset) === undefined ? result.assets.get(asset)! : (existing.get(asset) ?? 0n) + (result.assets.get(asset) ?? 0n);
      if (total === 0n) throw new RangeError("combined mint quantity must be non-zero"); existing.insert(asset, total);
    }
    this.#witnesses.add_required_wits(result.required); this.#redeemers.add_mint(result); this.addAggregate(result.aggregate);
  }
  public add_cert(result: CertificateBuilderResult): void { this.#certificates.push(result); this.#witnesses.add_required_wits(result.required); this.#redeemers.add_cert(result); this.addAggregate(result.aggregate); }
  public add_withdrawal(result: WithdrawalBuilderResult): void {
    const key = result.address.to_address().to_hex(); if (this.#withdrawals.some((value) => value.address.to_address().to_hex() === key)) throw new TypeError("duplicate withdrawal");
    this.#withdrawals.push(result); this.#witnesses.add_required_wits(result.required); this.#redeemers.add_reward(result); this.addAggregate(result.aggregate);
  }
  public add_proposal(result: ProposalBuilderResult): void { this.#proposals.push(...result.entries); this.#redeemers.add_proposal(result); for (const entry of result.entries) this.addAggregate(entry.aggregate); }
  public add_vote(result: VoteBuilderResult): void { this.#votes.push(...result.entries); this.#redeemers.add_vote(result); for (const entry of result.entries) this.addAggregate(entry.aggregate); }
  public add_collateral(result: InputBuilderResult): void {
    if (result.aggregate !== undefined) throw new TypeError("collateral must use a payment-key input");
    if (this.#collateral.length >= this.#config.maxCollateralInputs) throw new RangeError("maximum collateral input count exceeded");
    this.#collateral.push(result); this.#witnesses.add_required_wits(result.required);
  }
  private addAggregate(value: InputAggregateWitnessData | undefined): void {
    if (value === undefined) return;
    this.addAggregateRequirements(value);
    if (value.state.kind === "native") this.#witnesses.add_script(scriptFromNative(value.state.script));
    else {
      const script = value.state.partial.script().script(); if (script !== undefined) this.#witnesses.add_script(script.to_script());
      if (value.state.datum !== undefined) this.#witnesses.add_plutus_datum(value.state.datum);
      for (const signer of value.state.signers) this.add_required_signer(signer);
    }
  }
  public add_required_signer(value: Ed25519KeyHash): void { this.#requiredSigners.set(value.to_hex(), Ed25519KeyHash.from_raw_bytes(value.to_raw_bytes())); const required = RequiredWitnessSet.new(); required.add_vkey_key_hash(value); this.#witnesses.add_required_wits(required); }
  public set_fee(value: bigint): void { this.#fee = checkedCoin(value); }
  public set_ttl(value: bigint): void { this.#ttl = checkedCoin(value, "ttl"); }
  public set_validity_start_interval(value: bigint): void { this.#validityStart = checkedCoin(value, "validity interval start"); }
  public set_network_id(value: NetworkId): void { this.#networkId = clone(value, NetworkId); }
  public network_id(): NetworkId | undefined { return this.#networkId === undefined ? undefined : clone(this.#networkId, NetworkId); }
  public set_collateral_return(value: TransactionOutput): void { this.#collateralReturn = clone(value, TransactionOutput); }
  public set_donation(value: bigint): void { this.#donation = checkedCoin(value, "donation"); }
  public set_current_treasury_value(value: bigint): void { this.#currentTreasuryValue = checkedCoin(value, "current treasury value"); }
  public set_auxiliary_data(value: AuxiliaryData): void { this.#auxiliaryData = clone(value, AuxiliaryData); }
  public add_auxiliary_data(value: AuxiliaryData): void { this.#auxiliaryData = this.#auxiliaryData === undefined ? clone(value, AuxiliaryData) : auxiliaryMerge(this.#auxiliaryData, value); }
  public get_auxiliary_data(): AuxiliaryData | undefined { return this.#auxiliaryData === undefined ? undefined : clone(this.#auxiliaryData, AuxiliaryData); }
  public get_fee_if_set(): bigint | undefined { return this.#fee; }
  public get_mint(): Mint | undefined { return this.#mint === undefined ? undefined : cloneMint(this.#mint); }
  public get_withdrawals(): Withdrawals | undefined {
    if (this.#withdrawals.length === 0) return undefined;
    return Withdrawals.from_cbor_bytes(encodeCbor(map([...this.#withdrawals].sort((left, right) => left.address.to_address().to_hex().localeCompare(right.address.to_address().to_hex())).map((value) => [bytes(value.address.to_address().to_raw_bytes()), uint(value.amount)]))));
  }
  public get_explicit_input(): Value { return this.#inputs.reduce((total, value) => addValues(total, outputParts(value.outputValue).amount, "explicit input"), Value.zero()); }
  public get_explicit_output(): Value { return this.#outputs.reduce((total, value) => addValues(total, outputParts(value.output()).amount, "explicit output"), Value.zero()); }
  public get_implicit_input(): Value { return get_implicit_input(this.bodyForAccounting(), this.#config.poolDeposit, this.#config.keyDeposit); }
  public get_deposit(): bigint { return get_deposit(this.bodyForAccounting(), this.#config.poolDeposit, this.#config.keyDeposit); }
  public get_total_input(): Value { return addValues(addValues(this.get_explicit_input(), this.get_implicit_input(), "total input"), valueFromMint(this.#mint, true), "total input"); }
  public get_total_output(): Value {
    let output = addValues(this.get_explicit_output(), Value.from_coin(this.get_deposit()), "total output");
    output = addValues(output, valueFromMint(this.#mint, false), "total output");
    if (this.#donation !== undefined) output = addValues(output, Value.from_coin(this.#donation), "total output");
    return output;
  }
  public set_exunits(key: RedeemerWitnessKey, exUnits: ExUnits): void { this.#redeemers.update_ex_units(key, exUnits); }
  public select_utxos(strategy: CoinSelectionStrategyCIP2): void {
    const multiAsset = strategy === CoinSelectionStrategyCIP2.LargestFirstMultiAsset || strategy === CoinSelectionStrategyCIP2.RandomImproveMultiAsset;
    if (strategy === CoinSelectionStrategyCIP2.RandomImprove && this.#outputs.some((value) => outputParts(value.output()).amount.has_multiassets())) {
      throw new TypeError("RandomImprove cannot cover native assets; use RandomImproveMultiAsset");
    }
    const available = this.#utxos.filter((candidate) => !this.#inputs.some((value) => canonicalHex(value.inputValue) === canonicalHex(candidate.inputValue)));
    const remaining = new Set(available.map((_, index) => index));
    const valueAt = (index: number): Value => outputParts(available[index]!.outputValue).amount;
    const add = (index: number): void => { remaining.delete(index); this.add_input(available[index]!); };
    const largestFirstBy = (quantity: (value: Value) => bigint, target: bigint): void => {
      const relevant = [...remaining].filter((index) => quantity(valueAt(index)) > 0n).sort((left, right) => {
        const a = quantity(valueAt(left)), b = quantity(valueAt(right));
        return a === b ? canonicalHex(available[left]!.inputValue).localeCompare(canonicalHex(available[right]!.inputValue)) : a > b ? -1 : 1;
      });
      let total = quantity(this.get_total_input());
      for (const index of relevant) {
        if (total >= target) break;
        total = checkedCoin(total + quantity(valueAt(index)), "coin selection quantity");
        add(index);
      }
      if (total < target) throw new RangeError("available UTxOs do not cover transaction outputs and fees");
    };
    const randomImproveBy = (quantity: (value: Value) => bigint, targets: readonly bigint[]): void => {
      let relevant = randomOrder([...remaining].filter((index) => quantity(valueAt(index)) > 0n));
      const associated: number[][] = [];
      for (const target of [...targets].sort((left, right) => left < right ? -1 : left > right ? 1 : 0).reverse()) {
        const selected: number[] = []; let total = 0n;
        while (total < target) {
          const index = relevant.pop();
          if (index === undefined) throw new RangeError("available UTxOs do not cover transaction outputs and fees");
          remaining.delete(index); selected.push(index); total = checkedCoin(total + quantity(valueAt(index)), "coin selection quantity");
        }
        associated.push(selected);
      }
      if (relevant.length > 0) {
        const improvements = randomOrder(relevant);
        let cursor = 0;
        for (const [groupIndex, selected] of associated.entries()) for (const [selectedIndex, current] of selected.entries()) {
          const candidate = improvements[cursor % improvements.length]!; cursor += 1;
          const target = [...targets].sort((left, right) => left < right ? 1 : left > right ? -1 : 0)[groupIndex]!;
          const ideal = target * 2n, maximum = target * 3n, currentQuantity = quantity(valueAt(current)), candidateQuantity = quantity(valueAt(candidate));
          const currentDistance = currentQuantity > ideal ? currentQuantity - ideal : ideal - currentQuantity;
          const candidateDistance = candidateQuantity > ideal ? candidateQuantity - ideal : ideal - candidateQuantity;
          if (candidateDistance < currentDistance && candidateQuantity < maximum) {
            remaining.add(current); remaining.delete(candidate); selected[selectedIndex] = candidate;
            const position = relevant.indexOf(candidate); if (position >= 0) relevant[position] = current;
            improvements[(cursor - 1) % improvements.length] = current;
          }
        }
      }
      for (const selected of associated) for (const index of selected) add(index);
    };

    const target = this.get_total_output();
    if (strategy === CoinSelectionStrategyCIP2.LargestFirst || strategy === CoinSelectionStrategyCIP2.LargestFirstMultiAsset) {
      if (multiAsset) {
        const assets = target.multi_asset();
        for (const policy of assets?.keys() ?? []) for (const asset of assets?.get_assets(policy)?.keys() ?? []) {
          largestFirstBy((value) => value.multi_asset()?.get_value(policy, asset) ?? 0n, assets?.get_value(policy, asset) ?? 0n);
        }
      }
      for (const index of [...remaining].sort((left, right) => {
        const a = valueAt(left).coin(), b = valueAt(right).coin();
        return a === b ? canonicalHex(available[left]!.inputValue).localeCompare(canonicalHex(available[right]!.inputValue)) : a > b ? -1 : 1;
      })) {
        if (this.coversOutputsAndFee()) break;
        add(index);
      }
    } else {
      if (multiAsset) {
        const assets = target.multi_asset();
        for (const policy of assets?.keys() ?? []) for (const asset of assets?.get_assets(policy)?.keys() ?? []) {
          const targets = this.#outputs.map((value) => outputParts(value.output()).amount.multi_asset()?.get_value(policy, asset) ?? 0n).filter((value) => value > 0n);
          if (targets.length > 0) randomImproveBy((value) => value.multi_asset()?.get_value(policy, asset) ?? 0n, targets);
        }
      }
      const coinTargets = this.#outputs.map((value) => outputParts(value.output()).amount.coin()).filter((value) => value > 0n);
      if (coinTargets.length > 0) randomImproveBy((value) => value.coin(), coinTargets);
      for (const index of randomOrder([...remaining])) {
        if (this.coversOutputsAndFee()) break;
        add(index);
      }
    }
    if (!this.coversOutputsAndFee()) throw new RangeError("available UTxOs do not cover transaction outputs and fees");
  }
  private coversOutputsAndFee(): boolean {
    try { const fee = this.min_fee(true); return this.get_total_input().checked_sub(addValues(this.get_total_output(), Value.from_coin(fee), "selection target")) !== undefined; } catch { return false; }
  }
  public fee_for_input(value: InputBuilderResult): bigint { return BigInt(value.inputValue.to_cbor_bytes().length) * this.#config.feeAlgo.coefficient(); }
  public fee_for_output(value: SingleOutputBuilderResult): bigint { return BigInt(value.output().to_cbor_bytes().length) * this.#config.feeAlgo.coefficient(); }
  public add_change_if_needed(address: Address, includeExunits: boolean): boolean {
    const originalOutputs = this.#outputs.length;
    let fee = this.#fee ?? 0n;
    for (let iteration = 0; iteration < 8; iteration += 1) {
      this.#outputs.splice(originalOutputs);
      const available = this.get_total_input(), required = addValues(this.get_total_output(), Value.from_coin(fee), "transaction balance");
      const change = subtractValues(available, required, "transaction balance");
      this.addChangeOutputs(address, change);
      this.#fee = fee;
      const next = this.min_fee(includeExunits);
      if (next === fee) break;
      fee = next;
    }
    this.#outputs.splice(originalOutputs);
    const finalChange = subtractValues(this.get_total_input(), addValues(this.get_total_output(), Value.from_coin(fee), "transaction balance"), "transaction balance");
    this.addChangeOutputs(address, finalChange);
    if (this.#outputs.length === originalOutputs && !finalChange.is_zero()) {
      if (finalChange.has_multiassets()) throw new RangeError("native-asset change cannot be absorbed into the fee");
      fee = checkedCoin(fee + finalChange.coin(), "transaction fee");
    }
    this.#fee = fee;
    return this.#outputs.length > originalOutputs;
  }
  private addChangeOutputs(address: Address, change: Value): void {
    const assets = cleanMultiAsset(change.multi_asset() ?? MultiAsset.new());
    if (change.coin() === 0n && assets === undefined) return;
    const bundles = splitAssets(assets, this.#config.maxValueSize);
    if (bundles.length === 0) {
      const output = makeOutput(address, Value.from_coin(change.coin()));
      if (change.coin() < min_ada_required(output, this.#config.coinsPerUtxoByte)) return;
      this.#outputs.push(SingleOutputBuilderResult.new(output)); return;
    }
    const minimums = bundles.map((bundle) => {
      const probe = makeOutput(address, Value.new(0n, bundle));
      const first = min_ada_required(probe, this.#config.coinsPerUtxoByte);
      return min_ada_required(makeOutput(address, Value.new(first, bundle)), this.#config.coinsPerUtxoByte);
    });
    const minimumTotal = minimums.reduce((sum, value) => sum + value, 0n);
    if (change.coin() < minimumTotal) throw new RangeError("change does not contain enough ADA for its native assets");
    let remainder = change.coin() - minimumTotal;
    if (this.#config.preferPureChange && remainder > 0n) {
      const pure = makeOutput(address, Value.from_coin(remainder));
      const minimumPure = min_ada_required(pure, this.#config.coinsPerUtxoByte);
      if (remainder >= minimumPure) { this.#outputs.push(SingleOutputBuilderResult.new(pure)); remainder = 0n; }
    }
    for (const [index, bundle] of bundles.entries()) this.#outputs.push(SingleOutputBuilderResult.new(makeOutput(address, Value.new((minimums[index] ?? 0n) + (index === 0 ? remainder : 0n), bundle))));
  }
  public min_fee(includeExunits: boolean): bigint {
    const previous = this.#fee; this.#fee = UINT64_MAX;
    const body = this.buildBody(false), witness = this.buildWitnesses(true), transaction = Transaction.from_cbor_bytes(encodeCbor(transactionNode(body, witness, this.#auxiliaryData)));
    this.#fee = previous;
    return includeExunits
      ? transactionMinFee(transaction, this.#config.feeAlgo, this.#config.exUnitPrices, this.referenceScriptSize())
      : BigInt(transaction.to_cbor_bytes().length) * this.#config.feeAlgo.coefficient() + this.#config.feeAlgo.constant();
  }
  public full_size(): number { return this.transactionForSize().to_cbor_bytes().length; }
  public output_sizes(): Uint32Array { return Uint32Array.from(this.#outputs.map((value) => value.output().to_cbor_bytes().length)); }
  public build_for_evaluation(_algo: ChangeSelectionAlgo, changeAddress: Address): TxRedeemerBuilder {
    this.add_change_if_needed(changeAddress, false); return new TxRedeemerBuilder(this.buildBody(true), this.#witnesses.copy(), this.#redeemers, this.#auxiliaryData);
  }
  public build(_algo: ChangeSelectionAlgo, changeAddress: Address): SignedTxBuilder {
    this.add_change_if_needed(changeAddress, true); const body = this.buildBody(true), witnesses = this.#witnesses.copy();
    for (let index = 0; index < this.#redeemers.build(true).len(); index += 1) witnesses.add_redeemer(this.#redeemers.build(true).get(index));
    return this.#auxiliaryData === undefined ? SignedTxBuilder.new_without_data(body, witnesses, true) : SignedTxBuilder.new_with_data(body, witnesses, true, this.#auxiliaryData);
  }
  private addAggregateRequirements(value: InputAggregateWitnessData | undefined): void { if (value === undefined) return; const required = RequiredWitnessSet.new(); addAggregateRequirements(required, value); this.#witnesses.add_required_wits(required); }
  private bodyForAccounting(): TransactionBody { const fee = this.#fee; this.#fee ??= 0n; const body = this.buildBody(false, false); this.#fee = fee; return body; }
  private buildBody(enforceSize: boolean, includeScriptData = true): TransactionBody {
    if (this.#fee === undefined) throw new TypeError("transaction fee is not specified");
    const entries: Array<readonly [CborValue, CborValue]> = [
      [uint(0n), collectionNode([...this.#inputs].sort((a, b) => canonicalHex(a.inputValue).localeCompare(canonicalHex(b.inputValue))).map((value) => value.inputValue))],
      [uint(1n), array(this.#outputs.map((value) => node(value.output())))],
      [uint(2n), uint(this.#fee)],
    ];
    if (this.#ttl !== undefined) entries.push([uint(3n), uint(this.#ttl)]);
    if (this.#certificates.length > 0) entries.push([uint(4n), collectionNode(this.#certificates.map((value) => value.certificate))]);
    const withdrawals = this.get_withdrawals(); if (withdrawals !== undefined) entries.push([uint(5n), node(withdrawals)]);
    if (this.#auxiliaryData !== undefined) entries.push([uint(7n), bytes(hash_auxiliary_data(this.#auxiliaryData).to_raw_bytes())]);
    if (this.#validityStart !== undefined) entries.push([uint(8n), uint(this.#validityStart)]);
    if (this.#mint !== undefined) entries.push([uint(9n), mintNode(this.#mint)]);
    if (includeScriptData && !this.#redeemers.is_empty()) {
      const list = this.#redeemers.build(true), serializable = { to_cbor_bytes: () => encodeCbor(array(Array.from({ length: list.len() }, (_, index) => node(list.get(index))))) };
      const datums = Array.from({ length: this.#witnesses.get_plutus_datum().len() }, (_, index) => this.#witnesses.get_plutus_datum().get(index));
      const hash = calc_script_data_hash(serializable, datums.length === 0 ? undefined : datums, this.#config.costModels); if (hash !== undefined) entries.push([uint(11n), bytes(hash.to_raw_bytes())]);
    }
    if (this.#collateral.length > 0) entries.push([uint(13n), collectionNode(this.#collateral.map((value) => value.inputValue))]);
    if (this.#requiredSigners.size > 0) entries.push([uint(14n), collectionNode([...this.#requiredSigners.values()].map((value) => ({ to_cbor_bytes: () => encodeCbor(bytes(value.to_raw_bytes())) })))]);
    if (this.#networkId !== undefined) entries.push([uint(15n), node(this.#networkId)]);
    if (this.#collateralReturn !== undefined) {
      entries.push([uint(16n), node(this.#collateralReturn)]);
      const collateralCoin = this.#collateral.reduce((sum, value) => checkedCoin(sum + outputParts(value.outputValue).amount.coin()), 0n);
      const returnCoin = outputParts(this.#collateralReturn).amount.coin(); if (returnCoin > collateralCoin) throw new RangeError("collateral return exceeds collateral input");
      entries.push([uint(17n), uint(collateralCoin - returnCoin)]);
    }
    if (this.#referenceInputs.length > 0) entries.push([uint(18n), collectionNode(this.#referenceInputs.map((value) => value.input()))]);
    if (this.#votes.length > 0) entries.push([uint(19n), votingNode(this.#votes)]);
    if (this.#proposals.length > 0) entries.push([uint(20n), collectionNode(this.#proposals.map((value) => value.proposal))]);
    if (this.#currentTreasuryValue !== undefined) entries.push([uint(21n), uint(this.#currentTreasuryValue)]);
    if (this.#donation !== undefined) { if (this.#donation === 0n) throw new RangeError("donation must be positive"); entries.push([uint(22n), uint(this.#donation)]); }
    entries.sort((left, right) => Number((left[0] as { value: bigint }).value - (right[0] as { value: bigint }).value));
    const body = TransactionBody.from_cbor_bytes(encodeCbor(map(entries)));
    if (enforceSize && this.transactionForSize(body).to_cbor_bytes().length > this.#config.maxTxSize) throw new RangeError("maximum transaction size exceeded");
    return body;
  }
  private buildWitnesses(fake: boolean): TransactionWitnessSet { const value = this.#witnesses.copy(); if (fake) value.merge_fake_witness(value.remaining_wits()); const list = this.#redeemers.build(true); for (let index = 0; index < list.len(); index += 1) value.add_redeemer(list.get(index)); return value.build(); }
  private transactionForSize(body = this.buildBody(false)): Transaction { return Transaction.from_cbor_bytes(encodeCbor(transactionNode(body, this.buildWitnesses(true), this.#auxiliaryData))); }
  private referenceScriptSize(): bigint { return [...this.#inputs.map((value) => value.outputValue), ...this.#referenceInputs.map((value) => value.output())].reduce((sum, output) => sum + scriptRefSize(output), 0n); }
}

function scriptReferenceRequirement(hash: ScriptHash): RequiredWitnessSet { const required = RequiredWitnessSet.new(); required.add_script_ref(hash); return required; }

function splitAssets(value: MultiAsset | undefined, maximumSize: number): MultiAsset[] {
  if (value === undefined) return [];
  const output: MultiAsset[] = []; let current = MultiAsset.new();
  for (const policy of value.keys()) for (const asset of value.get_assets(policy)?.keys() ?? []) {
    const trial = current.checked_add(singleAsset(policy, asset, value.get_value(policy, asset))); if (trial === undefined) throw new RangeError("change asset overflow");
    if (Value.new(0n, trial).to_cbor_bytes().length > maximumSize) {
      if (current.len() === 0) throw new RangeError("single native asset exceeds maximum value size"); output.push(current); current = singleAsset(policy, asset, value.get_value(policy, asset));
    } else current = trial;
  }
  if (current.len() > 0) output.push(current); return output;
}

function singleAsset(policy: ScriptHash, asset: AssetName, amount: bigint): MultiAsset { const value = MultiAsset.new(); value.insert(policy, asset, amount); return value; }

export class TxRedeemerBuilder {
  readonly #body: TransactionBody;
  readonly #witnesses: TransactionWitnessSetBuilder;
  readonly #redeemers: RedeemerSetBuilder;
  readonly #auxiliary: AuxiliaryData | undefined;
  public constructor(body: TransactionBody, witnesses: TransactionWitnessSetBuilder, redeemers: RedeemerSetBuilder, auxiliary?: AuxiliaryData) { this.#body = clone(body, TransactionBody); this.#witnesses = witnesses; this.#redeemers = redeemers; this.#auxiliary = auxiliary; }
  public build(): LegacyRedeemerList { return this.#redeemers.build(false); }
  public set_exunits(key: RedeemerWitnessKey, value: ExUnits): void { this.#redeemers.update_ex_units(key, value); }
  public draft_body(): TransactionBody { return clone(this.#body, TransactionBody); }
  public auxiliary_data(): AuxiliaryData | undefined { return this.#auxiliary === undefined ? undefined : clone(this.#auxiliary, AuxiliaryData); }
  public draft_tx(): Transaction { const witness = this.#witnesses.copy(); witness.merge_fake_witness(witness.remaining_wits()); const list = this.#redeemers.build(true); for (let index = 0; index < list.len(); index += 1) witness.add_redeemer(list.get(index)); return Transaction.from_cbor_bytes(encodeCbor(transactionNode(this.#body, witness.build(), this.#auxiliary))); }
}

export class SignedTxBuilder {
  readonly #body: TransactionBody;
  readonly #witnesses: TransactionWitnessSetBuilder;
  readonly #valid: boolean;
  readonly #auxiliary: AuxiliaryData | undefined;
  private constructor(body: TransactionBody, witnesses: TransactionWitnessSetBuilder, valid: boolean, auxiliary?: AuxiliaryData) { this.#body = clone(body, TransactionBody); this.#witnesses = witnesses.copy(); this.#valid = valid; this.#auxiliary = auxiliary === undefined ? undefined : clone(auxiliary, AuxiliaryData); }
  public static new_with_data(body: TransactionBody, witnessSet: TransactionWitnessSetBuilder, valid: boolean, auxiliary: AuxiliaryData): SignedTxBuilder { return new SignedTxBuilder(body, witnessSet, valid, auxiliary); }
  public static new_without_data(body: TransactionBody, witnessSet: TransactionWitnessSetBuilder, valid: boolean): SignedTxBuilder { return new SignedTxBuilder(body, witnessSet, valid); }
  public add_vkey(value: Vkeywitness): void { this.#witnesses.add_vkey(value); }
  public add_bootstrap(value: BootstrapWitness): void { this.#witnesses.add_bootstrap(value); }
  public body(): TransactionBody { return clone(this.#body, TransactionBody); }
  public witness_set(): TransactionWitnessSetBuilder { return this.#witnesses.copy(); }
  public is_valid(): boolean { return this.#valid; }
  public auxiliary_data(): AuxiliaryData | undefined { return this.#auxiliary === undefined ? undefined : clone(this.#auxiliary, AuxiliaryData); }
  public build_checked(): Transaction { return Transaction.from_cbor_bytes(encodeCbor(transactionNode(this.#body, this.#witnesses.try_build(), this.#auxiliary, this.#valid))); }
  public build_unchecked(): Transaction { return Transaction.from_cbor_bytes(encodeCbor(transactionNode(this.#body, this.#witnesses.build(), this.#auxiliary, this.#valid))); }
}
