import { Bytes, BytesLike, bytesFrom } from "../bytes/index";
import { CellInput, CellInputLike } from "../ckb/index";
import { NumLike, numToBytes } from "../num/index";
import { CKB_BLAKE2B_PERSONAL } from "./advanced";
import { Hasher } from "./hasher.js";
import { Blake2b } from "@ckb-js-std/bindings";

/**
 * @public
 */
export class HasherCkb implements Hasher {
  private readonly hasher: Blake2b;
  private outLength: number;

  /**
   * Creates an instance of Hasher.
   *
   * @param outLength - The output length of the hash in bytes. Default is 32.
   * @param personal - The personal string for the Blake2b algorithm. Default is CKB_BLAKE2B_PERSONAL.
   */

  constructor(outLength = 32, personal = CKB_BLAKE2B_PERSONAL) {
    this.hasher = new Blake2b(personal);
    this.outLength = outLength;
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

  update(data: Bytes): HasherCkb {
    this.hasher.update(data);
    return this;
  }

  /**
   * Finalizes the hash and returns the digest.
   *
   * @returns The hash.
   *
   * @example
   * ```typescript
   * const hasher = new Hasher();
   * hasher.update("some data");
   * const hash = hasher.digest();
   * ```
   */

  digest(): Bytes {
    let result = this.hasher.finalize();
    return result.slice(0, this.outLength);
  }
}

/**
 * Computes the CKB hash of the given data using the Blake2b algorithm.
 * @public
 *
 * @param data - The data to hash.
 * @returns The hash.
 *
 * @example
 * ```typescript
 * const hash = hashCkb("some data");
 * ```
 */

export function hashCkb(...data: BytesLike[]): Bytes {
  const hasher = new HasherCkb();
  data.forEach((d) => hasher.update(d));
  return hasher.digest();
}

/**
 * Computes the Type ID hash of the given data.
 * @public
 *
 * @param cellInputLike - A input cell of the transaction.
 * @param outputIndex - The output index of the Type ID cell.
 * @returns The hash.
 *
 * @example
 * ```typescript
 * const hash = hashTypeId(cellInput, outputIndex);
 * ```
 */

export function hashTypeId(
  cellInputLike: CellInputLike,
  outputIndex: NumLike,
): Bytes {
  return hashCkb(
    CellInput.from(cellInputLike).toBytes(),
    numToBytes(outputIndex, 8),
  );
}
