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


export function loadScript(offset?: number, length?: number): ArrayBuffer;

/**
 * Load cell data from the transaction
 * @param index - The index of the cell
 * @param source - The source of the cell (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data
 * @param length - Optional length of data to load
 * @returns The loaded cell data as ArrayBuffer
 */
export function loadCell(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load input data from the transaction
 * @param index - The index of the input
 * @param source - The source of the input (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data
 * @param length - Optional length of data to load
 * @returns The loaded input data as ArrayBuffer
 */
export function loadInput(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load header data from the transaction
 * @param index - The index of the header
 * @param source - The source of the header (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data
 * @param length - Optional length of data to load
 * @returns The loaded header data as ArrayBuffer
 */
export function loadHeader(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load witness data from the transaction
 * @param index - The index of the witness
 * @param source - The source of the witness (use SOURCE_* constants)
 * @param offset - Optional starting offset in the data
 * @param length - Optional length of data to load
 * @returns The loaded witness data as ArrayBuffer
 */
export function loadWitness(index: number, source: SourceType, offset?: number, length?: number): ArrayBuffer;

/**
 * Load cell data by specific field
 * @param index - The index of the cell
 * @param source - The source of the cell (use SOURCE_* constants)
 * @param field - The field to load (use CELL_FIELD_* constants)
 * @param offset - Optional starting offset in the field data
 * @returns The loaded field data as ArrayBuffer
 */
export function loadCellByField(index: number, source: SourceType, field: number, offset?: number): ArrayBuffer;

/**
 * Load header data by specific field
 * @param index - The index of the header
 * @param source - The source of the header (use SOURCE_* constants)
 * @param field - The field to load (use HEADER_FIELD_* constants)
 * @param offset - Optional starting offset in the field data
 * @returns The loaded field data as ArrayBuffer
 */
export function loadHeaderByField(index: number, source: SourceType, field: number, offset?: number): ArrayBuffer;

/**
 * Load input data by specific field
 * @param index - The index of the input
 * @param source - The source of the input (use SOURCE_* constants)
 * @param field - The field to load (use INPUT_FIELD_* constants)
 * @param offset - Optional starting offset in the field data
 * @returns The loaded field data as ArrayBuffer
 */
export function loadInputByField(index: number, source: SourceType, field: number, offset?: number): ArrayBuffer;

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
 * @param offset - The offset in the cell data to start execution
 * @param length - The length of code to execute
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
 * @param offset - The offset in the cell data to start execution
 * @param length - The length of code to execute
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
 * @param offset - Optional starting offset in the data
 * @param length - Optional length of data to load
 * @returns The loaded block extension data as ArrayBuffer
 */
export function loadBlockExtension(index: number, source: number, offset?: number, length?: number): ArrayBuffer;

/**
 * Output debug message
 * @param message - The debug message to output
 */
export function debug(message: string): void;


/**
 * SHA256 hash implementation
 */
export class Sha256 {
    constructor();
    /**
     * Update the hash with new data
     * @param data - Data to be hashed
     */
    write(data: ArrayBuffer): void;
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
    write(data: ArrayBuffer): void;
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
    write(data: ArrayBuffer): void;
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
    write(data: ArrayBuffer): void;
    /**
     * Finalize and get the hash result
     * @returns The 20-byte hash result
     */
    finalize(): ArrayBuffer;
}


/**
 * Recover public key from signature and message hash
 * @param signature - The 64-byte signature
 * @param recoveryId - The recovery ID (0-3)
 * @param messageHash - The 32-byte message hash
 * @returns The recovered public key
 */
export function recover(signature: ArrayBuffer, recoveryId: number, messageHash: ArrayBuffer): ArrayBuffer;

/**
 * Serialize a public key to compressed or uncompressed format
 * @param pubkey - The public key to serialize
 * @param compressed - Whether to use compressed format (33 bytes) or uncompressed (65 bytes)
 * @returns The serialized public key
 */
export function serializePubkey(pubkey: ArrayBuffer, compressed?: boolean): ArrayBuffer;

/**
 * Parse a serialized public key
 * @param serializedPubkey - The serialized public key (33 or 65 bytes)
 * @returns The parsed public key
 */
export function parsePubkey(serializedPubkey: ArrayBuffer): ArrayBuffer;

/**
 * Verify an ECDSA signature
 * @param signature - The 64-byte signature
 * @param messageHash - The 32-byte message hash
 * @param pubkey - The public key
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