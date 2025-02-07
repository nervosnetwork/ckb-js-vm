import { Bytes, BytesLike, bytesFrom } from "../bytes/index";
import { mol } from "../molecule/index";
import { Num, NumLike, numFromBytes, numToBytes } from "../num/index";
import { apply } from "../utils/index";
import { Script, ScriptLike, ScriptOpt } from "./script";
import { hashCkb, Hasher, HasherCkb } from "../hasher";

/**
 * @public
 */
export type OutPointLike = {
  txHash: BytesLike;
  index: NumLike;
};
/**
 * @public
 */
@mol.codec(
  mol.struct({
    txHash: mol.Byte32,
    index: mol.Uint32,
  }),
)
export class OutPoint extends mol.Entity.Base<OutPointLike, OutPoint>() {
  /**
   * Creates an instance of OutPoint.
   *
   * @param txHash - The transaction hash.
   * @param index - The index of the output in the transaction.
   */

  constructor(
    public txHash: Bytes,
    public index: Num,
  ) {
    super();
  }

  /**
   * Creates an OutPoint instance from an OutPointLike object.
   *
   * @param outPoint - An OutPointLike object or an instance of OutPoint.
   * @returns An OutPoint instance.
   *
   * @example
   * ```typescript
   * const outPoint = OutPoint.from({ txHash: "0x...", index: 0 });
   * ```
   */
  static from(outPoint: OutPointLike): OutPoint {
    if (outPoint instanceof OutPoint) {
      return outPoint;
    }
    return new OutPoint(bytesFrom(outPoint.txHash), outPoint.index);
  }
}

/**
 * @public
 */
export type CellOutputLike = {
  capacity: NumLike;
  lock: ScriptLike;
  type?: ScriptLike | null;
};
/**
 * @public
 */
@mol.codec(
  mol.table({
    capacity: mol.Uint64,
    lock: Script,
    type: ScriptOpt,
  }),
)
export class CellOutput extends mol.Entity.Base<CellOutputLike, CellOutput>() {
  /**
   * Creates an instance of CellOutput.
   *
   * @param capacity - The capacity of the cell.
   * @param lock - The lock script of the cell.
   * @param type - The optional type script of the cell.
   */

  constructor(
    public capacity: Num,
    public lock: Script,
    public type?: Script,
  ) {
    super();
  }

  get occupiedSize(): number {
    return 8 + this.lock.occupiedSize + (this.type?.occupiedSize ?? 0);
  }

  /**
   * Creates a CellOutput instance from a CellOutputLike object.
   *
   * @param cellOutput - A CellOutputLike object or an instance of CellOutput.
   * @returns A CellOutput instance.
   *
   * @example
   * ```typescript
   * const cellOutput = CellOutput.from({
   *   capacity: 1000n,
   *   lock: { codeHash: "0x...", hashType: "type", args: "0x..." },
   *   type: { codeHash: "0x...", hashType: "type", args: "0x..." }
   * });
   * ```
   */
  static from(cellOutput: CellOutputLike): CellOutput {
    if (cellOutput instanceof CellOutput) {
      return cellOutput;
    }

    return new CellOutput(
      cellOutput.capacity,
      Script.from(cellOutput.lock),
      apply(Script.from, cellOutput.type),
    );
  }
}
export const CellOutputVec = mol.vector(CellOutput);

/**
 * @public
 */
export type CellLike = {
  outPoint: OutPointLike;
  cellOutput: CellOutputLike;
  outputData: BytesLike;
};
/**
 * @public
 */
export class Cell {
  /**
   * Creates an instance of Cell.
   *
   * @param outPoint - The output point of the cell.
   * @param cellOutput - The cell output of the cell.
   * @param outputData - The output data of the cell.
   */

  constructor(
    public outPoint: OutPoint,
    public cellOutput: CellOutput,
    public outputData: BytesLike,
  ) {}

  /**
   * Creates a Cell instance from a CellLike object.
   *
   * @param cell - A CellLike object or an instance of Cell.
   * @returns A Cell instance.
   */

