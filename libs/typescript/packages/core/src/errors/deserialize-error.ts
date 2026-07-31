import { CardanoError } from "./cardano-error.js";
import type {
  CardanoErrorOptions,
  DeserializeFailure,
  ErrorPathComponent,
} from "./types.js";

export class DeserializeError extends CardanoError {
  public override readonly name: string = "DeserializeError";
  public readonly failure: DeserializeFailure;
  public readonly offset: number | undefined;

  public constructor(
    failure: DeserializeFailure,
    message: string,
    options: CardanoErrorOptions & { readonly offset?: number } = {},
  ) {
    super("DESERIALIZE", message, options);
    this.failure = failure;
    this.offset = options.offset;
  }

  /** Return the same failure annotated with its enclosing field or index. */
  public annotate(component: ErrorPathComponent): DeserializeError {
    return new DeserializeError(this.failure, this.message, {
      cause: this,
      ...(this.offset === undefined ? {} : { offset: this.offset }),
      path: [component, ...this.path],
    });
  }
}
