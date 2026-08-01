import {
  CardanoError,
  bytesEqual,
  decodeCbor,
  encodeCbor,
  type CborValue,
} from "@xray-network/xray-cardano-lib-core";
import { blake2b224, blake2b256 } from "@xray-network/xray-cardano-lib-crypto";
import { encodePlutusData } from "../uplc/flat.js";

export interface ContextRedeemer {
  readonly tag: number;
  readonly index: bigint;
  readonly data: CborValue;
}

export interface ContextUtxo {
  readonly input: CborValue;
  readonly output: CborValue;
}

const definite = { kind: "definite", width: 0 } as const;

export function makeScriptContext(
  current: ContextRedeemer,
  redeemers: readonly ContextRedeemer[],
  body: Extract<CborValue, { kind: "map" }>,
  witnesses: Extract<CborValue, { kind: "map" }>,
  utxos: readonly ContextUtxo[],
  slotConfig: readonly [bigint, bigint, bigint],
  protocol: number,
  language: 0 | 1 | 2,
): CborValue {
  if (language === 0 && protocol >= 7) assertV1Features(body, utxos);
  if (protocol >= 11 && language === 2) assertDisjointInputs(body);

  const txInfo = language === 0
    ? makeV1TxInfo(body, witnesses, utxos, slotConfig, protocol)
    : language === 1
      ? makeV2TxInfo(body, witnesses, utxos, redeemers, slotConfig, protocol)
      : makeV3TxInfo(body, witnesses, utxos, redeemers, slotConfig, protocol);
  const purpose = scriptPurpose(current, body, utxos, language, protocol);

  if (language !== 2) return constr(0n, [txInfo, purpose]);
  const datum = current.tag === 0 ? currentSpendingDatum(current, body, utxos, witnesses) : undefined;
  const scriptInfo = current.tag === 0
    ? constr(1n, purposeFields(purpose).concat([maybe(datum)]))
    : constr(BigInt(current.tag), purposeFields(purpose));
  return constr(0n, [txInfo, current.data, scriptInfo]);
}

function makeV1TxInfo(
  body: Extract<CborValue, { kind: "map" }>,
  witnesses: Extract<CborValue, { kind: "map" }>,
  utxos: readonly ContextUtxo[],
  slots: readonly [bigint, bigint, bigint],
  protocol: number,
): CborValue {
  return constr(0n, [
    arrayNode(inputInfos(body, utxos, 0n, false)),
    arrayNode(outputs(body).map((output) => txOut(output, false))),
    valueData(mapGet(body, 2n), true, true),
    mintData(mapGet(body, 9n), true),
    arrayNode(certificates(body).map((certificate) => legacyCertificate(certificate, protocol))),
    arrayNode(withdrawals(body).map(([account, amount]) =>
      constr(0n, [stakingCredentialFromReward(account), integerFrom(amount)]))),
    validityRange(body, slots),
    arrayNode(signatories(body).map(bytesNode)),
    arrayNode(datums(witnesses).map((datum) =>
      constr(0n, [bytesNode(blake2b256(encodePlutusData(datum))), datum]))),
    txId(body, false),
  ]);
}

function makeV2TxInfo(
  body: Extract<CborValue, { kind: "map" }>,
  witnesses: Extract<CborValue, { kind: "map" }>,
  utxos: readonly ContextUtxo[],
  redeemers: readonly ContextRedeemer[],
  slots: readonly [bigint, bigint, bigint],
  protocol: number,
): CborValue {
  return constr(0n, [
    arrayNode(inputInfos(body, utxos, 0n, true)),
    arrayNode(inputInfos(body, utxos, 18n, true)),
    arrayNode(outputs(body).map((output) => txOut(output, true))),
    valueData(mapGet(body, 2n), true, true),
    mintData(mapGet(body, 9n), true),
    arrayNode(certificates(body).map((certificate) => legacyCertificate(certificate, protocol))),
    mapNode(withdrawals(body).map(([account, amount]) =>
      [stakingCredentialFromReward(account), integerFrom(amount)])),
    validityRange(body, slots),
    arrayNode(signatories(body).map(bytesNode)),
    mapNode(redeemers.map((redeemer) =>
      [scriptPurpose(redeemer, body, utxos, 1, protocol), redeemer.data])),
    mapNode(datums(witnesses).map((datum) =>
      [bytesNode(blake2b256(encodePlutusData(datum))), datum])),
    txId(body, false),
  ]);
}

