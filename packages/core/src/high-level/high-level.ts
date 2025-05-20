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
 * Some low-level functions from `bindings` (e.g., loadWitness, loadScriptHash, execCell, spawnCell)
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
  Header,
} from "../ckb";
import { Bytes, bytesEq } from "../bytes";
import { numFromBytes } from "../num";

/**
 * Re-export constants from bindings to simplify imports.
 *
 * This allows developers to import all necessary constants and functions
 * from a single module (`HighLevel`) without having to directly import
 * from the lower-level `bindings` module.
 *
 * @example
 * ```typescript
 * // Instead of:
 * import { loadCellCapacity } from '@ckb-js-std/high-level';
 * import { SOURCE_INPUT } from '@ckb-js-std/bindings';
 *
 * // You can simply use:
 * import { loadCellCapacity, SOURCE_INPUT } from '@ckb-js-std/high-level';
 * ```
 */
export const SOURCE_CELL_DEP = bindings.SOURCE_CELL_DEP;
export const SOURCE_HEADER_DEP = bindings.SOURCE_HEADER_DEP;
export const SOURCE_INPUT = bindings.SOURCE_INPUT;
export const SOURCE_OUTPUT = bindings.SOURCE_OUTPUT;
export const SOURCE_GROUP_INPUT = bindings.SOURCE_GROUP_INPUT;
export const SOURCE_GROUP_OUTPUT = bindings.SOURCE_GROUP_OUTPUT;

export const CELL_FIELD_CAPACITY = bindings.CELL_FIELD_CAPACITY;
export const CELL_FIELD_DATA_HASH = bindings.CELL_FIELD_DATA_HASH;
export const CELL_FIELD_LOCK = bindings.CELL_FIELD_LOCK;
export const CELL_FIELD_LOCK_HASH = bindings.CELL_FIELD_LOCK_HASH;
export const CELL_FIELD_TYPE = bindings.CELL_FIELD_TYPE;
export const CELL_FIELD_TYPE_HASH = bindings.CELL_FIELD_TYPE_HASH;
export const CELL_FIELD_OCCUPIED_CAPACITY =
  bindings.CELL_FIELD_OCCUPIED_CAPACITY;

export const HEADER_FIELD_EPOCH_NUMBER = bindings.HEADER_FIELD_EPOCH_NUMBER;
export const HEADER_FIELD_EPOCH_START_BLOCK_NUMBER =
  bindings.HEADER_FIELD_EPOCH_START_BLOCK_NUMBER;
export const HEADER_FIELD_EPOCH_LENGTH = bindings.HEADER_FIELD_EPOCH_LENGTH;

export const INPUT_FIELD_OUT_POINT = bindings.INPUT_FIELD_OUT_POINT;
export const INPUT_FIELD_SINCE = bindings.INPUT_FIELD_SINCE;

export const SCRIPT_HASH_TYPE_DATA = bindings.SCRIPT_HASH_TYPE_DATA;
export const SCRIPT_HASH_TYPE_TYPE = bindings.SCRIPT_HASH_TYPE_TYPE;
export const SCRIPT_HASH_TYPE_DATA1 = bindings.SCRIPT_HASH_TYPE_DATA1;
export const SCRIPT_HASH_TYPE_DATA2 = bindings.SCRIPT_HASH_TYPE_DATA2;

