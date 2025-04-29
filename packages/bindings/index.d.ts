type SourceType = number | bigint;

/**
 * Source constants for loading cells/inputs/headers
 * Used as parameters in load functions to specify data source
 */
export const SOURCE_CELL_DEP: SourceType;
export const SOURCE_HEADER_DEP: SourceType;
export const SOURCE_INPUT: SourceType;
export const SOURCE_OUTPUT: SourceType;
export const SOURCE_GROUP_INPUT: SourceType;
export const SOURCE_GROUP_OUTPUT: SourceType;

/**
 * Field constants for loading cell data
 * Used to specify which field to load when using `loadCellByField`
 */
export const CELL_FIELD_CAPACITY: number;
export const CELL_FIELD_DATA_HASH: number;
export const CELL_FIELD_LOCK: number;
export const CELL_FIELD_LOCK_HASH: number;
export const CELL_FIELD_TYPE: number;
export const CELL_FIELD_TYPE_HASH: number;
export const CELL_FIELD_OCCUPIED_CAPACITY: number;

/**
 * Field constants for loading header data
 * Used to specify which field to load when using loadHeaderByField
 */
export const HEADER_FIELD_EPOCH_NUMBER: number;
export const HEADER_FIELD_EPOCH_START_BLOCK_NUMBER: number;
export const HEADER_FIELD_EPOCH_LENGTH: number;

/**
 * Field constants for loading input data
 * Used to specify which field to load when using loadInputByField
 */
export const INPUT_FIELD_OUT_POINT: number;
export const INPUT_FIELD_SINCE: number;

/**
 * Script hash type constants
 * Used to specify the hash type when executing or spawning cells
 */
export const SCRIPT_HASH_TYPE_DATA: number;
export const SCRIPT_HASH_TYPE_TYPE: number;
export const SCRIPT_HASH_TYPE_DATA1: number;
export const SCRIPT_HASH_TYPE_DATA2: number;

export const INDEX_OUT_OF_BOUND: number;
export const ITEM_MISSING: number;
export const LENGTH_NOT_ENOUGH: number;
export const INVALID_DATA: number;
export const WAIT_FAILURE: number;
export const INVALID_FD: number;
export const OTHER_END_CLOSED: number;
export const MAX_VMS_SPAWNED: number;
export const MAX_FDS_CREATED: number;

/**
 * Exit the current script with a status code
 * @param status - The exit status code
 */
export function exit(status: number): void;

/**
 * Load script
 * @param offset - Optional starting offset in the data
 * @param length - Optional length of data to load
 * @returns The loaded script as ArrayBuffer
 */
export function loadScript(offset?: number, length?: number): ArrayBuffer;

/**
 * Load transaction
 * @param offset - Optional starting offset in the transaction (defaults to 0)
 * @param length - Optional length of transaction to load (defaults to reading until the end)
 * @returns The loaded transaction as ArrayBuffer
 */
export function loadTransaction(offset?: number, length?: number): ArrayBuffer;

/**
 * Load transaction hash
 * @returns The loaded transaction hash as ArrayBuffer
 */
export function loadTxHash(): ArrayBuffer;

/**
 * Load script hash
 * @returns The loaded script hash as ArrayBuffer
 */
export function loadScriptHash(): ArrayBuffer;

/**
 * Load script
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded script as ArrayBuffer
 */
export function loadScript(offset?: number, length?: number): ArrayBuffer;

/**
 * Load cell data from the transaction
 * @param index - The index of the cell
 * @param source - The source of the cell (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded cell data as ArrayBuffer
 */