function makeV3TxInfo(
  body: Extract<CborValue, { kind: "map" }>,
  witnesses: Extract<CborValue, { kind: "map" }>,
  utxos: readonly ContextUtxo[],
  redeemers: readonly ContextRedeemer[],
  slots: readonly [bigint, bigint, bigint],
  protocol: number,
): CborValue {
  const treasury = mapGet(body, 21n);
  const donation = mapGet(body, 22n);
  return constr(0n, [
    arrayNode(inputInfos(body, utxos, 0n, true, true)),
    arrayNode(inputInfos(body, utxos, 18n, true, true)),
    arrayNode(outputs(body).map((output) => txOut(output, true))),
    integerFrom(mapGet(body, 2n)),
    mintData(mapGet(body, 9n), false),
    arrayNode(certificates(body).map((certificate) => v3Certificate(certificate, protocol))),
    mapNode(withdrawals(body).map(([account, amount]) =>
      [credentialFromReward(account), integerFrom(amount)])),
    validityRange(body, slots),
    arrayNode(signatories(body).map(bytesNode)),
    mapNode(redeemers.map((redeemer) =>
      [scriptPurpose(redeemer, body, utxos, 2, protocol), redeemer.data])),
    mapNode(datums(witnesses).map((datum) =>
      [bytesNode(blake2b256(encodePlutusData(datum))), datum])),
    txId(body, true),
    votesData(mapGet(body, 19n)),
    arrayNode(proposals(body).map((proposal) => proposalData(proposal))),
    maybeInteger(treasury),
    donation?.kind === "unsigned" && donation.value !== 0n ? just(integerNode(donation.value)) : nothing(),
  ]);
}

function inputInfos(
  body: Extract<CborValue, { kind: "map" }>,
  utxos: readonly ContextUtxo[],
  key: bigint,
  v2Output: boolean,
  v3Id = false,
): CborValue[] {
  const value = mapGet(body, key);
  if (value === undefined) return [];
  const values = setValues(value);
  if (values === undefined) contextError(`transaction body key ${key} must be an input set`);
  return [...values].sort(compareCbor).map((input) => {
    const resolved = utxos.find((candidate) => compareCbor(candidate.input, input) === 0);
    if (resolved === undefined) contextError("transaction input does not exist in the supplied UTxO");
    return constr(0n, [txOutRef(input, v3Id), txOut(resolved.output, v2Output)]);
  });
}

function txOut(output: CborValue, v2: boolean): CborValue {
  const address = outputField(output, 0n, 0);
  const value = outputField(output, 1n, 1);
  if (address?.kind !== "bytes" || value === undefined) contextError("invalid transaction output");
  if (!v2) {
    const datum = output.kind === "array" ? output.values[2] : undefined;
    return constr(0n, [
      addressData(address.value),
      valueData(value, true, true),
      datum?.kind === "bytes" ? just(bytesNode(datum.value)) : nothing(),
    ]);
  }
  return constr(0n, [
    addressData(address.value),
    valueData(value, true, true),
    outputDatum(output),
    referenceScriptHash(output),
  ]);
}

function outputDatum(output: CborValue): CborValue {
  if (output.kind === "array") {
    const hash = output.values[2];
    return hash?.kind === "bytes" ? constr(1n, [bytesNode(hash.value)]) : constr(0n, []);
  }
  if (output.kind !== "map") return constr(0n, []);
  const datum = mapGet(output, 2n);
  if (datum === undefined) return constr(0n, []);
  if (
    datum.kind !== "array" ||
    datum.values.length !== 2 ||
    datum.values[0]?.kind !== "unsigned"
  ) contextError("invalid Babbage output datum option");
  if (datum.values[0].value === 0n && datum.values[1]?.kind === "bytes") {
    return constr(1n, [bytesNode(datum.values[1].value)]);
  }
  if (datum.values[0].value === 1n && datum.values[1] !== undefined) {
    return constr(2n, [datum.values[1]]);
  }
  contextError("invalid Babbage output datum option");
}

function referenceScriptHash(output: CborValue): CborValue {
  if (output.kind !== "map") return nothing();
  const reference = mapGet(output, 3n);
  if (reference === undefined) return nothing();
  if (reference.kind !== "tag" || reference.tag !== 24n || reference.value.kind !== "bytes") {
    contextError("invalid reference script");
  }
  let script: CborValue;
  try {
    script = decodeCbor(reference.value.value);
  } catch (cause) {
    throw new CardanoError("EVALUATE", "invalid reference script", { cause });
  }
  if (
    script.kind !== "array" ||
    script.values.length !== 2 ||
    script.values[0]?.kind !== "unsigned" ||
    script.values[1]?.kind !== "bytes"
  ) contextError("invalid reference script");
  const prefix = Number(script.values[0].value);
  if (prefix < 0 || prefix > 3) contextError("invalid reference script language");
  const hashInput = new Uint8Array(script.values[1].value.length + 1);
  hashInput[0] = prefix;
  hashInput.set(script.values[1].value, 1);
  return just(bytesNode(blake2b224(hashInput)));
}

