export const TRANSACTION_INPUT_SCHEMA = Object.freeze({
  $schema: "https://json-schema.org/draft/2020-12/schema",
  type: "object",
  required: ["transaction_id", "index"],
  additionalProperties: false,
  properties: {
    transaction_id: { type: "string", pattern: "^[0-9a-f]{64}$" },
    index: { type: "integer", minimum: 0 },
  },
} as const);

export const NATIVE_SCRIPT_SCHEMA = Object.freeze({
  $schema: "https://json-schema.org/draft/2020-12/schema",
  oneOf: ["ScriptPubkey", "ScriptAll", "ScriptAny", "ScriptNOfK", "ScriptInvalidBefore", "ScriptInvalidHereafter"].map((name) => ({
    type: "object",
    required: [name],
    additionalProperties: false,
  })),
} as const);

export const PLUTUS_DATA_SCHEMA = Object.freeze({
  $schema: "https://json-schema.org/draft/2020-12/schema",
  $id: "PlutusData",
  oneOf: ["constructor", "map", "list", "int", "bytes"].map((name) => ({
    type: "object",
    required: [name],
    additionalProperties: false,
  })),
} as const);
