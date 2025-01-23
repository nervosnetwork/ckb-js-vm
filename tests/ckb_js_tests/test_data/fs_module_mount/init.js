import * as ckb from "@ckb-js-std/bindings";

ckb.mount(2, ckb.SOURCE_CELL_DEP, "/")
ckb.mount(2, ckb.SOURCE_CELL_DEP, "a")
ckb.mount(2, ckb.SOURCE_CELL_DEP, "/b")
ckb.mount(2, ckb.SOURCE_CELL_DEP, "c/")
ckb.mount(2, ckb.SOURCE_CELL_DEP, "/d/")

console.log("init.js");
