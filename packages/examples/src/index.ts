import * as bindings from "@ckb-js-std/bindings";
import { HighLevel, log } from "@ckb-js-std/core";

function reportCycles() {
  let cycles = bindings.currentCycles();
  let num = (cycles / 1024 / 1024).toFixed(2);
  log.debug(`current cycles = ${num} M`);
}

function bigintReplacer(key: string, value: any) {
  return typeof value === "bigint" ? Number(value) : value;
}

function main() {
  log.setLevel(log.LogLevel.Debug);
  reportCycles();

  let script = HighLevel.loadScript();
  log.debug(`current script: ${JSON.stringify(script, bigintReplacer)}`);
  reportCycles();

  let cell = HighLevel.loadCell(0, HighLevel.SOURCE_INPUT);
  log.debug(`first input cell: ${JSON.stringify(cell, bigintReplacer)}`);
  reportCycles();

  let iter = new HighLevel.QueryIter(
    HighLevel.loadCellLock,
    HighLevel.SOURCE_INPUT,
  );
  for (let item of iter) {
    log.debug(
      `list all input lock scripts: ${JSON.stringify(item, bigintReplacer)}`,
    );
  }
  reportCycles();

  let tx = HighLevel.loadTransaction();
  for (let output of tx.outputs) {
    log.debug(
      `list all output cells: ${JSON.stringify(output, bigintReplacer)}`,
    );
  }
  reportCycles();
}

main();
