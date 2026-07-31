import type { BlockJSON, TransactionBodyJSON } from "../shared/json-types.js";
import type { AllegraBlockJSON, AllegraTransactionBodyJSON } from "../allegra/types.js";
import type { AlonzoBlockJSON, AlonzoTransactionBodyJSON } from "../alonzo/types.js";
import type { BabbageBlockJSON, BabbageTransactionBodyJSON } from "../babbage/types.js";
import type { ByronBlockJSON, ByronTxJSON } from "../byron/types.js";
import type { MaryBlockJSON, MaryTransactionBodyJSON } from "../mary/types.js";
import type { ShelleyBlockJSON, ShelleyTransactionBodyJSON } from "../shelley/types.js";

export type MultiEraBlockJSON = {
    Byron: ByronBlockJSON;
} | {
    Shelley: ShelleyBlockJSON;
} | {
    Allegra: AllegraBlockJSON;
} | {
    Mary: MaryBlockJSON;
} | {
    Alonzo: AlonzoBlockJSON;
} | {
    Babbage: BabbageBlockJSON;
} | {
    Conway: BlockJSON;
};
export type MultiEraTransactionBodyJSON = {
    Byron: ByronTxJSON;
} | {
    Shelley: ShelleyTransactionBodyJSON;
} | {
    Allegra: AllegraTransactionBodyJSON;
} | {
    Mary: MaryTransactionBodyJSON;
} | {
    Alonzo: AlonzoTransactionBodyJSON;
} | {
    Babbage: BabbageTransactionBodyJSON;
} | {
    Conway: TransactionBodyJSON;
};
