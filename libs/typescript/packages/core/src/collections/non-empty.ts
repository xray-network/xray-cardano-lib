import { CardanoError } from "../errors/index.js";
import { OrderedMap } from "./ordered-map.js";

function nonEmptyError(name: string): CardanoError {
  return new CardanoError("INVARIANT", `${name} must contain at least one item`);
}

/** A mutable vector that cannot be reduced below one item. */
export class NonEmptyVec<Value> implements Iterable<Value> {
  readonly #items: Value[];

  public constructor(first: Value, rest: readonly Value[] = []) {
    this.#items = [first, ...rest];
  }

  public static from<Value>(items: readonly Value[]): NonEmptyVec<Value> {
    const first = items[0];
    if (first === undefined && items.length === 0) throw nonEmptyError("NonEmptyVec");
    return new NonEmptyVec(first as Value, items.slice(1));
  }

  public get length(): number { return this.#items.length; }
  public isEmpty(): false { return false; }
  public get(index: number): Value | undefined { return this.#items[index]; }
  public push(value: Value): void { this.#items.push(value); }

  public set(index: number, value: Value): void {
    if (index < 0 || index >= this.length) throw new RangeError("Index is outside NonEmptyVec");
    this.#items[index] = value;
  }

  public pop(): Value {
    if (this.length === 1) throw nonEmptyError("NonEmptyVec");
    return this.#items.pop() as Value;
  }

  public remove(index: number): Value {
    if (this.length === 1) throw nonEmptyError("NonEmptyVec");
    if (index < 0 || index >= this.length) throw new RangeError("Index is outside NonEmptyVec");
    return this.#items.splice(index, 1)[0] as Value;
  }

  public toArray(): Value[] { return [...this.#items]; }
  public toJSON(): Value[] { return this.toArray(); }
  public [Symbol.iterator](): Iterator<Value> { return this.#items[Symbol.iterator](); }
}

export class NonEmptyMap<Key, Value> implements Iterable<readonly [Key, Value]> {
  readonly #map: OrderedMap<Key, Value>;

  public constructor(
    first: readonly [Key, Value],
    rest: readonly (readonly [Key, Value])[] = [],
  ) {
    this.#map = new OrderedMap([first, ...rest]);
  }

  public static from<Key, Value>(
    entries: readonly (readonly [Key, Value])[],
  ): NonEmptyMap<Key, Value> {
    const first = entries[0];
    if (first === undefined) throw nonEmptyError("NonEmptyMap");
    return new NonEmptyMap(first, entries.slice(1));
  }

  public get size(): number { return this.#map.size; }
  public isEmpty(): false { return false; }
  public has(key: Key): boolean { return this.#map.has(key); }
  public get(key: Key): Value | undefined { return this.#map.get(key); }
  public set(key: Key, value: Value): this {
    this.#map.set(key, value);
    return this;
  }
  public delete(key: Key): boolean {
    if (this.size === 1) throw nonEmptyError("NonEmptyMap");
    return this.#map.delete(key);
  }
  public [Symbol.iterator](): IterableIterator<[Key, Value]> {
    return this.#map[Symbol.iterator]();
  }
}
