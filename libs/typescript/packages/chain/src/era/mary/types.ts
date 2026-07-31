import type { TransactionInputJSON, ValueJSON } from "../shared/json-types.js";
import type {
  AllegraAuxiliaryDataJSON,
  AllegraCertificateJSON,
  AllegraTransactionWitnessSetJSON,
} from "../allegra/types.js";
import type { ShelleyHeaderJSON, ShelleyUpdateJSON } from "../shelley/types.js";

type AssetBundle2 = Record<string, Record<string, number>>;

export interface MaryBlockJSON {
    auxiliary_data_set: {
        [k: string]: AllegraAuxiliaryDataJSON;
    };
    header: ShelleyHeaderJSON;
    transaction_bodies: MaryTransactionBodyJSON[];
    transaction_witness_sets: AllegraTransactionWitnessSetJSON[];
}
export interface MaryTransactionJSON {
    auxiliary_data?: AllegraAuxiliaryDataJSON | null;
    body: MaryTransactionBodyJSON;
    witness_set: AllegraTransactionWitnessSetJSON;
}
export interface MaryTransactionBodyJSON {
    auxiliary_data_hash?: string | null;
    certs?: AllegraCertificateJSON[] | null;
    fee: number;
    inputs: TransactionInputJSON[];
    mint?: AssetBundle2 | null;
    outputs: MaryTransactionOutputJSON[];
    ttl?: number | null;
    update?: ShelleyUpdateJSON | null;
    validity_interval_start?: number | null;
    withdrawals?: {
        [k: string]: number;
    } | null;
}
export interface MaryTransactionOutputJSON {
    address: string;
    amount: ValueJSON;
}
