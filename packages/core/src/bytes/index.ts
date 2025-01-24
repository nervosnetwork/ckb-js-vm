// migrated from ccc
export type Bytes = Uint8Array;

export type BytesLike = Uint8Array;

export function bytesConcat(...args: BytesLike[]): Bytes {
  return bytesConcatTo(new Uint8Array(0), ...args);
}

export function bytesConcatTo(result: Bytes, ...args: Bytes[]): Bytes {
  // Calculate total length needed
  const totalLength = args.reduce(
    (sum, arr) => sum + arr.length,
    result.length,
  );

  // Create new array with exact size needed
  const newResult = new Uint8Array(totalLength);

  // Copy initial result
  newResult.set(result, 0);

  // Copy each argument at the correct offset
  let offset = result.length;
  for (const arg of args) {
    newResult.set(arg, offset);
    offset += arg.length;
  }

  return newResult;
}

export function bytesFrom(bytes: BytesLike): Bytes {
  return bytes;
}

export function bytesEq(a: BytesLike, b: BytesLike): boolean {
  if (a === b) {
    return true;
  }

  if (a.length !== b.length) {
    return false;
  }

  for (let i = 0; i < a.length; i++) {
    if (a[i] !== b[i]) {
      return false;
    }
  }

  return true;
}
