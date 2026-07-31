export interface Cloneable<Value> {
  clone(): Value;
}

export interface Equatable<Value> {
  equals(other: Value): boolean;
}

export type Comparator<Value> = (left: Value, right: Value) => -1 | 0 | 1;
