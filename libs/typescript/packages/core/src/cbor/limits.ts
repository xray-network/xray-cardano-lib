import type { CborDecoderLimits } from "./types.js";

export const DEFAULT_CBOR_LIMITS: CborDecoderLimits = Object.freeze({
  maxDepth: 512,
  maxCollectionLength: 1_000_000,
  maxStringBytes: 64 * 1024 * 1024,
  maxTokens: 2_000_000,
});
