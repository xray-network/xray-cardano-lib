export type Datum = string;
export type Redeemer = string;
export type Exact<T> = T;
export type Json =
  | null
  | boolean
  | number
  | bigint
  | string
  | Json[]
  | { [key: string]: Json };

export class Constr<T = PlutusDataValue> {
  public readonly index: number;
  public readonly fields: T[];

  public constructor(index: number, fields: T[]) {
    if (!Number.isSafeInteger(index) || index < 0) {
      throw new RangeError("Constructor index must be a non-negative safe integer");
    }
    this.index = index;
    this.fields = [...fields];
  }
}

export type PlutusDataValue =
  | bigint
  | string
  | PlutusDataValue[]
  | Map<PlutusDataValue, PlutusDataValue>
  | Constr<PlutusDataValue>;

interface SchemaType<T> {
  readonly "~type"?: T;
}

export interface IntegerSchema extends SchemaType<bigint> {
  readonly dataType: "integer";
  readonly minimum?: number | bigint;
  readonly maximum?: number | bigint;
  readonly exclusiveMinimum?: number | bigint;
  readonly exclusiveMaximum?: number | bigint;
}

export interface BytesSchema extends SchemaType<string> {
  readonly dataType: "bytes";
  readonly minLength?: number;
  readonly maxLength?: number;
  readonly enum?: readonly string[];
}

export interface BooleanSchema extends SchemaType<boolean> {
  readonly dataType: "boolean";
}

export interface VoidSchema extends SchemaType<undefined> {
  readonly dataType: "void";
}

export interface AnySchema extends SchemaType<PlutusDataValue> {
  readonly dataType: "any";
}

export interface ArraySchema<S extends DataSchema = DataSchema>
  extends SchemaType<StaticSchema<S>[]> {
  readonly dataType: "list";
  readonly items: S;
  readonly minItems?: number;
  readonly maxItems?: number;
  readonly uniqueItems?: boolean;
}

export interface TupleSchema<S extends readonly DataSchema[] = readonly DataSchema[]>
  extends SchemaType<{ [K in keyof S]: StaticSchema<S[K]> }> {
  readonly dataType: "tuple";
  readonly items: S;
  readonly hasConstr: boolean;
}

export type SchemaProperties = Readonly<Record<string, DataSchema>>;
export type StaticProperties<P extends SchemaProperties> = {
  [K in keyof P]: StaticSchema<P[K]>;
};

export interface ObjectSchema<P extends SchemaProperties = SchemaProperties>
  extends SchemaType<StaticProperties<P>> {
  readonly dataType: "object";
  readonly properties: P;
  readonly hasConstr: boolean;
  readonly index: number;
}

export interface MapSchema<K extends DataSchema = DataSchema, V extends DataSchema = DataSchema>
  extends SchemaType<Map<StaticSchema<K>, StaticSchema<V>>> {
  readonly dataType: "map";
  readonly keys: K;
  readonly values: V;
  readonly minItems?: number;
  readonly maxItems?: number;
}

export interface LiteralSchema<T extends string = string> extends SchemaType<T> {
  readonly dataType: "literal";
  readonly title: T;
  readonly index: number;
}

export interface NullableSchema<S extends DataSchema = DataSchema>
  extends SchemaType<StaticSchema<S> | null> {
  readonly dataType: "nullable";
  readonly item: S;
}

export interface EnumSchema<S extends readonly EnumItemSchema[] = readonly EnumItemSchema[]>
  extends SchemaType<StaticSchema<S[number]>> {
  readonly dataType: "enum";
  readonly items: S;
}

export type EnumItemSchema = LiteralSchema | ObjectSchema;
export type DataSchema =
  | IntegerSchema
  | BytesSchema
  | BooleanSchema
  | VoidSchema
  | AnySchema
  | ArraySchema
  | TupleSchema
  | ObjectSchema
  | MapSchema
  | LiteralSchema
  | NullableSchema
  | EnumSchema;

export type StaticSchema<S> = S extends SchemaType<infer T> ? T : never;
