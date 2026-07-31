import {
  Ed25519KeyHash,
  VRFKeyHash,
} from "@xray-network/cardano-crypto";
import { Address } from "../../address/index.js";
import {
  bigintValue,
  integer,
  record,
  stringValue,
} from "../shared/genesis-json.js";
import type { JsonRecord } from "../shared/genesis-json.js";

export interface ParsedShelleyGenesis {
  readonly epochLength: bigint;
  readonly initialFunds: ReadonlyMap<string, bigint>;
  readonly networkId: number;
  readonly networkMagic: number;
  readonly systemStart: string;
  readonly genesisDelegations: ReadonlyMap<
    string,
    { readonly delegate: Ed25519KeyHash; readonly vrf: VRFKeyHash }
  >;
  readonly raw: Readonly<JsonRecord>;
}

export function parseShelleyGenesis(json: string): ParsedShelleyGenesis {
  const raw = record(JSON.parse(json), "Shelley genesis");
  const network = stringValue(raw["networkId"], "networkId");
  if (network !== "Mainnet" && network !== "Testnet") {
    throw new TypeError(`unsupported networkId ${network}`);
  }

  const initialFunds = new Map<string, bigint>();
  for (const [hex, value] of Object.entries(record(raw["initialFunds"], "initialFunds"))) {
    const address = Address.from_hex(hex);
    initialFunds.set(address.to_hex(), bigintValue(value, `initialFunds.${hex}`));
  }

  const delegations = new Map<
    string,
    { readonly delegate: Ed25519KeyHash; readonly vrf: VRFKeyHash }
  >();
  for (const [hash, value] of Object.entries(record(raw["genDelegs"], "genDelegs"))) {
    Ed25519KeyHash.from_hex(hash);
    const entry = record(value, `genDelegs.${hash}`);
    delegations.set(hash, {
      delegate: Ed25519KeyHash.from_hex(stringValue(entry["delegate"], "delegate")),
      vrf: VRFKeyHash.from_hex(stringValue(entry["vrf"], "vrf")),
    });
  }

  const staking = raw["staking"];
  if (staking !== undefined && staking !== null) {
    const stakingRecord = record(staking, "staking");
    for (const params of Object.values(record(stakingRecord["pools"], "staking.pools"))) {
      const pool = record(params, "pool");
      if (!Array.isArray(pool["owners"]) || !Array.isArray(pool["relays"])) {
        throw new TypeError("pool owners and relays must be arrays");
      }
    }
  }

  return {
    epochLength: bigintValue(raw["epochLength"], "epochLength"),
    initialFunds,
    networkId: network === "Mainnet" ? 1 : 0,
    networkMagic: integer(raw["networkMagic"], "networkMagic"),
    systemStart: stringValue(raw["systemStart"], "systemStart"),
    genesisDelegations: delegations,
    raw,
  };
}
