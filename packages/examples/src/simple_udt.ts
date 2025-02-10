import * as bindings from "@ckb-js-std/bindings";
import { log, bytesEq, HighLevel } from "@ckb-js-std/core";

function loadCellLockHash(
  index: number,
  source: bindings.SourceType,
): ArrayBuffer {
  return bindings.loadCellByField(index, source, bindings.CELL_FIELD_LOCK_HASH);
}

function main() {
  log.setLevel(log.LogLevel.Debug);
  log.debug("simple UDT ...");

  let script = HighLevel.loadScript();
  // ckb-js-vm has leading 35 bytes args
  let real_args = script.args.slice(35);
  for (let lock_hash of new HighLevel.QueryIter(
    loadCellLockHash,
    bindings.SOURCE_INPUT,
  )) {
    if (bytesEq(new Uint8Array(lock_hash), real_args)) {
      // owner mode, return immediately
      return 0;
    }
  }

  let input_amount = 0n;
  for (let data of new HighLevel.QueryIter(
    bindings.loadCellData,
    bindings.SOURCE_GROUP_INPUT,
  )) {
    if (data.byteLength != 16) {
      throw `Invalid data length: ${data.byteLength}`;
    }
    let n = new BigUint64Array(data);
    let current_amount = n[0] | (n[1] << 64n);
    input_amount += current_amount;
  }
  let output_amount = 0n;
  for (let data of new HighLevel.QueryIter(
    bindings.loadCellData,
    bindings.SOURCE_GROUP_OUTPUT,
  )) {
    if (data.byteLength != 16) {
      throw `Invalid data length: ${data.byteLength}`;
    }
    let n = new BigUint64Array(data);
    let current_amount = n[0] | (n[1] << 64n);
    output_amount += current_amount;
  }
  log.debug(`verifying amount: ${input_amount} and ${output_amount}`);
  if (input_amount < output_amount) {
    return -1;
  }
  log.debug("Simple UDT quit successfully");
  return 0;
}

bindings.exit(main());
