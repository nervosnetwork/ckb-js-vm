import * as bindings from "@ckb-js-std/bindings";
import { logError } from "@ckb-js-std/core";

function main(): void {
  try {
    step1();
  } catch (e) {
    throw new Error("main failed", { cause: e as any });
  }
}

function step1() { step2(); }
function step2() {
  const err: any = new Error("crash at step2");
  err.code = "E_STEP2";
  throw err;
}

try {
  main();
} catch (e) {
  logError(e);
}

bindings.exit(-2);