function addressData(address: Uint8Array): CborValue {
  if (address.length < 29) contextError("Byron or malformed address cannot appear in a Plutus context");
  const kind = (address[0] ?? 0) >> 4;
  const payment = credential(kind % 2 === 1, address.slice(1, 29));
  let stake: CborValue;
  if (kind <= 3) {
    if (address.length !== 57) contextError("invalid base address");
    stake = just(constr(0n, [credential(kind >= 2, address.slice(29, 57))]));
  } else if (kind === 4 || kind === 5) {
    const [slot, transaction, certificate, consumed] = decodePointer(address.slice(29));
    if (consumed !== address.length - 29) contextError("invalid pointer address");
    stake = just(constr(1n, [integerNode(slot), integerNode(transaction), integerNode(certificate)]));
  } else if (kind === 6 || kind === 7) {
    if (address.length !== 29) contextError("invalid enterprise address");
    stake = nothing();
  } else {
    contextError("Byron or reward address cannot appear in a transaction output");
  }
  return constr(0n, [payment, stake]);
}

function decodePointer(bytes: Uint8Array): [bigint, bigint, bigint, number] {
  let offset = 0;
  const read = (): bigint => {
    let output = 0n;
    let groups = 0;
    for (;;) {
      if (offset >= bytes.length || groups >= 10) contextError("invalid pointer address");
      const octet = bytes[offset++] as number;
      output = (output << 7n) | BigInt(octet & 0x7f);
      groups += 1;
      if ((octet & 0x80) === 0) return output;
    }
  };
  return [read(), read(), read(), offset];
}

function credential(script: boolean, hash: Uint8Array): CborValue {
  if (hash.length !== 28) contextError("credential hash must contain 28 bytes");
  return constr(script ? 1n : 0n, [bytesNode(hash)]);
}

function credentialNode(value: CborValue | undefined): CborValue {
  if (
    value?.kind !== "array" ||
    value.values.length !== 2 ||
    value.values[0]?.kind !== "unsigned" ||
    value.values[0].value > 1n ||
    value.values[1]?.kind !== "bytes"
  ) contextError("invalid ledger credential");
  return credential(value.values[0].value === 1n, value.values[1].value);
}

function credentialFromReward(account: CborValue): CborValue {
  if (account.kind !== "bytes" || account.value.length !== 29) contextError("invalid reward account");
  const kind = (account.value[0] ?? 0) >> 4;
  if (kind !== 0xe && kind !== 0xf) contextError("invalid reward account");
  return credential(kind === 0xf, account.value.slice(1));
}

function stakingCredentialFromReward(account: CborValue): CborValue {
  return constr(0n, [credentialFromReward(account)]);
}

function valueData(value: CborValue | undefined, includeAda: boolean, requireCoin: boolean): CborValue {
  const { coin, assets } = readValue(value, requireCoin);
  const entries: [CborValue, CborValue][] = [];
  if (includeAda) entries.push([bytesNode(new Uint8Array()), mapNode([
    [bytesNode(new Uint8Array()), integerNode(coin)],
  ])]);
  for (const [policy, tokens] of assets) {
    if (policy.kind !== "bytes" || tokens.kind !== "map") contextError("invalid multiasset value");
    entries.push([bytesNode(policy.value), mapNode(tokens.entries.map(([asset, quantity]) => {
      if (asset.kind !== "bytes") contextError("invalid asset name");
      return [bytesNode(asset.value), integerFrom(quantity)];
    }))]);
  }
  return mapNode(entries);
}

function mintData(value: CborValue | undefined, includeAda: boolean): CborValue {
  if (value === undefined) return valueData(undefined, includeAda, false);
  if (value.kind !== "map") contextError("mint must be a multiasset map");
  return valueData(arrayNode([unsignedNode(0n), value]), includeAda, true);
}

function readValue(
  value: CborValue | undefined,
  requireCoin: boolean,
): { coin: bigint; assets: readonly (readonly [CborValue, CborValue])[] } {
  if (value === undefined) {
    if (requireCoin) contextError("missing coin value");
    return { coin: 0n, assets: [] };
  }
  if (value.kind === "unsigned") return { coin: value.value, assets: [] };
  if (
    value.kind === "array" &&
    value.values.length === 2 &&
    value.values[0]?.kind === "unsigned" &&
    value.values[1]?.kind === "map"
  ) return { coin: value.values[0].value, assets: value.values[1].entries };
  contextError("invalid ledger value");
}

