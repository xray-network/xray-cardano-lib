import { decodeCbor, encodeCbor, type CborHeadWidth, type CborValue } from "@xray-network/xray-cardano-lib-core";
import { Transaction } from "@xray-network/xray-cardano-lib-chain/conway";

export type CIP21ViolationCode =
  | "NON_CANONICAL_INTEGER" | "NON_CANONICAL_LENGTH" | "UNSORTED_MAP" | "INDEFINITE_LENGTH"
  | "INCONSISTENT_SET_TAG" | "UNSUPPORTED_BODY_ENTRY" | "INTEGER_OUT_OF_RANGE" | "TOO_MANY_ELEMENTS" | "EMPTY_OPTIONAL"
  | "LEGACY_EMPTY_MULTI_ASSET" | "EMPTY_INLINE_DATUM" | "EMPTY_SCRIPT_REF" | "DUPLICATE_POLICY" | "DUPLICATE_ASSET"
  | "UNSUPPORTED_CERTIFICATE" | "POOL_REGISTRATION_COMBINATION" | "DUPLICATE_WITHDRAWAL" | "MULTIPLE_VOTERS" | "MULTIPLE_VOTES"
  | "INVALID_CATALYST_AUXILIARY_DATA" | "DIAGNOSTIC_LIMIT";
export interface CIP21Violation { readonly code: CIP21ViolationCode; readonly path: string; readonly message: string }
export interface CIP21Options { readonly auxiliaryDataMode?: "hash-only" | "catalyst-registration" }

function shortest(value: bigint): CborHeadWidth { return value < 24n ? 0 : value <= 0xffn ? 1 : value <= 0xffffn ? 2 : value <= 0xffff_ffffn ? 4 : 8; }
function compare(left: Uint8Array, right: Uint8Array): number {
  if (left.length !== right.length) return left.length - right.length;
  for (let index = 0; index < left.length; index += 1) if (left[index] !== right[index]) return (left[index] ?? 0) - (right[index] ?? 0);
  return 0;
}
function childPath(path: string, child: string | number): string { return `${path}/${String(child).replaceAll("~", "~0").replaceAll("/", "~1")}`; }

