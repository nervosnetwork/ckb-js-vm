import * as ckb from "@ckb-js-std/bindings";
import { Script, HighLevel } from "@ckb-js-std/core";
import { loadCellLock, QueryIter } from "../../core/dist/high-level/high-level";

function report_cycles() {
  let cycles = ckb.currentCycles();
  let num = (cycles / 1024 / 1024).toFixed(2);
  console.log(`current cycles = ${num} M`);
}

function main() {
  report_cycles();
  let script = ckb.loadScript();
  console.log(`script length is ${script.byteLength}`);
  report_cycles();
  let script_obj = Script.decode(new Uint8Array(script));
  console.log("script code_hash = ", script_obj.codeHash);
  console.log("script hash_type = ", script_obj.hashType);
  console.log("script args = ", script_obj.args);
  report_cycles();
  let cell = HighLevel.loadCell(0, ckb.SOURCE_INPUT);
  console.log(`cell capacity is ${cell.capacity}`);
  report_cycles();

  let iter = new QueryIter(loadCellLock, ckb.SOURCE_INPUT);
  for (let item of iter) {
    console.log("lock script's code hash is", item.codeHash);
  }
  report_cycles();
}

main();
