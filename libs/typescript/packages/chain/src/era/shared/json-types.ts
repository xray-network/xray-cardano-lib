// JSON shapes are reviewed source contracts shared by the supported eras.
type AssetBundle = Record<string, Record<string, number>>;
type AssetBundle2 = Record<string, Record<string, number>>;

export type PlutusDataJSON =
  | {
      map: {
        k: PlutusDataJSON;
        v: PlutusDataJSON;
      }[];
    }
  | {
      list: PlutusDataJSON[];
    }
  | {
      int: number;
    }
  | {
      bytes: string;
    }
  | {
      constructor: number;
      fields: PlutusDataJSON[];
    };
export type TransactionMetadatumJSON =
  | {
      map: {
        k: TransactionMetadatumJSON;
        v: TransactionMetadatumJSON;
      }[];
    }
  | {
      list: TransactionMetadatumJSON[];
    }
  | {
      int: number;
    }
  | {
      bytes: string;
    }
  | {
      string: string;
    };
export interface DRepVotingThresholdsJSON {
  committee_no_confidence: UnitIntervalJSON;
  committee_normal: UnitIntervalJSON;
  hard_fork_initiation: UnitIntervalJSON;
  motion_no_confidence: UnitIntervalJSON;
  pp_economic_group: UnitIntervalJSON;
  pp_governance_group: UnitIntervalJSON;
  pp_network_group: UnitIntervalJSON;
  pp_technical_group: UnitIntervalJSON;
  treasury_withdrawal: UnitIntervalJSON;
  update_constitution: UnitIntervalJSON;
}
export type IntJSON = string;
export type NetworkIdJSON = number;
export type NonEmptyVecBootstrapWitnessJSON = BootstrapWitnessJSON[];
export type CertificateJSON =
  | {
      StakeRegistration: StakeRegistrationJSON;
    }
  | {
      StakeDeregistration: StakeDeregistrationJSON;
    }
  | {
      StakeDelegation: StakeDelegationJSON;
    }
  | {
      PoolRegistration: PoolRegistrationJSON;
    }
  | {
      PoolRetirement: PoolRetirementJSON;
    }
  | {
      RegCert: RegCertJSON;
    }
  | {
      UnregCert: UnregCertJSON;
    }
  | {
      VoteDelegCert: VoteDelegCertJSON;
    }
  | {
      StakeVoteDelegCert: StakeVoteDelegCertJSON;
    }
  | {
      StakeRegDelegCert: StakeRegDelegCertJSON;
    }
  | {
      VoteRegDelegCert: VoteRegDelegCertJSON;
    }
  | {
      StakeVoteRegDelegCert: StakeVoteRegDelegCertJSON;
    }
  | {
      AuthCommitteeHotCert: AuthCommitteeHotCertJSON;
    }
  | {
      ResignCommitteeColdCert: ResignCommitteeColdCertJSON;
    }
  | {
      RegDrepCert: RegDrepCertJSON;
    }
  | {
      UnregDrepCert: UnregDrepCertJSON;
    }
  | {
      UpdateDrepCert: UpdateDrepCertJSON;
    };
export type NonEmptyVecCertificateJSON = CertificateJSON[];
export type NativeScriptJSON =
  | {
      ScriptPubkey: ScriptPubkeyJSON;
    }
  | {
      ScriptAll: ScriptAllJSON;
    }
  | {
      ScriptAny: ScriptAnyJSON;
    }
  | {
      ScriptNOfK: ScriptNOfKJSON;
    }
  | {
      ScriptInvalidBefore: ScriptInvalidBeforeJSON;
    }
  | {
      ScriptInvalidHereafter: ScriptInvalidHereafterJSON;
    };
