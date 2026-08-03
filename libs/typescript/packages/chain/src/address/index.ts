import {
  bytesToHex,
  copyBytes,
  hexToBytes,
} from "@xray-network/xray-cardano-lib-core";
import { decodeBech32, encodeBech32 } from "@xray-network/xray-cardano-lib-core/bech32";
import { Ed25519KeyHash, ScriptHash } from "@xray-network/xray-cardano-lib-crypto";
import { Credential } from "../era/conway/model.js";
import { ByronAddress } from "../era/byron/address.js";

export enum AddressHeaderKind {
  BasePaymentKeyStakeKey = 0,
  BasePaymentScriptStakeKey = 1,
  BasePaymentKeyStakeScript = 2,
  BasePaymentScriptStakeScript = 3,
  PointerKey = 4,
  PointerScript = 5,
  EnterpriseKey = 6,
  EnterpriseScript = 7,
  Byron = 8,
  RewardKey = 14,
  RewardScript = 15,
}

export enum AddressKind { Base = 0, Ptr = 1, Enterprise = 2, Reward = 3, Byron = 4 }

function networkByte(value: number): number {
  if (!Number.isSafeInteger(value) || value < 0 || value > 255) throw new RangeError("network must fit uint8");
  return value;
}

function cloneCredential(value: Credential): Credential {
  return Credential.from_cbor_bytes(value.to_cbor_bytes());
}

function credentialBytes(value: Credential): Uint8Array {
  const hash = value.as_pub_key() ?? value.as_script();
  if (hash === undefined) throw new TypeError("Credential has no hash");
  return hash.to_raw_bytes();
}

function credentialFrom(header: number, bit: number, bytes: Uint8Array): Credential {
  return (header & (1 << bit)) === 0
    ? Credential.new_pub_key(Ed25519KeyHash.from_raw_bytes(bytes))
    : Credential.new_script(ScriptHash.from_raw_bytes(bytes));
}

export function encodeVariableNat(value: bigint): Uint8Array {
  if (value < 0n) throw new RangeError("variable natural must be non-negative");
  const output = [Number(value & 0x7fn)];
  for (let remaining = value >> 7n; remaining > 0n; remaining >>= 7n) {
    output.push(Number(remaining & 0x7fn) | 0x80);
  }
  return Uint8Array.from(output.reverse());
}

export function decodeVariableNat(bytes: Uint8Array, offset = 0): { readonly value: bigint; readonly read: number } {
  let value = 0n;
  for (let index = offset; index < bytes.length; index += 1) {
    const byte = bytes[index] as number;
    value = value * 128n + BigInt(byte & 0x7f);
    if ((byte & 0x80) === 0) return { value, read: index - offset + 1 };
  }
  throw new TypeError("unterminated variable natural");
}