export function loadCell(
  index: number,
  source: SourceType,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Load input data from the transaction
 * @param index - The index of the input
 * @param source - The source of the input (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded input data as ArrayBuffer
 */
export function loadInput(
  index: number,
  source: SourceType,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Load header data from the transaction
 * @param index - The index of the header
 * @param source - The source of the header (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded header data as ArrayBuffer
 */
export function loadHeader(
  index: number,
  source: SourceType,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Load witness data from the transaction
 * @param index - The index of the witness
 * @param source - The source of the witness (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded witness data as ArrayBuffer
 */
export function loadWitness(
  index: number,
  source: SourceType,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Load cell data from the transaction
 * @param index - The index of the cell
 * @param source - The source of the cell (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded cell data as ArrayBuffer
 */
export function loadCellData(
  index: number,
  source: SourceType,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Load cell data by specific field
 * @param index - The index of the cell
 * @param source - The source of the cell (use SOURCE_* constants)
 * @param field - The field to load (use CELL_FIELD_* constants)
 * @param offset - Optional starting offset in the field data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded field data as ArrayBuffer
 */
export function loadCellByField(
  index: number,
  source: SourceType,
  field: number,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Load header data by specific field
 * @param index - The index of the header
 * @param source - The source of the header (use SOURCE_* constants)
 * @param field - The field to load (use HEADER_FIELD_* constants)
 * @param offset - Optional starting offset in the field data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded field data as ArrayBuffer
 */
export function loadHeaderByField(
  index: number,
  source: SourceType,
  field: number,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Load input data by specific field
 * @param index - The index of the input
 * @param source - The source of the input (use SOURCE_* constants)
 * @param field - The field to load (use INPUT_FIELD_* constants)
 * @param offset - Optional starting offset in the field data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded field data as ArrayBuffer
 */
export function loadInputByField(
  index: number,
  source: SourceType,
  field: number,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Get the current VM version
 * @returns The VM version number
 */
export function vmVersion(): number;

/**
 * Get the current cycle count
 * @returns The current cycle count
 */
export function currentCycles(): number;

/**
 * Execute a cell with the given parameters
 * @param codeHash - The code hash of the cell to execute
 * @param hashType - The hash type of the code (use SCRIPT_HASH_TYPE_* constants)
 * @param offset - The offset in the cell data (defaults to 0)
 * @param length - The length of code to execute (defaults to reading until the end)
 * @param args - Additional arguments to pass to the cell
 */
export function execCell(
  codeHash: ArrayBuffer,
  hashType: number,
  offset: number,
  length: number,
  ...args: string[]
): void;

/**
 * Options for spawning a new cell process
 */
export interface SpawnArgs {
  /** Command line arguments to pass to the spawned process */
  argv?: string[];
  /** File descriptors to inherit in the spawned process */
  inheritedFds?: number[];

  /**
   * Indicates that the cell should be loaded from witness instead of the default location.
   * Only used by `spawn()` function; has no effect when using `spawnCell()`.
   * @default false
   */
  fromWitness?: boolean;
}

/**
 * Spawn a new process from a cell
 * @param codeHash - The code hash of the cell to spawn
 * @param hashType - The hash type of the code (use SCRIPT_HASH_TYPE_* constants)
 * @param offset - The offset in the cell data (defaults to 0)
 * @param length - The length of code to execute (defaults to reading until the end)
 * @param args - Spawn arguments including argv and inherited file descriptors
 * @returns The process ID of the spawned process
 */
export function spawnCell(
  codeHash: ArrayBuffer,
  hashType: number,
  offset: number,
  length: number,
  args: SpawnArgs,
): number;

/**
 * Spawn a new process from a specified source
 * @param index - The index of the source to spawn from
 * @param source - The type of source (use SOURCE_* constants)
 * @param offset - The offset in the source data (defaults to 0)
 * @param length - The length of code to execute (defaults to reading until the end)
 * @param args - Spawn arguments including argv and inherited file descriptors
 * @returns The process ID of the spawned process
 */
export function spawn(
  index: number,
  source: SourceType,
  offset: number,
  length: number,
  args: SpawnArgs,
): number;

/**
 * Create a new pipe
 * @returns A tuple of [read_fd, write_fd] for the pipe
 */
export function pipe(): [number, number];

/**
 * Get inherited file descriptors
 * @returns Array of inherited file descriptor numbers
 */
export function inheritedFds(): number[];

/**
 * Read from a file descriptor
 * @param fd - The file descriptor to read from
 * @param length - Number of bytes to read
 * @returns The read data as ArrayBuffer
 * @remark
 * The length of returned data might be smaller than `length`.
 */
export function read(fd: number, length: number): ArrayBuffer;

/**
 * Write data to a file descriptor
 * @param fd - The file descriptor to write to
 * @param data - The data to write as ArrayBuffer
 * @remarks
 * - All data will be written atomically - no partial writes will occur
 * - If the write cannot complete fully, it will throw an error
 * - The function blocks until all data is written
 */
export function write(fd: number, data: ArrayBuffer): void;

/**
 * Close a file descriptor
 * @param fd - The file descriptor to close
 */
export function close(fd: number): void;

/**
 * Wait for a process to exit
 * @param pid - The process ID to wait for
 * @returns The exit status of the process
 */
export function wait(pid: number): number;

/**
 * Get the current process ID
 * @returns The current process ID
 */
export function processId(): number;

/**
 * Load block extension data
 * @param index - The index of the block extension
 * @param source - The source of the block extension
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded block extension data as ArrayBuffer
 */
export function loadBlockExtension(
  index: number,
  source: number,
  offset?: number,
  length?: number,
): ArrayBuffer;

/**
 * Mount a file system
 * @param codeHash - The code hash of the cell to mount
 * @param hashType - The hash type of the code (use SCRIPT_HASH_TYPE_* constants)
 */
export function mount(codeHash: ArrayBuffer, hashType: number): void;

/**
 * Output debug message
 * @param message - The debug message to output
 */
export function debug(message: string): void;

/**
 * Evaluate a JavaScript script
 * @param jsScript - The JavaScript script to evaluate
 * @param enableModule - Whether to enable ES6 module (default: false)
 * @returns The result of the script evaluation
 * @note When enableModule is true, the script doesn't return any value.
 */
export function evalJsScript(jsScript: string, enableModule?: boolean): any;

/**
 * Load a JavaScript script
 * @param path - The path to the JavaScript script
 * @param enableModule - Whether to enable ES6 module (default: false)
 * @returns The loaded script as ArrayBuffer
 * @note When enableModule is true, the script doesn't return any value.
 */
export function loadJsScript(path: string, enableModule?: boolean): any;

/**
 * Load a file
 * @param path - The path to the file
 * @returns The loaded file as string
 */
export function loadFile(path: string): string;

/**
 * Parse a JSON string with extended options, like comments
 * @param json - The JSON string to parse
 * @returns The parsed JSON object
 */
export function parseExtJSON(json: string): Object;

/**
 * SHA256 hash implementation
 */
export class Sha256 {
  constructor();
  /**
   * Update the hash with new data
   * @param data - Data to be hashed
   */
  update(data: ArrayBuffer): void;
  /**
   * Finalize and get the hash result
   * @returns The 32-byte hash result
   */
  finalize(): ArrayBuffer;
}

/**
 * Keccak256 hash implementation
 */
export class Keccak256 {
  constructor();
  /**
   * Update the hash with new data
   * @param data - Data to be hashed
   */
  update(data: ArrayBuffer): void;
  /**
   * Finalize and get the hash result
   * @returns The 32-byte hash result
   */
  finalize(): ArrayBuffer;
}

/**
 * Blake2b hash implementation
 */
export class Blake2b {
  /**
   * Create a new Blake2b hash instance
   * @param personal - Optional personalization string. It must have length with 16.
   */
  constructor(personal?: string);
  /**
   * Update the hash with new data
   * @param data - Data to be hashed
   */
  update(data: ArrayBuffer): void;
  /**
   * Finalize and get the hash result
   * @returns The 32-byte hash result as ArrayBuffer
   */
  finalize(): ArrayBuffer;
}

/**
 * RIPEMD160 hash implementation
 */
export class Ripemd160 {
  constructor();
  /**
   * Update the hash with new data
   * @param data - Data to be hashed
   */
  update(data: ArrayBuffer): void;
  /**
   * Finalize and get the hash result
   * @returns The 20-byte hash result
   */
  finalize(): ArrayBuffer;
}

/**
 * Secp256k1 cryptographic functions
 */
export const secp256k1: {
  /**
   * Recover raw public key from signature and message hash
   * @param signature - The 64-byte signature
   * @param recoveryId - The recovery ID (0-3)
   * @param messageHash - The 32-byte message hash
   * @returns The recovered raw public key (64-bytes)
   */
  recover(
    signature: ArrayBuffer,
    recoveryId: number,
    messageHash: ArrayBuffer,
  ): ArrayBuffer;

  /**
   * Serialize a raw public key (64-bytes) to serialized format(compressed or uncompressed)
   * @param pubkey - The raw public key to serialize
   * @param compressed - Whether to use compressed format (33 bytes) or
   * uncompressed (65 bytes)
   * @returns The serialized public key (33 or 65 bytes)
   */
  serializePubkey(pubkey: ArrayBuffer, compressed?: boolean): ArrayBuffer;

  /**
   * Parse a serialized public key(compressed or uncompressed) to raw public key. It
   * is the reverse function of serializePubkey.
   * @param serializedPubkey - The serialized format public key (33 or 65 bytes)
   * @returns The parsed raw public key (64-bytes)
   */
  parsePubkey(serializedPubkey: ArrayBuffer): ArrayBuffer;

  /**
   * Verify an ECDSA signature
   * @param signature - The 64-byte signature
   * @param messageHash - The 32-byte message hash
   * @param pubkey - The raw public key (64-bytes)
   * @returns True if signature is valid, false otherwise
   */
  verify(
    signature: ArrayBuffer,
    messageHash: ArrayBuffer,
    pubkey: ArrayBuffer,
  ): boolean;
};

/**
 * Schnorr signature cryptographic functions following the BIP340 specification
 */
export const schnorr: {
  /**
   * Serialize an x-only public key to 32 bytes.
   * Takes a 64-byte public key (containing an X and Y coordinate) and serializes
   * just the X coordinate as per BIP340 specification.
   * @param pubkey - The x-only public key to serialize (64 bytes)
   * @returns The serialized x-only public key (32 bytes containing just the X coordinate)
   */
  serializeXonlyPubkey(pubkey: ArrayBuffer): ArrayBuffer;

  /**
   * Compute tagged SHA256 hash as specified in BIP340.
   * This creates a tagged hash by first hashing the tag name and then combining it
   * with the message in a specific way to create domain separation.
   * @param tag - The domain separation tag data
   * @param msg - The message data to hash
   * @returns The 32-byte tagged hash result following BIP340 specification
   */
  taggedSha256(tag: ArrayBuffer, msg: ArrayBuffer): ArrayBuffer;

  /**
   * Parse a serialized x-only public key from its 32-byte X coordinate.
   * This is the inverse of serializeXonlyPubkey. It reconstructs the full public key
   * point from just the X coordinate.
   * @param serializedPubkey - The serialized x-only public key (32 bytes X coordinate)
   * @returns The parsed x-only public key (64 bytes containing reconstructed point)
   */
  parseXonlyPubkey(serializedPubkey: ArrayBuffer): ArrayBuffer;

  /**
   * Verify a Schnorr signature according to BIP340 specification.
   * This verification includes the tagged hashing scheme defined in BIP340.
   * @param signature - The 64-byte Schnorr signature (R,s format)
   * @param messageHash - The 32-byte message hash to verify against
   * @param pubkey - The x-only public key to verify with (must be 64 bytes)
   * @returns True if the signature is valid according to BIP340, false otherwise
   */
  verify(
    signature: ArrayBuffer,
    messageHash: ArrayBuffer,
    pubkey: ArrayBuffer,
  ): boolean;
};

/**
 * Sparse Merkle Tree implementation
 */
export class Smt {
  constructor();
  /**
   * Insert a key-value pair into the tree
   * @param key - The key to insert (32 bytes)
   * @param value - The value to insert (32 bytes)
   */
  insert(key: ArrayBuffer, value: ArrayBuffer): void;
  /**
   * Verify a Merkle proof
   * @param root - The 32-byte Merkle root
   * @param proof - The proof data
   * @returns True if proof is valid, false otherwise
   */
  verify(root: ArrayBuffer, proof: ArrayBuffer): boolean;
}

/**
 * Hex encoding utilities
 */
export const hex: {
  /**
   * Encode binary data as hex string
   * @param data - Data to encode
   * @returns Hex string representation
   */
  encode(data: ArrayBuffer): string;
  /**
   * Decode hex string to binary data
   * @param hex - Hex string to decode
   * @returns Decoded binary data
   */
  decode(hex: string): ArrayBuffer;
};

/**
 * Base64 encoding utilities
 */
export const base64: {
  /**
   * Encode binary data as base64 string
   * @param data - Data to encode
   * @returns Base64 string representation
   */
  encode(data: ArrayBuffer): string;
  /**
   * Decode base64 string to binary data
   * @param base64 - Base64 string to decode
   * @returns Decoded binary data
   */
  decode(base64: string): ArrayBuffer;
};

/**
 * Format and return a string using printf-style formatting
 * @param format - Printf format string
 * @param args - Values to format
 * @returns The formatted string
 */
export function sprintf(format: string, ...args: any[]): string;

/**
 * Format and print a string using printf-style formatting
 * @param format - Printf format string
 * @param args - Values to format
 */
export function printf(format: string, ...args: any[]): void;

/**
 * Console object for logging and assertions
 */
export const console: {
  /**
   * Log messages to debug output
   * @param args - Values to log
   */
  log(...args: any[]): void;

  /**
   * Assert a condition, throw if false
   * @param condition - Condition to check
   * @param args - Values to log if assertion fails
   */
  assert(condition: boolean, ...args: any[]): void;
};

/**
 * Global scriptArgs array containing command line arguments
 */
export const scriptArgs: string[];

/**
 * TextEncoder provides functionality for encoding strings into UTF-8 encoded bytes.
 * This implementation follows the Web Standard TextEncoder interface.
 */
export class TextEncoder {
  constructor();
  /**
   * Encodes a string into UTF-8 bytes
   * @param input - The string to encode
   * @returns An Uint8Array containing the UTF-8 encoded bytes
   */
  encode(input: string): Uint8Array;
}

/**
 * TextDecoder provides functionality for decoding UTF-8 encoded bytes into strings.
 * This implementation follows the Web Standard TextDecoder interface.
 */
export class TextDecoder {
  constructor();
  /**
   * Decodes UTF-8 encoded bytes into a string
   * @param input - The Uint8Array containing UTF-8 encoded bytes to decode
   * @returns The decoded string
   */
  decode(input: Uint8Array): string;
}