export type NonEmptyVecNativeScriptJSON = NativeScriptJSON[];
export type NonEmptyVecPlutusDataJSON = PlutusDataJSON[];
export type NonEmptyVecPlutusV1ScriptJSON = string[];
export type NonEmptyVecPlutusV2ScriptJSON = string[];
export type NonEmptyVecPlutusV3ScriptJSON = string[];
export type NonEmptyVecProposalProcedureJSON = ProposalProcedureJSON[];
export type NonEmptyVecTransactionInputJSON = TransactionInputJSON[];
export type NonEmptyVecVkeywitnessJSON = VkeywitnessJSON[];
export interface PoolVotingThresholdsJSON {
  committee_no_confidence: UnitIntervalJSON;
  committee_normal: UnitIntervalJSON;
  hard_fork_initiation: UnitIntervalJSON;
  motion_no_confidence: UnitIntervalJSON;
  security_relevant_parameter_voting_threshold: UnitIntervalJSON;
}
export interface ProtocolParamUpdateJSON {
  ada_per_utxo_byte?: number | null;
  collateral_percentage?: number | null;
  committee_term_limit?: number | null;
  cost_models_for_script_languages?: {
    /**
     * This interface was referenced by `undefined`'s JSON-Schema definition
     * via the `patternProperty` "^\d+$".
     */
    [k: string]: number[];
  } | null;
  d_rep_deposit?: number | null;
  d_rep_inactivity_period?: number | null;
  d_rep_voting_thresholds?: DRepVotingThresholdsJSON | null;
  execution_costs?: ExUnitPricesJSON | null;
  expansion_rate?: UnitIntervalJSON | null;
  governance_action_deposit?: number | null;
  governance_action_validity_period?: number | null;
  key_deposit?: number | null;
  max_block_body_size?: number | null;
  max_block_ex_units?: ExUnitsJSON | null;
  max_block_header_size?: number | null;
  max_collateral_inputs?: number | null;
  max_transaction_size?: number | null;
  max_tx_ex_units?: ExUnitsJSON | null;
  max_value_size?: number | null;
  maximum_epoch?: number | null;
  min_committee_size?: number | null;
  min_fee_ref_script_cost_per_byte?: RationalJSON | null;
  min_pool_cost?: number | null;
  minfee_a?: number | null;
  minfee_b?: number | null;
  n_opt?: number | null;
  pool_deposit?: number | null;
  pool_pledge_influence?: RationalJSON | null;
  pool_voting_thresholds?: PoolVotingThresholdsJSON | null;
  treasury_growth_rate?: UnitIntervalJSON | null;
}
export interface RationalJSON {
  denominator: number;
  numerator: number;
}
export type ScriptJSON =
  | {
      Native: {
        script: NativeScriptJSON;
        [k: string]: unknown;
      };
    }
  | {
      PlutusV1: {
        script: string;
        [k: string]: unknown;
      };
    }
  | {
      PlutusV2: {
        script: string;
        [k: string]: unknown;
      };
    }
  | {
      PlutusV3: {
        script: string;
        [k: string]: unknown;
      };
    };
export type CredentialJSON =
  | {
      PubKey: {
        hash: string;
        [k: string]: unknown;
      };
    }
  | {
      Script: {
        hash: string;
        [k: string]: unknown;
      };
    };
export type ArrayOf_CredentialJSON = CredentialJSON[];
export type ArrayOf_Ed25519KeyHashJSON = string[];
export type ArrayOf_TransactionInputJSON = TransactionInputJSON[];
export interface UnitIntervalJSON {
  end: number;
  start: number;
}
export type AddressJSON = string;
export type RewardAddressJSON = string;
export type AssetNameJSON = string;
export interface ValueJSON {
  coin: number;
  multiasset: AssetBundle;
}
export type AuxiliaryDataJSON =
  | {
      Shelley: MetadataJSON;
    }
  | {
      ShelleyMA: ShelleyMAFormatAuxDataJSON;
    }
  | {
      Conway: ConwayFormatAuxDataJSON;
    };
