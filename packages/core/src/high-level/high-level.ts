/**
 * @module HighLevel
 * High-level APIs.
 *
 * This module provides convenient wrappers around low-level `bindings` functions,
 * offering type-safe interfaces for common CKB operations like:
 * - Loading and parsing cells, inputs, and transactions
 * - Querying cell data and metadata
 * - Iterating over collections of cells
 *
 * Some low-level functions from `bindings` (e.g., loadWitness, loadScriptHash, exec)
 * can be used directly when needed.
 */

import * as bindings from "@ckb-js-std/bindings";
import {
  CellOutput,
  CellInput,
  WitnessArgs,
  Transaction,
  Script,
  OutPoint,
} from "../ckb";
import { bigintFromBytes } from "../num";
import { bytesEq } from "../bytes";

export function loadCell(
  index: number,
  source: bindings.SourceType,
): CellOutput {
  let bytes = bindings.loadCell(index, source);
  return CellOutput.fromBytes(new Uint8Array(bytes));
}

export function loadInput(
  index: number,
  source: bindings.SourceType,
): CellInput {
  let bytes = bindings.loadInput(index, source);
  return CellInput.fromBytes(new Uint8Array(bytes));
}

export function loadWitnessArgs(
  index: number,
  source: bindings.SourceType,
): WitnessArgs {
  let bytes = bindings.loadWitness(index, source);
  return WitnessArgs.fromBytes(new Uint8Array(bytes));
}

export function loadTransaction(): Transaction {
  let bytes = bindings.loadTransaction();
  return Transaction.fromBytes(new Uint8Array(bytes));
}

export function loadCellCapacity(
  index: number,
  source: bindings.SourceType,
): bigint {
  let bytes = bindings.loadCellByField(
    index,
    source,
    bindings.CELL_FIELD_CAPACITY,
  );
  return bigintFromBytes(new Uint8Array(bytes));
}

export function loadCellOccupiedCapacity(
  index: number,
  source: bindings.SourceType,
): bigint {
  let bytes = bindings.loadCellByField(
    index,
    source,
    bindings.CELL_FIELD_OCCUPIED_CAPACITY,
  );
  return bigintFromBytes(new Uint8Array(bytes));
}

export function loadCellTypeHash(
  index: number,
  source: bindings.SourceType,
): ArrayBuffer | null {
  try {
    let bytes = bindings.loadCellByField(
      index,
      source,
      bindings.CELL_FIELD_TYPE_HASH,
    );
    return new Uint8Array(bytes);
  } catch (e: any) {
    if (e.errorCode === bindings.ITEM_MISSING) {
      return null;
    } else {
      throw e;
    }
  }
}

export function loadCellLock(
  index: number,
  source: bindings.SourceType,
): Script {
  let bytes = bindings.loadCellByField(index, source, bindings.CELL_FIELD_LOCK);
  return Script.fromBytes(new Uint8Array(bytes));
}

export function loadCellType(
  index: number,
  source: bindings.SourceType,
): Script | null {
  try {
    let bytes = bindings.loadCellByField(
      index,
      source,
      bindings.CELL_FIELD_TYPE,
    );
    return Script.fromBytes(new Uint8Array(bytes));
  } catch (e: any) {
    if (e.errorCode === bindings.ITEM_MISSING) {
      return null;
    } else {
      throw e;
    }
  }
}

export function loadHeaderEpochNumber(
  index: number,
  source: bindings.SourceType,
): bigint {
  let bytes = bindings.loadHeaderByField(
    index,
    source,
    bindings.HEADER_FIELD_EPOCH_NUMBER,
  );
  return bigintFromBytes(new Uint8Array(bytes));
}

export function loadHeaderEpochStartBlockNumber(
  index: number,
  source: bindings.SourceType,
): bigint {
  let bytes = bindings.loadHeaderByField(
    index,
    source,
    bindings.HEADER_FIELD_EPOCH_START_BLOCK_NUMBER,
  );
  return bigintFromBytes(new Uint8Array(bytes));
}

export function loadHeaderEpochLength(
  index: number,
  source: bindings.SourceType,
): bigint {
  let bytes = bindings.loadHeaderByField(
    index,
    source,
    bindings.HEADER_FIELD_EPOCH_LENGTH,
  );
  return bigintFromBytes(new Uint8Array(bytes));
}

export function loadInputSince(
  index: number,
  source: bindings.SourceType,
): bigint {
  let bytes = bindings.loadInputByField(
    index,
    source,
    bindings.INPUT_FIELD_SINCE,
  );
  return bigintFromBytes(new Uint8Array(bytes));
}

export function loadInputOutPoint(
  index: number,
  source: bindings.SourceType,
): OutPoint {
  let bytes = bindings.loadInputByField(
    index,
    source,
    bindings.INPUT_FIELD_OUT_POINT,
  );
  return OutPoint.fromBytes(new Uint8Array(bytes));
}

