import type { CborValue } from "@xray-network/cardano-core";

export type UplcType =
  | { readonly kind: "integer" }
  | { readonly kind: "bytes" }
  | { readonly kind: "string" }
  | { readonly kind: "unit" }
  | { readonly kind: "boolean" }
  | { readonly kind: "data" }
  | { readonly kind: "bls-g1" }
  | { readonly kind: "bls-g2" }
  | { readonly kind: "bls-ml" }
  | { readonly kind: "value" }
  | { readonly kind: "list"; readonly item: UplcType }
  | { readonly kind: "array"; readonly item: UplcType }
  | { readonly kind: "pair"; readonly first: UplcType; readonly second: UplcType };

export type UplcData = CborValue;

export interface UplcConstant {
  readonly type: UplcType;
  readonly value: unknown;
}

export type UplcTerm =
  | { readonly kind: "var"; readonly index: bigint }
  | { readonly kind: "delay"; readonly term: UplcTerm }
  | { readonly kind: "lambda"; readonly body: UplcTerm }
  | { readonly kind: "apply"; readonly function: UplcTerm; readonly argument: UplcTerm }
  | { readonly kind: "constant"; readonly constant: UplcConstant }
  | { readonly kind: "force"; readonly term: UplcTerm }
  | { readonly kind: "error" }
  | { readonly kind: "builtin"; readonly tag: number }
  | { readonly kind: "constr"; readonly tag: bigint; readonly fields: readonly UplcTerm[] }
  | { readonly kind: "case"; readonly scrutinee: UplcTerm; readonly branches: readonly UplcTerm[] };

export interface UplcProgram {
  readonly version: readonly [major: bigint, minor: bigint, patch: bigint];
  readonly term: UplcTerm;
}

export function dataConstant(value: UplcData): UplcConstant {
  return { type: { kind: "data" }, value };
}