export interface ConwayFormatAuxDataJSON {
  metadata?: MetadataJSON | null;
  native_scripts?: NativeScriptJSON[] | null;
  plutus_v1_scripts?: string[] | null;
  plutus_v2_scripts?: string[] | null;
  plutus_v3_scripts?: string[] | null;
}
/**
 * Collection of TransactionMetadatums indexed by TransactionMetadatumLabels
 * Handles the extremely rare edge-case of in previous generations allowing
 * duplicate metadatum labels.
 */
export interface MetadataJSON {
  entries: [unknown, unknown][];
}
export interface ShelleyMAFormatAuxDataJSON {
  auxiliary_scripts: NativeScriptJSON[];
  transaction_metadata: MetadataJSON;
}
export interface BlockJSON {
  auxiliary_data_set: {
    [k: string]: AuxiliaryDataJSON;
  };
  header: HeaderJSON;
  invalid_transactions: number[];
  transaction_bodies: TransactionBodyJSON[];
  transaction_witness_sets: TransactionWitnessSetJSON[];
}
export interface HeaderJSON {
  body_signature: string;
  header_body: HeaderBodyJSON;
}
export interface HeaderBodyJSON {
  block_body_hash: string;
  block_body_size: number;
  block_number: number;
  issuer_vkey: PublicKeyJSON;
  operational_cert: OperationalCertJSON;
  prev_hash?: string | null;
  protocol_version: ProtocolVersionJSON;
  slot: number;
  vrf_result: VRFCertJSON;
  vrf_vkey: string;
}
export interface OperationalCertJSON {
  hot_vkey: string;
  kes_period: number;
  sequence_number: number;
  sigma: string;
}
export interface ProtocolVersionJSON {
  major: number;
  minor: number;
}
export interface AddrAttributesJSON {
  derivation_path?: HDAddressPayloadJSON | null;
  protocol_magic?: ProtocolMagicJSON | null;
  stake_distribution?: StakeDistributionJSON | null;
}
export interface AddressContentJSON {
  addr_attributes: AddrAttributesJSON;
  addr_type: ByronAddrTypeJSON;
  address_id: string;
}
export type ByronAddrTypeJSON = "PublicKeyJSON" | "ScriptJSON" | "Redeem";
export type ByronAddressJSON = string;
export interface ByronTxOutJSON {
  address: string;
  amount: number;
}
/**
 * structure to compute the CRC32 of chunks of bytes.
 *
 * This structure allows implements the `Write` trait making it easier
 * to compute the crc32 of a stream.
 */
export type Crc32JSON = number;
export type HDAddressPayloadJSON = number[];
export type ProtocolMagicJSON = number;
export type SpendingDataJSON =
  | {
      SpendingDataPubKey: Bip32PublicKeyJSON;
    }
  | {
      SpendingDataScript: string;
    }
  | {
      SpendingDataRedeem: PublicKeyJSON;
    };
export type StakeDistributionJSON =
  | "BootstrapEra"
  | {
      SingleKey: string;
    };
export type StakeholderIdJSON = string;
export interface AuthCommitteeHotCertJSON {
  committee_cold_credential: CredentialJSON;
  committee_hot_credential: CredentialJSON;
}
export type DNSNameJSON = string;
export type DRepJSON =
  | {
      Key: {
        pool: string;
        [k: string]: unknown;
      };
    }
  | {
      Script: {
        script_hash: string;
        [k: string]: unknown;
      };
    }
  | {
      AlwaysAbstain: {
        [k: string]: unknown;
      };
    }
  | {
      AlwaysNoConfidence: {
        [k: string]: unknown;
      };
    };
