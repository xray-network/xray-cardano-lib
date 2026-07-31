import {
  Address,
  Metadata,
  NetworkId,
  TransactionMetadatum,
} from "@xray-network/cardano-chain";
import {
  UINT64_MAX,
  bytesToHex,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/cardano-core";
import type { CborValue } from "@xray-network/cardano-core";
import {
  Ed25519Signature,
  PublicKey,
  blake2b256,
} from "@xray-network/cardano-crypto";

const KEY_REGISTRATION_LABEL = 61284n;
const REGISTRATION_WITNESS_LABEL = 61285n;
const KEY_DEREGISTRATION_LABEL = 61286n;

function uint(value: bigint): CborValue { if (value < 0n || value > UINT64_MAX) throw new RangeError("value must fit uint64"); return { kind: "unsigned", value, encoding: { width: 0 } }; }
function bytes(value: Uint8Array): CborValue { return { kind: "bytes", value: value.slice(), encoding: { kind: "definite", width: 0 } }; }
function array(values: readonly CborValue[]): CborValue { return { kind: "array", values, encoding: { kind: "definite", width: 0 } }; }
function map(entries: ReadonlyArray<readonly [CborValue, CborValue]>): CborValue { return { kind: "map", entries, encoding: { kind: "definite", width: 0 } }; }
function expectMap(value: CborValue, name: string): Extract<CborValue, { kind: "map" }> { if (value.kind !== "map") throw new TypeError(`${name} requires a CBOR map`); return value; }
function expectBytes(value: CborValue, length: number, name: string): Uint8Array { if (value.kind !== "bytes" || length >= 0 && value.value.length !== length) throw new TypeError(length < 0 ? `${name} requires CBOR bytes` : `${name} requires ${length} CBOR bytes`); return value.value; }
function expectUint(value: CborValue, name: string): bigint { if (value.kind !== "unsigned") throw new TypeError(`${name} requires an unsigned integer`); return value.value; }
function object(value: unknown, name: string): Record<string, unknown> { if (typeof value !== "object" || value === null || Array.isArray(value)) throw new TypeError(`${name} JSON must be an object`); return value as Record<string, unknown>; }
function bigintJson(value: unknown, name: string): bigint { if (typeof value === "number" && Number.isSafeInteger(value) && value >= 0) return BigInt(value); if (typeof value === "string" && /^\d+$/u.test(value)) return BigInt(value); throw new TypeError(`${name} must be an unsigned integer`); }
function jsonUint(value: bigint): number | string { return value <= BigInt(Number.MAX_SAFE_INTEGER) ? Number(value) : value.toString(); }

abstract class Cip36Data {
  public abstract toNode(): CborValue;
  public abstract to_js_value(): unknown;
  public to_cbor_bytes(): Uint8Array { return encodeCbor(this.toNode()); }
  public to_cbor_hex(): string { return bytesToHex(this.to_cbor_bytes()); }
  public to_canonical_cbor_bytes(): Uint8Array { return encodeCbor(this.toNode(), { mode: "canonical" }); }
  public to_canonical_cbor_hex(): string { return bytesToHex(this.to_canonical_cbor_bytes()); }
  public to_json(): string { return JSON.stringify(this.to_js_value(), null, 2); }
}

export enum CIP36DelegationDistributionKind { Weighted = 0, Legacy = 1 }

export class CIP36Delegation extends Cip36Data {
  readonly #key: PublicKey; readonly #weight: number; readonly #original: CborValue | undefined;
  private constructor(key: PublicKey, weight: number, original?: CborValue) { super(); if (!Number.isSafeInteger(weight) || weight < 0 || weight > 0xffff_ffff) throw new RangeError("CIP36 weight must fit uint32"); this.#key = PublicKey.from_bytes(key.to_raw_bytes()); this.#weight = weight; this.#original = original; }
  public static new(key: PublicKey, weight: number): CIP36Delegation { return new CIP36Delegation(key, weight); }
  public static parse(value: CborValue): CIP36Delegation { if (value.kind !== "array" || value.values.length !== 2) throw new TypeError("CIP36Delegation requires a two-element array"); const weight = expectUint(value.values[1]!, "delegation weight"); if (weight > 0xffff_ffffn) throw new RangeError("CIP36 weight must fit uint32"); return new CIP36Delegation(PublicKey.from_bytes(expectBytes(value.values[0]!, 32, "voting public key")), Number(weight), value); }
  public static from_cbor_bytes(value: Uint8Array): CIP36Delegation { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP36Delegation { return this.from_cbor_bytes(hexToBytes(value)); }
  public static from_json(value: string): CIP36Delegation { const parsed = object(JSON.parse(value), "CIP36Delegation"); if (typeof parsed["voting_pub_key"] !== "string") throw new TypeError("voting_pub_key must be hex"); return this.new(PublicKey.from_bytes(hexToBytes(parsed["voting_pub_key"])), Number(bigintJson(parsed["weight"], "weight"))); }
  public voting_pub_key(): PublicKey { return PublicKey.from_bytes(this.#key.to_raw_bytes()); }
  public weight(): number { return this.#weight; }
  public toNode(): CborValue { return this.#original ?? array([bytes(this.#key.to_raw_bytes()), uint(BigInt(this.#weight))]); }
  public to_js_value(): unknown { return { voting_pub_key: bytesToHex(this.#key.to_raw_bytes()), weight: this.#weight }; }
}

export class CIP36DelegationList {
  protected readonly values: CIP36Delegation[] = [];
  public static new(_first?: CIP36Delegation): CIP36DelegationList { return new CIP36DelegationList(); }
  public len(): number { return this.values.length; }
  public get(index: number): CIP36Delegation { const value = this.values[index]; if (value === undefined) throw new RangeError("CIP36 delegation index out of bounds"); return CIP36Delegation.from_cbor_bytes(value.to_cbor_bytes()); }
  public add(value: CIP36Delegation): void { this.values.push(CIP36Delegation.from_cbor_bytes(value.to_cbor_bytes())); }
  public all(): CIP36Delegation[] { return this.values.map((value) => CIP36Delegation.from_cbor_bytes(value.to_cbor_bytes())); }
}

export class NonEmptyCIP36DelegationList extends CIP36DelegationList {
  private constructor(first: CIP36Delegation) { super(); this.add(first); }
  public static override new(first?: CIP36Delegation): NonEmptyCIP36DelegationList { if (first === undefined) throw new TypeError("first CIP36 delegation is required"); return new NonEmptyCIP36DelegationList(first); }
  public static try_from(value: CIP36DelegationList): NonEmptyCIP36DelegationList { if (value.len() === 0) throw new RangeError("CIP36 delegation list must not be empty"); const output = this.new(value.get(0)); for (let index = 1; index < value.len(); index += 1) output.add(value.get(index)); return output; }
}

export class CIP36DelegationDistribution extends Cip36Data {
  readonly #kind: CIP36DelegationDistributionKind; readonly #weighted: CIP36Delegation[] | undefined; readonly #legacy: PublicKey | undefined; readonly #original: CborValue | undefined;
  private constructor(kind: CIP36DelegationDistributionKind, weighted?: readonly CIP36Delegation[], legacy?: PublicKey, original?: CborValue) { super(); this.#kind = kind; this.#weighted = weighted?.map((value) => CIP36Delegation.from_cbor_bytes(value.to_cbor_bytes())); this.#legacy = legacy === undefined ? undefined : PublicKey.from_bytes(legacy.to_raw_bytes()); this.#original = original; }
  public static new_weighted(value: NonEmptyCIP36DelegationList): CIP36DelegationDistribution { return new CIP36DelegationDistribution(CIP36DelegationDistributionKind.Weighted, value.all()); }
  public static new_legacy(value: PublicKey): CIP36DelegationDistribution { return new CIP36DelegationDistribution(CIP36DelegationDistributionKind.Legacy, undefined, value); }
  public static parse(value: CborValue): CIP36DelegationDistribution { if (value.kind === "bytes") return new CIP36DelegationDistribution(CIP36DelegationDistributionKind.Legacy, undefined, PublicKey.from_bytes(expectBytes(value, 32, "legacy voting key")), value); if (value.kind === "array") { if (value.values.length === 0) throw new RangeError("CIP36 weighted delegation must not be empty"); return new CIP36DelegationDistribution(CIP36DelegationDistributionKind.Weighted, value.values.map((item: CborValue) => CIP36Delegation.parse(item)), undefined, value); } throw new TypeError("invalid CIP36 delegation distribution"); }
  public static from_cbor_bytes(value: Uint8Array): CIP36DelegationDistribution { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP36DelegationDistribution { return this.from_cbor_bytes(hexToBytes(value)); }
  public static from_json(value: string): CIP36DelegationDistribution { const parsed = object(JSON.parse(value), "CIP36DelegationDistribution"); if (typeof parsed["Legacy"] === "string") return this.new_legacy(PublicKey.from_bytes(hexToBytes(parsed["Legacy"]))); const weightedValue = parsed["Weighted"]; const weighted = Array.isArray(weightedValue) ? weightedValue : object(weightedValue, "weighted distribution")["weighted"]; if (!Array.isArray(weighted) || weighted.length === 0) throw new TypeError("weighted distribution must be non-empty"); const list = NonEmptyCIP36DelegationList.new(CIP36Delegation.from_json(JSON.stringify(weighted[0]))); for (const item of weighted.slice(1)) list.add(CIP36Delegation.from_json(JSON.stringify(item))); return this.new_weighted(list); }
  public kind(): CIP36DelegationDistributionKind { return this.#kind; }
  public as_weighted(): NonEmptyCIP36DelegationList | undefined { if (this.#weighted === undefined || this.#weighted[0] === undefined) return undefined; const output = NonEmptyCIP36DelegationList.new(this.#weighted[0]); for (const value of this.#weighted.slice(1)) output.add(value); return output; }
  public as_legacy(): PublicKey | undefined { return this.#legacy === undefined ? undefined : PublicKey.from_bytes(this.#legacy.to_raw_bytes()); }
  public toNode(): CborValue { return this.#original ?? (this.#kind === CIP36DelegationDistributionKind.Legacy ? bytes(this.#legacy!.to_raw_bytes()) : array((this.#weighted ?? []).map((value) => value.toNode()))); }
  public to_js_value(): unknown { return this.#kind === CIP36DelegationDistributionKind.Legacy ? { Legacy: bytesToHex(this.#legacy!.to_raw_bytes()) } : { Weighted: { weighted: (this.#weighted ?? []).map((value) => value.to_js_value()) } }; }
}

abstract class Cip36Witness extends Cip36Data {
  readonly #signature: Ed25519Signature; readonly #original: CborValue | undefined;
  protected constructor(signature: Ed25519Signature, original?: CborValue) { super(); this.#signature = Ed25519Signature.from_raw_bytes(signature.to_raw_bytes()); this.#original = original; }
  public stake_witness(): Ed25519Signature { return Ed25519Signature.from_raw_bytes(this.#signature.to_raw_bytes()); }
  public toNode(): CborValue { return this.#original ?? map([[uint(1n), bytes(this.#signature.to_raw_bytes())]]); }
  public to_js_value(): unknown { return { stake_witness: bytesToHex(this.#signature.to_raw_bytes()) }; }
}

export class CIP36RegistrationWitness extends Cip36Witness {
  private constructor(value: Ed25519Signature, original?: CborValue) { super(value, original); }
  public static new(value: Ed25519Signature): CIP36RegistrationWitness { return new CIP36RegistrationWitness(value); }
  public static parse(value: CborValue): CIP36RegistrationWitness { const fields = numericFields(expectMap(value, "CIP36RegistrationWitness"), [1n]); return new CIP36RegistrationWitness(Ed25519Signature.from_raw_bytes(expectBytes(required(fields, 1n), 64, "stake witness")), value); }
  public static from_cbor_bytes(value: Uint8Array): CIP36RegistrationWitness { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP36RegistrationWitness { return this.from_cbor_bytes(hexToBytes(value)); }
  public static from_json(value: string): CIP36RegistrationWitness { const parsed = object(JSON.parse(value), "CIP36RegistrationWitness"); if (typeof parsed["stake_witness"] !== "string") throw new TypeError("stake_witness must be hex"); return this.new(Ed25519Signature.from_raw_bytes(hexToBytes(parsed["stake_witness"]))); }
}

export class CIP36DeregistrationWitness extends Cip36Witness {
  private constructor(value: Ed25519Signature, original?: CborValue) { super(value, original); }
  public static new(value: Ed25519Signature): CIP36DeregistrationWitness { return new CIP36DeregistrationWitness(value); }
  public static parse(value: CborValue): CIP36DeregistrationWitness { const fields = numericFields(expectMap(value, "CIP36DeregistrationWitness"), [1n]); return new CIP36DeregistrationWitness(Ed25519Signature.from_raw_bytes(expectBytes(required(fields, 1n), 64, "stake witness")), value); }
  public static from_cbor_bytes(value: Uint8Array): CIP36DeregistrationWitness { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP36DeregistrationWitness { return this.from_cbor_bytes(hexToBytes(value)); }
  public static from_json(value: string): CIP36DeregistrationWitness { const parsed = object(JSON.parse(value), "CIP36DeregistrationWitness"); if (typeof parsed["stake_witness"] !== "string") throw new TypeError("stake_witness must be hex"); return this.new(Ed25519Signature.from_raw_bytes(hexToBytes(parsed["stake_witness"]))); }
}

export class CIP36KeyRegistration extends Cip36Data {
  readonly #delegation: CIP36DelegationDistribution; readonly #stake: PublicKey; readonly #payment: Address; readonly #nonce: bigint;
  #purpose: bigint; #purposePresent: boolean; #original: CborValue | undefined;
  private constructor(delegation: CIP36DelegationDistribution, stake: PublicKey, payment: Address, nonce: bigint, purpose = 0n, purposePresent?: boolean, original?: CborValue) { super(); if (nonce < 0n || nonce > UINT64_MAX || purpose < 0n || purpose > UINT64_MAX) throw new RangeError("CIP36 integer must fit uint64"); this.#delegation = CIP36DelegationDistribution.from_cbor_bytes(delegation.to_cbor_bytes()); this.#stake = PublicKey.from_bytes(stake.to_raw_bytes()); this.#payment = Address.from_raw_bytes(payment.to_raw_bytes()); this.#nonce = nonce; this.#purpose = purpose; this.#purposePresent = purposePresent ?? delegation.kind() === CIP36DelegationDistributionKind.Weighted; this.#original = original; }
  public static new(delegation: CIP36DelegationDistribution, stake: PublicKey, payment: Address, nonce: bigint): CIP36KeyRegistration { return new CIP36KeyRegistration(delegation, stake, payment, nonce); }
  public static parse(value: CborValue): CIP36KeyRegistration { const fields = numericFields(expectMap(value, "CIP36KeyRegistration"), [1n, 2n, 3n, 4n, 5n]); return new CIP36KeyRegistration(CIP36DelegationDistribution.parse(required(fields, 1n)), PublicKey.from_bytes(expectBytes(required(fields, 2n), 32, "stake credential")), Address.from_raw_bytes(expectBytes(required(fields, 3n), -1, "payment address")), expectUint(required(fields, 4n), "nonce"), fields.has(5n) ? expectUint(required(fields, 5n), "voting purpose") : 0n, fields.has(5n), value); }
  public static from_cbor_bytes(value: Uint8Array): CIP36KeyRegistration { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP36KeyRegistration { return this.from_cbor_bytes(hexToBytes(value)); }
  public static from_json(value: string): CIP36KeyRegistration { const parsed = object(JSON.parse(value), "CIP36KeyRegistration"); if (typeof parsed["stake_credential"] !== "string" || typeof parsed["payment_address"] !== "string") throw new TypeError("invalid CIP36 registration JSON"); const output = new CIP36KeyRegistration(CIP36DelegationDistribution.from_json(JSON.stringify(parsed["delegation"])), PublicKey.from_bytes(hexToBytes(parsed["stake_credential"])), Address.from_bech32(parsed["payment_address"]), bigintJson(parsed["nonce"], "nonce"), parsed["voting_purpose"] === undefined ? 0n : bigintJson(parsed["voting_purpose"], "voting purpose"), parsed["voting_purpose"] !== undefined); return output; }
  public delegation(): CIP36DelegationDistribution { return CIP36DelegationDistribution.from_cbor_bytes(this.#delegation.to_cbor_bytes()); }
  public stake_credential(): PublicKey { return PublicKey.from_bytes(this.#stake.to_raw_bytes()); }
  public payment_address(): Address { return Address.from_raw_bytes(this.#payment.to_raw_bytes()); }
  public nonce(): bigint { return this.#nonce; }
  public set_voting_purpose(value: bigint): void { if (value < 0n || value > UINT64_MAX) throw new RangeError("voting purpose must fit uint64"); this.#purpose = value; this.#original = undefined; }
  public voting_purpose(): bigint { return this.#purpose; }
  public hash_to_sign(forceCanonical: boolean): Uint8Array { return blake2b256(encodeCbor(map([[uint(KEY_REGISTRATION_LABEL), this.toNode()]]), { mode: forceCanonical ? "canonical" : "preserve" })); }
  public toNode(): CborValue {
    if (this.#original !== undefined) return this.#original;
    const entries: Array<readonly [CborValue, CborValue]> = [[uint(1n), this.#delegation.toNode()], [uint(2n), bytes(this.#stake.to_raw_bytes())], [uint(3n), bytes(this.#payment.to_raw_bytes())], [uint(4n), uint(this.#nonce)]];
    const includePurpose = this.#delegation.kind() === CIP36DelegationDistributionKind.Weighted && (this.#purpose !== 0n || this.#purposePresent);
    if (includePurpose) entries.push([uint(5n), uint(this.#purpose)]);
    return map(entries);
  }
  public to_js_value(): unknown { return { delegation: this.#delegation.to_js_value(), stake_credential: bytesToHex(this.#stake.to_raw_bytes()), payment_address: this.#payment.to_bech32(), nonce: jsonUint(this.#nonce), voting_purpose: jsonUint(this.#purpose) }; }
}

export class CIP36KeyDeregistration extends Cip36Data {
  readonly #stake: PublicKey; readonly #nonce: bigint; #purpose: bigint; #purposePresent: boolean; #original: CborValue | undefined;
  private constructor(stake: PublicKey, nonce: bigint, purpose = 0n, purposePresent = false, original?: CborValue) { super(); if (nonce < 0n || nonce > UINT64_MAX || purpose < 0n || purpose > UINT64_MAX) throw new RangeError("CIP36 integer must fit uint64"); this.#stake = PublicKey.from_bytes(stake.to_raw_bytes()); this.#nonce = nonce; this.#purpose = purpose; this.#purposePresent = purposePresent; this.#original = original; }
  public static new(stake: PublicKey, nonce: bigint): CIP36KeyDeregistration { return new CIP36KeyDeregistration(stake, nonce); }
  public static parse(value: CborValue): CIP36KeyDeregistration { const fields = numericFields(expectMap(value, "CIP36KeyDeregistration"), [1n, 2n, 3n]); return new CIP36KeyDeregistration(PublicKey.from_bytes(expectBytes(required(fields, 1n), 32, "stake credential")), expectUint(required(fields, 2n), "nonce"), fields.has(3n) ? expectUint(required(fields, 3n), "voting purpose") : 0n, fields.has(3n), value); }
  public static from_cbor_bytes(value: Uint8Array): CIP36KeyDeregistration { return this.parse(decodeCbor(value)); }
  public static from_cbor_hex(value: string): CIP36KeyDeregistration { return this.from_cbor_bytes(hexToBytes(value)); }
  public static from_json(value: string): CIP36KeyDeregistration { const parsed = object(JSON.parse(value), "CIP36KeyDeregistration"); if (typeof parsed["stake_credential"] !== "string") throw new TypeError("stake_credential must be hex"); return new CIP36KeyDeregistration(PublicKey.from_bytes(hexToBytes(parsed["stake_credential"])), bigintJson(parsed["nonce"], "nonce"), parsed["voting_purpose"] === undefined ? 0n : bigintJson(parsed["voting_purpose"], "voting purpose"), parsed["voting_purpose"] !== undefined); }
  public stake_credential(): PublicKey { return PublicKey.from_bytes(this.#stake.to_raw_bytes()); }
  public nonce(): bigint { return this.#nonce; }
  public set_voting_purpose(value: bigint): void { if (value < 0n || value > UINT64_MAX) throw new RangeError("voting purpose must fit uint64"); this.#purpose = value; this.#original = undefined; }
  public voting_purpose(): bigint { return this.#purpose; }
  public hash_to_sign(forceCanonical: boolean): Uint8Array { return blake2b256(encodeCbor(map([[uint(KEY_DEREGISTRATION_LABEL), this.toNode()]]), { mode: forceCanonical ? "canonical" : "preserve" })); }
  public toNode(): CborValue { if (this.#original !== undefined) return this.#original; const entries: Array<readonly [CborValue, CborValue]> = [[uint(1n), bytes(this.#stake.to_raw_bytes())], [uint(2n), uint(this.#nonce)]]; if (this.#purpose !== 0n || this.#purposePresent) entries.push([uint(3n), uint(this.#purpose)]); return map(entries); }
  public to_js_value(): unknown { return { stake_credential: bytesToHex(this.#stake.to_raw_bytes()), nonce: jsonUint(this.#nonce), voting_purpose: jsonUint(this.#purpose) }; }
}

abstract class Cip36MetadataView {
  public abstract to_js_value(): unknown;
  public to_json(): string { return JSON.stringify(this.to_js_value(), null, 2); }
}

export class CIP36RegistrationCbor extends Cip36MetadataView {
  readonly #registration: CIP36KeyRegistration; readonly #witness: CIP36RegistrationWitness;
  private constructor(registration: CIP36KeyRegistration, witness: CIP36RegistrationWitness) { super(); this.#registration = CIP36KeyRegistration.from_cbor_bytes(registration.to_cbor_bytes()); this.#witness = CIP36RegistrationWitness.from_cbor_bytes(witness.to_cbor_bytes()); }
  public static new(registration: CIP36KeyRegistration, witness: CIP36RegistrationWitness): CIP36RegistrationCbor { return new CIP36RegistrationCbor(registration, witness); }
  public static from_json(value: string): CIP36RegistrationCbor { const parsed = object(JSON.parse(value), "CIP36RegistrationCbor"); return this.new(CIP36KeyRegistration.from_json(JSON.stringify(parsed["key_registration"])), CIP36RegistrationWitness.from_json(JSON.stringify(parsed["registration_witness"]))); }
  public static from_metadata_bytes(value: Uint8Array): CIP36RegistrationCbor { const fields = metadataFields(value); return this.new(CIP36KeyRegistration.parse(required(fields, KEY_REGISTRATION_LABEL)), CIP36RegistrationWitness.parse(required(fields, REGISTRATION_WITNESS_LABEL))); }
  public static try_from_metadata(value: Metadata): CIP36RegistrationCbor { const registration = value.get(KEY_REGISTRATION_LABEL), witness = value.get(REGISTRATION_WITNESS_LABEL); if (registration === undefined || witness === undefined) throw new TypeError("CIP36 registration metadata labels are missing"); return this.new(CIP36KeyRegistration.from_cbor_bytes(registration.to_cbor_bytes()), CIP36RegistrationWitness.from_cbor_bytes(witness.to_cbor_bytes())); }
  public key_registration(): CIP36KeyRegistration { return CIP36KeyRegistration.from_cbor_bytes(this.#registration.to_cbor_bytes()); }
  public registration_witness(): CIP36RegistrationWitness { return CIP36RegistrationWitness.from_cbor_bytes(this.#witness.to_cbor_bytes()); }
  public verify(): void { const weighted = this.#registration.delegation().as_weighted(); if (weighted !== undefined) for (let index = 0; index < weighted.len(); index += 1) if (weighted.get(index).weight() !== 0) throw new TypeError("Invalid delegation weights"); }
  public to_metadata_bytes(): Uint8Array { this.verify(); return encodeCbor(map([[uint(KEY_REGISTRATION_LABEL), this.#registration.toNode()], [uint(REGISTRATION_WITNESS_LABEL), this.#witness.toNode()]])); }
  public add_to_metadata(value: Metadata): void { this.verify(); value.set(KEY_REGISTRATION_LABEL, TransactionMetadatum.from_cbor_bytes(this.#registration.to_cbor_bytes())); value.set(REGISTRATION_WITNESS_LABEL, TransactionMetadatum.from_cbor_bytes(this.#witness.to_cbor_bytes())); }
  public try_into_metadata(): Metadata { const output = Metadata.new(); this.add_to_metadata(output); return output; }
  public to_js_value(): unknown { return { key_registration: this.#registration.to_js_value(), registration_witness: this.#witness.to_js_value() }; }
}

export class CIP36DeregistrationCbor extends Cip36MetadataView {
  readonly #deregistration: CIP36KeyDeregistration; readonly #witness: CIP36DeregistrationWitness;
  private constructor(deregistration: CIP36KeyDeregistration, witness: CIP36DeregistrationWitness) { super(); this.#deregistration = CIP36KeyDeregistration.from_cbor_bytes(deregistration.to_cbor_bytes()); this.#witness = CIP36DeregistrationWitness.from_cbor_bytes(witness.to_cbor_bytes()); }
  public static new(deregistration: CIP36KeyDeregistration, witness: CIP36DeregistrationWitness): CIP36DeregistrationCbor { return new CIP36DeregistrationCbor(deregistration, witness); }
  public static from_json(value: string): CIP36DeregistrationCbor { const parsed = object(JSON.parse(value), "CIP36DeregistrationCbor"); return this.new(CIP36KeyDeregistration.from_json(JSON.stringify(parsed["key_deregistration"])), CIP36DeregistrationWitness.from_json(JSON.stringify(parsed["deregistration_witness"]))); }
  public static from_metadata_bytes(value: Uint8Array): CIP36DeregistrationCbor { const fields = metadataFields(value); return this.new(CIP36KeyDeregistration.parse(required(fields, KEY_DEREGISTRATION_LABEL)), CIP36DeregistrationWitness.parse(required(fields, REGISTRATION_WITNESS_LABEL))); }
  public static try_from_metadata(value: Metadata): CIP36DeregistrationCbor { const deregistration = value.get(KEY_DEREGISTRATION_LABEL), witness = value.get(REGISTRATION_WITNESS_LABEL); if (deregistration === undefined || witness === undefined) throw new TypeError("CIP36 deregistration metadata labels are missing"); return this.new(CIP36KeyDeregistration.from_cbor_bytes(deregistration.to_cbor_bytes()), CIP36DeregistrationWitness.from_cbor_bytes(witness.to_cbor_bytes())); }
  public key_deregistration(): CIP36KeyDeregistration { return CIP36KeyDeregistration.from_cbor_bytes(this.#deregistration.to_cbor_bytes()); }
  public deregistration_witness(): CIP36DeregistrationWitness { return CIP36DeregistrationWitness.from_cbor_bytes(this.#witness.to_cbor_bytes()); }
  public to_metadata_bytes(): Uint8Array { return encodeCbor(map([[uint(REGISTRATION_WITNESS_LABEL), this.#witness.toNode()], [uint(KEY_DEREGISTRATION_LABEL), this.#deregistration.toNode()]])); }
  public add_to_metadata(value: Metadata): void { value.set(KEY_DEREGISTRATION_LABEL, TransactionMetadatum.from_cbor_bytes(this.#deregistration.to_cbor_bytes())); value.set(REGISTRATION_WITNESS_LABEL, TransactionMetadatum.from_cbor_bytes(this.#witness.to_cbor_bytes())); }
  public try_into_metadata(): Metadata { const output = Metadata.new(); this.add_to_metadata(output); return output; }
  public to_js_value(): unknown { return { key_deregistration: this.#deregistration.to_js_value(), deregistration_witness: this.#witness.to_js_value() }; }
}

function numericFields(value: Extract<CborValue, { kind: "map" }>, allowed: readonly bigint[]): Map<bigint, CborValue> {
  const output = new Map<bigint, CborValue>();
  for (const [key, item] of value.entries) { if (key.kind !== "unsigned" || !allowed.includes(key.value)) throw new TypeError("unknown CIP36 map key"); if (output.has(key.value)) throw new TypeError(`duplicate CIP36 map key ${key.value}`); output.set(key.value, item); }
  return output;
}
function required(fields: Map<bigint, CborValue>, key: bigint): CborValue { const value = fields.get(key); if (value === undefined) throw new TypeError(`missing CIP36 map key ${key}`); return value; }
function metadataFields(value: Uint8Array): Map<bigint, CborValue> { const node = expectMap(decodeCbor(value), "CIP36 metadata view"), output = new Map<bigint, CborValue>(); for (const [key, item] of node.entries) if (key.kind === "unsigned") { if (output.has(key.value)) throw new TypeError(`duplicate metadata label ${key.value}`); output.set(key.value, item); } return output; }

export { Address as PaymentAddress, Metadata, NetworkId, TransactionMetadatum } from "@xray-network/cardano-chain";
export { Ed25519Signature, PublicKey } from "@xray-network/cardano-crypto";
export type CIP36LegacyKeyRegistration = PublicKey;
export type CIP36Nonce = bigint;
export type CIP36StakeCredential = PublicKey;
export type CIP36StakeWitness = Ed25519Signature;
export type CIP36StakingPubKey = PublicKey;
export type CIP36VotingPubKey = PublicKey;
export type CIP36VotingPurpose = bigint;
export type CIP36Weight = number;
