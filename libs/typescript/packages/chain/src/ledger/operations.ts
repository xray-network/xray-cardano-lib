import {
  BYRON_MAINNET_NETWORK_MAGIC,
  BYRON_TESTNET_NETWORK_MAGIC,
  BigInteger,
  PREPROD_NETWORK_MAGIC,
  PREVIEW_NETWORK_MAGIC,
  ProtocolMagic,
  SANCHO_TESTNET_NETWORK_MAGIC,
  UINT64_MAX,
  bytesToHex,
  decodeCbor,
  encodeCbor,
  hexToBytes,
  Int,
} from "@xray-network/cardano-core";
import type { CborValue } from "@xray-network/cardano-core";
import {
  AuxiliaryDataHash,
  Bip32PrivateKey,
  DatumHash,
  Ed25519Signature,
  LegacyDaedalusPrivateKey,
  PrivateKey,
  PublicKey,
  ScriptDataHash,
  ScriptHash,
  TransactionHash,
  blake2b224,
  blake2b256,
  legacyPublicKey,
  legacySign,
} from "@xray-network/cardano-crypto";
import { Address } from "../address/index.js";
import { AddressContent, ByronAddress } from "../era/byron/address.js";
import { BootstrapWitness } from "../era/byron/transaction.js";
import {
  CostModels,
  ExUnitPrices,
  ExUnits,
  MetadatumList,
  MetadatumMap,
  TransactionMetadatum,
  Value,
  Vkeywitness,
} from "../era/conway/model.js";
import { PlutusData } from "../era/shared/models.js";

interface CborSerializable { to_cbor_bytes(): Uint8Array }

