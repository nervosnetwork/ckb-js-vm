// migrated from ccc

import { BytesLike, Bytes } from "../bytes/index";

export type Num = number;
export type NumLike = number;

export function numFromBytes(bytes: BytesLike): Num {
  if (bytes.byteLength === 1) {
    return new Uint8Array(bytes)[0];
  } else if (bytes.byteLength === 2) {
    return new DataView(bytes).getUint16(0, true);
  } else if (bytes.byteLength === 4) {
    return new DataView(bytes).getUint32(0, true);
  } else {
    let result = 0;
    let view = new DataView(bytes);
    for (let i = 0; i < view.byteLength; i++) {
      if (i >= 7) {
        console.assert(
          view.getUint8(i) === 0,
          "the value is too big to fit in a number, use bigintFromBytes instead",
        );
      }
      result += view.getUint8(i) * Math.pow(256, i);
    }
    return result;
  }
}

export function bigintFromBytes(bytes: BytesLike): bigint {
  let result = 0n;
  if (bytes.byteLength === 1) {
    result = BigInt(new Uint8Array(bytes)[0]);
  } else if (bytes.byteLength === 2) {
    result = BigInt(new DataView(bytes).getUint16(0, true));
  } else if (bytes.byteLength === 4) {
    result = BigInt(new DataView(bytes).getUint32(0, true));
  } else if (bytes.byteLength === 8) {
    result = new DataView(bytes).getBigUint64(0, true);
  } else {
    let view = new DataView(bytes);
    for (let i = 0; i < view.byteLength; i++) {
      result += BigInt(view.getUint8(i)) * BigInt(Math.pow(256, i));
    }
  }
  return result;
}

export function numToBytes(val: NumLike, bytes: number): Bytes {
  if (bytes !== 1 && bytes !== 2 && bytes !== 4 && bytes !== 8) {
    throw new Error("Invalid bytes in numToBytes");
  }
  const result = new ArrayBuffer(bytes);
  const view = new DataView(result);

  if (bytes === 1) {
    view.setUint8(0, val);
  } else if (bytes === 2) {
    view.setUint16(0, val, true);
  } else if (bytes === 4) {
    view.setUint32(0, val, true);
  } else {
    view.setBigUint64(0, BigInt(val), true);
  }

  return result;
}

export function bigintToBytes(val: bigint, bytes: number): Bytes {
  if (
    bytes !== 1 &&
    bytes !== 2 &&
    bytes !== 4 &&
    bytes !== 8 &&
    bytes % 8 !== 0
  ) {
    throw new Error("Invalid bytes in bigintToBytes");
  }
  const result = new ArrayBuffer(bytes);
  const view = new DataView(result);

  if (bytes === 1) {
    view.setUint8(0, Number(val));
  } else if (bytes === 2) {
    view.setUint16(0, Number(val), true);
  } else if (bytes === 4) {
    view.setUint32(0, Number(val), true);
  } else if (bytes === 8) {
    view.setBigUint64(0, val, true);
  } else {
    const mask = BigInt(0xffffffffffffffffn);
    for (let i = 0; i < bytes / 8; i++) {
      const item = (val >> BigInt(i * 64)) & mask;
      view.setBigUint64(i * 8, item, true);
    }
  }

  return result;
}
