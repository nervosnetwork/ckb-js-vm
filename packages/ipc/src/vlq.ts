/**
 * A simple Variable-Length Quantity (VLQ) encoder/decoder implementation
 * that directly works with binary data using ArrayBuffer.
 */

/**
 * Maximum number of bytes needed to represent a 56-bit integer in VLQ format.
 * JavaScript's Number type (IEEE-754 double) can safely represent integers up to 2^53-1,
 * but we allocate enough space for slightly larger values to be conservative.
 * Each VLQ byte can store 7 bits of the integer (with 1 bit used as continuation flag).
 */
const MAX_BYTES_PER_INT = 8;
const MAX_BITS = MAX_BYTES_PER_INT * 7;

/**
 * Encodes a single number into VLQ format.
 *
 * @param value The number to encode
 * @returns A Uint8Array containing the encoded bytes
 */
export function encode(value: number): Uint8Array {
  const result = new Uint8Array(MAX_BYTES_PER_INT);

  if (value < 0) {
    throw new Error("VLQ does not support negative numbers");
  }

  let byteCount = 0;

  do {
    let byte = value & 0x7f;
    value >>>= 7;

    if (value !== 0) {
      byte |= 0x80;
    }

    result[byteCount++] = byte;
  } while (value !== 0);

  return result.slice(0, byteCount);
}

/**
 * Decodes a VLQ-encoded value from a byte array.
 *
 * @param buffer The buffer containing VLQ-encoded data
 * @param offset Starting position in the buffer
 * @returns Object containing the decoded value and the new position
 */
export function decode(
  buffer: ArrayBuffer | Uint8Array,
  offset: number = 0,
): { value: number; bytesRead: number } {
  const bytes = buffer instanceof ArrayBuffer ? new Uint8Array(buffer) : buffer;

  let value = 0;
  let shift = 0;
  let bytesRead = 0;
  let byte: number;

  do {
    if (offset + bytesRead >= bytes.length) {
      throw new Error("Incomplete VLQ value");
    }

    byte = bytes[offset + bytesRead++];

    // Extract the 7 data bits
    value |= (byte & 0x7f) << shift;
    shift += 7;

    // Check if we've read too many bytes
    if (shift > MAX_BITS) {
      throw new Error("VLQ value exceeds maximum supported size");
    }
  } while ((byte & 0x80) !== 0); // Continue until high bit is not set

  return { value, bytesRead };
}

/**
 * Calculates the size in bytes needed to encode a number in VLQ format.
 *
 * @param value The number to check
 * @returns Number of bytes needed
 */
export function encodedSize(value: number): number {
  if (value < 0) {
    throw new Error("VLQ does not support negative numbers");
  }

  let size = 0;
  do {
    size++;
    value >>>= 7;
  } while (value !== 0);

  return size;
}
