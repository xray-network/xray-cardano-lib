import type {
  CardanoErrorCode,
  CardanoErrorOptions,
  ErrorPathComponent,
} from "./types.js";

export class CardanoError extends Error {
  public override readonly name: string = "CardanoError";
  public readonly code: CardanoErrorCode;
  public readonly path: readonly ErrorPathComponent[];

  public constructor(
    code: CardanoErrorCode,
    message: string,
    options: CardanoErrorOptions = {},
  ) {
    super(message, options.cause === undefined ? undefined : { cause: options.cause });
    this.code = code;
    this.path = Object.freeze([...(options.path ?? [])]);
  }
}

export class CardanoBoundsError extends CardanoError {
  public override readonly name: string = "CardanoBoundsError";
  public readonly minimum: bigint;
  public readonly maximum: bigint;
  public readonly actual: bigint;

  public constructor(name: string, minimum: bigint, maximum: bigint, actual: bigint) {
    super("BOUNDS", `${name} must be between ${minimum} and ${maximum}; received ${actual}`);
    this.minimum = minimum;
    this.maximum = maximum;
    this.actual = actual;
  }
}
