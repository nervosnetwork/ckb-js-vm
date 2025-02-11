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
  log.debug(`script length is ${script.byteLength}`);
  report_cycles();
  let script_obj = Script.decode(script);
  log.debug(`script code_hash = ${bindings.hex.encode(script_obj.codeHash)}`);
  log.debug(`script hash_type = ${script_obj.hashType}`);
  log.debug(`script args = ${bindings.hex.encode(script_obj.args)}`);
  report_cycles();
  let cell = HighLevel.loadCell(0, bindings.SOURCE_INPUT);
  log.debug(`cell capacity is ${cell.capacity}`);
  report_cycles();

  let iter = new HighLevel.QueryIter(
    HighLevel.loadCellLock,
    bindings.SOURCE_INPUT,
  );
  for (let item of iter) {
    log.debug(
      `lock script's code hash is ${bindings.hex.encode(item.codeHash)}`,
    );
  }
  report_cycles();

  let tx = HighLevel.loadTransaction();
  tx.outputs.forEach((output) => {
    log.debug(`output capacity is ${output.capacity}`);
  });
  report_cycles();
}

main();
