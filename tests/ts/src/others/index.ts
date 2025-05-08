import * as bindings from "@ckb-js-std/bindings";
import { log } from "@ckb-js-std/core";

function main() {
  log.setLevel(log.LogLevel.Debug);
  let str = "hello, world";
  let encoder = new bindings.TextEncoder();
  let encoded = encoder.encode(str);
  let decoder = new bindings.TextDecoder();
  let decoded = decoder.decode(encoded);

  console.assert(decoded === str);
}

main();
