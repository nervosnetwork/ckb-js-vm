import * as ckb from "@ckb-js-std/bindings";

function main() {
    let script = ckb.loadScript();
    console.log(`script length is ${script.byteLength}`);
    let cycles = ckb.currentCycles();
    console.log(`current cycles is ${cycles}`);
}

main();