  static from(cell: CellLike): Cell {
    if (cell instanceof Cell) {
      return cell;
    }

    return new Cell(
      OutPoint.from(cell.outPoint),
      CellOutput.from(cell.cellOutput),
      bytesFrom(cell.outputData),
    );
  }

  /**
   * Clone a Cell
   *
   * @returns A cloned Cell instance.
   *
   * @example
   * ```typescript
   * const cell1 = cell0.clone();
   * ```
   */
  clone(): Cell {
    return new Cell(
      this.outPoint.clone(),
      this.cellOutput.clone(),
      this.outputData,
    );
  }
}

// TODO: since

/**
 * @public
 */
export type CellInputLike = {
  previousOutput: OutPointLike;
  since?: NumLike | null;
  cellOutput?: CellOutputLike | null;
  outputData?: BytesLike | null;
};
/**
 * @public
 */
@mol.codec(
  mol
    .struct({
      since: mol.Uint64,
      previousOutput: OutPoint,
    })
    .mapIn((encodable: CellInputLike) => ({
      ...encodable,
      since: encodable.since ?? 0,
    })),
)
export class CellInput extends mol.Entity.Base<CellInputLike, CellInput>() {
  /**
   * Creates an instance of CellInput.
   *
   * @param previousOutput - The previous outpoint of the cell.
   * @param since - The since value of the cell input.
   * @param cellOutput - The optional cell output associated with the cell input.
   * @param outputData - The optional output data associated with the cell input.
   */

  constructor(
    public previousOutput: OutPoint,
    public since: Num,
    public cellOutput?: CellOutput,
    public outputData?: BytesLike,
  ) {
    super();
  }

  /**
   * Creates a CellInput instance from a CellInputLike object.
   *
   * @param cellInput - A CellInputLike object or an instance of CellInput.
   * @returns A CellInput instance.
   *
   * @example
   * ```typescript
   * const cellInput = CellInput.from({
   *   previousOutput: { txHash: "0x...", index: 0 },
   *   since: 0n
   * });
   * ```
   */
  static from(cellInput: CellInputLike): CellInput {
    if (cellInput instanceof CellInput) {
      return cellInput;
    }

    return new CellInput(
      OutPoint.from(cellInput.previousOutput),
      cellInput.since ?? 0,
      apply(CellOutput.from, cellInput.cellOutput),
      apply(bytesFrom, cellInput.outputData),
    );
  }
}
export const CellInputVec = mol.vector(CellInput);

/**
 * @public
 */
export type CellDepLike = {
  outPoint: OutPointLike;
  depType: number;
};
/**
 * @public
 */
@mol.codec(
  mol.struct({
    outPoint: OutPoint,
    depType: mol.Uint8,
  }),
)
export class CellDep extends mol.Entity.Base<CellDepLike, CellDep>() {
  /**
   * Creates an instance of CellDep.
   *
   * @param outPoint - The outpoint of the cell dependency.
   * @param depType - The dependency type.
   */

  constructor(
    public outPoint: OutPoint,
    public depType: number,
  ) {
    super();
  }

  /**
   * Clone a CellDep.
   *
   * @returns A cloned CellDep instance.
   *
   * @example
   * ```typescript
   * const cellDep1 = cellDep0.clone();
   * ```
   */

  clone(): CellDep {
    return new CellDep(this.outPoint.clone(), this.depType);
  }

  /**
   * Creates a CellDep instance from a CellDepLike object.
   *
   * @param cellDep - A CellDepLike object or an instance of CellDep.
   * @returns A CellDep instance.
   *
   * @example
   * ```typescript
   * const cellDep = CellDep.from({
   *   outPoint: { txHash: "0x...", index: 0 },
   *   depType: "depGroup"
   * });
   * ```
   */

  static from(cellDep: CellDepLike): CellDep {
    if (cellDep instanceof CellDep) {
      return cellDep;
    }

    return new CellDep(OutPoint.from(cellDep.outPoint), cellDep.depType);
  }
}
export const CellDepVec = mol.vector(CellDep);

