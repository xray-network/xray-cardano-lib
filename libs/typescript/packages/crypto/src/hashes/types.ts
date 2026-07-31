import { FixedBytes } from "./fixed-bytes.js";

export class Ed25519KeyHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 28, "Ed25519KeyHash"); }
  public static from_raw_bytes(bytes: Uint8Array): Ed25519KeyHash { return new Ed25519KeyHash(bytes); }
  public static from_hex(input: string): Ed25519KeyHash { return new Ed25519KeyHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): Ed25519KeyHash { return new Ed25519KeyHash(FixedBytes.bech32(input)); }
}

export class ScriptHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 28, "ScriptHash"); }
  public static from_raw_bytes(bytes: Uint8Array): ScriptHash { return new ScriptHash(bytes); }
  public static from_hex(input: string): ScriptHash { return new ScriptHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): ScriptHash { return new ScriptHash(FixedBytes.bech32(input)); }
}

export class TransactionHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "TransactionHash"); }
  public static from_raw_bytes(bytes: Uint8Array): TransactionHash { return new TransactionHash(bytes); }
  public static from_hex(input: string): TransactionHash { return new TransactionHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): TransactionHash { return new TransactionHash(FixedBytes.bech32(input)); }
}

export class GenesisDelegateHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 28, "GenesisDelegateHash"); }
  public static from_raw_bytes(bytes: Uint8Array): GenesisDelegateHash { return new GenesisDelegateHash(bytes); }
  public static from_hex(input: string): GenesisDelegateHash { return new GenesisDelegateHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): GenesisDelegateHash { return new GenesisDelegateHash(FixedBytes.bech32(input)); }
}

export class GenesisHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 28, "GenesisHash"); }
  public static from_raw_bytes(bytes: Uint8Array): GenesisHash { return new GenesisHash(bytes); }
  public static from_hex(input: string): GenesisHash { return new GenesisHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): GenesisHash { return new GenesisHash(FixedBytes.bech32(input)); }
}

export class AuxiliaryDataHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "AuxiliaryDataHash"); }
  public static from_raw_bytes(bytes: Uint8Array): AuxiliaryDataHash { return new AuxiliaryDataHash(bytes); }
  public static from_hex(input: string): AuxiliaryDataHash { return new AuxiliaryDataHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): AuxiliaryDataHash { return new AuxiliaryDataHash(FixedBytes.bech32(input)); }
}

export class PoolMetadataHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "PoolMetadataHash"); }
  public static from_raw_bytes(bytes: Uint8Array): PoolMetadataHash { return new PoolMetadataHash(bytes); }
  public static from_hex(input: string): PoolMetadataHash { return new PoolMetadataHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): PoolMetadataHash { return new PoolMetadataHash(FixedBytes.bech32(input)); }
}

export class VRFKeyHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "VRFKeyHash"); }
  public static from_raw_bytes(bytes: Uint8Array): VRFKeyHash { return new VRFKeyHash(bytes); }
  public static from_hex(input: string): VRFKeyHash { return new VRFKeyHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): VRFKeyHash { return new VRFKeyHash(FixedBytes.bech32(input)); }
}

export class BlockBodyHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "BlockBodyHash"); }
  public static from_raw_bytes(bytes: Uint8Array): BlockBodyHash { return new BlockBodyHash(bytes); }
  public static from_hex(input: string): BlockBodyHash { return new BlockBodyHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): BlockBodyHash { return new BlockBodyHash(FixedBytes.bech32(input)); }
}

export class BlockHeaderHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "BlockHeaderHash"); }
  public static from_raw_bytes(bytes: Uint8Array): BlockHeaderHash { return new BlockHeaderHash(bytes); }
  public static from_hex(input: string): BlockHeaderHash { return new BlockHeaderHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): BlockHeaderHash { return new BlockHeaderHash(FixedBytes.bech32(input)); }
}

export class DatumHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "DatumHash"); }
  public static from_raw_bytes(bytes: Uint8Array): DatumHash { return new DatumHash(bytes); }
  public static from_hex(input: string): DatumHash { return new DatumHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): DatumHash { return new DatumHash(FixedBytes.bech32(input)); }
}

export class ScriptDataHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "ScriptDataHash"); }
  public static from_raw_bytes(bytes: Uint8Array): ScriptDataHash { return new ScriptDataHash(bytes); }
  public static from_hex(input: string): ScriptDataHash { return new ScriptDataHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): ScriptDataHash { return new ScriptDataHash(FixedBytes.bech32(input)); }
}

export class VRFVkey extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "VRFVkey"); }
  public static from_raw_bytes(bytes: Uint8Array): VRFVkey { return new VRFVkey(bytes); }
  public static from_hex(input: string): VRFVkey { return new VRFVkey(FixedBytes.raw(input)); }
  public static from_bech32(input: string): VRFVkey { return new VRFVkey(FixedBytes.bech32(input)); }
}

export class KESVkey extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "KESVkey"); }
  public static from_raw_bytes(bytes: Uint8Array): KESVkey { return new KESVkey(bytes); }
  public static from_hex(input: string): KESVkey { return new KESVkey(FixedBytes.raw(input)); }
  public static from_bech32(input: string): KESVkey { return new KESVkey(FixedBytes.bech32(input)); }
}

export class NonceHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "NonceHash"); }
  public static from_raw_bytes(bytes: Uint8Array): NonceHash { return new NonceHash(bytes); }
  public static from_hex(input: string): NonceHash { return new NonceHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): NonceHash { return new NonceHash(FixedBytes.bech32(input)); }
}

export class AnchorDocHash extends FixedBytes {
  private constructor(bytes: Uint8Array) { super(bytes, 32, "AnchorDocHash"); }
  public static from_raw_bytes(bytes: Uint8Array): AnchorDocHash { return new AnchorDocHash(bytes); }
  public static from_hex(input: string): AnchorDocHash { return new AnchorDocHash(FixedBytes.raw(input)); }
  public static from_bech32(input: string): AnchorDocHash { return new AnchorDocHash(FixedBytes.bech32(input)); }
}
