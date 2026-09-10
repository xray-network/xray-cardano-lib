import { decodeBech32WithPrefix, encodeBech32 } from "../hashes/fixed-bytes.js";
import {
  Bip32PrivateKey,
  Bip32PublicKey,
  PrivateKey,
  PublicKey,
} from "./ed25519.js";

const HARDENED = 0x8000_0000;
const MAX_SOFT_INDEX = 0x7fff_ffff;

export enum CardanoKeyRole {
  Root = 0,
  Account = 1,
  Payment = 2,
  Stake = 3,
  DRep = 4,
  ConstitutionalCommitteeCold = 5,
  ConstitutionalCommitteeHot = 6,
}

export enum Cip1852Role {
  External = 0,
  Internal = 1,
  Stake = 2,
  DRep = 3,
  ConstitutionalCommitteeCold = 4,
  ConstitutionalCommitteeHot = 5,
}

const roleNames:Record<CardanoKeyRole,string>={
  [CardanoKeyRole.Root]:"root",
  [CardanoKeyRole.Account]:"acct",
  [CardanoKeyRole.Payment]:"addr",
  [CardanoKeyRole.Stake]:"stake",
  [CardanoKeyRole.DRep]:"drep",
  [CardanoKeyRole.ConstitutionalCommitteeCold]:"cc_cold",
  [CardanoKeyRole.ConstitutionalCommitteeHot]:"cc_hot",
};

function roleName(role:CardanoKeyRole):string {
  const name=roleNames[role];
  if(name===undefined)throw new RangeError("unsupported Cardano key role");
  return name;
}

function expected(value:string,role:CardanoKeyRole,suffix:string,length:number|readonly number[]):Uint8Array {
  const decoded=decodeBech32WithPrefix(value),prefix=`${roleName(role)}_${suffix}`;
  if(decoded.prefix!==prefix)throw new TypeError(`Cardano key Bech32 prefix must be ${prefix}`);
  const lengths=typeof length==="number"?[length]:length;
  if(!lengths.includes(decoded.bytes.length))throw new RangeError(`Cardano key must contain ${lengths.join(" or ")} bytes`);
  return decoded.bytes;
}

function rawPrivateLengths(role:CardanoKeyRole):readonly number[] {
  return role===CardanoKeyRole.DRep||role===CardanoKeyRole.ConstitutionalCommitteeCold||role===CardanoKeyRole.ConstitutionalCommitteeHot?[32,64]:[32];
}

export function encodeCardanoPrivateKey(role:CardanoKeyRole,key:PrivateKey):string {
  const bytes=key.to_raw_bytes();
  if(!rawPrivateLengths(role).includes(bytes.length))throw new RangeError("this Cardano key role does not support a 64-byte raw signing key");
  return encodeBech32(`${roleName(role)}_sk`,bytes);
}
export function decodeCardanoPrivateKey(role:CardanoKeyRole,value:string):PrivateKey {
  const bytes=expected(value,role,"sk",rawPrivateLengths(role));
  return bytes.length===32?PrivateKey.from_normal_bytes(bytes):PrivateKey.from_extended_bytes(bytes);
}
export function encodeCardanoPublicKey(role:CardanoKeyRole,key:PublicKey):string { return encodeBech32(`${roleName(role)}_vk`,key.to_raw_bytes()); }
export function decodeCardanoPublicKey(role:CardanoKeyRole,value:string):PublicKey { return PublicKey.from_bytes(expected(value,role,"vk",32)); }
export function encodeCardanoBip32PrivateKey(role:CardanoKeyRole,key:Bip32PrivateKey):string { return encodeBech32(`${roleName(role)}_xsk`,key.to_raw_bytes()); }
export function decodeCardanoBip32PrivateKey(role:CardanoKeyRole,value:string):Bip32PrivateKey { return Bip32PrivateKey.from_raw_bytes(expected(value,role,"xsk",96)); }
export function encodeCardanoBip32PublicKey(role:CardanoKeyRole,key:Bip32PublicKey):string { return encodeBech32(`${roleName(role)}_xvk`,key.to_raw_bytes()); }
export function decodeCardanoBip32PublicKey(role:CardanoKeyRole,value:string):Bip32PublicKey { return Bip32PublicKey.from_raw_bytes(expected(value,role,"xvk",64)); }

function softIndex(value:number,name:string):number {
  if(!Number.isInteger(value)||value<0||value>MAX_SOFT_INDEX)throw new RangeError(`${name} must be between 0 and 0x7fffffff`);
  return value;
}
function pathRole(value:Cip1852Role):Cip1852Role {
  if(!Number.isInteger(value)||value<Cip1852Role.External||value>Cip1852Role.ConstitutionalCommitteeHot)throw new RangeError("CIP-1852 role must be between 0 and 5");
  return value;
}

export class Cip1852Path {
  readonly #account:number;readonly #role:Cip1852Role;readonly #index:number;
  private constructor(account:number,role:Cip1852Role,index:number){this.#account=softIndex(account,"account");this.#role=pathRole(role);this.#index=softIndex(index,"index");}
  public static new(account:number,role:Cip1852Role,index:number):Cip1852Path{return new Cip1852Path(account,role,index);}
  public account():number{return this.#account;}
  public role():Cip1852Role{return this.#role;}
  public index():number{return this.#index;}
  public indices():readonly number[]{return [1852+HARDENED,1815+HARDENED,this.#account+HARDENED,this.#role,this.#index];}
  public toString():string{return `m/1852'/1815'/${this.#account}'/${this.#role}/${this.#index}`;}
}

export function cip1852RootFromIcarusEntropy(entropy:Uint8Array,password:Uint8Array=new Uint8Array()):Bip32PrivateKey {
  return Bip32PrivateKey.from_bip39_entropy(entropy,password);
}
export function deriveCip1852AccountPrivate(root:Bip32PrivateKey,account:number):Bip32PrivateKey {
  return root.derive(1852+HARDENED).derive(1815+HARDENED).derive(softIndex(account,"account")+HARDENED);
}
export function cip1852AccountPublic(accountPrivate:Bip32PrivateKey):Bip32PublicKey { return accountPrivate.to_public(); }
export function deriveCip1852Private(root:Bip32PrivateKey,path:Cip1852Path):Bip32PrivateKey {
  return deriveCip1852AccountPrivate(root,path.account()).derive(path.role()).derive(path.index());
}
export function deriveCip1852Public(accountPublic:Bip32PublicKey,role:Cip1852Role,index:number):Bip32PublicKey {
  return accountPublic.derive(pathRole(role)).derive(softIndex(index,"index"));
}
