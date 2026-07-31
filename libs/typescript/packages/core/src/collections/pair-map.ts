import { structuralKeyEqual } from "./key-equality.js";

/** Ordered key/value pairs; unlike Map, duplicate keys are retained. */
export class PairMap<Key, Value> implements Iterable<readonly [Key, Value]> {
  readonly #entries: Array<readonly [Key, Value]>;
  readonly #equals: (left: Key, right: Key) => boolean;

  public constructor(
    entries: readonly (readonly [Key, Value])[] = [],
    equals: (left: Key, right: Key) => boolean = structuralKeyEqual,
  ) {
    this.#entries = [...entries];
    this.#equals = equals;
  }

  public get size(): number { return this.#entries.length; }
  public append(key: Key, value: Value): void { this.#entries.push([key, value]); }
  public get(key: Key): Value | undefined {
    return this.#entries.find(([candidate]) => this.#equals(candidate, key))?.[1];
  }
  public getAll(key: Key): Value[] {
    return this.#entries
      .filter(([candidate]) => this.#equals(candidate, key))
      .map(([, value]) => value);
  }
  public toArray(): Array<readonly [Key, Value]> { return [...this.#entries]; }
  public [Symbol.iterator](): Iterator<readonly [Key, Value]> {
    return this.#entries[Symbol.iterator]();
  }
}
