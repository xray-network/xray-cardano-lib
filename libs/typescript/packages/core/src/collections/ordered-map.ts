import { structuralKeyEqual } from "./key-equality.js";

/** Map with JavaScript's insertion-order and replacement semantics made explicit. */
export class OrderedMap<Key, Value> implements Iterable<readonly [Key, Value]> {
  readonly #entries: Array<[Key, Value]> = [];
  readonly #equals: (left: Key, right: Key) => boolean;

  public constructor(
    entries: readonly (readonly [Key, Value])[] = [],
    equals: (left: Key, right: Key) => boolean = structuralKeyEqual,
  ) {
    this.#equals = equals;
    for (const [key, value] of entries) this.set(key, value);
  }

  #index(key: Key): number {
    return this.#entries.findIndex(([candidate]) => this.#equals(candidate, key));
  }

  public get size(): number { return this.#entries.length; }
  public has(key: Key): boolean { return this.#index(key) >= 0; }
  public get(key: Key): Value | undefined { return this.#entries[this.#index(key)]?.[1]; }

  public set(key: Key, value: Value): this {
    const index = this.#index(key);
    if (index < 0) this.#entries.push([key, value]);
    else this.#entries[index] = [this.#entries[index]?.[0] as Key, value];
    return this;
  }

  public delete(key: Key): boolean {
    const index = this.#index(key);
    if (index < 0) return false;
    this.#entries.splice(index, 1);
    return true;
  }

  public clear(): void { this.#entries.length = 0; }
  public *entries(): IterableIterator<[Key, Value]> {
    for (const entry of this.#entries) yield [...entry];
  }
  public *keys(): IterableIterator<Key> {
    for (const [key] of this.#entries) yield key;
  }
  public *values(): IterableIterator<Value> {
    for (const [, value] of this.#entries) yield value;
  }
  public [Symbol.iterator](): IterableIterator<[Key, Value]> { return this.entries(); }
}
