import { ProtocolMagic } from "@xray-network/xray-cardano-lib-core";
import {
  BlockHeaderHash,
  Ed25519KeyHash,
  PublicKey,
  blake2b256,
} from "@xray-network/xray-cardano-lib-crypto";
import { LinearFee } from "../../ledger/operations.js";
import {
  base64,
  bigintValue,
  integer,
  record,
  stringValue,
} from "../shared/genesis-json.js";
import { ByronAddress } from "./address.js";

export interface ParsedByronGenesis {
  readonly genesisPrev:BlockHeaderHash;
  readonly epochStabilityDepth:number;
  readonly protocolMagic:ProtocolMagic;
  readonly feePolicy:LinearFee;
  readonly startTime:number;
  readonly slotDurationMilliseconds:number;
  readonly avvmDistribution:ReadonlyMap<string,bigint>;
  readonly nonAvvmBalances:ReadonlyMap<string,bigint>;
  readonly bootStakeholders:ReadonlySet<string>;
}

export function parseByronGenesis(json:string):ParsedByronGenesis {
  const raw:unknown=JSON.parse(json);const data=record(raw,"Byron genesis");
  const protocol=record(data["protocolConsts"],"protocolConsts");const block=record(data["blockVersionData"],"blockVersionData");const fees=record(block["txFeePolicy"],"txFeePolicy");
  const avvm=new Map<string,bigint>();for(const [key,value] of Object.entries(record(data["avvmDistr"],"avvmDistr"))){const decoded=base64(key,true);PublicKey.from_bytes(decoded);avvm.set(key,bigintValue(value,`avvmDistr.${key}`));}
  const nonAvvm=new Map<string,bigint>();for(const [key,value] of Object.entries(record(data["nonAvvmBalances"],"nonAvvmBalances"))){ByronAddress.from_base58(key);nonAvvm.set(key,bigintValue(value,`nonAvvmBalances.${key}`));}
  const stakeholders=new Set<string>();for(const key of Object.keys(record(data["bootStakeholders"],"bootStakeholders"))){Ed25519KeyHash.from_hex(key);stakeholders.add(key);}
  const heavy=record(data["heavyDelegation"],"heavyDelegation");for(const stakeholder of stakeholders){const entry=record(heavy[stakeholder],`heavyDelegation.${stakeholder}`);for(const field of ["issuerPk","delegatePk"]){const decoded=base64(stringValue(entry[field],`${field}`));if(decoded.length!==64)throw new RangeError(`${field} must be a 64-byte extended public key`);}}
  return {
    genesisPrev:BlockHeaderHash.from_raw_bytes(blake2b256(new TextEncoder().encode(JSON.stringify(raw)))),
    epochStabilityDepth:integer(protocol["k"],"protocolConsts.k"),
    protocolMagic:ProtocolMagic.new(integer(protocol["protocolMagic"],"protocolConsts.protocolMagic")),
    feePolicy:LinearFee.new(bigintValue(fees["multiplier"],"txFeePolicy.multiplier"),bigintValue(fees["summand"],"txFeePolicy.summand"),0n),
    startTime:integer(data["startTime"],"startTime"),
    slotDurationMilliseconds:Number(bigintValue(block["slotDuration"],"blockVersionData.slotDuration")),
    avvmDistribution:avvm,
    nonAvvmBalances:nonAvvm,
    bootStakeholders:stakeholders,
  };
}