export function loadScript(): Script {
  let bytes = bindings.loadScript();
  return Script.fromBytes(new Uint8Array(bytes));
}

/**
 * Generic type for query functions that can be used with QueryIter
 * F is a function that takes (index: number, source: SourceType) and returns T
 */
type QueryFunction<T> = (index: number, source: bindings.SourceType) => T;

/**
 * QueryIter provides iteration over CKB query functions
 * It handles the common pattern of querying cells/inputs/headers by index
 */
export class QueryIter<T> implements Iterator<T> {
  private queryFn: QueryFunction<T>;
  private index: number;
  private source: bindings.SourceType;

  /**
   * Creates a new QueryIter
   * @param queryFn - A high level query function, which accepts (index, source) as args
   * @param source - The source to query from
   *
   * @example
   * ```typescript
   * import { loadCell } from './high-level';
   * // iterate all input cells
   * const iter = new QueryIter(loadCell, SourceType.INPUT);
   * ```
   */
  constructor(queryFn: QueryFunction<T>, source: bindings.SourceType) {
    this.queryFn = queryFn;
    this.index = 0;
    this.source = source;
  }

  /**
   * Gets the next item in the iteration
   * @returns The next item or undefined if iteration is complete
   */
  next(): IteratorResult<T> {
    try {
      const item = this.queryFn(this.index, this.source);
      this.index += 1;
      return { value: item, done: false };
    } catch (err: any) {
      if (err.errorCode === bindings.INDEX_OUT_OF_BOUND) {
        return { value: undefined, done: true };
      }
      throw new Error(`QueryIter error: ${err.message || err}`);
    }
  }

  /**
   * Makes QueryIter iterable, allowing it to be used in for...of loops
   */
  [Symbol.iterator](): Iterator<T> {
    return this;
  }

  /**
   * Converts the iterator to an array
   * @returns Array containing all items
   */
  toArray(): T[] {
    const results: T[] = [];
    for (const item of this) {
      results.push(item);
    }
    return results;
  }
}

/**
 * Find cell by data_hash
 *
 * Iterate and find the cell which data hash equals `dataHash`,
 * return the index of the first cell found, otherwise return null.
 *
 * @param dataHash - The data hash to search for (must be 32 bytes)
 * @param source - The source to search in
 * @returns The index of the found cell or null if not found
 * @throws Error if dataHash is not 32 bytes or if there's a system error
 */
export function findCellByDataHash(
  dataHash: Uint8Array,
  source: bindings.SourceType,
): number | null {
  if (dataHash.length !== 32) {
    throw new Error("dataHash must be 32 bytes");
  }

  for (let i = 0; ; i++) {
    try {
      const hash = bindings.loadCellByField(
        i,
        source,
        bindings.CELL_FIELD_DATA_HASH,
      );
      const hashArray = new Uint8Array(hash);
      if (
        hashArray.length === dataHash.length &&
        bytesEq(hashArray, dataHash)
      ) {
        return i;
      }
    } catch (err: any) {
      if (err.errorCode === bindings.INDEX_OUT_OF_BOUND) {
        break;
      }
      throw err;
    }
  }
  return null;
}

/**
 * Look for a dep cell with specific code hash
 *
 * @param codeHash - The code hash to search for (must be 32 bytes)
 * @param hashType - The type of hash to search for (type or data)
 * @returns The index of the found cell
 * @throws Error if codeHash is not 32 bytes, if cell is not found, or if there's a system error
 */
export function lookForDepWithHash2(
  codeHash: Uint8Array,
  hashType: number,
): number {
  if (codeHash.length !== 32) {
    throw new Error("codeHash must be 32 bytes");
  }

  const field =
    hashType === bindings.SCRIPT_HASH_TYPE_TYPE
      ? bindings.CELL_FIELD_TYPE_HASH
      : bindings.CELL_FIELD_DATA_HASH;

  let current = 0;
  while (true) {
    try {
      const hash = bindings.loadCellByField(
        current,
        bindings.SOURCE_CELL_DEP,
        field,
      );
      const hashArray = new Uint8Array(hash);
      if (bytesEq(hashArray, codeHash)) {
        return current;
      }
    } catch (err: any) {
      if (err.errorCode === bindings.ITEM_MISSING) {
        // Skip missing items
      } else if (err.errorCode === bindings.INDEX_OUT_OF_BOUND) {
        throw err; // Propagate INDEX_OUT_OF_BOUND error
      } else {
        throw err; // Propagate other errors
      }
    }
    current++;
  }
}

/**
 * Look for a dep cell with specific data hash
 *
 * @param dataHash - The data hash to search for (must be 32 bytes)
 * @returns The index of the found cell
 * @throws Error if dataHash is not 32 bytes, if cell is not found, or if there's a system error
 */
export function lookForDepWithDataHash(dataHash: Uint8Array): number {
  return lookForDepWithHash2(dataHash, bindings.SCRIPT_HASH_TYPE_DATA);
}