export class LinearFee {
  readonly #coefficient: bigint;
  readonly #constant: bigint;
  readonly #referenceScriptCost: bigint;
  private constructor(coefficient:bigint,constant:bigint,referenceScriptCost:bigint){for(const value of [coefficient,constant,referenceScriptCost])if(value<0n||value>UINT64_MAX)throw new RangeError("fee parameter must fit uint64");this.#coefficient=coefficient;this.#constant=constant;this.#referenceScriptCost=referenceScriptCost;}
  public static new(coefficient:bigint,constant:bigint,refScriptCostPerByte:bigint):LinearFee{return new LinearFee(coefficient,constant,refScriptCostPerByte);}
  public coefficient():bigint{return this.#coefficient;}public constant():bigint{return this.#constant;}public ref_script_cost_per_byte():bigint{return this.#referenceScriptCost;}
}

export class NetworkInfo {
  readonly #networkId:number;readonly #protocolMagic:ProtocolMagic;
  private constructor(networkId:number,protocolMagic:ProtocolMagic){if(!Number.isInteger(networkId)||networkId<0||networkId>255)throw new RangeError("network id must fit uint8");this.#networkId=networkId;this.#protocolMagic=ProtocolMagic.new(protocolMagic.to_int());}
  public static new(networkId:number,protocolMagic:ProtocolMagic):NetworkInfo{return new NetworkInfo(networkId,protocolMagic);}
  public static testnet():NetworkInfo{return NetworkInfo.new(0,ProtocolMagic.new(BYRON_TESTNET_NETWORK_MAGIC));}
  public static mainnet():NetworkInfo{return NetworkInfo.new(1,ProtocolMagic.new(BYRON_MAINNET_NETWORK_MAGIC));}
  public static preview():NetworkInfo{return NetworkInfo.new(0,ProtocolMagic.new(PREVIEW_NETWORK_MAGIC));}
  public static preprod():NetworkInfo{return NetworkInfo.new(0,ProtocolMagic.new(PREPROD_NETWORK_MAGIC));}
  public static sancho_testnet():NetworkInfo{return NetworkInfo.new(0,ProtocolMagic.new(SANCHO_TESTNET_NETWORK_MAGIC));}
  public network_id():number{return this.#networkId;}public protocol_magic():ProtocolMagic{return ProtocolMagic.new(this.#protocolMagic.to_int());}
}

export function hash_auxiliary_data(value:CborSerializable):AuxiliaryDataHash{return AuxiliaryDataHash.from_raw_bytes(blake2b256(value.to_cbor_bytes()));}
export function hash_transaction(value:CborSerializable):TransactionHash{return TransactionHash.from_raw_bytes(blake2b256(value.to_cbor_bytes()));}
export function hash_plutus_data(value:PlutusData):DatumHash{return DatumHash.from_raw_bytes(blake2b256(value.to_cbor_bytes()));}

export function hash_script(namespace:0|1|2|3,script:Uint8Array):ScriptHash {
  return ScriptHash.from_raw_bytes(blake2b224(Uint8Array.from([namespace,...script])));
}

export function hash_script_data(redeemers:CborSerializable|undefined,costModels:CostModels,datums:readonly PlutusData[]|undefined):ScriptDataHash {
  const chunks:Uint8Array[]=[];
  if(redeemers===undefined&&datums!==undefined){chunks.push(Uint8Array.of(0xa0));chunks.push(encodeCbor({kind:"tag",tag:258n,value:{kind:"array",values:datums.map((datum)=>decodeCbor(datum.to_cbor_bytes())),encoding:{kind:"definite",width:0}},encoding:{width:2}}));chunks.push(Uint8Array.of(0xa0));const length=chunks.reduce((sum,chunk)=>sum+chunk.length,0);const joined=new Uint8Array(length);let offset=0;for(const chunk of chunks){joined.set(chunk,offset);offset+=chunk.length;}return ScriptDataHash.from_raw_bytes(blake2b256(joined));}
  chunks.push(redeemers?.to_cbor_bytes()??Uint8Array.of(0x80));
  if(datums!==undefined){chunks.push(encodeCbor({kind:"tag",tag:258n,value:{kind:"array",values:datums.map((datum)=>decodeCbor(datum.to_cbor_bytes())),encoding:{kind:"definite",width:0}},encoding:{width:2}}));}
  chunks.push(costModels.language_views_encoding());
  const length=chunks.reduce((sum,chunk)=>sum+chunk.length,0);const joined=new Uint8Array(length);let offset=0;for(const chunk of chunks){joined.set(chunk,offset);offset+=chunk.length;}
  return ScriptDataHash.from_raw_bytes(blake2b256(joined));
}

export function calc_script_data_hash(redeemers:CborSerializable|undefined,datums:readonly PlutusData[]|undefined,costModels:CostModels):ScriptDataHash|undefined {
  return redeemers===undefined&&datums===undefined?undefined:hash_script_data(redeemers,costModels,datums);
}

export function make_vkey_witness(bodyHash:TransactionHash,key:PrivateKey):Vkeywitness {
  const publicKey=key.to_public();const signature=key.sign(bodyHash.to_raw_bytes());
  return vkeyWitness(publicKey,signature);
}

function vkeyWitness(publicKey:PublicKey,signature:Ed25519Signature):Vkeywitness {
  return Vkeywitness.from_cbor_bytes(encodeCbor({kind:"array",values:[
    {kind:"bytes",value:publicKey.to_raw_bytes(),encoding:{kind:"definite",width:0}},
    {kind:"bytes",value:signature.to_raw_bytes(),encoding:{kind:"definite",width:0}},
  ],encoding:{kind:"definite",width:0}}));
}

function checkedCoin(value:bigint):bigint { if(value<0n||value>UINT64_MAX)throw new RangeError("coin arithmetic overflow");return value; }
export function min_no_script_fee(transaction:CborSerializable,linearFee:LinearFee):bigint{return checkedCoin(BigInt(transaction.to_cbor_bytes().length)*linearFee.coefficient()+linearFee.constant());}
export function min_ref_script_fee(linearFee:LinearFee,totalSize:bigint):bigint {
  if(totalSize<0n||totalSize>UINT64_MAX)throw new RangeError("reference script size must fit uint64");let remaining=totalSize,priceNumerator=linearFee.ref_script_cost_per_byte(),priceDenominator=1n,totalNumerator=0n,totalDenominator=1n;
  const gcd=(left:bigint,right:bigint):bigint=>{let a=left<0n?-left:left,b=right<0n?-right:right;while(b!==0n){const next=a%b;a=b;b=next;}return a;};
  while(remaining>0n){const tier=remaining>25_600n?25_600n:remaining;totalNumerator=totalNumerator*priceDenominator+tier*priceNumerator*totalDenominator;totalDenominator*=priceDenominator;const divisor=gcd(totalNumerator,totalDenominator);totalNumerator/=divisor;totalDenominator/=divisor;remaining-=tier;priceNumerator*=6n;priceDenominator*=5n;}
  return checkedCoin(totalNumerator/totalDenominator);
}
function exUnitsFromNode(node:CborValue):ExUnits { if(node.kind!=="array"||node.values.length!==2||node.values[0]?.kind!=="unsigned"||node.values[1]?.kind!=="unsigned")throw new TypeError("invalid execution units");return ExUnits.new(node.values[0].value,node.values[1].value); }
function redeemerExUnits(value:CborSerializable):ExUnits[] { const node=decodeCbor(value.to_cbor_bytes());if(node.kind==="array")return node.values.map((item)=>{if(item.kind!=="array"||item.values[3]===undefined)throw new TypeError("invalid legacy redeemer");return exUnitsFromNode(item.values[3]);});if(node.kind==="map")return node.entries.map(([,item])=>{if(item.kind!=="array"||item.values[1]===undefined)throw new TypeError("invalid redeemer value");return exUnitsFromNode(item.values[1]);});throw new TypeError("invalid redeemers"); }
function transactionRedeemers(transaction:CborSerializable):CborSerializable|undefined { const node=decodeCbor(transaction.to_cbor_bytes());if(node.kind!=="array"||node.values[1]?.kind!=="map")return undefined;const found=node.values[1].entries.find(([key])=>key.kind==="unsigned"&&key.value===5n)?.[1];return found===undefined?undefined:{to_cbor_bytes:()=>encodeCbor(found)}; }
function scriptFeeFor(total:ExUnits,prices:ExUnitPrices):bigint { const memory=prices.mem_price(),steps=prices.step_price();const denominator=memory.denominator()*steps.denominator();if(denominator<=0n)throw new RangeError("price denominator must be positive");const numerator=total.mem()*memory.numerator()*steps.denominator()+total.steps()*steps.numerator()*memory.denominator();return checkedCoin((numerator+denominator-1n)/denominator); }
export function min_script_fee(transaction:CborSerializable,prices:ExUnitPrices):bigint { const redeemers=transactionRedeemers(transaction);return redeemers===undefined?0n:scriptFeeFor(compute_total_ex_units(redeemers),prices); }
export function min_fee(transaction:CborSerializable,linearFee:LinearFee,prices:ExUnitPrices,referenceScriptSize:bigint):bigint{return checkedCoin(min_no_script_fee(transaction,linearFee)+min_script_fee(transaction,prices)+min_ref_script_fee(linearFee,referenceScriptSize));}

function cborIntegerSize(value:bigint):number { if(value<24n)return 1;if(value<=0xffn)return 2;if(value<=0xffffn)return 3;if(value<=0xffff_ffffn)return 5;return 9; }
function transactionOutputCoin(output:CborSerializable):bigint { const node=decodeCbor(output.to_cbor_bytes());let amount:CborValue|undefined;if(node.kind==="array")amount=node.values[1];else if(node.kind==="map")amount=node.entries.find(([key])=>key.kind==="unsigned"&&key.value===1n)?.[1];if(amount?.kind==="unsigned")return amount.value;if(amount?.kind==="array"&&amount.values[0]?.kind==="unsigned")return amount.values[0].value;throw new TypeError("transaction output has no coin value"); }
export function min_ada_required(output:CborSerializable,coinsPerUtxoByte:bigint,currentCoin?:bigint):bigint { const oldSize=cborIntegerSize(currentCoin??transactionOutputCoin(output));let latest=oldSize;for(;;){const tentative=checkedCoin(BigInt(output.to_cbor_bytes().length+160+latest-oldSize)*coinsPerUtxoByte);const next=cborIntegerSize(tentative);if(next===latest)return tentative;latest=next;} }

export function encode_arbitrary_bytes_as_metadatum(bytes:Uint8Array):TransactionMetadatum { const list=MetadatumList.new();for(let offset=0;offset<bytes.length;offset+=64)list.add(TransactionMetadatum.new_bytes(bytes.slice(offset,offset+64)));return TransactionMetadatum.new_list(list); }
export function decode_arbitrary_bytes_from_metadatum(value:TransactionMetadatum):Uint8Array|undefined { const list=value.as_list();if(list===undefined)return undefined;const chunks:Uint8Array[]=[];let length=0;for(let index=0;index<list.len();index+=1){const chunk=list.get(index).as_bytes();if(chunk===undefined)return undefined;chunks.push(chunk);length+=chunk.length;}const output=new Uint8Array(length);let offset=0;for(const chunk of chunks){output.set(chunk,offset);offset+=chunk.length;}return output; }

export enum MetadataJsonSchema { NoConversions=0,BasicConversions=1,DetailedSchema=2 }
export enum CardanoNodePlutusDatumSchema { BasicConversions=0,DetailedSchema=1 }

const MAX_JSON_DEPTH=128;
const MAX_JSON_NODES=100_000;
interface JsonBudget { nodes:number }
function visitJson(depth:number,budget:JsonBudget):void { if(depth>MAX_JSON_DEPTH)throw new RangeError(`JSON nesting exceeds ${MAX_JSON_DEPTH}`);budget.nodes+=1;if(budget.nodes>MAX_JSON_NODES)throw new RangeError(`JSON value count exceeds ${MAX_JSON_NODES}`); }

function metadataFromJson(value:unknown,schema:MetadataJsonSchema,depth=0,budget:JsonBudget={nodes:0}):TransactionMetadatum {
  visitJson(depth,budget);
  if(schema===MetadataJsonSchema.DetailedSchema){if(typeof value!=="object"||value===null||Array.isArray(value)||Object.keys(value).length!==1)throw new TypeError("detailed metadata values must have one tag");const [tag,item]=Object.entries(value)[0] as [string,unknown];if(tag==="int"&&(typeof item==="number"||typeof item==="string"))return TransactionMetadatum.new_int(Int.from_str(String(item)));if(tag==="string"&&typeof item==="string")return TransactionMetadatum.new_text(item);if(tag==="bytes"&&typeof item==="string")return TransactionMetadatum.new_bytes(hexToBytes(item));if(tag==="list"&&Array.isArray(item)){const list=MetadatumList.new();for(const entry of item)list.add(metadataFromJson(entry,schema,depth+1,budget));return TransactionMetadatum.new_list(list);}if(tag==="map"&&Array.isArray(item)){const map=MetadatumMap.new();for(const entry of item){if(typeof entry!=="object"||entry===null||!("k" in entry)||!("v" in entry))throw new TypeError("invalid detailed metadata map entry");map.insert(metadataFromJson(entry.k,schema,depth+1,budget),metadataFromJson(entry.v,schema,depth+1,budget));}return TransactionMetadatum.new_map(map);}throw new TypeError(`invalid detailed metadata tag ${tag}`);}
  if(typeof value==="number"&&Number.isSafeInteger(value))return TransactionMetadatum.new_int(Int.new(BigInt(value)));if(typeof value==="string"){if(schema===MetadataJsonSchema.BasicConversions&&/^0x(?:[0-9a-f]{2})*$/iu.test(value))return TransactionMetadatum.new_bytes(hexToBytes(value.slice(2)));return TransactionMetadatum.new_text(value);}if(Array.isArray(value)){const list=MetadatumList.new();for(const entry of value)list.add(metadataFromJson(entry,schema,depth+1,budget));return TransactionMetadatum.new_list(list);}if(typeof value==="object"&&value!==null){const map=MetadatumMap.new();for(const [key,item] of Object.entries(value).sort(([left],[right])=>left.localeCompare(right))){const metadataKey=schema===MetadataJsonSchema.BasicConversions&&/^-?\d+$/u.test(key)?TransactionMetadatum.new_int(Int.from_str(key)):TransactionMetadatum.new_text(key);map.insert(metadataKey,metadataFromJson(item,schema,depth+1,budget));}return TransactionMetadatum.new_map(map);}throw new TypeError("null, booleans, and non-integral numbers are not metadata");
}
export function encode_json_str_to_metadatum(json:string,schema:MetadataJsonSchema):TransactionMetadatum{return metadataFromJson(JSON.parse(json),schema);}

function metadataToJson(value:TransactionMetadatum,schema:MetadataJsonSchema):unknown { if(schema===MetadataJsonSchema.DetailedSchema)return value.to_json_value();const integer=value.as_int();if(integer!==undefined)return Number(integer.to_str());const bytes=value.as_bytes();if(bytes!==undefined){if(schema===MetadataJsonSchema.NoConversions)throw new TypeError("bytes are not supported by this schema");return `0x${bytesToHex(bytes)}`;}const text=value.as_text();if(text!==undefined)return text;const list=value.as_list();if(list!==undefined)return Array.from({length:list.len()},(_,index)=>metadataToJson(list.get(index),schema));const map=value.as_map();if(map!==undefined){const output:Record<string,unknown>=Object.create(null) as Record<string,unknown>;for(const key of map.keys().values()){const keyText=key.as_text()??(schema===MetadataJsonSchema.BasicConversions?(key.as_int()?.to_str()??(key.as_bytes()===undefined?undefined:`0x${bytesToHex(key.as_bytes() as Uint8Array)}`)):undefined);if(keyText===undefined)throw new TypeError("metadata key cannot be represented by this schema");const found=map.get(key);if(found!==undefined)output[keyText]=metadataToJson(found,schema);}return output;}throw new TypeError("invalid metadatum"); }
export function decode_metadatum_to_json_str(value:TransactionMetadatum,schema:MetadataJsonSchema):string{return JSON.stringify(metadataToJson(value,schema));}

export function encode_json_str_to_plutus_datum(json:string,schema:CardanoNodePlutusDatumSchema):PlutusData { if(schema===CardanoNodePlutusDatumSchema.DetailedSchema)return PlutusData.from_json(json);const budget:JsonBudget={nodes:0};const convert=(value:unknown,depth=0):PlutusData=>{visitJson(depth,budget);if(typeof value==="number"&&Number.isSafeInteger(value))return PlutusData.new_integer(BigInteger.from_str(String(value)));if(typeof value==="string")return PlutusData.new_bytes(/^0x/u.test(value)?hexToBytes(value.slice(2)):new TextEncoder().encode(value));if(Array.isArray(value)){const node={kind:"array",values:value.map((item)=>decodeCbor(convert(item,depth+1).to_cbor_bytes())),encoding:{kind:"definite",width:0}} as const;return PlutusData.from_cbor_bytes(encodeCbor(node));}throw new TypeError("unsupported basic Plutus JSON");};return convert(JSON.parse(json)); }
export function decode_plutus_datum_to_json_str(value:PlutusData,schema:CardanoNodePlutusDatumSchema):string { if(schema===CardanoNodePlutusDatumSchema.DetailedSchema)return value.to_json();const node=decodeCbor(value.to_cbor_bytes());const convert=(item:ReturnType<typeof decodeCbor>):unknown=>{if(item.kind==="unsigned"||item.kind==="negative")return Number(item.value);if(item.kind==="bytes")return `0x${bytesToHex(item.value)}`;if(item.kind==="array")return item.values.map(convert);throw new TypeError("Plutus value cannot be represented in basic schema");};return JSON.stringify(convert(node)); }

export function genesis_txid_shelley(address:Address):TransactionHash{return TransactionHash.from_raw_bytes(blake2b256(address.to_raw_bytes()));}

export class ByronGenesisRedeem {
  readonly #txid:TransactionHash;readonly #address:ByronAddress;
  private constructor(txid:TransactionHash,address:ByronAddress){this.#txid=TransactionHash.from_raw_bytes(txid.to_raw_bytes());this.#address=ByronAddress.from_cbor_bytes(address.to_cbor_bytes());}
  public static new(txid:TransactionHash,address:ByronAddress):ByronGenesisRedeem{return new ByronGenesisRedeem(txid,address);}
  public txid():TransactionHash{return TransactionHash.from_raw_bytes(this.#txid.to_raw_bytes());}public address():ByronAddress{return ByronAddress.from_cbor_bytes(this.#address.to_cbor_bytes());}
}

export function genesis_txid_byron(publicKey:PublicKey,protocolMagic?:number|null):ByronGenesisRedeem { const content=AddressContent.new_redeem(publicKey,protocolMagic==null?undefined:ProtocolMagic.new(protocolMagic));const address=content.to_address();return ByronGenesisRedeem.new(TransactionHash.from_raw_bytes(blake2b256(address.to_cbor_bytes())),address); }

export function make_icarus_bootstrap_witness(txBodyHash:TransactionHash,address:ByronAddress,key:Bip32PrivateKey):BootstrapWitness { const rawKey=key.to_raw_key();return BootstrapWitness.new(rawKey.to_public(),rawKey.sign(txBodyHash.to_raw_bytes()),key.chaincode(),address.content().addr_attributes()); }

export function make_daedalus_bootstrap_witness(txBodyHash:TransactionHash,address:ByronAddress,key:LegacyDaedalusPrivateKey):BootstrapWitness { const extendedPublic=legacyPublicKey(key);return BootstrapWitness.new(PublicKey.from_bytes(extendedPublic.slice(0,32)),Ed25519Signature.from_raw_bytes(legacySign(key,txBodyHash.to_raw_bytes())),key.chaincode(),address.content().addr_attributes()); }
export function checked_value_sum(values:readonly Value[]):Value|undefined { let total=Value.zero();for(const value of values){const next=total.checked_add(value);if(next===undefined)return undefined;total=next;}return total; }

export function compute_total_ex_units(values:readonly ExUnits[]|CborSerializable):ExUnits { const items:readonly ExUnits[]="to_cbor_bytes" in values?redeemerExUnits(values):values;let total=ExUnits.new(0n,0n);for(const value of items){const next=total.checked_add(value);if(next===undefined)throw new RangeError("execution-unit total overflow");total=next;}return total; }
export function calc_script_data_hash_from_witness(witness:CborSerializable,costModels:CostModels):ScriptDataHash|undefined{const node=decodeCbor(witness.to_cbor_bytes());if(node.kind!=="map")throw new TypeError("witness set must be a map");const redeemerNode=node.entries.find(([key])=>key.kind==="unsigned"&&key.value===5n)?.[1];const datumNode=node.entries.find(([key])=>key.kind==="unsigned"&&key.value===4n)?.[1];if(redeemerNode===undefined||datumNode===undefined)return undefined;const datumArray=datumNode.kind==="tag"?datumNode.value:datumNode;if(datumArray.kind!=="array")throw new TypeError("witness datums must be an array");return calc_script_data_hash({to_cbor_bytes:()=>encodeCbor(redeemerNode)},datumArray.values.map((datum)=>PlutusData.from_cbor_bytes(encodeCbor(datum))),costModels);}
function bodyField(body:CborSerializable,keyValue:bigint):CborValue|undefined { const node=decodeCbor(body.to_cbor_bytes());if(node.kind!=="map")throw new TypeError("transaction body must be a map");return node.entries.find(([key])=>key.kind==="unsigned"&&key.value===keyValue)?.[1]; }
function setValues(node:CborValue|undefined):readonly CborValue[] { if(node===undefined)return [];const value=node.kind==="tag"&&node.tag===258n?node.value:node;if(value.kind!=="array")throw new TypeError("transaction-body set must be an array");return value.values; }
function coinNode(node:CborValue|undefined,name:string):bigint { if(node?.kind!=="unsigned"||node.value>UINT64_MAX)throw new TypeError(`${name} must be a coin`);return node.value; }
function certificateCoin(certificate:CborValue,poolDeposit:bigint,keyDeposit:bigint,deposits:boolean):bigint { if(certificate.kind!=="array"||certificate.values[0]?.kind!=="unsigned")throw new TypeError("invalid certificate");const tag=Number(certificate.values[0].value);if(deposits){if(tag===0)return keyDeposit;if(tag===3)return poolDeposit;const position=new Map([[7,2],[11,3],[12,3],[13,4],[16,2]]).get(tag);return position===undefined?0n:coinNode(certificate.values[position],"certificate deposit");}if(tag===1||tag===15)return keyDeposit;if(tag===4)return poolDeposit;const position=new Map([[8,2],[17,2]]).get(tag);return position===undefined?0n:coinNode(certificate.values[position],"certificate refund"); }
export function get_deposit(body:CborSerializable,poolDeposit:bigint,keyDeposit:bigint):bigint { let total=0n;for(const certificate of setValues(bodyField(body,4n)))total=checkedCoin(total+certificateCoin(certificate,poolDeposit,keyDeposit,true));for(const proposal of setValues(bodyField(body,20n))){if(proposal.kind!=="array")throw new TypeError("invalid proposal procedure");total=checkedCoin(total+coinNode(proposal.values[0],"proposal deposit"));}return total; }
export function get_implicit_input(body:CborSerializable,poolDeposit:bigint,keyDeposit:bigint):Value { let total=0n;const withdrawals=bodyField(body,5n);if(withdrawals!==undefined){if(withdrawals.kind!=="map")throw new TypeError("withdrawals must be a map");for(const [,coin] of withdrawals.entries)total=checkedCoin(total+coinNode(coin,"withdrawal"));}for(const certificate of setValues(bodyField(body,4n)))total=checkedCoin(total+certificateCoin(certificate,poolDeposit,keyDeposit,false));return Value.from_coin(total); }