export const INDEX_OUT_OF_BOUND = bindings.INDEX_OUT_OF_BOUND;
export const ITEM_MISSING = bindings.ITEM_MISSING;
export const LENGTH_NOT_ENOUGH = bindings.LENGTH_NOT_ENOUGH;
export const INVALID_DATA = bindings.INVALID_DATA;
export const WAIT_FAILURE = bindings.WAIT_FAILURE;
export const INVALID_FD = bindings.INVALID_FD;
export const OTHER_END_CLOSED = bindings.OTHER_END_CLOSED;
export const MAX_VMS_SPAWNED = bindings.MAX_VMS_SPAWNED;
export const MAX_FDS_CREATED = bindings.MAX_FDS_CREATED;

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
 * const cellOutput = loadCell(0, SOURCE_INPUT);
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
  return CellOutput.fromBytes(bytes);
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
 * const input = loadInput(0, SOURCE_INPUT);
 * ```
 */
export function loadInput(
  index: number,
  source: bindings.SourceType,
): CellInput {
  let bytes = bindings.loadInput(index, source);
  return CellInput.fromBytes(bytes);
}

/**
 * Load raw witness
 *
 * @param index - The index of the witness to load
 * @param source - The source to load the witness from
 * @returns The loaded witness
 */
export function loadWitness(
  index: number,
  source: bindings.SourceType,
): ArrayBuffer {
  let bytes = bindings.loadWitness(index, source);
  return bytes;
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
 * const witnessArgs = loadWitnessArgs(0, SOURCE_INPUT);
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
  return WitnessArgs.fromBytes(bytes);
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
  return Transaction.fromBytes(bytes);
}

/**
 * Load transaction hash
 *
 * @returns The hash of the current transaction
 * @throws Error if there's a system error
 */
export function loadTxHash(): Bytes {
  let bytes = bindings.loadTxHash();
  return bytes;
}

/**
 * Load current script hash
 *
 * @returns The hash of the current script
 * @throws Error if there's a system error
 */
export function loadScriptHash(): Bytes {
  let bytes = bindings.loadScriptHash();
  return bytes;
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
 * const capacity = loadCellCapacity(0, SOURCE_INPUT);
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
  return numFromBytes(bytes);
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
 * const occupiedCapacity = loadCellOccupiedCapacity(0, SOURCE_INPUT);
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
  return numFromBytes(bytes);
}

/**
 * Load cell data
 *
 * @param index - The index of the cell to load
 * @param source - The source to load the cell from
 * @returns The data of the cell
 */
export function loadCellData(
  index: number,
  source: bindings.SourceType,
): ArrayBuffer {
  let bytes = bindings.loadCellData(index, source);
  return bytes;
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
 * const typeHash = loadCellTypeHash(0, SOURCE_INPUT);
 * if (typeHash !== null) {
 *   // Cell has a type script
 * }
 * ```
 */
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
    return bytes;
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
 * const lock = loadCellLock(0, SOURCE_INPUT);
 * ```
 */
export function loadCellLock(
  index: number,
  source: bindings.SourceType,
): Script {
  let bytes = bindings.loadCellByField(index, source, bindings.CELL_FIELD_LOCK);
  return Script.fromBytes(bytes);
}
/**
 * Load cell lock hash
 *
 * @param index - The index of the cell
 * @param source - The source to load the cell from
 * @returns The lock script hash of the cell
 * @throws Error if there's a system error or if the cell cannot be found
 *
 * @example
 * ```typescript
 * const lockHash = loadCellLockHash(0, SOURCE_INPUT);
 * ```
 */
export function loadCellLockHash(
  index: number,
  source: bindings.SourceType,
): Bytes {
  let bytes = bindings.loadCellByField(
    index,
    source,
    bindings.CELL_FIELD_LOCK_HASH,
  );
  return bytes;
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
 * const type = loadCellType(0, SOURCE_INPUT);
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
    return Script.fromBytes(bytes);
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
 * const epochNumber = loadHeaderEpochNumber(0, SOURCE_INPUT);
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
  return numFromBytes(bytes);
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
 * const epochStartBlockNumber = loadHeaderEpochStartBlockNumber(0, SOURCE_INPUT);
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
  return numFromBytes(bytes);
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
 * const epochLength = loadHeaderEpochLength(0, SOURCE_INPUT);
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
  return numFromBytes(bytes);
}

/**
 * Load header.
 *
 * @param index - The index of the header to load
 * @param source - The source to load the header from
 * @returns The header
 * @throws Error if there's a system error or if the header cannot be found
 *
 * @example
 * ```typescript
 * const header = loadHeader(0, bindings.SOURCE_HEADER_DEP);
 * ```
 */
export function loadHeader(index: number, source: bindings.SourceType): Header {
  let bytes = bindings.loadHeader(index, source);
  return Header.decode(bytes);
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
 * const since = loadInputSince(0, SOURCE_INPUT);
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
  return numFromBytes(bytes);
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
 * const outPoint = loadInputOutPoint(0, SOURCE_INPUT);
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
  return OutPoint.fromBytes(bytes);
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
  return Script.fromBytes(bytes);
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
 * const iter = new QueryIter(loadCellCapacity, SOURCE_INPUT);
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
  dataHash: Bytes,
  source: bindings.SourceType,
): number | null {
  if (dataHash.byteLength !== 32) {
    throw new Error("dataHash must be 32 bytes");
  }

  for (let i = 0; ; i++) {
    try {
      const hash = bindings.loadCellByField(
        i,
        source,
        bindings.CELL_FIELD_DATA_HASH,
      );
      if (bytesEq(hash, dataHash)) {
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
export function lookForDepWithHash2(codeHash: Bytes, hashType: number): number {
  if (codeHash.byteLength !== 32) {
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
      if (bytesEq(hash, codeHash)) {
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
export function lookForDepWithDataHash(dataHash: Bytes): number {
  return lookForDepWithHash2(dataHash, bindings.SCRIPT_HASH_TYPE_DATA);
}
