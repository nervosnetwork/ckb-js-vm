import { Bytes, BytesLike } from "../bytes/index";
import { mol } from "../molecule/index";

export const HashTypeCodec: mol.Codec<HashTypeLike, HashType> = mol.Codec.from({
  byteLength: 1,
  encode: hashTypeToBytes,
  decode: hashTypeFromBytes,
});

/**
 * @public
 */
export type HashTypeLike = number;
/**
 * @public
 */
export type HashType = 0x01 | 0x00 | 0x02 | 0x04;

export function hashTypeFrom(val: HashTypeLike): HashType {
  if (val !== 0x01 && val !== 0x00 && val !== 0x02 && val !== 0x04) {
    throw new Error(`Invalid hash type ${val}`);
  }
  return val as HashType;
}

/**
 * Converts a HashTypeLike value to its corresponding byte representation.
 * @public
 *
 * @param hashType - The hash type value to convert.
 * @returns A Uint8Array containing the byte representation of the hash type.
 *
 * @example
 * ```typescript
 * const hashTypeBytes = hashTypeToBytes("type"); // Outputs Uint8Array [0]
 * ```
 */

export function hashTypeToBytes(hashType: HashTypeLike): Bytes {
  return new Uint8Array([hashTypeFrom(hashType)]).buffer;
}

/**
 * Converts a byte-like value to a HashType.
 * @public
 *
 * @param bytes - The byte-like value to convert.
 * @returns The corresponding HashType.
 *
 * @throws Will throw an error if the input bytes do not correspond to a valid hash type.
 *
 * @example
 * ```typescript
 * const hashType = hashTypeFromBytes(new Uint8Array([0])); // Outputs "type"
 * ```
 */

export function hashTypeFromBytes(bytes: BytesLike): HashType {
  return hashTypeFrom(new Uint8Array(bytes)[0]);
}

/**
 * @public
 */
export type ScriptLike = {
  codeHash: BytesLike;
  hashType: HashTypeLike;
  args: BytesLike;
};
/**
 * @public
 */
@mol.codec(
  mol.table({
    codeHash: mol.Byte32,
    hashType: HashTypeCodec,
    args: mol.Bytes,
  }),
)
export class Script extends mol.Entity.Base<ScriptLike, Script>() {
  /**
   * Creates an instance of Script.
   *
   * @param codeHash - The code hash of the script.
   * @param hashType - The hash type of the script.
   * @param args - The arguments for the script.
   */
  constructor(
    public codeHash: Bytes,
    public hashType: HashType,
    public args: Bytes,
  ) {
    super();
  }

  get occupiedSize(): number {
    return 33 + this.args.byteLength;
  }

  /**
   * Clone a script.
   *
   * @returns A cloned Script instance.
   *
   * @example
   * ```typescript
   * const script1 = script0.clone();
   * ```
   */
  clone(): Script {
    return new Script(this.codeHash, this.hashType, this.args);
  }

  /**
   * Creates a Script instance from a ScriptLike object.
   *
   * @param script - A ScriptLike object or an instance of Script.
   * @returns A Script instance.
   *
   * @example
   * ```typescript
   * const script = Script.from({
   *   codeHash: "0x1234...",
   *   hashType: "type",
   *   args: "0xabcd..."
   * });
   * ```
   */

  static from(script: ScriptLike): Script {
    if (script instanceof Script) {
      return script;
    }

    return new Script(
      script.codeHash,
      hashTypeFrom(script.hashType),
      script.args,
    );
  }
}

export const ScriptOpt = mol.option(Script);
export const ScriptVec = mol.vector(Script);
