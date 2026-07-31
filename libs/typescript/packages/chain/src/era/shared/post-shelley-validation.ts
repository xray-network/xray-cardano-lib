import {
  decodeCbor,
  type CborValue,
} from "@xray-network/cardano-core";
import {
  anyCbor,
  arrayOf,
  boolean,
  bytes,
  discriminated,
  fixedMap,
  hash28,
  hash32,
  int64,
  invalid,
  mapOf,
  oneOf,
  optional,
  rational,
  signature64,
  tagged,
  taggedSet,
  text,
  tuple,
  tupleRange,
  uint16,
  uint32,
  uint64,
  unsigned,
  type CborValidator,
  type MapField,
} from "./validation.js";

export interface PostShelleyValidationOptions {
  readonly era: "allegra" | "mary" | "alonzo" | "babbage";
  readonly maximumProtocolVersion: bigint;
}

export interface PostShelleyValidators {
  readonly auxiliaryData: CborValidator;
  readonly auxiliaryDataMap: CborValidator | undefined;
  readonly block: CborValidator;
  readonly certificate: CborValidator;
  readonly mint: CborValidator | undefined;
  readonly nativeScript: CborValidator;
  readonly protocolParamUpdate: CborValidator;
  readonly proposedProtocolParameterUpdates: CborValidator;
  readonly redeemer: CborValidator | undefined;
  readonly script: CborValidator | undefined;
  readonly scriptRef: CborValidator | undefined;
  readonly transaction: CborValidator;
  readonly transactionBody: CborValidator;
  readonly transactionOutput: CborValidator;
  readonly transactionWitnessSet: CborValidator;
  readonly update: CborValidator;
}