function validityRange(
  body: Extract<CborValue, { kind: "map" }>,
  config: readonly [bigint, bigint, bigint],
): CborValue {
  const lower = mapGet(body, 8n);
  const upper = mapGet(body, 3n);
  const lowerBound = lower === undefined
    ? constr(0n, [constr(0n, []), booleanData(true)])
    : constr(0n, [constr(1n, [integerNode(slotToTime(lower, config))]), booleanData(true)]);
  const upperBound = upper === undefined
    ? constr(0n, [constr(2n, []), booleanData(true)])
    : constr(0n, [constr(1n, [integerNode(slotToTime(upper, config))]), booleanData(false)]);
  return constr(0n, [lowerBound, upperBound]);
}

function slotToTime(value: CborValue, config: readonly [bigint, bigint, bigint]): bigint {
  if (value.kind !== "unsigned") contextError("validity bound must be an unsigned slot");
  return config[0] + (value.value - config[1]) * config[2];
}

function scriptPurpose(
  redeemer: ContextRedeemer,
  body: Extract<CborValue, { kind: "map" }>,
  utxos: readonly ContextUtxo[],
  language: 0 | 1 | 2,
  protocol: number,
): CborValue {
  switch (redeemer.tag) {
    case 0: {
      const input = indexedSet(mapGet(body, 0n), redeemer.index, "spending input");
      return constr(1n, [txOutRef(input, language === 2)]);
    }
    case 1: {
      const mint = mapGet(body, 9n);
      if (mint?.kind !== "map") contextError("minting redeemer points to no policy");
      const policies = mint.entries.map(([policy]) => policy).sort(compareCbor);
      const policy = policies[numberIndex(redeemer.index)];
      if (policy?.kind !== "bytes") contextError("minting redeemer points to no policy");
      return constr(0n, [bytesNode(policy.value)]);
    }
    case 2: {
      const certificate = indexedList(mapGet(body, 4n), redeemer.index, "certificate");
      const translated = language === 2
        ? v3Certificate(certificate, protocol)
        : legacyCertificate(certificate, protocol);
      return language === 2
        ? constr(3n, [integerNode(redeemer.index), translated])
        : constr(3n, [translated]);
    }
    case 3: {
      const entries = withdrawals(body);
      const entry = entries[numberIndex(redeemer.index)];
      if (entry === undefined) contextError("rewarding redeemer points to no withdrawal");
      return language === 2
        ? constr(2n, [credentialFromReward(entry[0])])
        : constr(2n, [stakingCredentialFromReward(entry[0])]);
    }
    case 4: {
      if (language !== 2) contextError("voting purpose is unavailable to Plutus V1/V2");
      const votes = mapGet(body, 19n);
      if (votes?.kind !== "map") contextError("voting redeemer points to no voter");
      const voters = votes.entries.map(([voter]) => voter).sort(compareCbor);
      const voter = voters[numberIndex(redeemer.index)];
      if (voter === undefined) contextError("voting redeemer points to no voter");
      return constr(4n, [voterData(voter)]);
    }
    case 5: {
      if (language !== 2) contextError("proposing purpose is unavailable to Plutus V1/V2");
      const proposal = indexedList(mapGet(body, 20n), redeemer.index, "proposal");
      return constr(5n, [integerNode(redeemer.index), proposalData(proposal)]);
    }
    default: contextError("invalid redeemer purpose");
  }
}

function purposeFields(purpose: CborValue): CborValue[] {
  if (purpose.kind !== "tag" || purpose.value.kind !== "array") contextError("invalid script purpose");
  return [...purpose.value.values];
}

function txOutRef(value: CborValue, v3: boolean): CborValue {
  if (
    value.kind !== "array" ||
    value.values.length !== 2 ||
    value.values[0]?.kind !== "bytes" ||
    value.values[1]?.kind !== "unsigned"
  ) contextError("invalid transaction input");
  const id = v3 ? bytesNode(value.values[0].value) : constr(0n, [bytesNode(value.values[0].value)]);
  return constr(0n, [id, integerNode(value.values[1].value)]);
}

function txId(body: Extract<CborValue, { kind: "map" }>, v3: boolean): CborValue {
  const hash = bytesNode(blake2b256(encodeCbor(body, { mode: "preserve" })));
  return v3 ? hash : constr(0n, [hash]);
}

