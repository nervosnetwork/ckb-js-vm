/**
 * Interface for read operations, Inspired from Rust io::Read.
 */
export interface Read {
  /**
   * Reads data into the provided buffer.
   * @param buffer - The buffer to read data into.
   * @returns The number of bytes read, which may be less than the buffer length.
   */
  read(buffer: Uint8Array): number;

  /**
   * Reads exactly the specified number of bytes into the provided buffer.
   * @param buffer - The buffer to read data into.
   *
   * This method blocks until exactly the specified number of bytes have been read.
   * If fewer bytes are available, it will wait until more data arrives.
   */
  readExact(buffer: Uint8Array): void;
}

/**
 * Interface for write operations, Inspired from Rust io::Write.
 */
export interface Write {
  /**
   * Writes data from the provided buffer to the underlying stream.
   * @param buffer - The buffer containing the data to write.
   * @returns The number of bytes written, which may be less than the buffer length.
   */
  write(buffer: Uint8Array): number;

  /**
   * Flushes the underlying stream to ensure all data is written out.
   */
  flush(): void;
}