/**
 * @public
 */
export type WitnessArgsLike = {
  lock?: BytesLike | null;
  inputType?: BytesLike | null;
  outputType?: BytesLike | null;
};
/**
 * @public
 */
@mol.codec(
  mol.table({
    lock: mol.BytesOpt,
    inputType: mol.BytesOpt,
    outputType: mol.BytesOpt,
  }),
)
export class WitnessArgs extends mol.Entity.Base<
  WitnessArgsLike,
  WitnessArgs
>() {
  /**
   * Creates an instance of WitnessArgs.
   *
   * @param lock - The optional lock field of the witness.
   * @param inputType - The optional input type field of the witness.
   * @param outputType - The optional output type field of the witness.
   */

  constructor(
    public lock?: Bytes,
    public inputType?: Bytes,
    public outputType?: Bytes,
  ) {
    super();
  }

  /**
   * Creates a WitnessArgs instance from a WitnessArgsLike object.
   *
   * @param witnessArgs - A WitnessArgsLike object or an instance of WitnessArgs.
   * @returns A WitnessArgs instance.
   *
   * @example
   * ```typescript
   * const witnessArgs = WitnessArgs.from({
   *   lock: "0x...",
   *   inputType: "0x...",
   *   outputType: "0x..."
   * });
   * ```
   */

  static from(witnessArgs: WitnessArgsLike): WitnessArgs {
    if (witnessArgs instanceof WitnessArgs) {
      return witnessArgs;
    }

    return new WitnessArgs(
      apply(bytesFrom, witnessArgs.lock),
      apply(bytesFrom, witnessArgs.inputType),
      apply(bytesFrom, witnessArgs.outputType),
    );
  }
}

/**
 * @public
 */
export function udtBalanceFrom(dataLike: BytesLike): Num {
  const data = bytesFrom(dataLike).slice(0, 16);
  return numFromBytes(data);
}

export const RawTransaction = mol.table({
  version: mol.Uint32,
  cellDeps: CellDepVec,
  headerDeps: mol.Byte32Vec,
  inputs: CellInputVec,
  outputs: CellOutputVec,
  outputsData: mol.BytesVec,
});

/**
 * @public
 */
export type TransactionLike = {
  version?: NumLike | null;
  cellDeps?: CellDepLike[] | null;
  headerDeps?: BytesLike[] | null;
  inputs?: CellInputLike[] | null;
  outputs?:
    | (Omit<CellOutputLike, "capacity"> &
        Partial<Pick<CellOutputLike, "capacity">>)[]
    | null;
  outputsData?: BytesLike[] | null;
  witnesses?: BytesLike[] | null;
};
/**
 * @public
 */
@mol.codec(
  mol
    .table({
      raw: RawTransaction,
      witnesses: mol.BytesVec,
    })
    .mapIn((txLike: TransactionLike) => {
      const tx = Transaction.from(txLike);
      return {
        raw: tx,
        witnesses: tx.witnesses,
      };
    })
    .mapOut((tx) => Transaction.from({ ...tx.raw, witnesses: tx.witnesses })),
)
export class Transaction extends mol.Entity.Base<
  TransactionLike,
  Transaction
