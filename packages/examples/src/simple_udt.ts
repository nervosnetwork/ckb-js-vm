import * as bindings from "@ckb-js-std/bindings";
import { log, bytesEq, HighLevel } from "@ckb-js-std/core";

function main() {
  log.setLevel(log.LogLevel.Debug);
  log.debug("simple UDT ...");

  let script = HighLevel.loadScript();
  // ckb-js-vm has leading 35 bytes args
  let readArgs = script.args.slice(35);
  for (let lockHash of new HighLevel.QueryIter(
    (index, source) =>
      bindings.loadCellByField(index, source, bindings.CELL_FIELD_LOCK_HASH),
    bindings.SOURCE_INPUT,
  )) {
    if (bytesEq(lockHash, readArgs)) {
      // owner mode, return immediately
      return 0;
    }
  }

  let inputAmount = [
    ...new HighLevel.QueryIter(
      bindings.loadCellData,
      bindings.SOURCE_GROUP_INPUT,
    ),
  ]
    .map((data) => {
      if (data.byteLength != 16) {
        throw `Invalid data length: ${data.byteLength}`;
      }
      const n = new Uint32Array(data);
      return (
        BigInt(n[0]) |
        (BigInt(n[1]) << BigInt(32)) |
        (BigInt(n[2]) << BigInt(64)) |
        (BigInt(n[3]) << BigInt(96))
      );
    })
    .reduce((sum, amount) => sum + amount, 0n);

  let outputAmount = [
    ...new HighLevel.QueryIter(
      bindings.loadCellData,
      bindings.SOURCE_GROUP_OUTPUT,
    ),
  ]
    .map((data) => {
      if (data.byteLength != 16) {
        throw `Invalid data length: ${data.byteLength}`;
      }
      const n = new Uint32Array(data);
      return (
        BigInt(n[0]) |
        (BigInt(n[1]) << BigInt(32)) |
        (BigInt(n[2]) << BigInt(64)) |
        (BigInt(n[3]) << BigInt(96))
      );
    })
    .reduce((sum, amount) => sum + amount, 0n);

  log.debug(`verifying amount: ${inputAmount} and ${outputAmount}`);
  if (inputAmount < outputAmount) {
    return -1;
  }
  log.debug("Simple UDT quit successfully");
  return 0;
}

bindings.exit(main());
