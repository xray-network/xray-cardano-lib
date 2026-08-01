import {
  Int,
  bytesToHex,
  copyBytes,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/xray-cardano-lib-core";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";

import {
  decodeUserFacingMessage,
  encodeUserFacingMessage,
} from "./user-facing.js";

type CborArray = Extract<CborValue, { kind: "array" }>;
type CborMap = Extract<CborValue, { kind: "map" }>;

function cloneNode(value: CborValue): CborValue {
  return decodeCbor(encodeCbor(value, { mode: "preserve" }));
}

function unsigned(value: bigint): CborValue {
  return { kind: "unsigned", value, encoding: { width: 0 } };
}

function integer(value: bigint): CborValue {
  return value >= 0n
    ? unsigned(value)
    : { kind: "negative", value, encoding: { width: 0 } };
}

function bytes(value: Uint8Array): CborValue {
  return {
    kind: "bytes",
    value: copyBytes(value),
    encoding: { kind: "definite", width: 0 },
  };
}

function text(value: string): CborValue {
  return {
    kind: "text",
    value,
    encoding: { kind: "definite", width: 0 },
  };
}

function array(values: readonly CborValue[]): CborArray {
  return {
    kind: "array",
    values,
    encoding: { kind: "definite", width: 0 },
  };
}

function map(entries: readonly (readonly [CborValue, CborValue])[]): CborMap {
  return {
    kind: "map",
    entries,
    encoding: { kind: "definite", width: 0 },
  };
}

function expectArray(value: CborValue, length: number, name: string): CborArray {
  if (value.kind !== "array" || value.values.length !== length) {
    throw new TypeError(`${name} requires a ${length}-element CBOR array`);
  }
  return value;
}

function expectMap(value: CborValue, name: string): CborMap {
  if (value.kind !== "map") throw new TypeError(`${name} requires a CBOR map`);
  return value;
}

function expectBytes(value: CborValue, name: string): Uint8Array {
  if (value.kind !== "bytes") throw new TypeError(`${name} requires a CBOR byte string`);
  return copyBytes(value.value);
}

function expectText(value: CborValue, name: string): string {
  if (value.kind !== "text") throw new TypeError(`${name} requires CBOR text`);
  return value.value;
}

function nullableBytes(value: CborValue, name: string): Uint8Array | undefined {
  if (value.kind === "null") return undefined;
  return expectBytes(value, name);
}

abstract class CoseData {
  public abstract toNode(): CborValue;

  public to_cbor_bytes(): Uint8Array {
    return encodeCbor(this.toNode(), { mode: "preserve" });
  }

  public to_cbor_hex(): string {
    return bytesToHex(this.to_cbor_bytes());
  }

  public to_canonical_cbor_bytes(): Uint8Array {
    return encodeCbor(this.toNode(), { mode: "canonical" });
  }

  public to_canonical_cbor_hex(): string {
    return bytesToHex(this.to_canonical_cbor_bytes());
  }
}

export enum LabelKind {
  Int = 0,
  Text = 1,
}

export enum AlgorithmId {
  EdDSA = -8,
  ChaCha20Poly1305 = 24,
}

export enum KeyType {
  OKP = 1,
  EC2 = 2,
  Symmetric = 4,
}

export enum ECKey {
  CRV = -1,
  X = -2,
  Y = -3,
  D = -4,
}

export enum CurveType {
  P256 = 1,
  P384 = 2,
  P521 = 3,
  X25519 = 4,
  X448 = 5,
  Ed25519 = 6,
  Ed448 = 7,
}

export enum KeyOperation {
  Sign = 1,
  Verify = 2,
  Encrypt = 3,
  Decrypt = 4,
  WrapKey = 5,
  UnwrapKey = 6,
  DeriveKey = 7,
  DeriveBits = 8,
}

export class Label extends CoseData {
  readonly #value: bigint | string;
  readonly #original: CborValue | undefined;

  private constructor(value: bigint | string, original?: CborValue) {
    super();
    this.#value = value;
    this.#original = original;
  }

  public static new_int(value: Int): Label {
    return new Label(BigInt(value.to_str()));
  }

  public static new_text(value: string): Label {
    return new Label(value);
  }

  public static from_algorithm_id(value: AlgorithmId): Label {
    return new Label(BigInt(value));
  }

  public static from_key_type(value: KeyType): Label {
    return new Label(BigInt(value));
  }

  public static from_ec_key(value: ECKey): Label {
    return new Label(BigInt(value));
  }

  public static from_curve_type(value: CurveType): Label {
    return new Label(BigInt(value));
  }

  public static from_key_operation(value: KeyOperation): Label {
    return new Label(BigInt(value));
  }

  public static parse(value: CborValue): Label {
    if (value.kind === "unsigned" || value.kind === "negative") {
      return new Label(value.value, cloneNode(value));
    }
    if (value.kind === "text") return new Label(value.value, cloneNode(value));
    throw new TypeError("Label requires a CBOR integer or text string");
  }

  public static from_cbor_bytes(value: Uint8Array): Label {
    return Label.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): Label {
    return Label.from_cbor_bytes(hexToBytes(value));
  }

  public kind(): LabelKind {
    return typeof this.#value === "bigint" ? LabelKind.Int : LabelKind.Text;
  }

  public as_int(): Int | undefined {
    return typeof this.#value === "bigint" ? Int.from_str(this.#value.toString()) : undefined;
  }

  public as_text(): string | undefined {
    return typeof this.#value === "string" ? this.#value : undefined;
  }

  public toNode(): CborValue {
    return this.#original === undefined
      ? typeof this.#value === "bigint" ? integer(this.#value) : text(this.#value)
      : cloneNode(this.#original);
  }
}

function cloneLabel(value: Label): Label {
  return Label.from_cbor_bytes(value.to_cbor_bytes());
}

function labelIdentity(value: Label): string {
  const intValue = value.as_int();
  return intValue === undefined ? `text:${value.as_text() ?? ""}` : `int:${intValue.to_str()}`;
}

function labelInteger(value: Label): bigint | undefined {
  const intValue = value.as_int();
  return intValue === undefined ? undefined : BigInt(intValue.to_str());
}

function integerLabel(value: bigint): Label {
  return Label.new_int(Int.new(value));
}

export class Labels extends CoseData {
  readonly #values: Label[];
  #original: CborValue | undefined;

  private constructor(values: readonly Label[], original?: CborValue) {
    super();
    this.#values = values.map(cloneLabel);
    this.#original = original;
  }

  public static new(): Labels {
    return new Labels([]);
  }

  public static parse(value: CborValue): Labels {
    if (value.kind !== "array") throw new TypeError("Labels requires a CBOR array");
    return new Labels(value.values.map(Label.parse), cloneNode(value));
  }

  public static from_cbor_bytes(value: Uint8Array): Labels {
    return Labels.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): Labels {
    return Labels.from_cbor_bytes(hexToBytes(value));
  }

  public len(): number {
    return this.#values.length;
  }

  public get(index: number): Label {
    const value = this.#values[index];
    if (value === undefined) throw new RangeError("Labels index out of bounds");
    return cloneLabel(value);
  }

  public add(value: Label): void {
    this.#values.push(cloneLabel(value));
    this.#original = undefined;
  }

  public toNode(): CborValue {
    return this.#original === undefined
      ? array(this.#values.map((value) => value.toNode()))
      : cloneNode(this.#original);
  }
}

function cloneLabels(value: Labels): Labels {
  return Labels.from_cbor_bytes(value.to_cbor_bytes());
}

function validateHeaderValue(label: Label, value: CborValue): void {
  switch (labelInteger(label)) {
    case 1n:
    case 3n:
      Label.parse(value);
      return;
    case 2n:
      Labels.parse(value);
      return;
    case 4n:
    case 5n:
    case 6n:
      expectBytes(value, "Header value");
      return;
    case 7n:
      CounterSignature.parse(value);
      return;
    default:
      return;
  }
}

interface HeaderEntry {
  readonly label: Label;
  readonly value: CborValue;
}

export class HeaderMap extends CoseData {
  readonly #entries: HeaderEntry[];
  #original: CborValue | undefined;

  private constructor(entries: readonly HeaderEntry[], original?: CborValue) {
    super();
    this.#entries = entries.map(({ label, value }) => ({
      label: cloneLabel(label),
      value: cloneNode(value),
    }));
    this.#original = original;
  }

  public static new(): HeaderMap {
    return new HeaderMap([]);
  }

  public static parse(value: CborValue): HeaderMap {
    const node = expectMap(value, "HeaderMap");
    const entries: HeaderEntry[] = [];
    const seen = new Set<string>();
    for (const [keyNode, valueNode] of node.entries) {
      const label = Label.parse(keyNode);
      const identity = labelIdentity(label);
      if (seen.has(identity)) throw new TypeError(`Duplicate HeaderMap label ${identity}`);
      seen.add(identity);
      validateHeaderValue(label, valueNode);
      entries.push({ label, value: cloneNode(valueNode) });
    }
    return new HeaderMap(entries, cloneNode(value));
  }

  public static from_cbor_bytes(value: Uint8Array): HeaderMap {
    return HeaderMap.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): HeaderMap {
    return HeaderMap.from_cbor_bytes(hexToBytes(value));
  }

  public set_algorithm_id(value: Label): void {
    this.set_header(integerLabel(1n), value.toNode());
  }

  public algorithm_id(): Label | undefined {
    const value = this.header(integerLabel(1n));
    return value === undefined ? undefined : Label.parse(value);
  }

  public set_criticality(value: Labels): void {
    this.set_header(integerLabel(2n), value.toNode());
  }

  public criticality(): Labels | undefined {
    const value = this.header(integerLabel(2n));
    return value === undefined ? undefined : Labels.parse(value);
  }

  public set_content_type(value: Label): void {
    this.set_header(integerLabel(3n), value.toNode());
  }

  public content_type(): Label | undefined {
    const value = this.header(integerLabel(3n));
    return value === undefined ? undefined : Label.parse(value);
  }

  public set_key_id(value: Uint8Array): void {
    this.set_header(integerLabel(4n), bytes(value));
  }

  public key_id(): Uint8Array | undefined {
    const value = this.header(integerLabel(4n));
    return value === undefined ? undefined : expectBytes(value, "HeaderMap key ID");
  }

  public set_init_vector(value: Uint8Array): void {
    this.set_header(integerLabel(5n), bytes(value));
  }

  public init_vector(): Uint8Array | undefined {
    const value = this.header(integerLabel(5n));
    return value === undefined ? undefined : expectBytes(value, "HeaderMap initialization vector");
  }

  public set_partial_init_vector(value: Uint8Array): void {
    this.set_header(integerLabel(6n), bytes(value));
  }

  public partial_init_vector(): Uint8Array | undefined {
    const value = this.header(integerLabel(6n));
    return value === undefined ? undefined : expectBytes(value, "HeaderMap partial initialization vector");
  }

  public set_counter_signature(value: CounterSignature): void {
    this.set_header(integerLabel(7n), value.toNode());
  }

  public counter_signature(): CounterSignature | undefined {
    const value = this.header(integerLabel(7n));
    return value === undefined ? undefined : CounterSignature.parse(value);
  }

  public header(label: Label): CborValue | undefined {
    const identity = labelIdentity(label);
    const found = this.#entries.find((entry) => labelIdentity(entry.label) === identity);
    return found === undefined ? undefined : cloneNode(found.value);
  }

  public set_header(label: Label, value: CborValue): void {
    const copiedLabel = cloneLabel(label);
    const copiedValue = cloneNode(value);
    validateHeaderValue(copiedLabel, copiedValue);
    const identity = labelIdentity(copiedLabel);
    const index = this.#entries.findIndex((entry) => labelIdentity(entry.label) === identity);
    const entry = { label: copiedLabel, value: copiedValue };
    if (index === -1) this.#entries.push(entry);
    else this.#entries[index] = entry;
    this.#original = undefined;
  }

  public keys(): Labels {
    const output = Labels.new();
    for (const entry of this.#entries) output.add(entry.label);
    return output;
  }

  public toNode(): CborValue {
    return this.#original === undefined
      ? map(this.#entries.map(({ label, value }) => [label.toNode(), cloneNode(value)]))
      : cloneNode(this.#original);
  }
}

function cloneHeaderMap(value: HeaderMap): HeaderMap {
  return HeaderMap.from_cbor_bytes(value.to_cbor_bytes());
}

export class ProtectedHeaderMap extends CoseData {
  readonly #bytes: Uint8Array;
  readonly #original: CborValue | undefined;

  private constructor(value: Uint8Array, original?: CborValue) {
    super();
    if (value.length > 0) HeaderMap.from_cbor_bytes(value);
    this.#bytes = copyBytes(value);
    this.#original = original;
  }

  public static new_empty(): ProtectedHeaderMap {
    return new ProtectedHeaderMap(new Uint8Array());
  }

  public static new(value: HeaderMap): ProtectedHeaderMap {
    return value.keys().len() === 0
      ? ProtectedHeaderMap.new_empty()
      : new ProtectedHeaderMap(value.to_cbor_bytes());
  }

  public static parse(value: CborValue): ProtectedHeaderMap {
    const raw = expectBytes(value, "ProtectedHeaderMap");
    return new ProtectedHeaderMap(raw, cloneNode(value));
  }

  public static from_cbor_bytes(value: Uint8Array): ProtectedHeaderMap {
    return ProtectedHeaderMap.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): ProtectedHeaderMap {
    return ProtectedHeaderMap.from_cbor_bytes(hexToBytes(value));
  }

  public deserialized_headers(): HeaderMap {
    return this.#bytes.length === 0 ? HeaderMap.new() : HeaderMap.from_cbor_bytes(this.#bytes);
  }

  public toNode(): CborValue {
    return this.#original === undefined ? bytes(this.#bytes) : cloneNode(this.#original);
  }
}

function cloneProtected(value: ProtectedHeaderMap): ProtectedHeaderMap {
  return ProtectedHeaderMap.from_cbor_bytes(value.to_cbor_bytes());
}

export class Headers extends CoseData {
  readonly #protected: ProtectedHeaderMap;
  readonly #unprotected: HeaderMap;
  readonly #original: CborValue | undefined;

  private constructor(
    protectedHeaders: ProtectedHeaderMap,
    unprotectedHeaders: HeaderMap,
    original?: CborValue,
  ) {
    super();
    this.#protected = cloneProtected(protectedHeaders);
    this.#unprotected = cloneHeaderMap(unprotectedHeaders);
    this.#original = original;
  }

  public static new(
    protectedHeaders: ProtectedHeaderMap,
    unprotectedHeaders: HeaderMap,
  ): Headers {
    return new Headers(protectedHeaders, unprotectedHeaders);
  }

  public static parse(value: CborValue): Headers {
    const node = expectArray(value, 2, "Headers");
    return Headers.fromEmbedded(node.values[0]!, node.values[1]!, cloneNode(value));
  }

  public static from_cbor_bytes(value: Uint8Array): Headers {
    return Headers.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): Headers {
    return Headers.from_cbor_bytes(hexToBytes(value));
  }

  public static fromEmbedded(
    protectedNode: CborValue,
    unprotectedNode: CborValue,
    original?: CborValue,
  ): Headers {
    return new Headers(
      ProtectedHeaderMap.parse(protectedNode),
      HeaderMap.parse(unprotectedNode),
      original,
    );
  }

  public protected(): ProtectedHeaderMap {
    return cloneProtected(this.#protected);
  }

  public unprotected(): HeaderMap {
    return cloneHeaderMap(this.#unprotected);
  }

  public embeddedNodes(): readonly [CborValue, CborValue] {
    return [this.#protected.toNode(), this.#unprotected.toNode()];
  }

  public toNode(): CborValue {
    return this.#original === undefined
      ? array(this.embeddedNodes())
      : cloneNode(this.#original);
  }
}

function cloneHeaders(value: Headers): Headers {
  return Headers.from_cbor_bytes(value.to_cbor_bytes());
}

export class COSESignature extends CoseData {
  readonly #headers: Headers;
  readonly #signature: Uint8Array;
  readonly #original: CborValue | undefined;

  private constructor(headers: Headers, signature: Uint8Array, original?: CborValue) {
    super();
    this.#headers = cloneHeaders(headers);
    this.#signature = copyBytes(signature);
    this.#original = original;
  }

  public static new(headers: Headers, signature: Uint8Array): COSESignature {
    return new COSESignature(headers, signature);
  }

  public static parse(value: CborValue): COSESignature {
    const node = expectArray(value, 3, "COSESignature");
    return new COSESignature(
      Headers.fromEmbedded(node.values[0]!, node.values[1]!),
      expectBytes(node.values[2]!, "COSESignature signature"),
      cloneNode(value),
    );
  }

  public static from_cbor_bytes(value: Uint8Array): COSESignature {
    return COSESignature.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): COSESignature {
    return COSESignature.from_cbor_bytes(hexToBytes(value));
  }

  public headers(): Headers {
    return cloneHeaders(this.#headers);
  }

  public signature(): Uint8Array {
    return copyBytes(this.#signature);
  }

  public toNode(): CborValue {
    if (this.#original !== undefined) return cloneNode(this.#original);
    const [protectedNode, unprotectedNode] = this.#headers.embeddedNodes();
    return array([protectedNode, unprotectedNode, bytes(this.#signature)]);
  }
}

function cloneSignature(value: COSESignature): COSESignature {
  return COSESignature.from_cbor_bytes(value.to_cbor_bytes());
}

export class COSESignatures extends CoseData {
  readonly #values: COSESignature[];
  #original: CborValue | undefined;

  private constructor(values: readonly COSESignature[], original?: CborValue) {
    super();
    this.#values = values.map(cloneSignature);
    this.#original = original;
  }

  public static new(): COSESignatures {
    return new COSESignatures([]);
  }

  public static parse(value: CborValue): COSESignatures {
    if (value.kind !== "array") throw new TypeError("COSESignatures requires a CBOR array");
    return new COSESignatures(value.values.map(COSESignature.parse), cloneNode(value));
  }

  public static from_cbor_bytes(value: Uint8Array): COSESignatures {
    return COSESignatures.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): COSESignatures {
    return COSESignatures.from_cbor_bytes(hexToBytes(value));
  }

  public len(): number {
    return this.#values.length;
  }

  public get(index: number): COSESignature {
    const value = this.#values[index];
    if (value === undefined) throw new RangeError("COSESignatures index out of bounds");
    return cloneSignature(value);
  }

  public add(value: COSESignature): void {
    this.#values.push(cloneSignature(value));
    this.#original = undefined;
  }

  public toNode(): CborValue {
    return this.#original === undefined
      ? array(this.#values.map((value) => value.toNode()))
      : cloneNode(this.#original);
  }
}

function cloneSignatures(value: COSESignatures): COSESignatures {
  return COSESignatures.from_cbor_bytes(value.to_cbor_bytes());
}

export class CounterSignature extends CoseData {
  readonly #signatures: COSESignatures;
  readonly #original: CborValue | undefined;

  private constructor(signatures: COSESignatures, original?: CborValue) {
    super();
    this.#signatures = cloneSignatures(signatures);
    this.#original = original;
  }

  public static new_single(value: COSESignature): CounterSignature {
    const signatures = COSESignatures.new();
    signatures.add(value);
    return new CounterSignature(signatures);
  }

  public static new_multi(value: COSESignatures): CounterSignature {
    return new CounterSignature(value);
  }

  public static parse(value: CborValue): CounterSignature {
    if (
      value.kind === "array" &&
      value.values.length === 3 &&
      value.values[0]?.kind === "bytes" &&
      value.values[1]?.kind === "map" &&
      value.values[2]?.kind === "bytes"
    ) {
      const signature = COSESignature.parse(value);
      const signatures = COSESignatures.new();
      signatures.add(signature);
      return new CounterSignature(signatures, cloneNode(value));
    }
    return new CounterSignature(COSESignatures.parse(value), cloneNode(value));
  }

  public static from_cbor_bytes(value: Uint8Array): CounterSignature {
    return CounterSignature.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): CounterSignature {
    return CounterSignature.from_cbor_bytes(hexToBytes(value));
  }

  public signatures(): COSESignatures {
    return cloneSignatures(this.#signatures);
  }

  public toNode(): CborValue {
    if (this.#original !== undefined) return cloneNode(this.#original);
    return this.#signatures.len() === 1
      ? this.#signatures.get(0).toNode()
      : this.#signatures.toNode();
  }
}

export class COSESign1 extends CoseData {
  readonly #headers: Headers;
  readonly #payload: Uint8Array | undefined;
  readonly #signature: Uint8Array;
  readonly #original: CborValue | undefined;

  private constructor(
    headers: Headers,
    payload: Uint8Array | undefined,
    signature: Uint8Array,
    original?: CborValue,
  ) {
    super();
    this.#headers = cloneHeaders(headers);
    this.#payload = payload === undefined ? undefined : copyBytes(payload);
    this.#signature = copyBytes(signature);
    this.#original = original;
  }

  public static new(
    headers: Headers,
    payload: Uint8Array | undefined,
    signature: Uint8Array,
  ): COSESign1 {
    return new COSESign1(headers, payload, signature);
  }

  public static parse(value: CborValue): COSESign1 {
    const node = expectArray(value, 4, "COSESign1");
    return new COSESign1(
      Headers.fromEmbedded(node.values[0]!, node.values[1]!),
      nullableBytes(node.values[2]!, "COSESign1 payload"),
      expectBytes(node.values[3]!, "COSESign1 signature"),
      cloneNode(value),
    );
  }

  public static from_cbor_bytes(value: Uint8Array): COSESign1 {
    return COSESign1.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): COSESign1 {
    return COSESign1.from_cbor_bytes(hexToBytes(value));
  }

  public headers(): Headers {
    return cloneHeaders(this.#headers);
  }

  public payload(): Uint8Array | undefined {
    return this.#payload === undefined ? undefined : copyBytes(this.#payload);
  }

  public signature(): Uint8Array {
    return copyBytes(this.#signature);
  }

  public signed_data(
    externalAad?: Uint8Array,
    externalPayload?: Uint8Array,
  ): SigStructure {
    const payload = externalPayload ?? this.#payload;
    if (payload === undefined) {
      throw new TypeError("Payload was not present and no external payload was supplied");
    }
    return SigStructure.new(
      SigContext.Signature1,
      this.#headers.protected(),
      externalAad ?? new Uint8Array(),
      payload,
    );
  }

  public toNode(): CborValue {
    if (this.#original !== undefined) return cloneNode(this.#original);
    const [protectedNode, unprotectedNode] = this.#headers.embeddedNodes();
    return array([
      protectedNode,
      unprotectedNode,
      this.#payload === undefined ? { kind: "null" } : bytes(this.#payload),
      bytes(this.#signature),
    ]);
  }
}

export class COSESign extends CoseData {
  readonly #headers: Headers;
  readonly #payload: Uint8Array | undefined;
  readonly #signatures: COSESignatures;
  readonly #original: CborValue | undefined;

  private constructor(
    headers: Headers,
    payload: Uint8Array | undefined,
    signatures: COSESignatures,
    original?: CborValue,
  ) {
    super();
    this.#headers = cloneHeaders(headers);
    this.#payload = payload === undefined ? undefined : copyBytes(payload);
    this.#signatures = cloneSignatures(signatures);
    this.#original = original;
  }

  public static new(
    headers: Headers,
    payload: Uint8Array | undefined,
    signatures: COSESignatures,
  ): COSESign {
    return new COSESign(headers, payload, signatures);
  }

  public static parse(value: CborValue): COSESign {
    const node = expectArray(value, 4, "COSESign");
    return new COSESign(
      Headers.fromEmbedded(node.values[0]!, node.values[1]!),
      nullableBytes(node.values[2]!, "COSESign payload"),
      COSESignatures.parse(node.values[3]!),
      cloneNode(value),
    );
  }

  public static from_cbor_bytes(value: Uint8Array): COSESign {
    return COSESign.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): COSESign {
    return COSESign.from_cbor_bytes(hexToBytes(value));
  }

  public headers(): Headers {
    return cloneHeaders(this.#headers);
  }

  public payload(): Uint8Array | undefined {
    return this.#payload === undefined ? undefined : copyBytes(this.#payload);
  }

  public signatures(): COSESignatures {
    return cloneSignatures(this.#signatures);
  }

  public toNode(): CborValue {
    if (this.#original !== undefined) return cloneNode(this.#original);
    const [protectedNode, unprotectedNode] = this.#headers.embeddedNodes();
    return array([
      protectedNode,
      unprotectedNode,
      this.#payload === undefined ? { kind: "null" } : bytes(this.#payload),
      this.#signatures.toNode(),
    ]);
  }
}

export enum SignedMessageKind {
  COSESIGN = 0,
  COSESIGN1 = 1,
}

export class SignedMessage extends CoseData {
  readonly #value: COSESign | COSESign1;
  readonly #kind: SignedMessageKind;

  private constructor(value: COSESign | COSESign1, kind: SignedMessageKind) {
    super();
    this.#value = kind === SignedMessageKind.COSESIGN
      ? COSESign.from_cbor_bytes(value.to_cbor_bytes())
      : COSESign1.from_cbor_bytes(value.to_cbor_bytes());
    this.#kind = kind;
  }

  public static new_cose_sign(value: COSESign): SignedMessage {
    return new SignedMessage(value, SignedMessageKind.COSESIGN);
  }

  public static new_cose_sign1(value: COSESign1): SignedMessage {
    return new SignedMessage(value, SignedMessageKind.COSESIGN1);
  }

  public static parse(value: CborValue): SignedMessage {
    const node = expectArray(value, 4, "SignedMessage");
    return node.values[3]?.kind === "bytes"
      ? SignedMessage.new_cose_sign1(COSESign1.parse(value))
      : SignedMessage.new_cose_sign(COSESign.parse(value));
  }

  public static from_cbor_bytes(value: Uint8Array): SignedMessage {
    return SignedMessage.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): SignedMessage {
    return SignedMessage.from_cbor_bytes(hexToBytes(value));
  }

  public static from_user_facing_encoding(value: string): SignedMessage {
    return SignedMessage.from_cbor_bytes(decodeUserFacingMessage(value));
  }

  public to_user_facing_encoding(): string {
    return encodeUserFacingMessage(this.to_cbor_bytes());
  }

  public kind(): SignedMessageKind {
    return this.#kind;
  }

  public as_cose_sign(): COSESign | undefined {
    return this.#kind === SignedMessageKind.COSESIGN
      ? COSESign.from_cbor_bytes(this.#value.to_cbor_bytes())
      : undefined;
  }

  public as_cose_sign1(): COSESign1 | undefined {
    return this.#kind === SignedMessageKind.COSESIGN1
      ? COSESign1.from_cbor_bytes(this.#value.to_cbor_bytes())
      : undefined;
  }

  public toNode(): CborValue {
    return this.#value.toNode();
  }
}

export enum SigContext {
  Signature = 0,
  Signature1 = 1,
  CounterSignature = 2,
}

function contextText(value: SigContext): string {
  switch (value) {
    case SigContext.Signature:
      return "Signature";
    case SigContext.Signature1:
      return "Signature1";
    case SigContext.CounterSignature:
      return "CounterSignature";
  }
}

function parseContext(value: string): SigContext {
  switch (value) {
    case "Signature":
      return SigContext.Signature;
    case "Signature1":
      return SigContext.Signature1;
    case "CounterSignature":
      return SigContext.CounterSignature;
    default:
      throw new TypeError(`Unknown signature context ${value}`);
  }
}

export class SigStructure extends CoseData {
  readonly #context: SigContext;
  readonly #bodyProtected: ProtectedHeaderMap;
  #signProtected: ProtectedHeaderMap | undefined;
  readonly #externalAad: Uint8Array;
  readonly #payload: Uint8Array;
  #original: CborValue | undefined;

  private constructor(
    context: SigContext,
    bodyProtected: ProtectedHeaderMap,
    externalAad: Uint8Array,
    payload: Uint8Array,
    signProtected?: ProtectedHeaderMap,
    original?: CborValue,
  ) {
    super();
    this.#context = context;
    this.#bodyProtected = cloneProtected(bodyProtected);
    this.#signProtected = signProtected === undefined ? undefined : cloneProtected(signProtected);
    this.#externalAad = copyBytes(externalAad);
    this.#payload = copyBytes(payload);
    this.#original = original;
  }

  public static new(
    context: SigContext,
    bodyProtected: ProtectedHeaderMap,
    externalAad: Uint8Array,
    payload: Uint8Array,
  ): SigStructure {
    return new SigStructure(context, bodyProtected, externalAad, payload);
  }

  public static parse(value: CborValue): SigStructure {
    if (value.kind !== "array" || value.values.length < 4 || value.values.length > 5) {
      throw new TypeError("SigStructure requires a four- or five-element CBOR array");
    }
    const context = parseContext(expectText(value.values[0]!, "SigStructure context"));
    const bodyProtected = ProtectedHeaderMap.parse(value.values[1]!);
    const hasSignProtected = value.values.length === 5;
    const offset = hasSignProtected ? 1 : 0;
    return new SigStructure(
      context,
      bodyProtected,
      expectBytes(value.values[2 + offset]!, "SigStructure external AAD"),
      expectBytes(value.values[3 + offset]!, "SigStructure payload"),
      hasSignProtected ? ProtectedHeaderMap.parse(value.values[2]!) : undefined,
      cloneNode(value),
    );
  }

  public static from_cbor_bytes(value: Uint8Array): SigStructure {
    return SigStructure.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): SigStructure {
    return SigStructure.from_cbor_bytes(hexToBytes(value));
  }

  public context(): SigContext {
    return this.#context;
  }

  public body_protected(): ProtectedHeaderMap {
    return cloneProtected(this.#bodyProtected);
  }

  public sign_protected(): ProtectedHeaderMap | undefined {
    return this.#signProtected === undefined ? undefined : cloneProtected(this.#signProtected);
  }

  public external_aad(): Uint8Array {
    return copyBytes(this.#externalAad);
  }

  public payload(): Uint8Array {
    return copyBytes(this.#payload);
  }

  public set_sign_protected(value: ProtectedHeaderMap): void {
    this.#signProtected = cloneProtected(value);
    this.#original = undefined;
  }

  public toNode(): CborValue {
    if (this.#original !== undefined) return cloneNode(this.#original);
    const values = [
      text(contextText(this.#context)),
      this.#bodyProtected.toNode(),
    ];
    if (this.#signProtected !== undefined) values.push(this.#signProtected.toNode());
    values.push(bytes(this.#externalAad), bytes(this.#payload));
    return array(values);
  }
}

function validateKeyValue(label: Label, value: CborValue): void {
  switch (labelInteger(label)) {
    case 1n:
    case 3n:
      Label.parse(value);
      return;
    case 2n:
    case 5n:
      expectBytes(value, "COSEKey value");
      return;
    case 4n:
      Labels.parse(value);
      return;
    default:
      return;
  }
}

export class COSEKey extends CoseData {
  readonly #entries: HeaderEntry[];
  #original: CborValue | undefined;

  private constructor(entries: readonly HeaderEntry[], original?: CborValue) {
    super();
    this.#entries = entries.map(({ label, value }) => ({
      label: cloneLabel(label),
      value: cloneNode(value),
    }));
    this.#original = original;
  }

  public static new(keyType: Label): COSEKey {
    const value = new COSEKey([]);
    value.set_key_type(keyType);
    return value;
  }

  public static parse(value: CborValue): COSEKey {
    const node = expectMap(value, "COSEKey");
    const entries: HeaderEntry[] = [];
    const seen = new Set<string>();
    for (const [keyNode, valueNode] of node.entries) {
      const label = Label.parse(keyNode);
      const identity = labelIdentity(label);
      if (seen.has(identity)) throw new TypeError(`Duplicate COSEKey label ${identity}`);
      seen.add(identity);
      validateKeyValue(label, valueNode);
      entries.push({ label, value: cloneNode(valueNode) });
    }
    if (!seen.has("int:1")) throw new TypeError("COSEKey is missing required key type label 1");
    return new COSEKey(entries, cloneNode(value));
  }

  public static from_cbor_bytes(value: Uint8Array): COSEKey {
    return COSEKey.parse(decodeCbor(value));
  }

  public static from_cbor_hex(value: string): COSEKey {
    return COSEKey.from_cbor_bytes(hexToBytes(value));
  }

  public set_key_type(value: Label): void {
    this.set_header(integerLabel(1n), value.toNode());
  }

  public key_type(): Label {
    const value = this.header(integerLabel(1n));
    if (value === undefined) throw new TypeError("COSEKey is missing its key type");
    return Label.parse(value);
  }

  public set_key_id(value: Uint8Array): void {
    this.set_header(integerLabel(2n), bytes(value));
  }

  public key_id(): Uint8Array | undefined {
    const value = this.header(integerLabel(2n));
    return value === undefined ? undefined : expectBytes(value, "COSEKey key ID");
  }

  public set_algorithm_id(value: Label): void {
    this.set_header(integerLabel(3n), value.toNode());
  }

  public algorithm_id(): Label | undefined {
    const value = this.header(integerLabel(3n));
    return value === undefined ? undefined : Label.parse(value);
  }

  public set_key_ops(value: Labels): void {
    this.set_header(integerLabel(4n), value.toNode());
  }

  public key_ops(): Labels | undefined {
    const value = this.header(integerLabel(4n));
    return value === undefined ? undefined : Labels.parse(value);
  }

  public set_base_init_vector(value: Uint8Array): void {
    this.set_header(integerLabel(5n), bytes(value));
  }

  public base_init_vector(): Uint8Array | undefined {
    const value = this.header(integerLabel(5n));
    return value === undefined ? undefined : expectBytes(value, "COSEKey base initialization vector");
  }

  public header(label: Label): CborValue | undefined {
    const identity = labelIdentity(label);
    const found = this.#entries.find((entry) => labelIdentity(entry.label) === identity);
    return found === undefined ? undefined : cloneNode(found.value);
  }

  public set_header(label: Label, value: CborValue): void {
    const copiedLabel = cloneLabel(label);
    const copiedValue = cloneNode(value);
    validateKeyValue(copiedLabel, copiedValue);
    const identity = labelIdentity(copiedLabel);
    const index = this.#entries.findIndex((entry) => labelIdentity(entry.label) === identity);
    const entry = { label: copiedLabel, value: copiedValue };
    if (index === -1) this.#entries.push(entry);
    else this.#entries[index] = entry;
    this.#original = undefined;
  }

  public toNode(): CborValue {
    return this.#original === undefined
      ? map(this.#entries.map(({ label, value }) => [label.toNode(), cloneNode(value)]))
      : cloneNode(this.#original);
  }
}
