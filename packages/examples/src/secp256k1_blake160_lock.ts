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

/**
 * Generates a sighash for all inputs in the transaction
 * @returns The 32-byte sighash
 * @throws Error if witness args are invalid or system error occurs
 */
export function generateSighashAll(): Bytes {
  const hasher = new HasherCkb();
  // Hash transaction
  const tx_hash = bindings.loadTxHash();
  hasher.update(tx_hash);

  // Handle first witness (from group input)
  const firstWitnessArgs = HighLevel.loadWitnessArgs(
    0,
    bindings.SOURCE_GROUP_INPUT,
  );
  if (!firstWitnessArgs || !firstWitnessArgs.lock) {
    throw new Error("Wrong witness args");
  }

  // Create zero-filled lock bytes of same length
  const zeroLock = new ArrayBuffer(firstWitnessArgs.lock.byteLength);
  const modifiedWitnessArgs = WitnessArgs.from({
    ...firstWitnessArgs,
    lock: zeroLock,
  });

  // Hash first witness
  const firstWitnessBytes = modifiedWitnessArgs.toBytes();
  hashWitnessToHasher(firstWitnessBytes, hasher);

  // Hash remaining group input witnesses
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

  // Calculate total inputs length
  let inputsLength = 0;
  try {
    // Use QueryIter to count inputs
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

  // Hash remaining input witnesses
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
  log.debug("secp256k1_blake160_lock");
  const message = generateSighashAll();
  log.debug(`message = ${new Uint8Array(message)}`);
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
  log.debug(`signature = ${new Uint8Array(signature)}`);
  const rec_id = new Uint8Array(witness.lock)[64];
  log.debug(`rec_id = ${rec_id}`);
  const pubkey = bindings.secp256k1.recover(signature, rec_id, message);
  const comp_pubkey = bindings.secp256k1.serializePubkey(pubkey, true);
  const pubkey_hash = hashCkb(comp_pubkey).slice(0, 20);

  return bytesEq(expected_pubkey_hash, pubkey_hash) ? 0 : 1;
}

bindings.exit(main());
