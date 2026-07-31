export type CborHeadWidth = 0 | 1 | 2 | 4 | 8;
export type CborMode = "canonical" | "preserve";

export interface CborSpan {
  readonly start: number;
  readonly end: number;
}

export interface CborHeadEncoding {
  readonly width: CborHeadWidth;
}

export type CborLengthEncoding =
  | { readonly kind: "definite"; readonly width: CborHeadWidth }
  | { readonly kind: "indefinite" };

export interface CborByteChunk {
  readonly value: Uint8Array;
  readonly width: CborHeadWidth;
}

export interface CborTextChunk {
  readonly value: string;
  readonly width: CborHeadWidth;
}

export type CborStringEncoding =
  | { readonly kind: "definite"; readonly width: CborHeadWidth }
  | {
      readonly kind: "indefinite";
      readonly chunks: readonly (CborByteChunk | CborTextChunk)[];
    };

interface CborNode {
  readonly span?: CborSpan;
}

export type CborValue =
  | (CborNode & {
      readonly kind: "unsigned";
      readonly value: bigint;
      readonly encoding: CborHeadEncoding;
    })
  | (CborNode & {
      readonly kind: "negative";
      readonly value: bigint;
      readonly encoding: CborHeadEncoding;
    })
  | (CborNode & {
      readonly kind: "bytes";
      readonly value: Uint8Array;
      readonly encoding: CborStringEncoding;
    })
  | (CborNode & {
      readonly kind: "text";
      readonly value: string;
      readonly encoding: CborStringEncoding;
    })
  | (CborNode & {
      readonly kind: "array";
      readonly values: readonly CborValue[];
      readonly encoding: CborLengthEncoding;
    })
  | (CborNode & {
      readonly kind: "map";
      readonly entries: readonly (readonly [CborValue, CborValue])[];
      readonly encoding: CborLengthEncoding;
    })
  | (CborNode & {
      readonly kind: "tag";
      readonly tag: bigint;
      readonly value: CborValue;
      readonly encoding: CborHeadEncoding;
    })
  | (CborNode & { readonly kind: "boolean"; readonly value: boolean })
  | (CborNode & { readonly kind: "null" })
  | (CborNode & { readonly kind: "undefined" })
  | (CborNode & { readonly kind: "simple"; readonly value: number; readonly width: 0 | 1 })
  | (CborNode & { readonly kind: "float"; readonly value: number; readonly width: 2 | 4 | 8 });

export interface CborDecoderLimits {
  readonly maxDepth: number;
  readonly maxCollectionLength: number;
  readonly maxStringBytes: number;
  readonly maxTokens: number;
}

export interface DecodeCborOptions {
  readonly limits?: Partial<CborDecoderLimits>;
}

export interface EncodeCborOptions {
  readonly mode?: CborMode;
}
