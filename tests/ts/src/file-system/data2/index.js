import * as ckb from "@ckb-js-std/bindings";

ckb.loadJsScript("fib.js");
const result = fib(10);
console.assert(result == 55, "fib(10) != 55");

ckb.loadJsScript("fib_module.js", true);
console.assert(globalThis.fib2(10) == 55, "fib2(10) != 55");

let code = ckb.loadFile("fib.js");
console.assert(code.includes("function fib"), "load file failed");
