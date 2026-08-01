import type { CborValue } from "@xray-network/xray-cardano-lib-core";
import {
  bls12_381_uncompress,
} from "@xray-network/xray-cardano-lib-crypto";
import type {
  UplcConstant,
  UplcProgram,
  UplcTerm,
  UplcType,
} from "./ast.js";
import { builtinTag } from "./cost-model.js";

const definite = { kind: "definite", width: 0 } as const;

export function parseUplcText(source: string): UplcProgram {
  const parser = new TextParser(tokenize(source));
  const program = parser.program();
  parser.finish();
  return program;
}

class TextParser {
  readonly #tokens: readonly string[];
  #offset = 0;
  #version: readonly [bigint, bigint, bigint] = [1n, 0n, 0n];

  public constructor(tokens: readonly string[]) {
    this.#tokens = tokens;
  }

  public program(): UplcProgram {
    this.expect("(");
    this.expect("program");
    const version = this.take().split(".");
    if (version.length !== 3 || version.some((item) => !/^\d+$/.test(item))) this.error("invalid UPLC version");
    this.#version = [
      BigInt(version[0] as string),
      BigInt(version[1] as string),
      BigInt(version[2] as string),
    ];
    const term = this.term([]);
    this.expect(")");
    return {
      version: this.#version,
      term,
    };
  }

  public finish(): void {
    if (this.peek() !== undefined) this.error("trailing UPLC text");
  }

  private term(environment: readonly string[]): UplcTerm {
    const token = this.peek();
    if (token === "[") {
      this.take();
      const terms: UplcTerm[] = [];
      while (this.peek() !== "]") {
        if (this.peek() === undefined) this.error("unterminated application");
        terms.push(this.term(environment));
      }
      this.take();
      if (terms.length < 2) this.error("application requires at least two terms");
      return terms.slice(1).reduce<UplcTerm>(
        (function_, argument) => ({ kind: "apply", function: function_, argument }),
        terms[0] as UplcTerm,
      );
    }
    if (token === "(") {
      this.take();
      const constructor = this.take();
      switch (constructor) {
        case "lam": {
          const name = this.take();
          if (isPunctuation(name)) this.error("invalid lambda name");
          const body = this.term([...environment, name]);
          this.expect(")");
          return { kind: "lambda", body };
        }
        case "delay": {
          const term = this.term(environment);
          this.expect(")");
          return { kind: "delay", term };
        }
        case "force": {
          const term = this.term(environment);
          this.expect(")");
          return { kind: "force", term };
        }
        case "builtin": {
          const name = this.take();
          const tag = builtinTag(name);
          if (tag === undefined) this.error(`unknown builtin ${name}`);
          this.expect(")");
          return { kind: "builtin", tag };
        }
        case "con": {
          const type = this.type();
          const value = this.constantValue(type);
          this.expect(")");
          return { kind: "constant", constant: { type, value } };
        }
        case "error":
          this.expect(")");
          return { kind: "error" };
        case "constr": {
          this.requireVersion1_1("constr");
          const tag = this.integer();
          if (tag < 0n || tag > 0xffff_ffff_ffff_ffffn) this.error("constructor tag is outside Word64");
          const fields: UplcTerm[] = [];
          while (this.peek() !== ")") fields.push(this.term(environment));
          this.take();
          return { kind: "constr", tag, fields };
        }
        case "case": {
          this.requireVersion1_1("case");
          const scrutinee = this.term(environment);
          const branches: UplcTerm[] = [];
          while (this.peek() !== ")") branches.push(this.term(environment));
          this.take();
          return { kind: "case", scrutinee, branches };
        }
        default:
          this.error(`unknown term constructor ${constructor}`);
      }
    }
    const name = this.take();
    const position = environment.lastIndexOf(name);
    return {
      kind: "var",
      index: position < 0
        ? BigInt(environment.length + 1)
        : BigInt(environment.length - position),
    };
  }

  private type(): UplcType {
    const token = this.take();
    if (token === "(") {
      const constructor = this.take();
      if (constructor === "list" || constructor === "array") {
        const item = this.type();
        this.expect(")");
        return { kind: constructor, item };
      }
      if (constructor === "pair") {
        const first = this.type();
        const second = this.type();
        this.expect(")");
        return { kind: "pair", first, second };
      }
      this.error(`invalid type constructor ${constructor}`);
    }
    switch (token) {
      case "integer": return { kind: "integer" };
      case "bytestring": return { kind: "bytes" };
      case "string": return { kind: "string" };
      case "unit": return { kind: "unit" };
      case "bool": return { kind: "boolean" };
      case "data": return { kind: "data" };
      case "bls12_381_G1_element": return { kind: "bls-g1" };
      case "bls12_381_G2_element": return { kind: "bls-g2" };
      case "bls12_381_mlresult": return { kind: "bls-ml" };
      case "value": return { kind: "value" };
      default: this.error(`unknown constant type ${token}`);
    }
  }

  private constantValue(type: UplcType, nested = false): unknown {
    switch (type.kind) {
      case "integer": return this.integer();
      case "bytes": return this.bytes("#");
      case "string": return decodeString(this.take(), (message) => this.error(message));
      case "unit":
        this.expect("(");
        this.expect(")");
        return null;
      case "boolean": {
        const token = this.take();
        if (token !== "True" && token !== "False") this.error("invalid boolean constant");
        return token === "True";
      }
      case "data":
        if (nested) return this.data();
        this.expect("(");
        {
          const value = this.data();
          this.expect(")");
          return value;
        }
      case "bls-g1": {
        const value = this.bytes("0x");
        if (value.length !== 48 || ((value[0] ?? 0) & 0x80) === 0) {
          this.error("invalid compressed BLS12-381 G1 constant");
        }
        try { return bls12_381_uncompress("g1", value); }
        catch { this.error("invalid BLS12-381 G1 constant"); }
      }
      case "bls-g2": {
        const value = this.bytes("0x");
        if (value.length !== 96 || ((value[0] ?? 0) & 0x80) === 0) {
          this.error("invalid compressed BLS12-381 G2 constant");
        }
        try { return bls12_381_uncompress("g2", value); }
        catch { this.error("invalid BLS12-381 G2 constant"); }
      }
      case "bls-ml": this.error("ML results have no textual constant encoding");
      case "list":
      case "array":
        return this.list(() => this.constantValue(type.item, true));
      case "pair":
        this.expect("(");
        {
          const first = this.constantValue(type.first, true);
          this.expect(",");
          const second = this.constantValue(type.second, true);
          this.expect(")");
          return [first, second];
        }
      case "value": {
        const value = this.list(() => {
          this.expect("(");
          const currency = this.bytes("#");
          this.expect(",");
          const tokens = this.list(() => {
            this.expect("(");
            const token = this.bytes("#");
            this.expect(",");
            const quantity = this.integer();
            this.expect(")");
            return [token, quantity] as const;
          });
          this.expect(")");
          return [currency, tokens] as const;
        });
        validateValue(value, (message) => this.error(message));
        return value;
      }
    }
  }

  private data(): CborValue {
    const constructor = this.take();
    let output: CborValue;
    switch (constructor) {
      case "I": output = integerNode(this.integer()); break;
      case "B": output = bytesNode(this.bytes("#")); break;
      case "List":
        output = { kind: "array", values: this.list(() => this.data()), encoding: definite };
        break;
      case "Map":
        output = {
          kind: "map",
          entries: this.list(() => {
            this.expect("(");
            const key = this.data();
            this.expect(",");
            const value = this.data();
            this.expect(")");
            return [key, value] as const;
          }),
          encoding: definite,
        };
        break;
      case "Constr": {
        const alternative = this.integer();
        if (alternative < 0n) this.error("negative Data constructor");
        const fields = this.list(() => this.data());
        output = constrData(alternative, fields);
        break;
      }
      default: this.error(`invalid Data constructor ${constructor}`);
    }
    return output;
  }

  private list<T>(item: () => T): T[] {
    this.expect("[");
    const output: T[] = [];
    while (this.peek() !== "]") {
      output.push(item());
      if (this.peek() === ",") this.take();
      else if (this.peek() !== "]") this.error("missing list separator");
    }
    this.take();
    return output;
  }

  private integer(): bigint {
    const token = this.take();
    if (!/^[+-]?\d+$/.test(token)) this.error(`invalid integer ${token}`);
    return BigInt(token);
  }

  private bytes(prefix: "#" | "0x"): Uint8Array {
    const token = this.take();
    if (!token.startsWith(prefix)) this.error(`byte string must start with ${prefix}`);
    const hex = token.slice(prefix.length);
    if (hex.length % 2 !== 0 || !/^[0-9a-fA-F]*$/.test(hex)) this.error("invalid hexadecimal byte string");
    return Uint8Array.from(hex.match(/../g)?.map((item) => Number.parseInt(item, 16)) ?? []);
  }

  private peek(): string | undefined {
    return this.#tokens[this.#offset];
  }

  private take(): string {
    const token = this.#tokens[this.#offset];
    if (token === undefined) this.error("unexpected end of UPLC text");
    this.#offset += 1;
    return token;
  }

  private expect(expected: string): void {
    const actual = this.take();
    if (actual !== expected) this.error(`expected ${expected}, received ${actual}`);
  }

  private requireVersion1_1(feature: string): void {
    const [major, minor] = this.#version;
    if (major < 1n || major === 1n && minor < 1n) {
      this.error(`${feature} requires UPLC version 1.1.0`);
    }
  }

  private error(message: string): never {
    throw new SyntaxError(`${message} at token ${this.#offset}`);
  }
}