>() {
  /**
   * Creates an instance of Transaction.
   *
   * @param version - The version of the transaction.
   * @param cellDeps - The cell dependencies of the transaction.
   * @param headerDeps - The header dependencies of the transaction.
   * @param inputs - The inputs of the transaction.
   * @param outputs - The outputs of the transaction.
   * @param outputsData - The data associated with the outputs.
   * @param witnesses - The witnesses of the transaction.
   */

  constructor(
    public version: Num,
    public cellDeps: CellDep[],
    public headerDeps: Bytes[],
    public inputs: CellInput[],
    public outputs: CellOutput[],
    public outputsData: Bytes[],
    public witnesses: Bytes[],
  ) {
    super();
  }

  /**
   * Creates a default Transaction instance with empty fields.
   *
   * @returns A default Transaction instance.
   *
   * @example
   * ```typescript
   * const defaultTx = Transaction.default();
   * ```
   */
  static default(): Transaction {
    return new Transaction(0, [], [], [], [], [], []);
  }

  /**
   * Copy every properties from another transaction.
   *
   * @example
   * ```typescript
   * this.copy(Transaction.default());
   * ```
   */
  copy(txLike: TransactionLike) {
    const tx = Transaction.from(txLike);
    this.version = tx.version;
    this.cellDeps = tx.cellDeps;
    this.headerDeps = tx.headerDeps;
    this.inputs = tx.inputs;
    this.outputs = tx.outputs;
    this.outputsData = tx.outputsData;
    this.witnesses = tx.witnesses;
  }

  /**
   * Creates a Transaction instance from a TransactionLike object.
   *
   * @param tx - A TransactionLike object or an instance of Transaction.
   * @returns A Transaction instance.
   *
   * @example
   * ```typescript
   * const transaction = Transaction.from({
   *   version: 0,
   *   cellDeps: [],
   *   headerDeps: [],
   *   inputs: [],
   *   outputs: [],
   *   outputsData: [],
   *   witnesses: []
   * });
   * ```
   */

  static from(tx: TransactionLike): Transaction {
    if (tx instanceof Transaction) {
      return tx;
    }
    const outputs =
      tx.outputs?.map((output, i) => {
        const o = CellOutput.from({
          ...output,
          capacity: output.capacity ?? 0,
        });
        if (o.capacity === 0) {
          o.capacity =
            o.occupiedSize +
            (apply(bytesFrom, tx.outputsData?.[i])?.length ?? 0);
        }
        return o;
      }) ?? [];
    const outputsData = outputs.map((_, i) =>
      bytesFrom(tx.outputsData?.[i] ?? new Uint8Array(0)),
    );
    if (tx.outputsData != null && outputsData.length < tx.outputsData.length) {
      outputsData.push(
        ...tx.outputsData.slice(outputsData.length).map((d) => bytesFrom(d)),
      );
    }

    return new Transaction(
      tx.version ?? 0,
      tx.cellDeps?.map((cellDep) => CellDep.from(cellDep)) ?? [],
      tx.headerDeps?.map(bytesFrom) ?? [],
      tx.inputs?.map((input) => CellInput.from(input)) ?? [],
      outputs,
      outputsData,
      tx.witnesses?.map(bytesFrom) ?? [],
    );
  }

  stringify(): string {
    return JSON.stringify(this, (_, value) => {
      if (typeof value === "bigint") {
        return numToBytes(Number(value), 8);
      }
      // eslint-disable-next-line @typescript-eslint/no-unsafe-return
      return value;
    });
  }

  /**
   * Converts the raw transaction data to bytes.
   *
   * @returns A Uint8Array containing the raw transaction bytes.
   *
   * @example
   * ```typescript
   * const rawTxBytes = transaction.rawToBytes();
   * ```
   */
  rawToBytes(): Bytes {
    return RawTransaction.encode(this);
  }

  /**
   * Calculates the hash of the transaction without witnesses. This is the transaction hash in the usual sense.
   * To calculate the hash of the whole transaction including the witnesses, use transaction.hashFull() instead.
   *
   * @returns The hash of the transaction.
   *
   * @example
   * ```typescript
   * const txHash = transaction.hash();
   * ```
   */
  hash(): Bytes {
    return hashCkb(this.rawToBytes());
  }

  /**
   * Calculates the hash of the transaction with witnesses.
   *
   * @returns The hash of the transaction with witnesses.
   *
   * @example
   * ```typescript
   * const txFullHash = transaction.hashFull();
   * ```
   */
  hashFull(): Bytes {
    return hashCkb(this.toBytes());
  }

  /**
   * Hashes a witness and updates the hasher.
   *
   * @param witness - The witness to hash.
   * @param hasher - The hasher instance to update.
   *
   * @example
   * ```typescript
   * Transaction.hashWitnessToHasher("0x...", hasher);
   * ```
   */
  static hashWitnessToHasher(witness: BytesLike, hasher: Hasher) {
    const raw = bytesFrom(witness);
    hasher.update(numToBytes(raw.length, 8));
    hasher.update(raw);
  }

  /**
   * Computes the signing hash information for a given script.
   *
   * @param scriptLike - The script associated with the transaction, represented as a ScriptLike object.
   * @returns A promise that resolves to an object containing the signing message and the witness position,
   *          or undefined if no matching input is found.
   *
   * @example
   * ```typescript
   * const signHashInfo = tx.getSignHashInfo(scriptLike, client);
   * if (signHashInfo) {
   *   console.log(signHashInfo.message); // Outputs the signing message
   *   console.log(signHashInfo.position); // Outputs the witness position
   * }
   * ```
   */
  getSignHashInfo(
    scriptLike: ScriptLike,
    hasher: Hasher = new HasherCkb(),
  ): { message: Bytes; position: number } | undefined {
    const script = Script.from(scriptLike);
    let position = -1;
    hasher.update(this.hash());

    for (let i = 0; i < this.witnesses.length; i += 1) {
      const input = this.inputs[i];
      if (input) {
        // input.completeExtraInfos(client);

        if (!input.cellOutput) {
          throw new Error("Unable to complete input");
        }

        if (!script.eq(input.cellOutput.lock)) {
          continue;
        }

        if (position === -1) {
          position = i;
        }
      }

      if (position === -1) {
        return undefined;
      }

      Transaction.hashWitnessToHasher(this.witnesses[i], hasher);
    }

    if (position === -1) {
      return undefined;
    }

    return {
      message: hasher.digest(),
      position,
    };
  }
  /**
   * Get witness at index as WitnessArgs
   *
   * @param index - The index of the witness.
   * @returns The witness parsed as WitnessArgs.
   *
   * @example
   * ```typescript
   * const witnessArgs = tx.getWitnessArgsAt(0);
   * ```
   */
  getWitnessArgsAt(index: number): WitnessArgs | undefined {
    const rawWitness = this.witnesses[index];
    return rawWitness.length > 0
      ? WitnessArgs.fromBytes(rawWitness)
      : undefined;
  }

  /**
   * Set witness at index by WitnessArgs
   *
   * @param index - The index of the witness.
   * @param witness - The WitnessArgs to set.
   *
   * @example
   * ```typescript
   * tx.setWitnessArgsAt(0, witnessArgs);
   * ```
   */
  setWitnessArgsAt(index: number, witness: WitnessArgs): void {
    if (this.witnesses.length < index) {
      this.witnesses.push(
        ...Array.from(
          new Array(index - this.witnesses.length),
          (): Bytes => new Uint8Array(0),
        ),
      );
    }

    this.witnesses[index] = witness.toBytes();
  }

  findInputIndexByLock(scriptIdLike: ScriptLike): number | undefined {
    const script = Script.from(scriptIdLike);

    for (let i = 0; i < this.inputs.length; i += 1) {
      const input = this.inputs[i];
      if (!input.cellOutput) {
        throw new Error("Unable to complete input");
      }

      if (
        script.codeHash === input.cellOutput.lock.codeHash &&
        script.hashType === input.cellOutput.lock.hashType
      ) {
        return i;
      }
    }
    return undefined;
  }

  /**
   * Prepare dummy witness for sighash all method
   *
   * @param scriptLike - The script associated with the transaction, represented as a ScriptLike object.
   * @param lockLen - The length of dummy lock bytes.
   * @returns A promise that resolves to the prepared transaction
   *
   * @example
   * ```typescript
   * tx.prepareSighashAllWitness(scriptLike, 85, client);
   * ```
   */
  prepareSighashAllWitness(scriptLike: ScriptLike, lockLen: number): void {
    const position = this.findInputIndexByLock(scriptLike);
    if (position === undefined) {
      return;
    }

    const witness = this.getWitnessArgsAt(position) ?? WitnessArgs.from({});
    witness.lock = new Uint8Array(lockLen);
    this.setWitnessArgsAt(position, witness);
  }
}
