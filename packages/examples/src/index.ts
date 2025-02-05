import * as ckb from "@ckb-js-std/bindings";
import { Script } from "@ckb-js-std/core";

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
}

main();