/** Pure diagnostics for the deterministic rules captured by CIP-21; this is not a device-support promise. */
export function diagnose_cip21_transaction(transaction: Transaction, options: CIP21Options = {}): readonly CIP21Violation[] {
  const root = decodeCbor(transaction.to_cbor_bytes()), violations: CIP21Violation[] = [];
  let limited = false;
  const add = (code: CIP21ViolationCode, path: string, message: string): void => {
    if (violations.length < 4096) violations.push(Object.freeze({ code, path, message })); else limited = true;
  };
  const walk = (value: CborValue, path: string): void => {
    if (value.kind === "unsigned" || value.kind === "negative") {
      const magnitude = value.kind === "unsigned" ? value.value : -1n - value.value;
      if (value.encoding.width !== shortest(magnitude)) add("NON_CANONICAL_INTEGER", path, "integer does not use its shortest CBOR head");
      if (value.kind === "unsigned" ? value.value > 0xffff_ffff_ffff_ffffn : value.value < -0x8000_0000_0000_0000n) add("INTEGER_OUT_OF_RANGE", path, "integer is outside the CIP-21 64-bit range");
    }
    if (value.kind === "bytes" || value.kind === "text") {
      if (value.encoding.kind === "indefinite") add("INDEFINITE_LENGTH", path, "indefinite strings are not CIP-21 compatible");
      else if (value.encoding.width !== shortest(BigInt(value.kind === "bytes" ? value.value.length : new TextEncoder().encode(value.value).length))) add("NON_CANONICAL_LENGTH", path, "string length does not use its shortest CBOR head");
    }
    if (value.kind === "array" || value.kind === "map") {
      if (value.encoding.kind === "indefinite") add("INDEFINITE_LENGTH", path, "indefinite containers are not CIP-21 compatible");
      else if (value.encoding.width !== shortest(BigInt(value.kind === "array" ? value.values.length : value.entries.length))) add("NON_CANONICAL_LENGTH", path, "collection length does not use its shortest CBOR head");
    }
    if (value.kind === "array") value.values.forEach((child, index) => walk(child, childPath(path, index)));
    if (value.kind === "tag") { if (value.encoding.width !== shortest(value.tag)) add("NON_CANONICAL_INTEGER", path, "tag does not use its shortest CBOR head"); walk(value.value, path); }
    if (value.kind === "map") {
      let prior: Uint8Array | undefined;
      for (const [index, [key, item]] of value.entries.entries()) {
        const encoded = encodeCbor(key, { mode: "canonical" });
        if (prior !== undefined && compare(prior, encoded) >= 0) add("UNSORTED_MAP", childPath(path, index), "map keys are not in canonical order");
        prior = encoded; walk(key, childPath(path, `${index}:key`)); walk(item, childPath(path, index));
      }
    }
  };
  walk(root, "");
  const numericEntries = (value: CborValue): Map<bigint, CborValue> => {
    const result = new Map<bigint, CborValue>(); if (value.kind !== "map") return result;
    for (const [key, item] of value.entries) if (key.kind === "unsigned" && !result.has(key.value)) result.set(key.value, item);
    return result;
  };
  const collection = (value: CborValue): { value: CborValue; tagged: boolean } => value.kind === "tag" && value.tag === 258n ? { value: value.value, tagged: true } : { value, tagged: false };
  const size = (value: CborValue): number => value.kind === "array" ? value.values.length : value.kind === "map" ? value.entries.length : 0;
  const empty = (value: CborValue): boolean => (value.kind === "array" && value.values.length === 0) || (value.kind === "map" && value.entries.length === 0) || (value.kind === "bytes" && value.value.length === 0);
  const identity = (value: CborValue): string => Array.from(encodeCbor(value, { mode: "canonical" })).join(",");
  const duplicateMap = (value: CborValue, path: string, code: "DUPLICATE_POLICY" | "DUPLICATE_ASSET" | "DUPLICATE_WITHDRAWAL"): void => {
    if (value.kind !== "map") return; const seen = new Set<string>();
    value.entries.forEach(([key], index) => { const item = identity(key); if (seen.has(item)) add(code, childPath(path, index), "duplicate map key"); seen.add(item); });
  };
  const checkLimit = (value: CborValue, path: string): void => { const count = size(collection(value).value); if (count > 65_535) add("TOO_MANY_ELEMENTS", path, "collection exceeds 65535 elements"); };
  const setTags: Array<{ path: string; tagged: boolean }> = [];
  const semanticSet = (value: CborValue | undefined, path: string): CborValue | undefined => { if (value === undefined) return undefined; const set = collection(value); setTags.push({ path, tagged: set.tagged }); checkLimit(value, path); return set.value; };
  const checkMultiAsset = (value: CborValue, path: string): void => {
    if (value.kind !== "map") return; checkLimit(value, path); duplicateMap(value, path, "DUPLICATE_POLICY");
    value.entries.forEach(([, assets], index) => { const assetPath = childPath(path, index); checkLimit(assets, assetPath); duplicateMap(assets, assetPath, "DUPLICATE_ASSET"); });
  };
  const outputRestrictions = (output: CborValue, path: string): boolean => {
    let restricted = false;
    if (output.kind === "array") {
      const amount = output.values[1]; if (amount?.kind === "array" && amount.values[1]?.kind === "map") {
        const assets = amount.values[1]; checkMultiAsset(assets, `${path}/amount/assets`); if (assets.entries.length === 0) add("LEGACY_EMPTY_MULTI_ASSET", `${path}/amount`, "legacy coin-only output must encode its amount as coin");
      }
      if (output.values[2] !== undefined) restricted = true;
      return restricted;
    }
    if (output.kind !== "map") return restricted;
    const fields = numericEntries(output), amount = fields.get(1n);
    if (amount?.kind === "array" && amount.values[1] !== undefined) checkMultiAsset(amount.values[1], `${path}/amount/assets`);
    const datum = fields.get(2n); if (datum !== undefined) {
      restricted = true; if (datum.kind === "array" && datum.values[0]?.kind === "unsigned" && datum.values[0].value === 1n && (datum.values[1] === undefined || empty(datum.values[1]))) add("EMPTY_INLINE_DATUM", `${path}/datum`, "inline datum content must be nonempty");
    }
    const script = fields.get(3n); if (script !== undefined) {
      restricted = true; const content = script.kind === "tag" ? script.value : script; if (empty(content)) add("EMPTY_SCRIPT_REF", `${path}/scriptRef`, "reference script content must be nonempty");
    }
    return restricted;
  };
  if (root.kind === "array" && root.values[0]?.kind === "map") {
    const body = root.values[0], fields = numericEntries(body), optionalCollections = new Set([4n, 5n, 7n, 8n, 9n, 11n, 13n, 14n, 18n, 19n, 20n]);
    for (const [key, value] of body.entries) if (key.kind === "unsigned") {
      const path = `/body/${key.value}`;
      if (key.value === 6n || key.value === 20n) add("UNSUPPORTED_BODY_ENTRY", path, "body entry is unsupported by CIP-21");
      if (optionalCollections.has(key.value) && empty(collection(value).value)) add("EMPTY_OPTIONAL", path, "optional collection must be absent instead of empty");
    }
    const inputs = semanticSet(fields.get(0n), "/body/0"), certificates = semanticSet(fields.get(4n), "/body/4"), collateral = semanticSet(fields.get(13n), "/body/13"), signers = semanticSet(fields.get(14n), "/body/14"), references = semanticSet(fields.get(18n), "/body/18");
    void inputs; void collateral; void signers; void references;
    const outputs = fields.get(1n); if (outputs !== undefined) checkLimit(outputs, "/body/1");
    let outputHasPoolConflict = false; if (outputs?.kind === "array") outputs.values.forEach((output, index) => { outputHasPoolConflict = outputRestrictions(output, `/body/1/${index}`) || outputHasPoolConflict; });
    const mint = fields.get(9n); if (mint !== undefined) checkMultiAsset(mint, "/body/9");
    const withdrawals = fields.get(5n); if (withdrawals !== undefined) { checkLimit(withdrawals, "/body/5"); duplicateMap(withdrawals, "/body/5", "DUPLICATE_WITHDRAWAL"); }
    const votes = fields.get(19n); if (votes?.kind === "map") {
      if (votes.entries.length > 1) add("MULTIPLE_VOTERS", "/body/19", "CIP-21 permits at most one voter");
      votes.entries.forEach(([, procedures], index) => { if (procedures.kind === "map" && procedures.entries.length > 1) add("MULTIPLE_VOTES", `/body/19/${index}`, "CIP-21 permits at most one vote"); });
    }
    let poolRegistration = false; const certificateValues = certificates?.kind === "array" ? certificates.values : [];
    certificateValues.forEach((certificate, index) => {
      if (certificate.kind !== "array" || certificate.values[0]?.kind !== "unsigned") return; const kind = certificate.values[0].value, path = `/body/4/${index}`;
      if ([5n, 6n, 10n, 11n, 12n, 13n].includes(kind)) add("UNSUPPORTED_CERTIFICATE", path, `certificate alternative ${kind} is unsupported by CIP-21`);
      if (kind === 3n) { poolRegistration = true; const parameters = certificate.values[1]; if (parameters?.kind === "array") { const owners = parameters.values[6], relays = parameters.values[7]; if (owners !== undefined) { semanticSet(owners, `${path}/owners`); } if (relays !== undefined) checkLimit(relays, `${path}/relays`); } }
    });
    if (poolRegistration) {
      certificateValues.forEach((certificate, index) => { if (!(certificate.kind === "array" && certificate.values[0]?.kind === "unsigned" && certificate.values[0].value === 3n) || certificateValues.length > 1) add("POOL_REGISTRATION_COMBINATION", `/body/4/${index}`, "pool registration must be the only certificate and incompatible body feature"); });
      for (const key of [5n, 9n, 11n, 13n, 14n, 16n, 17n, 18n, 19n, 21n, 22n]) if (fields.has(key)) add("POOL_REGISTRATION_COMBINATION", `/body/${key}`, "body entry cannot accompany pool registration");
      if (outputHasPoolConflict) add("POOL_REGISTRATION_COMBINATION", "/body/1", "output datum or reference script cannot accompany pool registration");
    }
  }
  if (root.kind === "array" && root.values[1]?.kind === "map") {
    let witnessCount = 0; root.values[1].entries.forEach(([key, witnesses]) => { if (key.kind !== "unsigned") return; const path = `/witnessSet/${key.value}`, set = key.value === 5n ? collection(witnesses).value : semanticSet(witnesses, path); witnessCount += set === undefined ? 0 : size(set); });
    if (witnessCount > 65_535) add("TOO_MANY_ELEMENTS", "/witnessSet", "total witness count exceeds 65535");
  }
  if (setTags.some((item) => item.tagged) && setTags.some((item) => !item.tagged)) for (const item of setTags) add("INCONSISTENT_SET_TAG", item.path, "semantic sets must consistently use or omit tag 258");
  if (options.auxiliaryDataMode === "catalyst-registration" && root.kind === "array") {
    const auxiliary = root.values[3];
    if (auxiliary?.kind !== "array" || auxiliary.values.length !== 2 || auxiliary.values[1]?.kind !== "array" || auxiliary.values[1].values.length !== 0) add("INVALID_CATALYST_AUXILIARY_DATA", "/auxiliaryData", "Catalyst auxiliary data must be [metadata, []]");
  }
  violations.sort((left, right) => left.path.localeCompare(right.path) || left.code.localeCompare(right.code));
  if (limited) violations.push(Object.freeze({ code: "DIAGNOSTIC_LIMIT", path: "", message: "diagnostic limit of 4096 was reached" }));
  return Object.freeze(violations);
}