function currentSpendingDatum(
  redeemer: ContextRedeemer,
  body: Extract<CborValue, { kind: "map" }>,
  utxos: readonly ContextUtxo[],
  witnesses: Extract<CborValue, { kind: "map" }>,
): CborValue | undefined {
  const input = indexedSet(mapGet(body, 0n), redeemer.index, "spending input");
  const resolved = utxos.find((candidate) => compareCbor(candidate.input, input) === 0);
  if (resolved === undefined) contextError("spending input does not exist in supplied UTxO");
  return spendingDatum(resolved.output, witnesses);
}

function spendingDatum(
  output: CborValue,
  witnesses: Extract<CborValue, { kind: "map" }>,
): CborValue | undefined {
  if (output.kind === "map") {
    const datum = mapGet(output, 2n);
    if (
      datum?.kind === "array" &&
      datum.values[0]?.kind === "unsigned"
    ) {
      if (datum.values[0].value === 1n) return datum.values[1];
      if (datum.values[0].value === 0n && datum.values[1]?.kind === "bytes") {
        return findDatum(datum.values[1].value, witnesses);
      }
    }
  }
  if (output.kind === "array" && output.values[2]?.kind === "bytes") {
    return findDatum(output.values[2].value, witnesses);
  }
  return undefined;
}

function findDatum(
  hash: Uint8Array,
  witnesses: Extract<CborValue, { kind: "map" }>,
): CborValue | undefined {
  return datums(witnesses).find((datum) => bytesEqual(blake2b256(encodePlutusData(datum)), hash));
}

function legacyCertificate(certificate: CborValue, protocol: number): CborValue {
  const fields = ledgerArray(certificate, "certificate");
  const tag = unsignedValue(fields[0], "certificate tag");
  const stake = (): CborValue => constr(0n, [credentialNode(fields[1])]);
  switch (tag) {
    case 0n: case 7n: return constr(0n, [stake()]);
    case 1n: case 8n: return constr(1n, [stake()]);
    case 2n:
      return constr(2n, [stake(), bytesFrom(fields[2], "stake pool hash")]);
    case 3n:
      return constr(3n, [bytesFrom(fields[1], "stake pool hash"), bytesFrom(fields[2], "VRF hash")]);
    case 4n:
      return constr(4n, [bytesFrom(fields[1], "stake pool hash"), integerFrom(fields[2])]);
    case 5n: return constr(5n, []);
    case 6n: return constr(6n, []);
    default:
      contextError(`certificate ${tag} cannot be represented in a Plutus V1/V2 context at protocol ${protocol}`);
  }
}

function v3Certificate(certificate: CborValue, protocol: number): CborValue {
  const fields = ledgerArray(certificate, "certificate");
  const tag = unsignedValue(fields[0], "certificate tag");
  const cred = (): CborValue => credentialNode(fields[1]);
  switch (tag) {
    case 0n: return constr(0n, [cred(), nothing()]);
    case 1n: return constr(1n, [cred(), nothing()]);
    case 2n: return constr(2n, [cred(), constr(0n, [bytesFrom(fields[2], "stake pool hash")])]);
    case 3n:
      return constr(7n, [bytesFrom(fields[1], "stake pool hash"), bytesFrom(fields[2], "VRF hash")]);
    case 4n:
      return constr(8n, [bytesFrom(fields[1], "stake pool hash"), integerFrom(fields[2])]);
    case 7n:
      return constr(0n, [cred(), protocol === 9 ? nothing() : just(integerFrom(fields[2]))]);
    case 8n:
      return constr(1n, [cred(), protocol === 9 ? nothing() : just(integerFrom(fields[2]))]);
    case 9n:
      return constr(2n, [cred(), constr(1n, [drepData(fields[2])])]);
    case 10n:
      return constr(2n, [cred(), constr(2n, [bytesFrom(fields[2], "stake pool hash"), drepData(fields[3])])]);
    case 11n:
      return constr(3n, [cred(), constr(0n, [bytesFrom(fields[2], "stake pool hash")]), integerFrom(fields[3])]);
    case 12n:
      return constr(3n, [cred(), constr(1n, [drepData(fields[2])]), integerFrom(fields[3])]);
    case 13n:
      return constr(3n, [
        cred(),
        constr(2n, [bytesFrom(fields[2], "stake pool hash"), drepData(fields[3])]),
        integerFrom(fields[4]),
      ]);
    case 14n:
      return constr(9n, [credentialNode(fields[1]), credentialNode(fields[2])]);
    case 15n:
      return constr(10n, [credentialNode(fields[1])]);
    case 16n:
      return constr(4n, [credentialNode(fields[1]), integerFrom(fields[2])]);
    case 17n:
      return constr(6n, [credentialNode(fields[1]), integerFrom(fields[2])]);
    case 18n:
      return constr(5n, [credentialNode(fields[1])]);
    default: contextError(`unsupported Conway certificate ${tag}`);
  }
}

