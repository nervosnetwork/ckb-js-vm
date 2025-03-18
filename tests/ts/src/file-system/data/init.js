import * as ckb from "@ckb-js-std/bindings";

ckb.mount(2, ckb.SOURCE_CELL_DEP, "/");
ckb.mount(2, ckb.SOURCE_CELL_DEP, "/a");
ckb.mount(2, ckb.SOURCE_CELL_DEP, "/b/");

console.log("init.js");
