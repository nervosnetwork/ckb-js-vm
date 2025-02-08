/**
 * @module HighLevel
 * High-level APIs for CKB-VM script development.
 *
 * This module provides convenient wrappers around low-level `bindings` functions,
 * offering type-safe interfaces for common CKB operations like:
 * - Loading and parsing cells, inputs, and transactions
 * - Querying cell data and metadata
 * - Iterating over collections of cells
 *
 * Some low-level functions from `bindings` (e.g., loadWitness, loadScriptHash, exec)
 * can be used directly when needed.
 *
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

/**
 * Load cell
 *
 * @param index - The index of the cell to load
 * @param source - The source to load the cell from
 * @returns The loaded cell output
 * @throws Error if there's a system error or if the cell cannot be found
 *
 * @example
 * ```typescript
 * const cellOutput = loadCell(0, SourceType.INPUT);
 * ```
 *
 * **Note:** This function can throw if the underlying data is too large,
 * potentially causing an out-of-memory error.
 */
export function loadCell(
  index: number,
  source: bindings.SourceType,
): CellOutput {
  let bytes = bindings.loadCell(index, source);
  return CellOutput.fromBytes(new Uint8Array(bytes));
}

/**
 * Load input
 *
 * @param index - The index of the input to load
 * @param source - The source to load the input from
 * @returns The loaded cell input
 * @throws Error if there's a system error or if the input cannot be found
 *
 * @example
 * ```typescript
 * const input = loadInput(0, SourceType.INPUT);
 * ```
 */
export function loadInput(
  index: number,
  source: bindings.SourceType,
): CellInput {
  let bytes = bindings.loadInput(index, source);
  return CellInput.fromBytes(new Uint8Array(bytes));
}

/**
 * Load witness args
 *
 * @param index - The index of the witness to load
 * @param source - The source to load the witness from
 * @returns The loaded witness args
 * @throws Error if there's a system error or if the witness cannot be found
 *
 * @example
 * ```typescript
 * const witnessArgs = loadWitnessArgs(0, SourceType.INPUT);
 * ```
 *
 * **Note:** This function can throw if the underlying data is too large,
 * potentially causing an out-of-memory error.
 */
export function loadWitnessArgs(
  index: number,
  source: bindings.SourceType,
): WitnessArgs {
  let bytes = bindings.loadWitness(index, source);
  return WitnessArgs.fromBytes(new Uint8Array(bytes));
}

/**
 * Load transaction
 *
 * @returns The current transaction
 * @throws Error if there's a system error
 *
 * @example
 * ```typescript
 * const tx = loadTransaction();
 * ```
 *
 * **Note:** This function can throw if the underlying data is too large,
 * potentially causing an out-of-memory error.
 */
export function loadTransaction(): Transaction {
  let bytes = bindings.loadTransaction();
  return Transaction.fromBytes(new Uint8Array(bytes));
}

/**
 * Load cell capacity
 *
 * @param index - The index of the cell to load
 * @param source - The source to load the cell from
 * @returns The capacity of the cell
 * @throws Error if there's a system error or if the cell cannot be found
 *
 * @example
 * ```typescript
 * const capacity = loadCellCapacity(0, SourceType.INPUT);
 * ```
 */
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

/**
 * Load cell occupied capacity
 *
 * @param index - The index of the cell to load
 * @param source - The source to load the cell from
 * @returns The occupied capacity of the cell
 * @throws Error if there's a system error or if the cell cannot be found
 *
 * @example
 * ```typescript
 * const occupiedCapacity = loadCellOccupiedCapacity(0, SourceType.INPUT);
 * ```
 */
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

/**
 * Load cell type hash
 *
 * @param index - The index of the cell to load
 * @param source - The source to load the cell from
 * @returns The type hash of the cell, or null if the cell has no type script
 * @throws Error if there's a system error or if the cell cannot be found
 *
 * @example
 * ```typescript
 * const typeHash = loadCellTypeHash(0, SourceType.INPUT);
 * if (typeHash !== null) {
 *   // Cell has a type script
 * }
 * ```
 */