function drepData(value: CborValue | undefined): CborValue {
  const fields = ledgerArray(value, "DRep");
  const tag = unsignedValue(fields[0], "DRep tag");
  if (tag === 0n || tag === 1n) {
    return constr(0n, [credential(tag === 1n, byteValue(fields[1], "DRep hash"))]);
  }
  if (tag === 2n) return constr(1n, []);
  if (tag === 3n) return constr(2n, []);
  contextError("invalid DRep");
}

function voterData(value: CborValue): CborValue {
  const fields = ledgerArray(value, "voter");
  const tag = unsignedValue(fields[0], "voter tag");
  if (tag === 0n || tag === 1n) {
    return constr(0n, [credential(tag === 1n, byteValue(fields[1], "committee voter hash"))]);
  }
  if (tag === 2n || tag === 3n) {
    return constr(1n, [credential(tag === 3n, byteValue(fields[1], "DRep voter hash"))]);
  }
  if (tag === 4n) return constr(2n, [bytesFrom(fields[1], "stake pool hash")]);
  contextError("invalid voter");
}

function votesData(value: CborValue | undefined): CborValue {
  if (value === undefined) return mapNode([]);
  if (value.kind !== "map") contextError("voting procedures must be a map");
  return mapNode(value.entries.map(([voter, procedures]) => {
    if (procedures.kind !== "map") contextError("voting procedure must be a map");
    return [voterData(voter), mapNode(procedures.entries.map(([action, procedure]) => {
      const fields = ledgerArray(procedure, "voting procedure");
      const vote = unsignedValue(fields[0], "vote");
      if (vote > 2n) contextError("invalid vote");
      return [governanceActionId(action), constr(vote, [])];
    }))];
  }));
}

function governanceActionId(value: CborValue): CborValue {
  const fields = ledgerArray(value, "governance action id");
  if (fields[0]?.kind !== "bytes") contextError("invalid governance action transaction id");
  return constr(0n, [bytesNode(fields[0].value), integerFrom(fields[1])]);
}

function proposalData(value: CborValue): CborValue {
  const fields = ledgerArray(value, "proposal procedure");
  if (fields.length < 3) contextError("invalid proposal procedure");
  const returnAddress = fields[1];
  return constr(0n, [
    integerFrom(fields[0]),
    credentialFromReward(returnAddress as CborValue),
    governanceActionData(fields[2] as CborValue),
  ]);
}

function governanceActionData(value: CborValue): CborValue {
  const fields = ledgerArray(value, "governance action");
  const tag = unsignedValue(fields[0], "governance action tag");
  switch (tag) {
    case 0n:
      return constr(0n, [maybeGovAction(fields[1]), changedParameters(fields[2]), maybeScriptHash(fields[3])]);
    case 1n: {
      const version = ledgerArray(fields[2], "protocol version");
      return constr(1n, [maybeGovAction(fields[1]), constr(0n, [integerFrom(version[0]), integerFrom(version[1])])]);
    }
    case 2n: {
      if (fields[1]?.kind !== "map") contextError("treasury withdrawals must be a map");
      return constr(2n, [
        mapNode(fields[1].entries.map(([account, amount]) => [credentialFromReward(account), integerFrom(amount)])),
        maybeScriptHash(fields[2]),
      ]);
    }
    case 3n: return constr(3n, [maybeGovAction(fields[1])]);
    case 4n: {
      if (fields[2]?.kind !== "array" || fields[3]?.kind !== "map") contextError("invalid committee update");
      return constr(4n, [
        maybeGovAction(fields[1]),
        arrayNode(fields[2].values.map((item) => constr(0n, [credentialNode(item)]))),
        mapNode(fields[3].entries.map(([cred, epoch]) =>
          [constr(0n, [credentialNode(cred)]), integerFrom(epoch)])),
        rationalData(fields[4]),
      ]);
    }
    case 5n: {
      const constitution = ledgerArray(fields[2], "constitution");
      return constr(5n, [maybeGovAction(fields[1]), constr(0n, [maybeScriptHash(constitution[1])])]);
    }
    case 6n: return constr(6n, []);
    default: contextError("invalid governance action");
  }
}

function changedParameters(value: CborValue | undefined): CborValue {
  if (value === undefined) contextError("missing changed parameters");
  // The Ledger ToPlutusData instance uses a Data map keyed by numeric parameter IDs.
  if (value.kind === "map") {
    return mapNode(value.entries.map(([key, item]) => [integerFrom(key), ledgerParameterData(item)]));
  }
  return value;
}

