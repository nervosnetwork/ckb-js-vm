import * as ckb from "@ckb-js-std/bindings";
import { Script, HighLevel } from "@ckb-js-std/core";
import { loadCellLock, QueryIter } from "../../core/dist/high-level/high-level";
import { log } from "@ckb-js-std/core";

function report_cycles() {
  let cycles = ckb.currentCycles();
  let num = (cycles / 1024 / 1024).toFixed(2);
  log.debug(`current cycles = ${num} M`);
}

function main() {
  log.setLevel(log.LogLevel.Debug);
  report_cycles();
  let script = ckb.loadScript();
  log.debug(`script length is ${script.byteLength}`);
  report_cycles();
  let script_obj = Script.decode(new Uint8Array(script));
  log.debug("script code_hash = ", script_obj.codeHash);
  log.debug("script hash_type = ", script_obj.hashType);
  log.debug("script args = ", script_obj.args);
  report_cycles();
  let cell = HighLevel.loadCell(0, ckb.SOURCE_INPUT);
  log.debug(`cell capacity is ${cell.capacity}`);
  report_cycles();

  let iter = new QueryIter(loadCellLock, ckb.SOURCE_INPUT);
  for (let item of iter) {
    log.debug("lock script's code hash is", item.codeHash);
  }
  report_cycles();
}

main();
