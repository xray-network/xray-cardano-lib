/** Browser-safe byte input accepted by domain APIs. */
export type ByteArray = Uint8Array;

/** An injectable CSPRNG boundary shared by browsers and modern Node. */
export interface SecureRandomSource {
  fill(target: Uint8Array): void;
}
