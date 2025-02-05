import { Bytes, BytesLike, bytesFrom } from "../bytes/index.js";
import { Hasher } from "./hasher.js";
import { Keccak256 } from "@ckb-js-std/bindings";

/**
 * @public
 */
export class HasherKeecak256 implements Hasher {
  private readonly hasher: Keccak256;

  /**
   * Creates an instance of Hasher.
   */

  constructor() {
    this.hasher = new Keccak256();
  }

  /**
   * Updates the hash with the given data.
   *
   * @param data - The data to update the hash with.
   * @returns The current Hasher instance for chaining.
   *
   * @example
   * ```typescript
   * const hasher = new Hasher();
   * hasher.update("some data").update("more data");
   * const hash = hasher.digest();
   * ```
   */

  update(data: BytesLike): HasherKeecak256 {
    this.hasher.update(data);
    return this;
  }

  /**
   * Finalizes the hash and returns the digest as a hexadecimal string.
   *
   * @returns The hexadecimal string representation of the hash.
   *
   * @example
   * ```typescript
   * const hasher = new Hasher();
   * hasher.update("some data");
   * const hash = hasher.digest(); // Outputs something like "0x..."
   * ```
   */

  digest(): Bytes {
    return new Uint8Array(this.hasher.finalize());
  }
}