function tokenize(source: string): string[] {
  const output: string[] = [];
  let offset = 0;
  while (offset < source.length) {
    const char = source[offset] as string;
    if (/\s/.test(char)) {
      offset += 1;
      continue;
    }
    if (char === "-" && source[offset + 1] === "-") {
      offset += 2;
      while (offset < source.length && source[offset] !== "\n") offset += 1;
      continue;
    }
    if ("()[]{}, ".includes(char) && char !== " ") {
      output.push(char);
      offset += 1;
      continue;
    }
    if (char === "\"") {
      const start = offset++;
      let escaped = false;
      while (offset < source.length) {
        const current = source[offset++] as string;
        if (!escaped && current === "\"") break;
        if (!escaped && current === "\\") escaped = true;
        else escaped = false;
      }
      if (source[offset - 1] !== "\"") throw new SyntaxError("unterminated string");
      output.push(source.slice(start, offset));
      continue;
    }
    const start = offset;
    while (
      offset < source.length &&
      !/\s/.test(source[offset] as string) &&
      !"()[]{},\"".includes(source[offset] as string) &&
      !(source[offset] === "-" && source[offset + 1] === "-")
    ) offset += 1;
    output.push(source.slice(start, offset));
  }
  return output;
}

function decodeString(token: string, fail: (message: string) => never): string {
  if (!token.startsWith("\"") || !token.endsWith("\"")) fail("invalid string constant");
  const source = token.slice(1, -1);
  let output = "";
  for (let offset = 0; offset < source.length;) {
    const char = source[offset++] as string;
    if (char !== "\\") {
      output += char;
      continue;
    }
    if (offset >= source.length) fail("truncated string escape");
    const escape = source[offset++] as string;
    const simple: Record<string, string> = {
      a: "\u0007", b: "\b", f: "\f", n: "\n", r: "\r", t: "\t", v: "\v",
      "\\": "\\", "\"": "\"", "&": "",
    };
    if (simple[escape] !== undefined) {
      output += simple[escape];
      continue;
    }
    const named: Record<string, number> = {
      NUL: 0, SOH: 1, STX: 2, ETX: 3, EOT: 4, ENQ: 5, ACK: 6, BEL: 7,
      BS: 8, HT: 9, LF: 10, VT: 11, FF: 12, CR: 13, SO: 14, SI: 15,
      DEL: 127,
    };
    const namedKey = Object.keys(named).find((name) => source.startsWith(name, offset - 1));
    if (namedKey !== undefined) {
      output += String.fromCodePoint(named[namedKey] as number);
      offset += namedKey.length - 1;
      continue;
    }
    let radix = 10;
    let digits = "";
    if (escape === "x" || escape === "o") {
      radix = escape === "x" ? 16 : 8;
      const pattern = radix === 16 ? /[0-9a-fA-F]/ : /[0-7]/;
      while (offset < source.length && pattern.test(source[offset] as string)) {
        digits += source[offset++] as string;
      }
    } else if (/\d/.test(escape)) {
      digits = escape;
      while (offset < source.length && /\d/.test(source[offset] as string)) {
        digits += source[offset++] as string;
      }
    } else {
      fail(`unsupported string escape \\${escape}`);
    }
    if (digits.length === 0) fail("empty numeric escape");
    output += String.fromCodePoint(Number.parseInt(digits, radix));
  }
  return output;
}

