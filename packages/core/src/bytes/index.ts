// migrated from ccc
export type Bytes = ArrayBuffer;

export type BytesLike = ArrayBuffer;

export function bytesConcat(...args: BytesLike[]): Bytes {
  return bytesConcatTo(new ArrayBuffer(0), ...args);
}

export function bytesConcatTo(result: Bytes, ...args: Bytes[]): Bytes {
  // Calculate total length needed
  const totalLength = args.reduce(
    (sum, arr) => sum + arr.byteLength,
    result.byteLength,
  );

  // Create new array with exact size needed
  const newResult = new Uint8Array(totalLength);

  // Copy initial result
  newResult.set(new Uint8Array(result), 0);

  // Copy each argument at the correct offset
  let offset = result.byteLength;
  for (const arg of args) {
    newResult.set(new Uint8Array(arg), offset);
    offset += arg.byteLength;
  }

  return newResult.buffer;
}

export function bytesFrom(bytes: BytesLike): Bytes {
  return bytes;
}

export function bytesEq(a: BytesLike, b: BytesLike): boolean {
  if (a === b) {
    return true;
  }

  if (a.byteLength !== b.byteLength) {
    return false;
  }
  if (a.byteLength % 8 == 0) {
    const aBigUint64Array = new BigUint64Array(a);
    const bBigUint64Array = new BigUint64Array(b);

    for (let i = 0; i < aBigUint64Array.length; i++) {
      if (aBigUint64Array[i] !== bBigUint64Array[i]) {
        return false;
      }
    }

    return true;
  } else {
    const aUint8Array = new Uint8Array(a);
    const bUint8Array = new Uint8Array(b);

    for (let i = 0; i < aUint8Array.length; i++) {
      if (aUint8Array[i] !== bUint8Array[i]) {
        return false;
      }
    }

    return true;
  }
}