export type Ipv4JSON = string;
export type Ipv6JSON = string;
export interface MultiHostNameJSON {
  /**
   * A SRV DNS record
   */
  dns_name: string;
}
export interface PoolMetadataJSON {
  pool_metadata_hash: string;
  url: string;
}
export interface PoolParamsJSON {
  cost: number;
  margin: UnitIntervalJSON;
  operator: string;
  pledge: number;
  pool_metadata?: PoolMetadataJSON | null;
  pool_owners: string[];
  relays: RelayJSON[];
  reward_account: string;
  vrf_keyhash: string;
}
export interface PoolRegistrationJSON {
  pool_params: PoolParamsJSON;
}
export interface PoolRetirementJSON {
  epoch: number;
  pool: string;
}
export interface RegCertJSON {
  deposit: number;
  stake_credential: CredentialJSON;
}
export interface RegDrepCertJSON {
  anchor?: AnchorJSON | null;
  deposit: number;
  drep_credential: CredentialJSON;
}
export type RelayJSON =
  | {
      SingleHostAddr: SingleHostAddrJSON;
    }
  | {
      SingleHostName: SingleHostNameJSON;
    }
  | {
      MultiHostName: MultiHostNameJSON;
    };
export interface ResignCommitteeColdCertJSON {
  anchor?: AnchorJSON | null;
  committee_cold_credential: CredentialJSON;
}
export interface SingleHostAddrJSON {
  ipv4?: string | null;
  ipv6?: string | null;
  port?: number | null;
}
export interface SingleHostNameJSON {
  /**
   * An A or AAAA DNS record
   */
  dns_name: string;
  port?: number | null;
}
export interface StakeDelegationJSON {
  pool: string;
  stake_credential: CredentialJSON;
}
export interface StakeDeregistrationJSON {
  stake_credential: CredentialJSON;
}
export interface StakeRegDelegCertJSON {
  deposit: number;
  pool: string;
  stake_credential: CredentialJSON;
}
export interface StakeRegistrationJSON {
  stake_credential: CredentialJSON;
}
export interface StakeVoteDelegCertJSON {
  d_rep: DRepJSON;
  pool: string;
  stake_credential: CredentialJSON;
}
export interface StakeVoteRegDelegCertJSON {
  d_rep: DRepJSON;
  deposit: number;
  pool: string;
  stake_credential: CredentialJSON;
}
export interface UnregCertJSON {
  deposit: number;
  stake_credential: CredentialJSON;
}
export interface UnregDrepCertJSON {
  deposit: number;
  drep_credential: CredentialJSON;
}
export interface UpdateDrepCertJSON {
  anchor?: AnchorJSON | null;
  drep_credential: CredentialJSON;
}
export type UrlJSON = string;
export interface VoteDelegCertJSON {
  d_rep: DRepJSON;
  stake_credential: CredentialJSON;
}
export interface VoteRegDelegCertJSON {
  d_rep: DRepJSON;
  deposit: number;
  stake_credential: CredentialJSON;
}
export type AnchorDocHashJSON = string;
export type AuxiliaryDataHashJSON = string;
export type BlockBodyHashJSON = string;
export type BlockHeaderHashJSON = string;
export interface BootstrapWitnessJSON {
  attributes: AddrAttributesJSON;
  chain_code: number[];
  public_key: PublicKeyJSON;
  signature: string;
}
export type DatumHashJSON = string;
export type Ed25519KeyHashJSON = string;
export type Ed25519SignatureJSON = string;
export type GenesisDelegateHashJSON = string;
export type GenesisHashJSON = string;
export type KESSignatureJSON = string;
export type KESVkeyJSON = string;
export type NonceJSON =
  | {
      Identity: {
        [k: string]: unknown;
      };
    }
  | {
      Hash: {
        hash: string;
        [k: string]: unknown;
      };
    };
export type NonceHashJSON = string;
export type PoolMetadataHashJSON = string;
export type ScriptDataHashJSON = string;
export type ScriptHashJSON = string;
export type TransactionHashJSON = string;
export interface VRFCertJSON {
  output: number[];
  proof: number[];
}
export type VRFKeyHashJSON = string;
export type VRFVkeyJSON = string;
/**
 * ED25519 key used as public key
 */
