import { decodeBech32, encodeBech32 } from "@xray-network/xray-cardano-lib-core/bech32";
import { Credential, GovActionId } from "@xray-network/xray-cardano-lib-chain/conway";
import { Ed25519KeyHash, ScriptHash, TransactionHash } from "@xray-network/xray-cardano-lib-crypto";

export enum ProvisionalGovernanceCredentialRole {
  ConstitutionalCommitteeHot = 0,
  ConstitutionalCommitteeCold = 1,
  DRep = 2,
}

const roleHrps:Record<ProvisionalGovernanceCredentialRole,string>={
  [ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeHot]:"cc_hot",
  [ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeCold]:"cc_cold",
  [ProvisionalGovernanceCredentialRole.DRep]:"drep",
};

function roleHrp(role:ProvisionalGovernanceCredentialRole):string {
  const hrp=roleHrps[role];if(hrp===undefined)throw new RangeError("unsupported provisional governance credential role");return hrp;
}
function roleFromHrp(hrp:string):ProvisionalGovernanceCredentialRole {
  for(const [role,candidate] of Object.entries(roleHrps))if(candidate===hrp)return Number(role) as ProvisionalGovernanceCredentialRole;
  throw new TypeError("unsupported provisional governance credential HRP");
}
function credentialFromPayload(kind:number,payload:Uint8Array):Credential {
  return kind===2?Credential.new_pub_key(Ed25519KeyHash.from_raw_bytes(payload)):Credential.new_script(ScriptHash.from_raw_bytes(payload));
}

export class ProvisionalGovernanceCredentialId {
  readonly #role:ProvisionalGovernanceCredentialRole;readonly #credential:Credential;
  private constructor(role:ProvisionalGovernanceCredentialRole,credential:Credential){roleHrp(role);this.#role=role;this.#credential=Credential.from_cbor_bytes(credential.to_cbor_bytes());}
  public static from_credential(role:ProvisionalGovernanceCredentialRole,credential:Credential):ProvisionalGovernanceCredentialId{return new ProvisionalGovernanceCredentialId(role,credential);}
  public static from_bech32(value:string):ProvisionalGovernanceCredentialId {
    const decoded=decodeBech32(value);if(decoded.bytes.length!==29)throw new RangeError("provisional governance credential ID must contain 29 bytes");
    const role=roleFromHrp(decoded.prefix),header=decoded.bytes[0]??0,headerRole=header>>>4,kind=header&15;
    if(headerRole!==role)throw new TypeError("provisional governance credential header role does not match its HRP");
    if(kind!==2&&kind!==3)throw new TypeError("provisional governance credential kind must be key hash 2 or script hash 3");
    return new ProvisionalGovernanceCredentialId(role,credentialFromPayload(kind,decoded.bytes.slice(1)));
  }
  public role():ProvisionalGovernanceCredentialRole{return this.#role;}
  public credential():Credential{return Credential.from_cbor_bytes(this.#credential.to_cbor_bytes());}
  public to_raw_bytes():Uint8Array { const hash=this.#credential.as_pub_key()??this.#credential.as_script();if(hash===undefined)throw new TypeError("governance credential has no hash");return Uint8Array.from([(this.#role<<4)|(this.#credential.as_pub_key()===undefined?3:2),...hash.to_raw_bytes()]); }
  public to_bech32():string{return encodeBech32(roleHrp(this.#role),this.to_raw_bytes());}
}

export class ProvisionalGovernanceActionId {
  readonly #transactionId:TransactionHash;readonly #index:number;
  private constructor(transactionId:TransactionHash,index:number){if(!Number.isInteger(index)||index<0||index>255)throw new RangeError("provisional governance action index must fit one byte");this.#transactionId=TransactionHash.from_raw_bytes(transactionId.to_raw_bytes());this.#index=index;}
  public static from_parts(transactionId:TransactionHash,index:number):ProvisionalGovernanceActionId{return new ProvisionalGovernanceActionId(transactionId,index);}
  public static from_gov_action_id(value:GovActionId):ProvisionalGovernanceActionId{return new ProvisionalGovernanceActionId(value.transaction_id(),value.index());}
  public static from_bech32(value:string):ProvisionalGovernanceActionId { const decoded=decodeBech32(value);if(decoded.prefix!=="gov_action")throw new TypeError("provisional governance action HRP must be gov_action");if(decoded.bytes.length!==33)throw new RangeError("provisional governance action ID must contain 33 bytes");return new ProvisionalGovernanceActionId(TransactionHash.from_raw_bytes(decoded.bytes.slice(0,32)),decoded.bytes[32]??0); }
  public transaction_id():TransactionHash{return TransactionHash.from_raw_bytes(this.#transactionId.to_raw_bytes());}
  public index():number{return this.#index;}
  public gov_action_id():GovActionId{return GovActionId.new(this.#transactionId,this.#index);}
  public to_raw_bytes():Uint8Array{return Uint8Array.from([...this.#transactionId.to_raw_bytes(),this.#index]);}
  public to_bech32():string{return encodeBech32("gov_action",this.to_raw_bytes());}
}

export interface LegacyCip105GovernanceCredential {
  readonly role:ProvisionalGovernanceCredentialRole;
  readonly credential:Credential;
}

export function decodeLegacyCip105GovernanceCredential(value:string):LegacyCip105GovernanceCredential {
  const decoded=decodeBech32(value);if(decoded.bytes.length!==28)throw new RangeError("legacy CIP-0105 governance credential must contain 28 bytes");
  const legacy:Record<string,readonly [ProvisionalGovernanceCredentialRole,number]>={
    drep:[ProvisionalGovernanceCredentialRole.DRep,2],drep_script:[ProvisionalGovernanceCredentialRole.DRep,3],
    cc_cold:[ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeCold,2],cc_cold_script:[ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeCold,3],
    cc_hot:[ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeHot,2],cc_hot_script:[ProvisionalGovernanceCredentialRole.ConstitutionalCommitteeHot,3],
  };
  const match=legacy[decoded.prefix];if(match===undefined)throw new TypeError("unsupported legacy CIP-0105 governance credential HRP");
  return {role:match[0],credential:credentialFromPayload(match[1],decoded.bytes)};
}
