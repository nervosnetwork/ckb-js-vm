type SourceType = number | bigint;

/**
 * Source constants for loading cells/inputs/headers
 * Used as parameters in load functions to specify data source
 */
export const SOURCE_CELL_DEP: number;
export const SOURCE_HEADER_DEP: number;
export const SOURCE_INPUT: SourceType;
export const SOURCE_OUTPUT: SourceType;
export const SOURCE_GROUP_INPUT: SourceType;
export const SOURCE_GROUP_OUTPUT: SourceType;


/**
 * Field constants for loading cell data
 * Used to specify which field to load when using loadCellByField
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
export function loadCell(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load input data from the transaction
 * @param index - The index of the input
 * @param source - The source of the input (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded input data as ArrayBuffer
 */
export function loadInput(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load header data from the transaction
 * @param index - The index of the header
 * @param source - The source of the header (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded header data as ArrayBuffer
 */
export function loadHeader(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load witness data from the transaction
 * @param index - The index of the witness
 * @param source - The source of the witness (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded witness data as ArrayBuffer
 */
export function loadWitness(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load cell data from the transaction
 * @param index - The index of the cell
 * @param source - The source of the cell (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded cell data as ArrayBuffer
 */
export function loadCellData(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load cell data by specific field
 * @param index - The index of the cell
 * @param source - The source of the cell (use SOURCE_* constants)
 * @param field - The field to load (use CELL_FIELD_* constants)
 * @param offset - Optional starting offset in the field data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded field data as ArrayBuffer
 */
export function loadCellByField(index: number, source: SourceType, field: number, offset?: number, length?: number): ArrayBuffer;

/**
 * Load header data by specific field
 * @param index - The index of the header
 * @param source - The source of the header (use SOURCE_* constants)
 * @param field - The field to load (use HEADER_FIELD_* constants)
 * @param offset - Optional starting offset in the field data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded field data as ArrayBuffer
 */
export function loadHeaderByField(index: number, source: SourceType, field: number, offset?: number, length?: number): ArrayBuffer;

/**
 * Load input data by specific field
 * @param index - The index of the input
 * @param source - The source of the input (use SOURCE_* constants)
 * @param field - The field to load (use INPUT_FIELD_* constants)
 * @param offset - Optional starting offset in the field data (defaults to 0)
 * @param length - Optional length of data to load (defaults to reading until the end)
 * @returns The loaded field data as ArrayBuffer
 */
export function loadInputByField(index: number, source: SourceType, field: number, offset?: number, length?: number): ArrayBuffer;

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
export function execCell(codeHash: ArrayBuffer, hashType: number, offset: number, length: number, ...args: string[]): void;

/**
 * Options for spawning a new cell process
 */
export interface SpawnArgs {
    /** Command line arguments to pass to the spawned process */
    argv?: string[];
    /** File descriptors to inherit in the spawned process */
    inherited_fds?: number[];
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
export function spawnCell(codeHash: ArrayBuffer, hashType: number, offset: number, length: number, args: SpawnArgs): number;

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
 */
export function read(fd: number, length: number): ArrayBuffer;

/**
 * Write to a file descriptor
 * @param fd - The file descriptor to write to
 * @param data - The data to write
 * @returns Number of bytes written
 */
export function write(fd: number, data: ArrayBuffer): number;

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
export function loadBlockExtension(index: number, source: number, offset?: number, length?: number): ArrayBuffer;

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
export function parseExtJSON(json: string) : Object;

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
     * @param personal - Optional personalization string
     */
    constructor(personal?: ArrayBuffer);
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
 * Recover raw public key from signature and message hash
 * @param signature - The 64-byte signature
 * @param recoveryId - The recovery ID (0-3)
 * @param messageHash - The 32-byte message hash
 * @returns The recovered raw public key (64-bytes)
 */
export function recover(signature: ArrayBuffer, recoveryId: number, messageHash: ArrayBuffer): ArrayBuffer;

/**
 * Serialize a raw public key (64-bytes) to serialized format(compressed or uncompressed)
 * @param pubkey - The raw public key to serialize
 * @param compressed - Whether to use compressed format (33 bytes) or
 * uncompressed (65 bytes)
 * @returns The serialized public key (33 or 65 bytes)
 */
export function serializePubkey(pubkey: ArrayBuffer, compressed?: boolean): ArrayBuffer;

/**
 * Parse a serialized public key(compressed or uncompressed) to raw public key. It
 * is the reverse function of serializePubkey.
 * @param serializedPubkey - The serialized format public key (33 or 65 bytes)
 * @returns The parsed raw public key (64-bytes)
 */
export function parsePubkey(serializedPubkey: ArrayBuffer): ArrayBuffer;

/**
 * Verify an ECDSA signature
 * @param signature - The 64-byte signature
 * @param messageHash - The 32-byte message hash
 * @param pubkey - The raw public key (64-bytes)
 * @returns True if signature is valid, false otherwise
 */
export function verify(signature: ArrayBuffer, messageHash: ArrayBuffer, pubkey: ArrayBuffer): boolean;

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


// TODO: double check and review BigFloat and BigDecimal

/**
 * Rounding modes for BigFloat operations
 */
export const enum BigFloatRoundingMode {
    /** Round to nearest, with ties to even */
    RNDN = 0,
    /** Round to zero */
    RNDZ = 1,
    /** Round to -Infinity */
    RNDD = 2,
    /** Round to +Infinity */
    RNDU = 3,
    /** Round to nearest, with ties away from zero */
    RNDNA = 4,
    /** Round away from zero */
    RNDA = 5,
    /** Faithful rounding (non-deterministic) */
    RNDF = 6,
}

/**
 * Environment for BigFloat operations controlling precision and rounding
 */
export interface BigFloatEnv {
    // Static properties
    readonly precMin: number;      // Minimum allowed precision (at least 2)
    readonly precMax: number;      // Maximum allowed precision (at least 113)
    readonly expBitsMin: number;   // Minimum allowed exponent bits (at least 3)
    readonly expBitsMax: number;   // Maximum allowed exponent bits (at least 15)

    // Static methods
    setPrec(fn: Function, prec: number, expBits?: number): any;

    // Instance properties
    /** Precision in bits */
    prec: number;
    /** Exponent size in bits */
    expBits: number;
    /** Rounding mode */
    rndMode: BigFloatRoundingMode;
    /** Whether subnormal numbers are allowed */
    subnormal: boolean;

    // Status flags
    /** Invalid operation flag */
    invalidOperation: boolean;
    /** Division by zero flag */
    divideByZero: boolean;
    /** Overflow flag */
    overflow: boolean;
    /** Underflow flag */
    underflow: boolean;
    /** Inexact flag */
    inexact: boolean;

    // Methods
    /** Clear all status flags */
    clearStatus(): void;
}

/**
 * Constructor for BigFloatEnv
 */
export interface BigFloatEnvConstructor {
    new(): BigFloatEnv;
    new(prec: number, rndMode?: BigFloatRoundingMode): BigFloatEnv;

    readonly prototype: BigFloatEnv;

    // Static properties from the interface
    readonly precMin: number;
    readonly precMax: number;
    readonly expBitsMin: number;
    readonly expBitsMax: number;

    // Static methods
    setPrec(fn: Function, prec: number, expBits?: number): any;
}

export const BigFloatEnv: BigFloatEnvConstructor;


/**
 * BigFloat provides arbitrary-precision floating-point arithmetic
 */
export const BigFloat: {
    // Constants
    readonly PI: BigFloat;
    readonly LN2: BigFloat;
    readonly MIN_VALUE: BigFloat;
    readonly MAX_VALUE: BigFloat;
    readonly EPSILON: BigFloat;

    /**
     * Convert value to BigFloat. Cannot be used as constructor.
     * If number: converts without rounding
     * If string: converts using global floating point environment precision
     */
    (value: number | string | BigFloat): BigFloat;

    /**
     * Parse string as floating point number
     * @param str - String to parse
     * @param radix - Optional base (0 or 2-36, default 0)
     * @param env - Optional floating point environment
     */
    parseFloat(str: string, radix?: number, env?: BigFloatEnv): BigFloat;

    /**
     * Check if value is finite
     */
    isFinite(value: BigFloat): boolean;

    /**
     * Check if value is NaN
     */
    isNaN(value: BigFloat): boolean;

    /**
     * Round according to floating point environment
     * @param x - Value to round
     * @param env - Optional floating point environment
     */
    fpRound(x: BigFloat, env?: BigFloatEnv): BigFloat;

    // Arithmetic operations with optional environment
    add(a: BigFloat, b: BigFloat, env?: BigFloatEnv): BigFloat;
    sub(a: BigFloat, b: BigFloat, env?: BigFloatEnv): BigFloat;
    mul(a: BigFloat, b: BigFloat, env?: BigFloatEnv): BigFloat;
    div(a: BigFloat, b: BigFloat, env?: BigFloatEnv): BigFloat;

    // Integer rounding operations (no additional rounding)
    floor(x: BigFloat): BigFloat;
    ceil(x: BigFloat): BigFloat;
    round(x: BigFloat): BigFloat;
    trunc(x: BigFloat): BigFloat;

    /**
     * Absolute value (no additional rounding)
     */
    abs(x: BigFloat): BigFloat;

    // Remainder operations
    fmod(x: BigFloat, y: BigFloat, env?: BigFloatEnv): BigFloat;
    remainder(x: BigFloat, y: BigFloat, env?: BigFloatEnv): BigFloat;

    // Transcendental operations with optional environment
    sqrt(x: BigFloat, env?: BigFloatEnv): BigFloat;
    sin(x: BigFloat, env?: BigFloatEnv): BigFloat;
    cos(x: BigFloat, env?: BigFloatEnv): BigFloat;
    tan(x: BigFloat, env?: BigFloatEnv): BigFloat;
    asin(x: BigFloat, env?: BigFloatEnv): BigFloat;
    acos(x: BigFloat, env?: BigFloatEnv): BigFloat;
    atan(x: BigFloat, env?: BigFloatEnv): BigFloat;
    atan2(y: BigFloat, x: BigFloat, env?: BigFloatEnv): BigFloat;
    exp(x: BigFloat, env?: BigFloatEnv): BigFloat;
    log(x: BigFloat, env?: BigFloatEnv): BigFloat;
    pow(x: BigFloat, y: BigFloat, env?: BigFloatEnv): BigFloat;
};

// Represents a BigFloat value
export interface BigFloat {
    toString(): string;
    valueOf(): number;
    toPrecision(precision: number, mode?: BigFloatRoundingMode, radix?: number): string;
    toFixed(digits: number, mode?: BigFloatRoundingMode, radix?: number): string;
    toExponential(fractionDigits: number, mode?: BigFloatRoundingMode, radix?: number): string;
}

/**
 * Rounding mode for BigDecimal operations
 */
export type BigDecimalRoundingMode = "floor" | "ceiling" | "down" | "up" | "half-even" | "half-up";

/**
 * Rounding configuration for BigDecimal operations
 */
export interface BigDecimalRounding {
    roundingMode: BigDecimalRoundingMode;
    maximumSignificantDigits?: number;
    maximumFractionDigits?: number;
}

/**
 * BigDecimal is a wrapper around the BigDecimal type in QuickJS.
 * It provides a subset of the methods from the BigDecimal class in QuickJS.
 */
export interface BigDecimal {
    // Arithmetic operations
    add(x: BigDecimal, y: BigDecimal, rounding?: BigDecimalRounding): BigDecimal;
    sub(x: BigDecimal, y: BigDecimal, rounding?: BigDecimalRounding): BigDecimal;
    mul(x: BigDecimal, y: BigDecimal, rounding?: BigDecimalRounding): BigDecimal;
    div(x: BigDecimal, y: BigDecimal, rounding?: BigDecimalRounding): BigDecimal;
    mod(x: BigDecimal, y: BigDecimal, rounding?: BigDecimalRounding): BigDecimal;

    // Mathematical functions
    round(x: BigDecimal, rounding: BigDecimalRounding): BigDecimal;
    sqrt(x: BigDecimal, rounding: BigDecimalRounding): BigDecimal;

    // Formatting methods from QuickJS implementation
    toString(): string;
    valueOf(): string;
    toPrecision(precision: number): string;
    toFixed(digits: number): string;
    toExponential(fractionDigits: number): string;
}

