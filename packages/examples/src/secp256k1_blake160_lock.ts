/**
 * TypeScript implementation of [SECP256K1/blake160](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0024-ckb-genesis-script-list/0024-ckb-genesis-script-list.md#secp256k1blake160)
 *
 */

import * as bindings from "@ckb-js-std/bindings";
import {
  HighLevel,
  WitnessArgs,
  HasherCkb,
  bytesFrom,
  BytesLike,
  Hasher,
  numToBytes,
  hashCkb,
  bytesEq,
  log,
} from "@ckb-js-std/core";

function hashWitnessToHasher(witness: BytesLike, hasher: Hasher): void {
  const raw = bytesFrom(witness);
  hasher.update(numToBytes(raw.length, 8));
  hasher.update(raw);
}

/**
 * Generates a sighash for all inputs in the transaction
 * @returns The 32-byte sighash
 * @throws Error if witness args are invalid or system error occurs
 */
export function generateSighashAll(): Uint8Array {
  const hasher = new HasherCkb();
  // Hash transaction
  const tx_hash = bindings.loadTxHash();
  hasher.update(new Uint8Array(tx_hash));

  // Handle first witness (from group input)
  const firstWitnessArgs = HighLevel.loadWitnessArgs(
    0,
    bindings.SOURCE_GROUP_INPUT,
  );
  if (!firstWitnessArgs || !firstWitnessArgs.lock) {
    throw new Error("Wrong witness args");
  }

  // Create zero-filled lock bytes of same length
  const zeroLock = new Uint8Array(firstWitnessArgs.lock.length);
  const modifiedWitnessArgs = WitnessArgs.from({
    ...firstWitnessArgs,
    lock: zeroLock,
  });

  // Hash first witness
  const firstWitnessBytes = modifiedWitnessArgs.toBytes();
  hasher.update(firstWitnessBytes);

  // Hash remaining group input witnesses
  let index = 1;
  while (true) {
    try {
      const witness = bindings.loadWitness(index, bindings.SOURCE_GROUP_INPUT);
      hashWitnessToHasher(new Uint8Array(witness), hasher);
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
      hashWitnessToHasher(new Uint8Array(witness), hasher);
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
  log.debug(`message = ${message}`);
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
  log.debug(`signature = ${signature}`);
  const rec_id = witness.lock.at(64);
  log.debug(`rec_id = ${rec_id}`);
  if (rec_id === undefined) {
    throw new Error("Invalid recovery ID in witness lock");
  }

  const pubkey = bindings.secp256k1.recover(
    signature.buffer,
    rec_id,
    message.buffer,
  );
  const comp_pubkey = bindings.secp256k1.serializePubkey(pubkey, true);
  const pubkey_hash = hashCkb(new Uint8Array(comp_pubkey)).slice(0, 20);

  return bytesEq(expected_pubkey_hash, pubkey_hash) ? 0 : 1;
}

bindings.exit(main());