export type PublicKeyJSON = string;
export interface VkeywitnessJSON {
  ed25519_signature: string;
  vkey: PublicKeyJSON;
}
export interface AnchorJSON {
  anchor_doc_hash: string;
  anchor_url: string;
}
export interface ConstitutionJSON {
  anchor: AnchorJSON;
  script_hash?: string | null;
}
export type GovActionJSON =
  | {
      ParameterChangeAction: ParameterChangeActionJSON;
    }
  | {
      HardForkInitiationAction: HardForkInitiationActionJSON;
    }
  | {
      TreasuryWithdrawalsAction: TreasuryWithdrawalsActionJSON;
    }
  | {
      NoConfidence: NoConfidenceJSON;
    }
  | {
      UpdateCommittee: UpdateCommitteeJSON;
    }
  | {
      NewConstitution: NewConstitutionJSON;
    }
  | {
      InfoAction: {
        [k: string]: unknown;
      };
    };
export interface GovActionIdJSON {
  gov_action_index: number;
  transaction_id: string;
}
export interface HardForkInitiationActionJSON {
  action_id?: GovActionIdJSON | null;
  version: ProtocolVersionJSON;
}
export interface NewConstitutionJSON {
  action_id?: GovActionIdJSON | null;
  constitution: ConstitutionJSON;
}
export interface NoConfidenceJSON {
  action_id?: GovActionIdJSON | null;
}
export interface ParameterChangeActionJSON {
  action_id?: GovActionIdJSON | null;
  policy_hash?: string | null;
  update: ProtocolParamUpdateJSON;
}
export interface ProposalProcedureJSON {
  anchor: AnchorJSON;
  deposit: number;
  gov_action: GovActionJSON;
  reward_account: string;
}
export interface TreasuryWithdrawalsActionJSON {
  policy_hash?: string | null;
  withdrawal: {
    [k: string]: number;
  };
}
export interface UpdateCommitteeJSON {
  action_id?: GovActionIdJSON | null;
  cold_credentials: CredentialJSON[];
  credentials: {
    [k: string]: number;
  };
  unit_interval: UnitIntervalJSON;
}
export type VoteJSON = "No" | "Yes" | "Abstain";
export type VoterJSON =
  | {
      ConstitutionalCommitteeHotKeyHash: {
        ed25519_key_hash: string;
        [k: string]: unknown;
      };
    }
  | {
      ConstitutionalCommitteeHotScriptHash: {
        script_hash: string;
        [k: string]: unknown;
      };
    }
  | {
      DRepKeyHash: {
        ed25519_key_hash: string;
        [k: string]: unknown;
      };
    }
  | {
      DRepScriptHash: {
        script_hash: string;
        [k: string]: unknown;
      };
    }
  | {
      StakingPoolKeyHash: {
        ed25519_key_hash: string;
        [k: string]: unknown;
      };
    };
export interface VotingProcedureJSON {
  anchor?: AnchorJSON | null;
  vote: VoteJSON;
}
export interface CostModelsJSON {
  /**
   * This interface was referenced by `CostModelsJSON`'s JSON-Schema definition
   * via the `patternProperty` "^\d+$".
   */
  [k: string]: number[];
}
export interface ExUnitPricesJSON {
  mem_price: RationalJSON;
  step_price: RationalJSON;
}
export interface ExUnitsJSON {
  mem: number;
  steps: number;
}
export type LanguageJSON = "PlutusV1" | "PlutusV2" | "PlutusV3";
export interface LegacyRedeemerJSON {
  data: PlutusDataJSON;
  ex_units: ExUnitsJSON;
  index: number;
  tag: RedeemerTagJSON;
}
export type PlutusV1ScriptJSON = string;
export type PlutusV2ScriptJSON = string;
export type PlutusV3ScriptJSON = string;
export interface RedeemerKeyJSON {
  index: number;
  tag: RedeemerTagJSON;
}
export type RedeemerTagJSON = "Spend" | "Mint" | "Cert" | "Reward" | "Voting" | "Proposing";
export interface RedeemerValJSON {
  data: PlutusDataJSON;
  ex_units: ExUnitsJSON;
}
export type RedeemersJSON =
  | {
      ArrLegacyRedeemer: {
        arr_legacy_redeemer: LegacyRedeemerJSON[];
        [k: string]: unknown;
      };
    }
  | {
      MapRedeemerKeyToRedeemerVal: {
        map_redeemer_key_to_redeemer_val: {
          [k: string]: RedeemerValJSON;
        };
        [k: string]: unknown;
      };
    };
