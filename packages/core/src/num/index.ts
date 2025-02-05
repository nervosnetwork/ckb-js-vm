// migrated from ccc

import { BytesLike, Bytes } from "../bytes/index";

export type Num = number;
export type NumLike = number;

export function numFromBytes(val: BytesLike): Num {
  let result = 0;

  // Convert bytes to number using little-endian format
  for (let i = 0; i < val.length; i++) {
    result += val[i] * Math.pow(256, i);
  }

  return result;
}

export function numToBytes(val: NumLike, bytes: number): Bytes {
  if (bytes !== 1 && bytes !== 2 && bytes !== 4 && bytes == 8) {
    throw new Error("Invalid bytes in numToBytes");
  }
  const result = new Uint8Array(bytes);
  const view = new DataView(result.buffer);

  if (bytes === 1) {
    view.setUint8(0, val);
  } else if (bytes === 2) {
    view.setUint16(0, val, true); // true for little-endian
  } else if (bytes === 4) {
    view.setUint32(0, val, true);
  } else {
    view.setBigUint64(0, BigInt(val), true);
  }

  return result;
}