function integerNode(value: bigint): CborValue {
  if (value >= 0n && value <= 0xffff_ffff_ffff_ffffn) {
    return { kind: "unsigned", value, encoding: { width: 0 } };
  }
  if (value < 0n && value >= -0x1_0000_0000_0000_0000n) {
    return { kind: "negative", value, encoding: { width: 0 } };
  }
  const positive = value >= 0n;
  return {
    kind: "tag",
    tag: positive ? 2n : 3n,
    value: bytesNode(bigintBytes(positive ? value : -value - 1n)),
    encoding: { width: 0 },
  };
}

function bigintBytes(value: bigint): Uint8Array {
  const output: number[] = [];
  let remaining = value;
  do {
    output.unshift(Number(remaining & 0xffn));
    remaining >>= 8n;
  } while (remaining !== 0n);
  return Uint8Array.from(output);
}

function bytesNode(value: Uint8Array): CborValue {
  return { kind: "bytes", value, encoding: definite };
}

function constrData(alternative: bigint, fields: readonly CborValue[]): CborValue {
  const value: CborValue = { kind: "array", values: fields, encoding: definite };
  if (alternative <= 6n) return { kind: "tag", tag: 121n + alternative, value, encoding: { width: 0 } };
  if (alternative <= 127n) return { kind: "tag", tag: 1280n + alternative - 7n, value, encoding: { width: 0 } };
  return {
    kind: "tag",
    tag: 102n,
    value: {
      kind: "array",
      values: [integerNode(alternative), value],
      encoding: definite,
    },
    encoding: { width: 0 },
  };
}

