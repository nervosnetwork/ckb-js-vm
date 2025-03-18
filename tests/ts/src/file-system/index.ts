import * as bindings from "@ckb-js-std/bindings";
import { log } from "@ckb-js-std/core";

function reportCycles() {
  let cycles = bindings.currentCycles();
  let num = (cycles / 1024 / 1024).toFixed(2);
  log.debug(`current cycles = ${num} M`);
}

function main() {
  log.setLevel(log.LogLevel.Debug);
  reportCycles();
}

main();