export function makePostShelleyValidators(
  options: PostShelleyValidationOptions,
): PostShelleyValidators {
  const isMary = options.era !== "allegra";
  const isAlonzo = options.era === "alonzo" || options.era === "babbage";
  const isBabbage = options.era === "babbage";
  const coin = unsigned();
  const credential = discriminated({ 0: [hash28], 1: [hash28] });
  const transactionInput = tuple(hash32, uint16);
  const unitInterval = rational(true);
  const nonnegativeInterval = rational(false);

  let nativeScript: CborValidator;
  nativeScript = (node, path) => discriminated({
    0: [hash28],
    1: [arrayOf(nativeScript)],
    2: [arrayOf(nativeScript)],
    3: [int64, arrayOf(nativeScript)],
    4: [uint64],
    5: [uint64],
  })(node, path);

  const relay = discriminated({
    0: [optional(uint16), optional(bytes(4)), optional(bytes(16))],
    1: [optional(uint16), text(64)],
    2: [text(64)],
  });
  const poolMetadata = tuple(text(64), bytes());
  const poolFields: readonly CborValidator[] = [
    hash28,
    hash32,
    coin,
    coin,
    unitInterval,
    bytes(),
    taggedSet(hash28),
    arrayOf(relay),
    optional(poolMetadata),
  ];
  const moveInstantaneousReward = tuple(
    unsigned(1n),
    oneOf(mapOf(credential, integer), coin),
  );
  const certificate = discriminated({
    0: [credential],
    1: [credential],
    2: [credential, hash28],
    3: poolFields,
    4: [hash28, uint64],
    5: [hash28, hash28, hash32],
    6: [moveInstantaneousReward],
  });

  const multiassetUnsigned = multiasset(unsigned());
  const value = isMary
    ? oneOf(coin, tuple(coin, multiassetUnsigned))
    : coin;
  const mint = isMary ? multiasset(int64) : undefined;
  const alonzoOutput = tupleRange([bytes(), value, hash32], 2, 3);

  const script = isBabbage
    ? discriminated({ 0: [nativeScript], 1: [bytes()], 2: [bytes()] })
    : undefined;
  const scriptRef = script === undefined ? undefined : embeddedCbor(script);
  const datumOption = discriminated({ 0: [hash32], 1: [embeddedCbor(plutusData)] });
  const babbageOutput = fixedMap({
    0: { required: true, validate: bytes() },
    1: { required: true, validate: value },
    2: { validate: datumOption },
    3: { validate: scriptRef ?? anyCbor },
  });
  const transactionOutput = isBabbage
    ? oneOf(alonzoOutput, babbageOutput)
    : isAlonzo
      ? alonzoOutput
      : tuple(bytes(), value);

  const costModels = isAlonzo
    ? (isBabbage
      ? fixedMap({
        0: { validate: exactArray(int64, 166) },
        1: { validate: exactArray(int64, 175) },
      })
      : mapOf((node, path) => {
        if (node.kind !== "unsigned" || node.value !== 0n) invalid(path ?? "language", "language 0");
      }, exactArray(int64, 166)))
    : undefined;
  const exUnits = tuple(unsigned(0x7fff_ffff_ffff_ffffn), unsigned(0x7fff_ffff_ffff_ffffn));
  const protocolFields: Record<number, MapField> = {
    0: { validate: unsigned() },
    1: { validate: unsigned() },
    2: { validate: uint32 },
    3: { validate: uint32 },
    4: { validate: uint16 },
    5: { validate: coin },
    6: { validate: coin },
    7: { validate: uint32 },
    8: { validate: uint16 },
    9: { validate: nonnegativeInterval },
    10: { validate: unitInterval },
    11: { validate: unitInterval },
    14: {
      validate: tuple(unsigned(options.maximumProtocolVersion), uint32),
    },
    16: { validate: coin },
  };
  if (!isBabbage) {
    protocolFields[12] = { validate: unitInterval };
    protocolFields[13] = { validate: discriminated({ 0: [], 1: [hash32] }) };
  }
  if (!isAlonzo) protocolFields[15] = { validate: coin };
  if (isAlonzo) {
    protocolFields[17] = { validate: coin };
    protocolFields[18] = { validate: costModels as CborValidator };
    protocolFields[19] = { validate: tuple(nonnegativeInterval, nonnegativeInterval) };
    protocolFields[20] = { validate: exUnits };
    protocolFields[21] = { validate: exUnits };
    protocolFields[22] = { validate: uint32 };
    protocolFields[23] = { validate: uint16 };
    protocolFields[24] = { validate: uint16 };
  }
  const protocolParamUpdate = fixedMap(protocolFields);
  const proposedProtocolParameterUpdates = mapOf(hash28, protocolParamUpdate);
  const update = tuple(proposedProtocolParameterUpdates, uint64);

  const bodyFields: Record<number, MapField> = {
    0: { required: true, validate: taggedSet(transactionInput) },
    1: { required: true, validate: arrayOf(transactionOutput) },
    2: { required: true, validate: coin },
    3: { validate: uint64 },
    4: { validate: arrayOf(certificate) },
    5: { validate: mapOf(bytes(), coin) },
    6: { validate: update },
    7: { validate: hash32 },
    8: { validate: uint64 },
  };
  if (mint !== undefined) bodyFields[9] = { validate: mint };
  if (isAlonzo) {
    bodyFields[11] = { validate: hash32 };
    bodyFields[13] = { validate: taggedSet(transactionInput) };
    bodyFields[14] = { validate: taggedSet(hash28) };
    bodyFields[15] = { validate: unsigned(1n) };
  }
  if (isBabbage) {
    bodyFields[16] = { validate: transactionOutput };
    bodyFields[17] = { validate: coin };
    bodyFields[18] = { validate: taggedSet(transactionInput) };
  }
  const transactionBody = fixedMap(bodyFields);

  const redeemer = isAlonzo
    ? tuple(unsigned(3n), uint32, plutusData, exUnits)
    : undefined;
  const witnessFields: Record<number, MapField> = {
    0: { validate: arrayOf(tuple(bytes(32), signature64)) },
    1: { validate: arrayOf(nativeScript) },
    2: { validate: arrayOf(tuple(bytes(32), signature64, bytes(), bytes())) },
  };
  if (isAlonzo) {
    witnessFields[3] = { validate: arrayOf(bytes()) };
    witnessFields[4] = { validate: arrayOf(plutusData) };
    witnessFields[5] = { validate: arrayOf(redeemer as CborValidator) };
  }
  if (isBabbage) witnessFields[6] = { validate: arrayOf(bytes()) };
  const transactionWitnessSet = fixedMap(witnessFields);

  let metadatum: CborValidator;
  metadatum = (node, path) => oneOf(
    mapOf(metadatum, metadatum),
    arrayOf(metadatum),
    integer,
    bytes([0, 64]),
    text(64),
  )(node, path);
  const metadata = mapOf(uint64, metadatum);
  const auxiliaryDataArray = tuple(metadata, arrayOf(nativeScript));
  let auxiliaryDataMap: CborValidator | undefined;
  if (isAlonzo) {
    const fields: Record<number, MapField> = {
      0: { validate: metadata },
      1: { validate: arrayOf(nativeScript) },
      2: { validate: arrayOf(bytes()) },
    };
    if (isBabbage) fields[3] = { validate: arrayOf(bytes()) };
    auxiliaryDataMap = tagged(259n, fixedMap(fields));
  }
  const auxiliaryData = auxiliaryDataMap === undefined
    ? oneOf(metadata, auxiliaryDataArray)
    : oneOf(metadata, auxiliaryDataArray, auxiliaryDataMap);
  const transaction = isAlonzo
    ? tuple(transactionBody, transactionWitnessSet, boolean, optional(auxiliaryData))
    : tuple(transactionBody, transactionWitnessSet, optional(auxiliaryData));

  const headerBody = isBabbage
    ? tuple(
      uint64,
      uint64,
      optional(hash32),
      bytes(32),
      bytes(32),
      tuple(bytes(), bytes(80)),
      uint32,
      hash32,
      tuple(bytes(32), uint64, uint64, signature64),
      tuple(unsigned(options.maximumProtocolVersion), uint32),
    )
    : tuple(
      uint64,
      uint64,
      optional(hash32),
      bytes(32),
      bytes(32),
      tuple(bytes(), bytes(80)),
      tuple(bytes(), bytes(80)),
      uint32,
      hash32,
      bytes(32),
      uint64,
      uint64,
      signature64,
      unsigned(options.maximumProtocolVersion),
      uint32,
    );
  const header = tuple(headerBody, bytes(448));
  const blockShape = isAlonzo
    ? tuple(
      header,
      arrayOf(transactionBody),
      arrayOf(transactionWitnessSet),
      mapOf(uint16, auxiliaryData),
      arrayOf(uint16),
    )
    : tuple(
      header,
      arrayOf(transactionBody),
      arrayOf(transactionWitnessSet),
      mapOf(uint16, auxiliaryData),
    );
  const block: CborValidator = (node, path = `${options.era}.block`) => {
    blockShape(node, path);
    validateBlockCollections(node, path);
  };

  return {
    auxiliaryData,
    auxiliaryDataMap,
    block,
    certificate,
    mint,
    nativeScript,
    protocolParamUpdate,
    proposedProtocolParameterUpdates,
    redeemer,
    script,
    scriptRef,
    transaction,
    transactionBody,
    transactionOutput,
    transactionWitnessSet,
    update,
  };
}