function validateValue(
  value: readonly (readonly [
    Uint8Array,
    readonly (readonly [Uint8Array, bigint])[],
  ])[],
  fail: (message: string) => never,
): void {
  let previousCurrency: Uint8Array | undefined;
  for (const [currency, tokens] of value) {
    if (currency.length > 32) fail("Value currency exceeds 32 bytes");
    if (previousCurrency !== undefined && compareBytes(previousCurrency, currency) >= 0) {
      fail("Value currencies are not strictly ordered");
    }
    if (tokens.length === 0) fail("Value currency contains no tokens");
    let previousToken: Uint8Array | undefined;
    for (const [token, quantity] of tokens) {
      if (token.length > 32) fail("Value token exceeds 32 bytes");
      if (previousToken !== undefined && compareBytes(previousToken, token) >= 0) {
        fail("Value tokens are not strictly ordered");
      }
      if (quantity === 0n) fail("Value quantity must be nonzero");
      if (quantity < -(1n << 127n) || quantity >= 1n << 127n) {
        fail("Value quantity exceeds signed 128-bit bounds");
      }
      previousToken = token;
    }
    previousCurrency = currency;
  }
}

function compareBytes(left: Uint8Array, right: Uint8Array): number {
  const length = Math.min(left.length, right.length);
  for (let index = 0; index < length; index += 1) {
    const difference = (left[index] ?? 0) - (right[index] ?? 0);
    if (difference !== 0) return difference;
  }
  return left.length - right.length;
}

function isPunctuation(value: string): boolean {
  return value.length === 1 && "()[]{}, ".includes(value);
}
