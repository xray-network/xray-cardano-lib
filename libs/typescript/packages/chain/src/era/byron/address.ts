import {
  BYRON_MAINNET_NETWORK_MAGIC,
  ProtocolMagic,
  bytesToHex,
  copyBytes,
  decodeCbor,
  encodeCbor,
  hexToBytes,
} from "@xray-network/xray-cardano-lib-core";
import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import { decodeBech32, encodeBech32 } from "@xray-network/xray-cardano-lib-core/bech32";
import {
  Bip32PublicKey,
  PublicKey,
  blake2b224,
  sha3_256,
} from "@xray-network/xray-cardano-lib-crypto";
import { Address, AddressKind } from "../../address/index.js";
import { Crc32, crc32, decodeBase58, encodeBase58 } from "./encoding.js";

export enum ByronAddrType { PublicKey=0,Script=1,Redeem=2 }

abstract class FixedByronBytes {
  readonly #bytes:Uint8Array;
  protected constructor(name:string,bytes:Uint8Array,length:number){if(bytes.length!==length)throw new RangeError(`${name} must be ${length} bytes`);this.#bytes=copyBytes(bytes);}
  public to_raw_bytes():Uint8Array{return copyBytes(this.#bytes);}
  public to_hex():string{return bytesToHex(this.#bytes);}
  public to_bech32(prefix:string):string{return encodeBech32(prefix,this.#bytes);}
}

export class ByronScript extends FixedByronBytes {
  private constructor(bytes:Uint8Array){super("ByronScript",bytes,32);}
  public static from_raw_bytes(bytes:Uint8Array):ByronScript{return new ByronScript(bytes);}
  public static from_hex(hex:string):ByronScript{return new ByronScript(hexToBytes(hex));}
  public static from_bech32(value:string):ByronScript{return new ByronScript(decodeBech32(value).bytes);}
}

export class StakeholderId extends FixedByronBytes {
  private constructor(bytes:Uint8Array){super("StakeholderId",bytes,28);}
  public static from_raw_bytes(bytes:Uint8Array):StakeholderId{return new StakeholderId(bytes);}
  public static from_hex(hex:string):StakeholderId{return new StakeholderId(hexToBytes(hex));}
  public static from_bech32(value:string):StakeholderId{return new StakeholderId(decodeBech32(value).bytes);}
  public static new(publicKey:Bip32PublicKey):StakeholderId{
    const encoded=encodeCbor({kind:"bytes",value:publicKey.to_raw_bytes(),encoding:{kind:"definite",width:0}});
    return new StakeholderId(blake2b224(sha3_256(encoded)));
  }
}

export enum SpendingDataKind { SpendingDataPubKey=0,SpendingDataScript=1,SpendingDataRedeem=2 }

export class SpendingData {
  readonly #kind:SpendingDataKind;readonly #bytes:Uint8Array;
  private constructor(kind:SpendingDataKind,bytes:Uint8Array){const length=kind===SpendingDataKind.SpendingDataPubKey?64:kind===SpendingDataKind.SpendingDataScript?32:32;if(bytes.length!==length)throw new RangeError("invalid Byron spending data length");this.#kind=kind;this.#bytes=copyBytes(bytes);}
  public static new_spending_data_pub_key(value:Bip32PublicKey):SpendingData{return new SpendingData(SpendingDataKind.SpendingDataPubKey,value.to_raw_bytes());}
  public static new_spending_data_script(value:ByronScript):SpendingData{return new SpendingData(SpendingDataKind.SpendingDataScript,value.to_raw_bytes());}
  public static new_spending_data_redeem(value:PublicKey):SpendingData{return new SpendingData(SpendingDataKind.SpendingDataRedeem,value.to_raw_bytes());}
  public static from_cbor_bytes(bytes:Uint8Array):SpendingData{const node=decodeCbor(bytes);if(node.kind!=="array"||node.values.length!==2||node.values[0]?.kind!=="unsigned"||node.values[1]?.kind!=="bytes"||node.values[0].value>2n)throw new TypeError("invalid Byron spending data");return new SpendingData(Number(node.values[0].value),node.values[1].value);}
  public static from_cbor_hex(hex:string):SpendingData{return SpendingData.from_cbor_bytes(hexToBytes(hex));}
  public kind():SpendingDataKind{return this.#kind;}
  public as_spending_data_pub_key():Bip32PublicKey|undefined{return this.#kind===SpendingDataKind.SpendingDataPubKey?Bip32PublicKey.from_raw_bytes(this.#bytes):undefined;}
  public as_spending_data_script():ByronScript|undefined{return this.#kind===SpendingDataKind.SpendingDataScript?ByronScript.from_raw_bytes(this.#bytes):undefined;}
  public as_spending_data_redeem():PublicKey|undefined{return this.#kind===SpendingDataKind.SpendingDataRedeem?PublicKey.from_bytes(this.#bytes):undefined;}
  public to_cbor_bytes():Uint8Array{return encodeCbor({kind:"array",values:[{kind:"unsigned",value:BigInt(this.#kind),encoding:{width:0}},{kind:"bytes",value:this.#bytes,encoding:{kind:"definite",width:0}}],encoding:{kind:"definite",width:0}});}
  public to_cbor_hex():string{return bytesToHex(this.to_cbor_bytes());}
}

export enum StakeDistributionKind { SingleKey=0,BootstrapEra=1 }

export class StakeDistribution {
  readonly #stakeholder:StakeholderId|undefined;
  private constructor(stakeholder?:StakeholderId){this.#stakeholder=stakeholder;}
  public static new_single_key(value:StakeholderId):StakeDistribution{return new StakeDistribution(StakeholderId.from_raw_bytes(value.to_raw_bytes()));}
  public static new_bootstrap_era():StakeDistribution{return new StakeDistribution();}
  public static from_cbor_bytes(bytes:Uint8Array):StakeDistribution{const node=decodeCbor(bytes);if(node.kind!=="array")throw new TypeError("invalid Byron stake distribution");if(node.values.length===1&&node.values[0]?.kind==="unsigned"&&node.values[0].value===1n)return StakeDistribution.new_bootstrap_era();if(node.values.length===2&&node.values[0]?.kind==="unsigned"&&node.values[0].value===0n&&node.values[1]?.kind==="bytes")return StakeDistribution.new_single_key(StakeholderId.from_raw_bytes(node.values[1].value));throw new TypeError("invalid Byron stake distribution");}
  public static from_cbor_hex(hex:string):StakeDistribution{return StakeDistribution.from_cbor_bytes(hexToBytes(hex));}
  public kind():StakeDistributionKind{return this.#stakeholder===undefined?StakeDistributionKind.BootstrapEra:StakeDistributionKind.SingleKey;}
  public as_single_key():StakeholderId|undefined{return this.#stakeholder===undefined?undefined:StakeholderId.from_raw_bytes(this.#stakeholder.to_raw_bytes());}
  public to_cbor_bytes():Uint8Array{const values:CborValue[]=this.#stakeholder===undefined?[{kind:"unsigned",value:1n,encoding:{width:0}}]:[{kind:"unsigned",value:0n,encoding:{width:0}},{kind:"bytes",value:this.#stakeholder.to_raw_bytes(),encoding:{kind:"definite",width:0}}];return encodeCbor({kind:"array",values,encoding:{kind:"definite",width:0}});}
  public to_cbor_hex():string{return bytesToHex(this.to_cbor_bytes());}
}

export class AddressContent {
  readonly #node:CborValue;
  private constructor(node:CborValue){if(node.kind!=="array"||node.values.length!==3||node.values[0]?.kind!=="bytes"||node.values[0].value.length!==28||node.values[1]?.kind!=="map"||node.values[2]?.kind!=="unsigned"||node.values[2].value>2n)throw new TypeError("invalid Byron address content");this.#node=node;}
  public static from_cbor_bytes(bytes:Uint8Array):AddressContent{return new AddressContent(decodeCbor(bytes));}
  public static from_cbor_hex(hex:string):AddressContent{return AddressContent.from_cbor_bytes(hexToBytes(hex));}
  public static new(addressId:AddressId,attributes:AddrAttributes,kind:ByronAddrType):AddressContent{return AddressContent.from_cbor_bytes(encodeCbor({kind:"array",values:[{kind:"bytes",value:addressId.to_raw_bytes(),encoding:{kind:"definite",width:0}},decodeCbor(attributes.to_cbor_bytes()),{kind:"unsigned",value:BigInt(kind),encoding:{width:0}}],encoding:{kind:"definite",width:0}}));}
  public static hash_and_create(kind:ByronAddrType,spendingData:SpendingData,attributes:AddrAttributes):AddressContent{return AddressContent.new(AddressId.new(kind,spendingData,attributes),attributes,kind);}
  public static new_redeem(publicKey:PublicKey,protocolMagic?:ProtocolMagic|null):AddressContent{return AddressContent.hash_and_create(ByronAddrType.Redeem,SpendingData.new_spending_data_redeem(publicKey),AddrAttributes.new_bootstrap_era(undefined,protocolMagic));}
  public static new_simple(publicKey:Bip32PublicKey,protocolMagic?:ProtocolMagic|null):AddressContent{return AddressContent.hash_and_create(ByronAddrType.PublicKey,SpendingData.new_spending_data_pub_key(publicKey),AddrAttributes.new_bootstrap_era(undefined,protocolMagic));}
  public static icarus_from_key(publicKey:Bip32PublicKey,protocolMagic:ProtocolMagic):AddressContent{return AddressContent.new_simple(publicKey,protocolMagic.to_int()===BYRON_MAINNET_NETWORK_MAGIC?undefined:protocolMagic);}
  public address_id():AddressId{if(this.#node.kind!=="array"||this.#node.values[0]?.kind!=="bytes")throw new TypeError("invalid content");return AddressId.from_raw_bytes(this.#node.values[0].value);}
  public addr_attributes():AddrAttributes{if(this.#node.kind!=="array"||this.#node.values[1]===undefined)throw new TypeError("invalid content");return AddrAttributes.from_cbor_bytes(encodeCbor(this.#node.values[1]));}
  public addr_type():ByronAddrType{if(this.#node.kind!=="array"||this.#node.values[2]?.kind!=="unsigned")throw new TypeError("invalid content");return Number(this.#node.values[2].value);}
  public byron_protocol_magic():ProtocolMagic{return this.addr_attributes().protocol_magic()??ProtocolMagic.new(BYRON_MAINNET_NETWORK_MAGIC);}
  public network_id():number{const magic=this.byron_protocol_magic().to_int();if(magic===BYRON_MAINNET_NETWORK_MAGIC)return 1;if([1_097_911_063,1,2,4].includes(magic))return 0;throw new RangeError(`unknown Byron protocol magic ${magic}`);}
  public identical_with_pubkey(publicKey:Bip32PublicKey):boolean{return AddressContent.hash_and_create(ByronAddrType.PublicKey,SpendingData.new_spending_data_pub_key(publicKey),this.addr_attributes()).to_cbor_hex()===this.to_cbor_hex();}
  public to_address():ByronAddress{return ByronAddress.from_address_content(this);}
  public to_cbor_bytes():Uint8Array{return encodeCbor(this.#node);}public to_cbor_hex():string{return bytesToHex(this.to_cbor_bytes());}
}

export class AddressId {
  readonly #bytes:Uint8Array;private constructor(bytes:Uint8Array){if(bytes.length!==28)throw new RangeError("AddressId must be 28 bytes");this.#bytes=copyBytes(bytes);}
  public static from_raw_bytes(bytes:Uint8Array):AddressId{return new AddressId(bytes);}public static from_hex(hex:string):AddressId{return new AddressId(hexToBytes(hex));}
  public static from_bech32(value:string):AddressId{return new AddressId(decodeBech32(value).bytes);}
  public static new(kind:ByronAddrType,spendingData:SpendingData,attributes:AddrAttributes):AddressId{const digestInput=encodeCbor({kind:"array",values:[{kind:"unsigned",value:BigInt(kind),encoding:{width:0}},decodeCbor(spendingData.to_cbor_bytes()),decodeCbor(attributes.to_cbor_bytes())],encoding:{kind:"definite",width:0}});return new AddressId(blake2b224(sha3_256(digestInput)));}
  public to_raw_bytes():Uint8Array{return copyBytes(this.#bytes);}public to_hex():string{return bytesToHex(this.#bytes);}public to_bech32(prefix:string):string{return encodeBech32(prefix,this.#bytes);}
}

export class HDAddressPayload {
  readonly #bytes:Uint8Array;private constructor(bytes:Uint8Array){this.#bytes=copyBytes(bytes);}public static new(bytes:Uint8Array):HDAddressPayload{return new HDAddressPayload(bytes);}public get():Uint8Array{return copyBytes(this.#bytes);}
}

export class AddrAttributes {
  #stakeDistribution:StakeDistribution|undefined;#derivationPath:HDAddressPayload|undefined;#protocolMagic:number|undefined;
  private constructor(){}
  public static new():AddrAttributes{return new AddrAttributes();}
  public static new_bootstrap_era(hdap?:HDAddressPayload|null,protocolMagic?:ProtocolMagic|null):AddrAttributes{const value=new AddrAttributes();value.#stakeDistribution=StakeDistribution.new_bootstrap_era();if(hdap!=null)value.#derivationPath=HDAddressPayload.new(hdap.get());if(protocolMagic!=null&&protocolMagic.to_int()!==BYRON_MAINNET_NETWORK_MAGIC)value.#protocolMagic=protocolMagic.to_int();return value;}
  public static new_single_key(publicKey:Bip32PublicKey,hdap:HDAddressPayload|null|undefined,protocolMagic:ProtocolMagic):AddrAttributes{const value=new AddrAttributes();value.#stakeDistribution=StakeDistribution.new_single_key(StakeholderId.new(publicKey));if(hdap!=null)value.#derivationPath=HDAddressPayload.new(hdap.get());value.#protocolMagic=protocolMagic.to_int();return value;}
  public static from_cbor_bytes(bytes:Uint8Array):AddrAttributes{const node=decodeCbor(bytes);if(node.kind!=="map")throw new TypeError("attributes must be a map");const value=new AddrAttributes();const seen=new Set<bigint>();for(const [key,item] of node.entries){if(key.kind!=="unsigned"||item.kind!=="bytes"||seen.has(key.value))throw new TypeError("invalid attribute");seen.add(key.value);if(key.value===0n)value.#stakeDistribution=StakeDistribution.from_cbor_bytes(item.value);else if(key.value===1n){const embedded=decodeCbor(item.value);if(embedded.kind!=="bytes")throw new TypeError("invalid derivation path");value.#derivationPath=HDAddressPayload.new(embedded.value);}else if(key.value===2n){const embedded=decodeCbor(item.value);if(embedded.kind!=="unsigned"||embedded.value>0xffff_ffffn)throw new TypeError("invalid protocol magic");value.#protocolMagic=Number(embedded.value);}else throw new TypeError("unknown Byron address attribute");}return value;}
  public static from_cbor_hex(hex:string):AddrAttributes{return AddrAttributes.from_cbor_bytes(hexToBytes(hex));}
  public stake_distribution():StakeDistribution|undefined{return this.#stakeDistribution===undefined?undefined:StakeDistribution.from_cbor_bytes(this.#stakeDistribution.to_cbor_bytes());}public derivation_path():HDAddressPayload|undefined{return this.#derivationPath===undefined?undefined:HDAddressPayload.new(this.#derivationPath.get());}public protocol_magic():ProtocolMagic|undefined{return this.#protocolMagic===undefined?undefined:ProtocolMagic.new(this.#protocolMagic);}
  public set_stake_distribution(value:StakeDistribution):void{this.#stakeDistribution=StakeDistribution.from_cbor_bytes(value.to_cbor_bytes());}
  public set_derivation_path(value:HDAddressPayload):void{this.#derivationPath=value;}public set_protocol_magic(value:{to_int():number}):void{this.#protocolMagic=value.to_int();}
  public to_cbor_bytes():Uint8Array{const entries:Array<readonly[CborValue,CborValue]>=[];if(this.#stakeDistribution?.kind()===StakeDistributionKind.SingleKey)entries.push([{kind:"unsigned",value:0n,encoding:{width:0}},{kind:"bytes",value:this.#stakeDistribution.to_cbor_bytes(),encoding:{kind:"definite",width:0}}]);if(this.#derivationPath!==undefined)entries.push([{kind:"unsigned",value:1n,encoding:{width:0}},{kind:"bytes",value:encodeCbor({kind:"bytes",value:this.#derivationPath.get(),encoding:{kind:"definite",width:0}}),encoding:{kind:"definite",width:0}}]);if(this.#protocolMagic!==undefined)entries.push([{kind:"unsigned",value:2n,encoding:{width:0}},{kind:"bytes",value:encodeCbor({kind:"unsigned",value:BigInt(this.#protocolMagic),encoding:{width:0}}),encoding:{kind:"definite",width:0}}]);return encodeCbor({kind:"map",entries,encoding:{kind:"definite",width:0}});}public to_cbor_hex():string{return bytesToHex(this.to_cbor_bytes());}
}

export class ByronAddress {
  readonly #content:AddressContent;readonly #crc:number;
  private constructor(content:AddressContent,crc:number){this.#content=AddressContent.from_cbor_bytes(content.to_cbor_bytes());this.#crc=crc>>>0;}
  public static new(content:AddressContent,checksum:Crc32):ByronAddress{return new ByronAddress(content,checksum.finalize());}
  public static from_address_content(content:AddressContent):ByronAddress{return new ByronAddress(content,crc32(content.to_cbor_bytes()));}
  public static from_cbor_bytes(bytes:Uint8Array):ByronAddress{const node=decodeCbor(bytes);if(node.kind!=="array"||node.values.length!==2||node.values[0]?.kind!=="tag"||node.values[0].tag!==24n||node.values[0].value.kind!=="bytes"||node.values[1]?.kind!=="unsigned"||node.values[1].value>0xffff_ffffn)throw new TypeError("invalid Byron address");const content=AddressContent.from_cbor_bytes(node.values[0].value.value);const expected=crc32(content.to_cbor_bytes()),found=Number(node.values[1].value);if(found!==expected)throw new TypeError(`invalid Byron CRC32 ${found}, expected ${expected}`);return new ByronAddress(content,found);}
  public static from_cbor_hex(hex:string):ByronAddress{return ByronAddress.from_cbor_bytes(hexToBytes(hex));}
  public static from_base58(value:string):ByronAddress{return ByronAddress.from_cbor_bytes(decodeBase58(value));}
  public static is_valid(value:string):boolean{try{ByronAddress.from_base58(value);return true;}catch{return false;}}
  public static from_address(value:Address):ByronAddress|undefined{return value.kind()===AddressKind.Byron?ByronAddress.from_cbor_bytes(value.to_raw_bytes()):undefined;}
  public content():AddressContent{return AddressContent.from_cbor_bytes(this.#content.to_cbor_bytes());}public crc():Crc32{const value=Crc32.new();value.update(this.#content.to_cbor_bytes());return value;}
  public to_cbor_bytes():Uint8Array{return encodeCbor({kind:"array",values:[{kind:"tag",tag:24n,value:{kind:"bytes",value:this.#content.to_cbor_bytes(),encoding:{kind:"definite",width:0}},encoding:{width:0}},{kind:"unsigned",value:BigInt(this.#crc),encoding:{width:4}}],encoding:{kind:"definite",width:0}});}
  public to_cbor_hex():string{return bytesToHex(this.to_cbor_bytes());}public to_base58():string{return encodeBase58(this.to_cbor_bytes());}public to_address():Address{return Address.from_raw_bytes(this.to_cbor_bytes());}
}