function ledgerParameterData(value: CborValue): CborValue {
  if (value.kind === "unsigned" || value.kind === "negative" || value.kind === "bytes" || value.kind === "map") {
    return value;
  }
  if (value.kind === "array") return arrayNode(value.values.map(ledgerParameterData));
  contextError("unsupported changed parameter value");
}

function rationalData(value: CborValue | undefined): CborValue {
  if (value?.kind === "tag" && value.tag === 30n && value.value.kind === "array") {
    return constr(0n, value.value.values.map(integerFrom));
  }
  if (value?.kind === "array" && value.values.length === 2) {
    return constr(0n, value.values.map(integerFrom));
  }
  contextError("invalid rational");
}

function maybeGovAction(value: CborValue | undefined): CborValue {
  if (value === undefined || value.kind === "null") return nothing();
  return just(governanceActionId(value));
}

function maybeScriptHash(value: CborValue | undefined): CborValue {
  if (value === undefined || value.kind === "null") return nothing();
  return just(bytesFrom(value, "script hash"));
}

function assertV1Features(
  body: Extract<CborValue, { kind: "map" }>,
  utxos: readonly ContextUtxo[],
): void {
  const referenceInputs = mapGet(body, 18n);
  if ((setValues(referenceInputs)?.length ?? 0) !== 0) {
    contextError("reference inputs are unavailable to Plutus V1");
  }
  for (const output of [...outputs(body), ...utxos.map((utxo) => utxo.output)]) {
    if (output.kind !== "map") continue;
    const datum = mapGet(output, 2n);
    if (
      datum?.kind === "array" &&
      datum.values[0]?.kind === "unsigned" &&
      datum.values[0].value === 1n
    ) {
      contextError("inline datums are unavailable to Plutus V1");
    }
    if (mapGet(output, 3n) !== undefined) contextError("reference scripts are unavailable to Plutus V1");
  }
}

function assertDisjointInputs(body: Extract<CborValue, { kind: "map" }>): void {
  const inputs = mapGet(body, 0n);
  const references = mapGet(body, 18n);
  const inputValues = setValues(inputs);
  const referenceValues = setValues(references);
  if (inputValues === undefined || referenceValues === undefined) return;
  for (const input of inputValues) {
    if (referenceValues.some((reference) => compareCbor(input, reference) === 0)) {
      contextError("reference inputs must be disjoint from spending inputs at protocol 11");
    }
  }
}

function outputs(body: Extract<CborValue, { kind: "map" }>): readonly CborValue[] {
  const value = mapGet(body, 1n);
  if (value === undefined) return [];
  if (value.kind !== "array") contextError("transaction outputs must be an array");
  return value.values;
}

function certificates(body: Extract<CborValue, { kind: "map" }>): readonly CborValue[] {
  const value = mapGet(body, 4n);
  if (value === undefined) return [];
  if (value.kind !== "array") contextError("transaction certificates must be an array");
  return value.values;
}

function proposals(body: Extract<CborValue, { kind: "map" }>): readonly CborValue[] {
  const value = mapGet(body, 20n);
  if (value === undefined) return [];
  if (value.kind !== "array") contextError("proposal procedures must be an array");
  return value.values;
}

function withdrawals(
  body: Extract<CborValue, { kind: "map" }>,
): readonly (readonly [CborValue, CborValue])[] {
  const value = mapGet(body, 5n);
  if (value === undefined) return [];
  if (value.kind !== "map") contextError("withdrawals must be a map");
  return [...value.entries].sort(([left], [right]) => compareCbor(left, right));
}

function signatories(body: Extract<CborValue, { kind: "map" }>): Uint8Array[] {
  const value = mapGet(body, 14n);
  if (value === undefined) return [];
  const values = setValues(value);
  if (values === undefined) contextError("required signers must be a set");
  return [...values].sort(compareCbor).map((item) => byteValue(item, "required signer"));
}

function datums(witnesses: Extract<CborValue, { kind: "map" }>): readonly CborValue[] {
  const value = mapGet(witnesses, 4n);
  if (value === undefined) return [];
  const values = setValues(value);
  if (values === undefined) contextError("datum witnesses must be a set");
  return [...values].sort((left, right) =>
    compareBytes(blake2b256(encodePlutusData(left)), blake2b256(encodePlutusData(right))));
}

function outputField(output: CborValue, mapKey: bigint, arrayIndex: number): CborValue | undefined {
  return output.kind === "map"
    ? mapGet(output, mapKey)
    : output.kind === "array"
      ? output.values[arrayIndex]
      : undefined;
}