export function loadCellTypeHash(
  index: number,
  source: bindings.SourceType,
): Uint8Array | null {
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

/**
 * Load cell lock
 *
 * @param index - The index of the cell to load
 * @param source - The source to load the cell from
 * @returns The lock script of the cell
 * @throws Error if there's a system error or if the cell cannot be found
 *
 * @example
 * ```typescript
 * const lock = loadCellLock(0, SourceType.INPUT);
 * ```
 */
export function loadCellLock(
  index: number,
  source: bindings.SourceType,
): Script {
  let bytes = bindings.loadCellByField(index, source, bindings.CELL_FIELD_LOCK);
  return Script.fromBytes(new Uint8Array(bytes));
}

/**
 * Load cell type
 *
 * @param index - The index of the cell to load
 * @param source - The source to load the cell from
 * @returns The type script of the cell, or null if the cell has no type script
 * @throws Error if there's a system error or if the cell cannot be found
 *
 * @example
 * ```typescript
 * const type = loadCellType(0, SourceType.INPUT);
 * if (type !== null) {
 *   // Cell has a type script
 * }
 * ```
 */
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

/**
 * Load header epoch number
 *
 * @param index - The index of the header to load
 * @param source - The source to load the header from
 * @returns The epoch number of the header
 * @throws Error if there's a system error or if the header cannot be found
 *
 * @example
 * ```typescript
 * const epochNumber = loadHeaderEpochNumber(0, SourceType.INPUT);
 * ```
 */
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

/**
 * Load header epoch start block number
 *
 * @param index - The index of the header to load
 * @param source - The source to load the header from
 * @returns The epoch start block number of the header
 * @throws Error if there's a system error or if the header cannot be found
 *
 * @example
 * ```typescript
 * const epochStartBlockNumber = loadHeaderEpochStartBlockNumber(0, SourceType.INPUT);
 * ```
 */
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

/**
 * Load header epoch length
 *
 * @param index - The index of the header to load
 * @param source - The source to load the header from
 * @returns The epoch length of the header
 * @throws Error if there's a system error or if the header cannot be found
 *
 * @example
 * ```typescript
 * const epochLength = loadHeaderEpochLength(0, SourceType.INPUT);
 * ```
 */
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

/**
 * Load input since
 *
 * @param index - The index of the input to load
 * @param source - The source to load the input from
 * @returns The since value of the input
 * @throws Error if there's a system error or if the input cannot be found
 *
 * @example
 * ```typescript
 * const since = loadInputSince(0, SourceType.INPUT);
 * ```
 */
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

/**
 * Load input out point
 *
 * @param index - The index of the input to load
 * @param source - The source to load the input from
 * @returns The out point of the input
 * @throws Error if there's a system error or if the input cannot be found
 *
 * @example
 * ```typescript
 * const outPoint = loadInputOutPoint(0, SourceType.INPUT);
 * ```
 */
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

/**
 * Load script
 *
 * @returns The current script
 * @throws Error if there's a system error
 *
 * @example
 * ```typescript
 * const script = loadScript();
 * ```
 */
export function loadScript(): Script {
  let bytes = bindings.loadScript();
  return Script.fromBytes(new Uint8Array(bytes));
}

/**
 * Generic type for query functions that can be used with QueryIter
 * F is a function that takes (index: number, source: SourceType) and returns T
 */
export type QueryFunction<T> = (
  index: number,
  source: bindings.SourceType,
) => T;

/**
 * QueryIter provides iteration over CKB query functions
 * It handles the common pattern of querying cells/inputs/headers by index
 *
 * @example
 * ```typescript
 * import { loadCellCapacity } from './high-level';
 * // Calculate all inputs capacity
 * const iter = new QueryIter(loadCellCapacity, SourceType.INPUT);
 * const inputsCapacity = iter.toArray().reduce((sum, cap) => sum + cap, 0n);
 *
 * // Calculate all outputs capacity
 * const outputsCapacity = new QueryIter(loadCellCapacity, SourceType.OUTPUT)
 *   .toArray()
 *   .reduce((sum, cap) => sum + cap, 0n);
 *
 * console.assert(inputsCapacity === outputsCapacity);
 * ```
 */
export class QueryIter<T> implements Iterator<T> {
  private queryFn: QueryFunction<T>;
  private index: number;
  private source: bindings.SourceType;

  /**
   * Creates a new QueryIter
   * @param queryFn - A high level query function, which accepts (index, source) as args
   * @param source - The source to query from
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
      if (bytesEq(hashArray, dataHash)) {
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
