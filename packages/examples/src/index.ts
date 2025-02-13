import * as bindings from "@ckb-js-std/bindings";
import { Script, HighLevel, log } from "@ckb-js-std/core";

function report_cycles() {
  let cycles = bindings.currentCycles();
  let num = (cycles / 1024 / 1024).toFixed(2);
  log.debug(`current cycles = ${num} M`);
}

function main() {
  log.setLevel(log.LogLevel.Debug);
  report_cycles();

  let script = bindings.loadScript();
  log.debug(`raw current script: ${JSON.stringify(script)}`);
  report_cycles();

  let script_obj = Script.decode(script);
  log.debug(`current script: ${JSON.stringify(script_obj)}`);
  report_cycles();

  let cell = HighLevel.loadCell(0, bindings.SOURCE_INPUT);
  log.debug(`first input cell: ${JSON.stringify(cell)}`);
  report_cycles();

  let iter = new HighLevel.QueryIter(
    HighLevel.loadCellLock,
    bindings.SOURCE_INPUT,
  );
  for (let item of iter) {
    log.debug(`list all input lock scripts: ${JSON.stringify(item)}`);
  }
  report_cycles();

  let tx = HighLevel.loadTransaction();
  for (let output of tx.outputs) {
    log.debug(`list all output cells: ${JSON.stringify(output)}`);
  }
  report_cycles();
}

main();
