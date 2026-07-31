import {
  bytesToHex,
  copyBytes,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/cardano-core";
import {
  Bip32PublicKey,
  Ed25519Signature,
  PublicKey,
} from "@xray-network/cardano-crypto";
import {
  AddrAttributes,
  AddressContent,
  ByronAddress,
  ByronAddrType,
  SpendingData,
} from "./address.js";

export class ByronTxOut {
  readonly #address: ByronAddress;
  readonly #amount: bigint;

  private constructor(address: ByronAddress, amount: bigint) {
    if (amount < 0n || amount > 0xffff_ffff_ffff_ffffn) {
      throw new RangeError("Byron output amount must fit uint64");
    }
    this.#address = ByronAddress.from_cbor_bytes(address.to_cbor_bytes());
    this.#amount = amount;
  }

  public static new(address: ByronAddress, amount: bigint): ByronTxOut {
    return new ByronTxOut(address, amount);
  }

  public static from_cbor_bytes(bytes: Uint8Array): ByronTxOut {
    const node = decodeCbor(bytes);
    if (
      node.kind !== "array"
      || node.values.length !== 2
      || node.values[0] === undefined
      || node.values[1]?.kind !== "unsigned"
    ) {
      throw new TypeError("invalid Byron transaction output");
    }
    return new ByronTxOut(
      ByronAddress.from_cbor_bytes(encodeCbor(node.values[0])),
      node.values[1].value,
    );
  }

  public static from_cbor_hex(hex: string): ByronTxOut {
    return ByronTxOut.from_cbor_bytes(hexToBytes(hex));
  }

  public address(): ByronAddress {
    return ByronAddress.from_cbor_bytes(this.#address.to_cbor_bytes());
  }

  public amount(): bigint {
    return this.#amount;
  }

  public to_cbor_bytes(): Uint8Array {
    return encodeCbor({
      kind: "array",
      values: [
        decodeCbor(this.#address.to_cbor_bytes()),
        { kind: "unsigned", value: this.#amount, encoding: { width: 0 } },
      ],
      encoding: { kind: "definite", width: 0 },
    });
  }

  public to_cbor_hex(): string {
    return bytesToHex(this.to_cbor_bytes());
  }
}

export class BootstrapWitness {
  readonly #publicKey: PublicKey;
  readonly #signature: Ed25519Signature;
  readonly #chainCode: Uint8Array;
  readonly #attributes: AddrAttributes;

  private constructor(
    publicKey: PublicKey,
    signature: Ed25519Signature,
    chainCode: Uint8Array,
    attributes: AddrAttributes,
  ) {
    if (chainCode.length !== 32) throw new RangeError("bootstrap witness chain code must be 32 bytes");
    this.#publicKey = PublicKey.from_bytes(publicKey.to_raw_bytes());
    this.#signature = Ed25519Signature.from_raw_bytes(signature.to_raw_bytes());
    this.#chainCode = copyBytes(chainCode);
    this.#attributes = AddrAttributes.from_cbor_bytes(attributes.to_cbor_bytes());
  }

  public static new(
    publicKey: PublicKey,
    signature: Ed25519Signature,
    chainCode: Uint8Array,
    attributes: AddrAttributes,
  ): BootstrapWitness {
    return new BootstrapWitness(publicKey, signature, chainCode, attributes);
  }

  public static from_cbor_bytes(bytes: Uint8Array): BootstrapWitness {
    const node = decodeCbor(bytes);
    if (
      node.kind !== "array"
      || node.values.length !== 4
      || node.values[0]?.kind !== "bytes"
      || node.values[1]?.kind !== "bytes"
      || node.values[2]?.kind !== "bytes"
      || node.values[3]?.kind !== "bytes"
    ) {
      throw new TypeError("invalid bootstrap witness");
    }
    return new BootstrapWitness(
      PublicKey.from_bytes(node.values[0].value),
      Ed25519Signature.from_raw_bytes(node.values[1].value),
      node.values[2].value,
      AddrAttributes.from_cbor_bytes(node.values[3].value),
    );
  }

  public static from_cbor_hex(hex: string): BootstrapWitness {
    return BootstrapWitness.from_cbor_bytes(hexToBytes(hex));
  }

  public static from_json(json: string): BootstrapWitness {
    const value: unknown = JSON.parse(json);
    if (typeof value !== "object" || value === null) {
      throw new TypeError("invalid bootstrap witness JSON");
    }
    const record = value as Record<string, unknown>;
    if (
      typeof record["public_key"] !== "string"
      || typeof record["signature"] !== "string"
      || typeof record["chain_code"] !== "string"
      || typeof record["attributes"] !== "string"
    ) {
      throw new TypeError("invalid bootstrap witness JSON");
    }
    return BootstrapWitness.new(
      PublicKey.from_bytes(hexToBytes(record["public_key"])),
      Ed25519Signature.from_raw_bytes(hexToBytes(record["signature"])),
      hexToBytes(record["chain_code"]),
      AddrAttributes.from_cbor_hex(record["attributes"]),
    );
  }

  public public_key(): PublicKey {
    return PublicKey.from_bytes(this.#publicKey.to_raw_bytes());
  }

  public signature(): Ed25519Signature {
    return Ed25519Signature.from_raw_bytes(this.#signature.to_raw_bytes());
  }

  public chain_code(): Uint8Array {
    return copyBytes(this.#chainCode);
  }

  public attributes(): AddrAttributes {
    return AddrAttributes.from_cbor_bytes(this.#attributes.to_cbor_bytes());
  }

  public to_address(): AddressContent {
    return AddressContent.hash_and_create(
      ByronAddrType.PublicKey,
      SpendingData.new_spending_data_pub_key(
        Bip32PublicKey.from_raw_bytes(Uint8Array.from([
          ...this.#publicKey.to_raw_bytes(),
          ...this.#chainCode,
        ])),
      ),
      this.#attributes,
    );
  }

  public to_cbor_bytes(): Uint8Array {
    return encodeCbor({
      kind: "array",
      values: [
        {
          kind: "bytes",
          value: this.#publicKey.to_raw_bytes(),
          encoding: { kind: "definite", width: 0 },
        },
        {
          kind: "bytes",
          value: this.#signature.to_raw_bytes(),
          encoding: { kind: "definite", width: 0 },
        },
        {
          kind: "bytes",
          value: this.#chainCode,
          encoding: { kind: "definite", width: 0 },
        },
        {
          kind: "bytes",
          value: this.#attributes.to_cbor_bytes(),
          encoding: { kind: "definite", width: 0 },
        },
      ],
      encoding: { kind: "definite", width: 0 },
    });
  }

  public to_canonical_cbor_bytes(): Uint8Array {
    return this.to_cbor_bytes();
  }

  public to_cbor_hex(): string {
    return bytesToHex(this.to_cbor_bytes());
  }

  public to_canonical_cbor_hex(): string {
    return this.to_cbor_hex();
  }

  public to_js_value(): unknown {
    return {
      public_key: this.#publicKey.to_raw_bytes(),
      signature: this.#signature.to_raw_bytes(),
      chain_code: copyBytes(this.#chainCode),
      attributes: this.#attributes.to_cbor_hex(),
    };
  }

  public to_json(): string {
    return JSON.stringify({
      public_key: bytesToHex(this.#publicKey.to_raw_bytes()),
      signature: bytesToHex(this.#signature.to_raw_bytes()),
      chain_code: bytesToHex(this.#chainCode),
      attributes: this.#attributes.to_cbor_hex(),
    });
  }
}
