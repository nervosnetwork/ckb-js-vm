import * as bindings from "@ckb-js-std/bindings";
import { HighLevel, log } from "@ckb-js-std/core";

function reportCycles() {
  let cycles = bindings.currentCycles();
  let num = (cycles / 1024 / 1024).toFixed(2);
  log.debug(`current cycles = ${num} M`);
}

function main() {
  log.setLevel(log.LogLevel.Debug);
  reportCycles();

  let script = HighLevel.loadScript();
  log.debug(`current script: ${JSON.stringify(script)}`);
  reportCycles();

  let cell = HighLevel.loadCell(0, HighLevel.SOURCE_INPUT);
  log.debug(`first input cell: ${JSON.stringify(cell)}`);
  reportCycles();

  let iter = new HighLevel.QueryIter(
    HighLevel.loadCellLock,
    HighLevel.SOURCE_INPUT,
  );
  for (let item of iter) {
    log.debug(`list all input lock scripts: ${JSON.stringify(item)}`);
  }
  reportCycles();

  let tx = HighLevel.loadTransaction();
  for (let output of tx.outputs) {
    log.debug(`list all output cells: ${JSON.stringify(output)}`);
  }
  reportCycles();
}

main();
