// migrated from ccc

import { BytesLike, Bytes } from "../bytes/index";

export type Num = bigint;
export type NumLike = number | bigint;

export function numFromBytes(bytes: BytesLike): Num {
  let result = 0n;
  const length = bytes.byteLength;
  if (length === 1) {
    result = BigInt(new Uint8Array(bytes)[0]);
  } else if (length === 2) {
    result = BigInt(new DataView(bytes).getUint16(0, true));
  } else if (length === 4) {
    result = BigInt(new DataView(bytes).getUint32(0, true));
  } else if (length === 8) {
    result = new DataView(bytes).getBigUint64(0, true);
  } else if (length % 8 === 0) {
    let view = new DataView(bytes);
    for (let i = 0; i < length / 8; i++) {
      result += view.getBigUint64(i * 8, true) << BigInt(i * 64);
    }
  } else {
    throw new Error("Invalid bytes in numFromBytes");
  }
  return result;
}

export function numToBytes(val: NumLike, bytes: number): Bytes {
  const result = new ArrayBuffer(bytes);
  const view = new DataView(result);

  if (bytes === 1) {
    view.setUint8(0, Number(val));
  } else if (bytes === 2) {
    view.setUint16(0, Number(val), true);
  } else if (bytes === 4) {
    view.setUint32(0, Number(val), true);
  } else if (bytes === 8) {
    view.setBigUint64(0, BigInt(val), true);
  } else if (bytes % 8 === 0) {
    const mask = BigInt(0xffffffffffffffffn);
    const bigVal = BigInt(val);
    for (let i = 0; i < bytes / 8; i++) {
      const item = (bigVal >> BigInt(i * 64)) & mask;
      view.setBigUint64(i * 8, item, true);
    }
  } else {
    throw new Error("Invalid value in numToBytes");
  }

  return result;
}