export class Pointer {
  readonly #slot: bigint;
  readonly #transactionIndex: bigint;
  readonly #certificateIndex: bigint;
  private constructor(slot: bigint, transactionIndex: bigint, certificateIndex: bigint) {
    if ([slot, transactionIndex, certificateIndex].some((value) => value < 0n)) throw new RangeError("pointer components must be non-negative");
    this.#slot = slot; this.#transactionIndex = transactionIndex; this.#certificateIndex = certificateIndex;
  }
  public static new(slot: bigint, transactionIndex: bigint, certificateIndex: bigint): Pointer { return new Pointer(slot, transactionIndex, certificateIndex); }
  public slot(): bigint { return this.#slot; }
  public transaction_index(): bigint { return this.#transactionIndex; }
  public certificate_index(): bigint { return this.#certificateIndex; }
}

type AddressState =
  | { readonly kind: AddressKind.Base; readonly network: number; readonly payment: Credential; readonly stake: Credential; readonly trailing?:Uint8Array|undefined }
  | { readonly kind: AddressKind.Ptr; readonly network: number; readonly payment: Credential; readonly pointer: Pointer; readonly trailing?:Uint8Array|undefined }
  | { readonly kind: AddressKind.Enterprise; readonly network: number; readonly payment: Credential; readonly trailing?:Uint8Array|undefined }
  | { readonly kind: AddressKind.Reward; readonly network: number; readonly payment: Credential; readonly trailing?:Uint8Array|undefined }
  | { readonly kind: AddressKind.Byron; readonly raw: Uint8Array };

const addressStates = new WeakMap<Address, AddressState>();
const trailingWhitelist=[
  [203,87,175,176,179,95,200,156,99,6,28,153,20,224,85,0,26,81,140,117,22],
  [19,213,244,163,254,4,120,178,36,30,1,104,227,203,165,0,26,34,193,90,17],
  [0],
  [106,51,48,102,53,97,109,107,119,104,119,113,97,52,119,118,102,121,106,100,101,122,121,97,101,108,109,110,110,103,100,54,100,52,101],
  [53,97,99,121,50,114,48,101,107,114,112,113,122,113,106,108,113,100,107,56,108,122,113,110,53,114,52,53,110],
  [6,29,7,12,13,4,27,7,2,15,11,13,11,15,2,9,18,5,29,28,16,9,17,4,14,31,7,19,17,3,1,0,11,16,22,0],
  [18,110,119,53,51,53,103,54,118,115,112,55,120,55,102,104,120,112,113,50,112,116,115,104,57,103,107,114],
  [44],
].map((bytes)=>Uint8Array.from(bytes));

function acceptedTrailing(data:Uint8Array,offset:number):Uint8Array|undefined { if(data.length===offset)return undefined;if(data.length<offset)throw new TypeError("address is truncated");const trailing=data.slice(offset);if(!trailingWhitelist.some((allowed)=>allowed.length===trailing.length&&allowed.every((byte,index)=>byte===trailing[index])))throw new TypeError("address has unrecognized trailing bytes");return trailing; }

function strictAddressLength(data:Uint8Array):void {
  const variant=(data[0]??0)>>>4;
  if(variant<=3){if(data.length!==57)throw new TypeError("base address must contain exactly 57 bytes");return;}
  if(variant===4||variant===5){let offset=29;for(let part=0;part<3;part+=1){const decoded=decodeVariableNat(data,offset);offset+=decoded.read;}if(offset!==data.length)throw new TypeError("pointer address has trailing bytes");return;}
  if(variant===6||variant===7||variant===14||variant===15){if(data.length!==29)throw new TypeError("address must contain exactly 29 bytes");return;}
  if(variant===8)throw new TypeError("Byron addresses use Base58, not Bech32");
  throw new TypeError("reserved address header kind");
}

function canonicalAddressHrp(kind:AddressKind,network:number):string {
  if(kind===AddressKind.Byron)throw new TypeError("Byron addresses use Base58, not Bech32");
  const prefix=kind===AddressKind.Reward?"stake":"addr";
  return network===1?prefix:`${prefix}_test`;
}

export class Address {
  public constructor(state: AddressState) { addressStates.set(this, state); }
  public static from_raw_bytes(data: Uint8Array): Address {
    if (data.length === 0) throw new TypeError("address must not be empty");
    const header = data[0] as number;
    const variant = header >>> 4;
    const network = header & 15;
    if (variant <= 3) {
      if (data.length < 57) throw new TypeError("base address must be at least 57 bytes");
      return new Address({ kind: AddressKind.Base, network, payment: credentialFrom(header, 4, data.slice(1,29)), stake: credentialFrom(header,5,data.slice(29,57)), trailing:acceptedTrailing(data,57) });
    }
    if (variant === 4 || variant === 5) {
      if (data.length < 32) throw new TypeError("pointer address is truncated");
      const payment = credentialFrom(header,4,data.slice(1,29));
      let offset=29;const slot=decodeVariableNat(data,offset);offset+=slot.read;const transaction=decodeVariableNat(data,offset);offset+=transaction.read;const certificate=decodeVariableNat(data,offset);offset+=certificate.read;
      return new Address({kind:AddressKind.Ptr,network,payment,pointer:Pointer.new(slot.value,transaction.value,certificate.value),trailing:acceptedTrailing(data,offset)});
    }
    if (variant === 6 || variant === 7) {
      if (data.length < 29) throw new TypeError("enterprise address must be at least 29 bytes");
      return new Address({kind:AddressKind.Enterprise,network,payment:credentialFrom(header,4,data.slice(1,29)),trailing:acceptedTrailing(data,29)});
    }
    if (variant === 14 || variant === 15) {
      if (data.length < 29) throw new TypeError("reward address must be at least 29 bytes");
      return new Address({kind:AddressKind.Reward,network,payment:credentialFrom(header,4,data.slice(1,29)),trailing:acceptedTrailing(data,29)});
    }
    if (variant === 8) return new Address({kind:AddressKind.Byron,raw:copyBytes(data)});
    throw new TypeError(`unsupported address header ${header}`);
  }
  public static from_hex(hex: string): Address { return Address.from_raw_bytes(hexToBytes(hex)); }
  public static from_bech32(value: string): Address { const decoded=decodeBech32(value);strictAddressLength(decoded.bytes);const address=Address.from_raw_bytes(decoded.bytes);if(decoded.prefix!==canonicalAddressHrp(address.kind(),address.network_id()))throw new TypeError("address Bech32 HRP does not match its kind and network");return address; }
  public static from_json(json: string): Address { const value:unknown=JSON.parse(json);if(typeof value!=="string")throw new TypeError("Address JSON must be a string");return Address.from_bech32(value); }
  public static is_valid_bech32(value: string): boolean { try { Address.from_bech32(value);return true; } catch { return false; } }
  public static is_valid_byron(value:string):boolean{return ByronAddress.is_valid(value);}
  public static is_valid(value: string): boolean { return Address.is_valid_bech32(value)||Address.is_valid_byron(value); }
  public static header_matches_kind(header: number, kind: AddressHeaderKind): boolean { return (header >>> 4) === kind; }
  public header(): number { return this.to_raw_bytes()[0] as number; }
  public kind(): AddressKind { return addressState(this).kind; }
  public network_id(): number { const state=addressState(this);if(state.kind===AddressKind.Byron)throw new TypeError("Byron network id requires decoded attributes");return state.network; }
  public payment_cred(): Credential | undefined { const state=addressState(this);return state.kind===AddressKind.Byron?undefined:cloneCredential(state.payment); }
  public staking_cred(): Credential | undefined { const state=addressState(this);return state.kind===AddressKind.Base?cloneCredential(state.stake):undefined; }
  public to_raw_bytes(): Uint8Array {
    const state=addressState(this);if(state.kind===AddressKind.Byron)return copyBytes(state.raw);
    const payment=credentialBytes(state.payment);let header:number;
    if(state.kind===AddressKind.Base){header=(state.payment.kind()<<4)|(state.stake.kind()<<5)|(state.network&15);return Uint8Array.from([header,...payment,...credentialBytes(state.stake),...(state.trailing??[])]);}
    if(state.kind===AddressKind.Ptr){header=0x40|(state.payment.kind()<<4)|(state.network&15);return Uint8Array.from([header,...payment,...encodeVariableNat(state.pointer.slot()),...encodeVariableNat(state.pointer.transaction_index()),...encodeVariableNat(state.pointer.certificate_index()),...(state.trailing??[])]);}
    if(state.kind===AddressKind.Enterprise)header=0x60|(state.payment.kind()<<4)|(state.network&15);else header=0xe0|(state.payment.kind()<<4)|(state.network&15);
    return Uint8Array.from([header,...payment,...(state.trailing??[])]);
  }
  public to_hex(): string { return bytesToHex(this.to_raw_bytes()); }
  public to_bech32(prefix?: string | null): string { if(prefix!=null)return this.to_bech32_unchecked(prefix);const raw=this.to_raw_bytes();strictAddressLength(raw);return encodeBech32(canonicalAddressHrp(this.kind(),this.network_id()),raw); }
  public to_bech32_unchecked(hrp:string):string { return encodeBech32(hrp,this.to_raw_bytes()); }
  public to_js_value(): string { return this.to_bech32(); }
  public to_json(): string { return JSON.stringify(this.to_js_value()); }
}

function addressState(value: Address): AddressState { const state=addressStates.get(value);if(state===undefined)throw new TypeError("invalid Address receiver");return state; }
function addressFromState(state: AddressState): Address { return Address.from_raw_bytes(new Address(state).to_raw_bytes()); }

export class BaseAddress {
  readonly #network:number;readonly #payment:Credential;readonly #stake:Credential;
  private constructor(network:number,payment:Credential,stake:Credential){this.#network=networkByte(network);this.#payment=cloneCredential(payment);this.#stake=cloneCredential(stake);}
  public static new(network:number,payment:Credential,stake:Credential):BaseAddress{return new BaseAddress(network,payment,stake);}
  public static from_address(address:Address):BaseAddress|undefined{const state=addressState(address);return state.kind===AddressKind.Base?new BaseAddress(state.network,state.payment,state.stake):undefined;}
  public network_id():number{return this.#network;}public payment():Credential{return cloneCredential(this.#payment);}public stake():Credential{return cloneCredential(this.#stake);}
  public to_address():Address{return addressFromState({kind:AddressKind.Base,network:this.#network,payment:this.#payment,stake:this.#stake});}
}

export class EnterpriseAddress {
  readonly #network:number;readonly #payment:Credential;
  private constructor(network:number,payment:Credential){this.#network=networkByte(network);this.#payment=cloneCredential(payment);}
  public static new(network:number,payment:Credential):EnterpriseAddress{return new EnterpriseAddress(network,payment);}
  public static from_address(address:Address):EnterpriseAddress|undefined{const state=addressState(address);return state.kind===AddressKind.Enterprise?new EnterpriseAddress(state.network,state.payment):undefined;}
  public network_id():number{return this.#network;}public payment():Credential{return cloneCredential(this.#payment);}
  public to_address():Address{return addressFromState({kind:AddressKind.Enterprise,network:this.#network,payment:this.#payment});}
}

export class RewardAddress {
  readonly #network:number;readonly #payment:Credential;
  private constructor(network:number,payment:Credential){this.#network=networkByte(network);this.#payment=cloneCredential(payment);}
  public static new(network:number,payment:Credential):RewardAddress{return new RewardAddress(network,payment);}
  public static from_address(address:Address):RewardAddress|undefined{const state=addressState(address);return state.kind===AddressKind.Reward?new RewardAddress(state.network,state.payment):undefined;}
  public static from_json(json:string):RewardAddress{const address=Address.from_json(json);const reward=RewardAddress.from_address(address);if(reward===undefined)throw new TypeError("not a reward address");return reward;}
  public network_id():number{return this.#network;}public payment():Credential{return cloneCredential(this.#payment);}
  public to_address():Address{return addressFromState({kind:AddressKind.Reward,network:this.#network,payment:this.#payment});}
  public to_js_value():string{return this.to_address().to_bech32();}public to_json():string{return JSON.stringify(this.to_js_value());}
}

export class PointerAddress {
  readonly #network:number;readonly #payment:Credential;readonly #pointer:Pointer;
  private constructor(network:number,payment:Credential,pointer:Pointer){this.#network=networkByte(network);this.#payment=cloneCredential(payment);this.#pointer=pointer;}
  public static new(network:number,payment:Credential,pointer:Pointer):PointerAddress{return new PointerAddress(network,payment,pointer);}
  public static from_address(address:Address):PointerAddress|undefined{const state=addressState(address);return state.kind===AddressKind.Ptr?new PointerAddress(state.network,state.payment,state.pointer):undefined;}
  public network_id():number{return this.#network;}public payment():Credential{return cloneCredential(this.#payment);}public stake():Pointer{return this.#pointer;}
  public to_address():Address{return addressFromState({kind:AddressKind.Ptr,network:this.#network,payment:this.#payment,pointer:this.#pointer});}
}
