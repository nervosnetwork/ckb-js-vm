import {
    hexFrom,
    Transaction,
    hashTypeToBytes,
} from "@ckb-ccc/core";
import { readFileSync } from "fs";
import { Resource, Verifier } from "ckb-testtool";

async function main() {
    const resource = Resource.default();
    const lockCell = resource.mockDebugCellAsCellDep("./dist/simple_stack_trace.debug.js")
    // const lockCell = resource.mockDebugCellAsCellDep("./dist/simple_stack_trace.bc")
    const lockScript = resource.createScriptByData(lockCell, "0x");
    const mainCell = resource.mockCellAsCellDep(hexFrom(readFileSync("../../build/ckb-js-vm")));
    const mainScript = resource.createScriptByData(mainCell, hexFrom(
        "0x0000" +
        lockScript.codeHash.slice(2) +
        hexFrom(hashTypeToBytes(lockScript.hashType)).slice(2),
    ));
    const inputCell = resource.mockCell(mainScript);

    const tx = Transaction.from({
        cellDeps: [
            Resource.createCellDep(lockCell, "code"),
            Resource.createCellDep(mainCell, "code"),
        ],
        inputs: [
            Resource.createCellInput(inputCell),
        ],
        outputs: [
            Resource.createCellOutput(mainScript),
        ],
        outputsData: [
            hexFrom("0x"),
        ],
    })

    const verifier = Verifier.from(resource, tx);
    verifier.verifyFailure(-2, true);
}

describe("simple-stack-trace", () => {
    test("failed", () => {
        main();
    });
});
