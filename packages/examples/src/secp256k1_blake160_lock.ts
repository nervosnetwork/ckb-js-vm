/**
 * TypeScript implementation of [SECP256K1/blake160](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0024-ckb-genesis-script-list/0024-ckb-genesis-script-list.md#secp256k1blake160)
 *
 */

import * as bindings from "@ckb-js-std/bindings";
import {
  HighLevel,
  WitnessArgs,
  HasherCkb,
  BytesLike,
  Hasher,
  numToBytes,
  hashCkb,
  bytesEq,
  log,
  Bytes,
} from "@ckb-js-std/core";

function hashWitnessToHasher(witness: BytesLike, hasher: Hasher): void {
  hasher.update(numToBytes(witness.byteLength, 8));
  hasher.update(witness);
}

export function generateSighashAll(): Bytes {
  const hasher = new HasherCkb();
  const txHash = bindings.loadTxHash();
  hasher.update(txHash);

  const firstWitnessArgs = HighLevel.loadWitnessArgs(
    0,
    bindings.SOURCE_GROUP_INPUT,
  );
  if (!firstWitnessArgs || !firstWitnessArgs.lock) {
    throw new Error("Wrong witness args");
  }

  const zeroLock = new ArrayBuffer(firstWitnessArgs.lock.byteLength);
  const modifiedWitnessArgs = WitnessArgs.from({
    ...firstWitnessArgs,
    lock: zeroLock,
  });

  const firstWitnessBytes = modifiedWitnessArgs.toBytes();
  hashWitnessToHasher(firstWitnessBytes, hasher);

  let index = 1;
  while (true) {
    try {
      const witness = bindings.loadWitness(index, bindings.SOURCE_GROUP_INPUT);
      hashWitnessToHasher(witness, hasher);
      index++;
    } catch (err: any) {
      if (err.errorCode === bindings.INDEX_OUT_OF_BOUND) {
        break;
      }
      throw err;
    }
  }

  let inputsLength = 0;
  try {
    const iter = new HighLevel.QueryIter(
      HighLevel.loadInputSince,
      bindings.SOURCE_INPUT,
    );
    for (const _ of iter) {
      inputsLength++;
    }
  } catch (err: any) {
    if (err.errorCode !== bindings.INDEX_OUT_OF_BOUND) {
      throw err;
    }
  }

  for (let i = inputsLength; ; i++) {
    try {
      const witness = bindings.loadWitness(i, bindings.SOURCE_INPUT);
      hashWitnessToHasher(witness, hasher);
    } catch (err: any) {
      if (err.errorCode === bindings.INDEX_OUT_OF_BOUND) {
        break;
      }
      throw err;
    }
  }

  return hasher.digest();
}

function main(): number {
  log.setLevel(log.LogLevel.Debug);
  log.debug("secp256k1_blake160_lock ...");
  const message = generateSighashAll();
  const script = HighLevel.loadScript();
  const expected_pubkey_hash = script.args.slice(35);

  if (expected_pubkey_hash.byteLength !== 20) {
    throw new Error("Invalid pubkey hash length. Expected 20 bytes.");
  }

  const witness = HighLevel.loadWitnessArgs(0, bindings.SOURCE_GROUP_INPUT);
  if (!witness?.lock || witness.lock.byteLength !== 65) {
    throw new Error("Invalid witness args: missing lock or incorrect length");
  }

  const signature = witness.lock.slice(0, 64);
  const ricId = new Uint8Array(witness.lock)[64];
  const pubkey = bindings.secp256k1.recover(signature, ricId, message);
  const compPubkey = bindings.secp256k1.serializePubkey(pubkey, true);
  const pubkeyHash = hashCkb(compPubkey).slice(0, 20);

  return bytesEq(expected_pubkey_hash, pubkeyHash) ? 0 : 1;
}

bindings.exit(main());