function indexedSet(value: CborValue | undefined, index: bigint, label: string): CborValue {
  const values = setValues(value);
  if (values === undefined) contextError(`${label} set is missing`);
  const selected = [...values].sort(compareCbor)[numberIndex(index)];
  if (selected === undefined) contextError(`${label} index is out of range`);
  return selected;
}

function indexedList(value: CborValue | undefined, index: bigint, label: string): CborValue {
  if (value?.kind !== "array") contextError(`${label} list is missing`);
  const selected = value.values[numberIndex(index)];
  if (selected === undefined) contextError(`${label} index is out of range`);
  return selected;
}

function numberIndex(value: bigint): number {
  if (value < 0n || value > BigInt(Number.MAX_SAFE_INTEGER)) contextError("redeemer index is out of range");
  return Number(value);
}

function ledgerArray(value: CborValue | undefined, label: string): readonly CborValue[] {
  if (value?.kind !== "array") contextError(`${label} must be an array`);
  return value.values;
}

function unsignedValue(value: CborValue | undefined, label: string): bigint {
  if (value?.kind !== "unsigned") contextError(`${label} must be unsigned`);
  return value.value;
}

function byteValue(value: CborValue | undefined, label: string): Uint8Array {
  if (value?.kind !== "bytes") contextError(`${label} must be bytes`);
  return value.value;
}

function bytesFrom(value: CborValue | undefined, label: string): CborValue {
  return bytesNode(byteValue(value, label));
}

function integerFrom(value: CborValue | undefined): CborValue {
  if (value?.kind !== "unsigned" && value?.kind !== "negative") contextError("expected integer");
  return integerNode(value.value);
}

function maybeInteger(value: CborValue | undefined): CborValue {
  return value === undefined ? nothing() : just(integerFrom(value));
}

function constr(alternative: bigint, fields: readonly CborValue[]): CborValue {
  const value: CborValue = arrayNode(fields);
  if (alternative <= 6n) return { kind: "tag", tag: 121n + alternative, value, encoding: { width: 0 } };
  if (alternative <= 127n) return { kind: "tag", tag: 1280n + alternative - 7n, value, encoding: { width: 0 } };
  return {
    kind: "tag",
    tag: 102n,
    value: arrayNode([integerNode(alternative), value]),
    encoding: { width: 0 },
  };
}

function maybe(value: CborValue | undefined): CborValue {
  return value === undefined ? nothing() : just(value);
}

function just(value: CborValue): CborValue {
  return constr(0n, [value]);
}

function nothing(): CborValue {
  return constr(1n, []);
}

function booleanData(value: boolean): CborValue {
  return constr(value ? 1n : 0n, []);
}

function integerNode(value: bigint): CborValue {
  return value >= 0n
    ? unsignedNode(value)
    : { kind: "negative", value, encoding: { width: 0 } };
}

function unsignedNode(value: bigint): CborValue {
  if (value < 0n) contextError("expected unsigned integer");
  return { kind: "unsigned", value, encoding: { width: 0 } };
}

function bytesNode(value: Uint8Array): CborValue {
  return { kind: "bytes", value: Uint8Array.from(value), encoding: definite };
}

function arrayNode(values: readonly CborValue[]): Extract<CborValue, { kind: "array" }> {
  return { kind: "array", values, encoding: definite };
}

function mapNode(
  entries: readonly (readonly [CborValue, CborValue])[],
): Extract<CborValue, { kind: "map" }> {
  return { kind: "map", entries, encoding: definite };
}

function mapGet(
  map: Extract<CborValue, { kind: "map" }>,
  key: bigint,
): CborValue | undefined {
  return map.entries.find(([candidate]) =>
    candidate.kind === "unsigned" && candidate.value === key)?.[1];
}

function setValues(value: CborValue | undefined): readonly CborValue[] | undefined {
  if (value?.kind === "array") return value.values;
  if (value?.kind === "tag" && value.tag === 258n && value.value.kind === "array") {
    return value.value.values;
  }
  return undefined;
}

function compareCbor(left: CborValue, right: CborValue): number {
  return compareBytes(
    encodeCbor(left, { mode: "canonical" }),
    encodeCbor(right, { mode: "canonical" }),
  );
}

function compareBytes(left: Uint8Array, right: Uint8Array): number {
  const length = Math.min(left.length, right.length);
  for (let index = 0; index < length; index += 1) {
    const difference = (left[index] ?? 0) - (right[index] ?? 0);
    if (difference !== 0) return difference;
  }
  return left.length - right.length;
}

function contextError(message: string): never {
  throw new CardanoError("EVALUATE", message);
}
