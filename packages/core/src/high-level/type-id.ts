/**
 * Implementation of Type ID validation for CKB transactions
 *
 * @module TypeID
 *
 * @remarks
 * This module provides functionality for validating and checking Type IDs in CKB transactions.
 *
 * For more details, see the {@link https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0022-transaction-structure/0022-transaction-structure.md#type-id | Type ID RFC}.
 *
 * Type ID cells are allowed to be burned.
 */

import {
  SourceType,
  SOURCE_OUTPUT,
  SOURCE_INPUT,
  SOURCE_GROUP_INPUT,
  SOURCE_GROUP_OUTPUT,
  CELL_FIELD_CAPACITY,
  loadScriptHash,
  loadCellByField,
} from "@ckb-js-std/bindings";
import { hashTypeId } from "../hasher/hasherCkb";
import {
  loadCellTypeHash,
  loadInput,
  loadScript,
  QueryIter,
} from "./high-level";
import { bytesEq } from "../bytes";

/**
 * Checks if a cell exists at the specified index and source
 * @internal
 */
function isCellPresent(index: number, source: SourceType): boolean {
  try {
    // capacity is the smallest field in a cell, better performance.
    loadCellByField(index, source, CELL_FIELD_CAPACITY);
    return true;
  } catch (_) {
    return false;
  }
}

/**
 * Locates the index of the current script in transaction outputs
 * @internal
 */
function locateIndex(): number {
  const hash = loadScriptHash();

  const query = new QueryIter<ArrayBuffer | null>(
    (i, s) => loadCellTypeHash(i, s),
    SOURCE_OUTPUT,
  );

  const index = query
    .toArray()
    .findIndex(
      (typeHash) =>
        typeHash && bytesEq(new Uint8Array(typeHash), new Uint8Array(hash)),
    );

  if (index === -1) throw new Error("TypeIDError");
  return index;
}

/**
 * Validates the Type ID in a flexible manner
 *
 * @param typeId - 32-byte ArrayBuffer representing the Type ID to validate
 *
 * @throws Error
 * Throws with TypeIDError code if validation fails
 *
 * @remarks
 * Performs low-level validation of Type ID. Handles three cases:
 * 1. Minting operation (0 input cells, 1 output cell)
 * 2. Transfer operation (1 input cell, 1 output cell)
 * 3. Burning operation (1 input cell, 0 output cells - allowed)
 *
 * @example
 * ```typescript
 * import { HighLevel } from "@ckb-js-std/core";
 *
 * try {
 *   HighLevel.validateTypeId(typeId);
 * } catch (error) {
 *   // Handle validation error
 * }
 * ```
 */
export function validateTypeId(typeId: ArrayBuffer): void {
  if (
    isCellPresent(1, SOURCE_GROUP_INPUT) ||
    isCellPresent(1, SOURCE_GROUP_OUTPUT)
  ) {
    throw new Error("validateTypeId error");
  }

  if (!isCellPresent(0, SOURCE_GROUP_INPUT)) {
    const index = locateIndex();
    const input = loadInput(0, SOURCE_INPUT);

    const calculatedId = hashTypeId(input, index);

    if (!bytesEq(calculatedId, typeId)) {
      throw new Error("TypeIDError");
    }
  }
}

/**
 * Loads Type ID from script arguments
 * @internal
 */
function loadIdFromArgs(offset: number): ArrayBuffer {
  const script = loadScript();
  const args = script.args;

  if (offset + 32 > args.byteLength) {
    throw new Error("loadIdFromArgs error");
  }

  return args.slice(offset, offset + 32);
}

/**
 * Validates that the script follows the Type ID rule
 *
 * @param offset - Byte offset in script args where Type ID starts
 *
 * @throws Error
 * Throws with TypeIDError code if validation fails or ID cannot be retrieved
 *
 * @remarks
 * Checks if the Type ID (32-byte value) stored in script args at specified offset
 * is valid according to Type ID rules. Internally uses {@link validateTypeId}.
 *
 * @example
 * ```typescript
 * import { HighLevel } from "@ckb-js-std/core";
 *
 * function main() {
 *   try {
 *     HighLevel.checkTypeId(0); // Check Type ID at start of script args
 *   } catch (error) {
 *     // Handle validation error
 *   }
 * }
 * ```
 */
export function checkTypeId(offset: number): void {
  const typeId = loadIdFromArgs(offset);
  validateTypeId(typeId);
}