function validateBlockCollections(node: CborValue, path: string): void {
  if (node.kind !== "array") return;
  const bodies = node.values[1];
  const witnesses = node.values[2];
  const auxiliary = node.values[3];
  if (bodies?.kind !== "array" || witnesses?.kind !== "array") return;
  if (bodies.values.length !== witnesses.values.length) {
    invalid(path, "equal transaction-body and witness-set counts");
  }
  if (auxiliary?.kind === "map") {
    for (const [key] of auxiliary.entries) {
      if (key.kind !== "unsigned" || key.value >= BigInt(bodies.values.length)) {
        invalid(path, "auxiliary-data indexes within the transaction-body list");
      }
    }
  }
  const invalidTransactions = node.values[4];
  if (invalidTransactions?.kind === "array") {
    for (const index of invalidTransactions.values) {
      if (index.kind !== "unsigned" || index.value >= BigInt(bodies.values.length)) {
        invalid(path, "invalid-transaction indexes within the transaction-body list");
      }
    }
  }
}

export const integer: CborValidator = (node, path = "integer") => {
  if (node.kind !== "unsigned" && node.kind !== "negative") invalid(path, "an integer");
};

function exactArray(item: CborValidator, length: number): CborValidator {
  return arrayOf(item, length, length);
}

function multiasset(amount: CborValidator): CborValidator {
  return mapOf(hash28, mapOf(bytes([0, 32]), amount, 1));
}

const boundedBytes: CborValidator = (node, path = "bounded_bytes") => {
  if (node.kind !== "bytes") invalid(path, "CBOR bytes");
  if (node.encoding.kind === "definite") {
    if (node.value.length > 64) invalid(path, "at most 64 definite bytes");
  } else if (node.encoding.chunks.some((chunk) => chunk.value.length > 64)) {
    invalid(path, "indefinite byte chunks no longer than 64 bytes");
  }
};

const bigInteger = oneOf(integer, tagged(2n, boundedBytes), tagged(3n, boundedBytes));

export const plutusData: CborValidator = (node, path = "plutus_data") => {
  if (node.kind === "unsigned" || node.kind === "negative") return;
  if (node.kind === "bytes") return boundedBytes(node, path);
  if (node.kind === "array") {
    node.values.forEach((item, index) => plutusData(item, `${path}[${index}]`));
    return;
  }
  if (node.kind === "map") {
    node.entries.forEach(([key, value], index) => {
      plutusData(key, `${path}.key[${index}]`);
      plutusData(value, `${path}.value[${index}]`);
    });
    return;
  }
  if (node.kind === "tag") {
    if (node.tag === 2n || node.tag === 3n) return bigInteger(node, path);
    if (node.tag >= 121n && node.tag <= 127n || node.tag >= 1280n && node.tag <= 1400n) {
      return arrayOf(plutusData)(node.value, `${path}.fields`);
    }
    if (node.tag === 102n) {
      return tuple(unsigned(), arrayOf(plutusData))(node.value, `${path}.general`);
    }
  }
  invalid(path, "Plutus Data");
};

export function embeddedCbor(validator: CborValidator): CborValidator {
  return tagged(24n, (node, path = "embedded_cbor") => {
    if (node.kind !== "bytes") invalid(path, "embedded CBOR bytes");
    validator(decodeCbor(node.value), `${path}.decoded`);
  });
}
