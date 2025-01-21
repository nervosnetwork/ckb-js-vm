import * as ckb from "@ckb-js-std/bindings";
globalThis.fib2 = function(n) {
    if (n <= 0)
        return 0;
    else if (n == 1)
        return 1;
    else
        return fib(n - 1) + fib(n - 2);
}
// we can't write it like fib.js because the module feature is enabled.