export interface AlonzoFormatTxOutJSON {
  address: string;
  amount: ValueJSON;
  datum_hash?: string | null;
}
export interface ConwayFormatTxOutJSON {
  address: string;
  amount: ValueJSON;
  datum_option?: DatumOptionJSON | null;
  script_reference?: ScriptRefJSON | null;
}
export type DatumOptionJSON =
  | {
      Hash: {
        datum_hash: string;
        [k: string]: unknown;
      };
    }
  | {
      Datum: {
        datum: PlutusDataJSON;
        [k: string]: unknown;
      };
    };
export type NonEmptyVecEd25519KeyHashJSON = string[];
export interface ScriptAllJSON {
  native_scripts: NativeScriptJSON[];
}
export interface ScriptAnyJSON {
  native_scripts: NativeScriptJSON[];
}
export interface ScriptInvalidBeforeJSON {
  before: number;
}
export interface ScriptInvalidHereafterJSON {
  after: number;
}
export interface ScriptNOfKJSON {
  n: number;
  native_scripts: NativeScriptJSON[];
}
export interface ScriptPubkeyJSON {
  ed25519_key_hash: string;
}
export type ScriptRefJSON =
  | {
      Native: {
        script: NativeScriptJSON;
        [k: string]: unknown;
      };
    }
  | {
      PlutusV1: {
        script: string;
        [k: string]: unknown;
      };
    }
  | {
      PlutusV2: {
        script: string;
        [k: string]: unknown;
      };
    }
  | {
      PlutusV3: {
        script: string;
        [k: string]: unknown;
      };
    };
export interface TransactionJSON {
  auxiliary_data?: AuxiliaryDataJSON | null;
  body: TransactionBodyJSON;
  is_valid: boolean;
  witness_set: TransactionWitnessSetJSON;
}
export interface TransactionBodyJSON {
  auxiliary_data_hash?: string | null;
  certs?: CertificateJSON[] | null;
  collateral_inputs?: TransactionInputJSON[] | null;
  collateral_return?: TransactionOutputJSON | null;
  current_treasury_value?: number | null;
  donation?: number | null;
  fee: number;
  inputs: TransactionInputJSON[];
  mint?: AssetBundle2 | null;
  network_id?: number | null;
  outputs: TransactionOutputJSON[];
  proposal_procedures?: ProposalProcedureJSON[] | null;
  reference_inputs?: TransactionInputJSON[] | null;
  required_signers?: string[] | null;
  script_data_hash?: string | null;
  total_collateral?: number | null;
  ttl?: number | null;
  validity_interval_start?: number | null;
  voting_procedures?: {
    [k: string]: {
      [k: string]: VotingProcedureJSON;
    };
  } | null;
  withdrawals?: {
    [k: string]: number;
  } | null;
}
export interface TransactionInputJSON {
  index: number;
  transaction_id: string;
}
export type TransactionOutputJSON =
  | {
      AlonzoFormatTxOut: AlonzoFormatTxOutJSON;
    }
  | {
      ConwayFormatTxOut: ConwayFormatTxOutJSON;
    };
export interface TransactionWitnessSetJSON {
  bootstrap_witnesses?: BootstrapWitnessJSON[] | null;
  native_scripts?: NativeScriptJSON[] | null;
  plutus_datums?: PlutusDataJSON[] | null;
  plutus_v1_scripts?: string[] | null;
  plutus_v2_scripts?: string[] | null;
  plutus_v3_scripts?: string[] | null;
  redeemers?: RedeemersJSON | null;
  vkeywitnesses?: VkeywitnessJSON[] | null;
}
export type BigIntegerJSON = string;
export type Bip32PublicKeyJSON = string;
