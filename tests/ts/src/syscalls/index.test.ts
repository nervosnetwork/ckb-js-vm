import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
  createHeaderViewTemplate,
} from "ckb-testtool";

async function main(path: string) {
  const resource = Resource.default();

  const mainCell = resource.mockCellAsCellDep(hexFrom(readFileSync("../../build/ckb-js-vm")));
  const alwaysSuccessCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
  const onChainCell = resource.mockCellAsCellDep(hexFrom(readFileSync(path)));
  const alwaysSuccessScript = resource.createScriptByType(alwaysSuccessCell, "0x");
  const onChainScript = resource.createScriptByData(onChainCell, "0x");
  const mainScript = resource.createScriptByType(mainCell, hexFrom(
    "0x0000" + // flag: NO file system
    onChainScript.codeHash.slice(2) +
    hexFrom(hashTypeToBytes(onChainScript.hashType)).slice(2),
  ))
  const inputCell = resource.mockCell(mainScript, undefined, "0x");

  const tx = Transaction.from({
    cellDeps: [
      Resource.createCellDep(mainCell, "code"),
      Resource.createCellDep(alwaysSuccessCell, "code"),
      Resource.createCellDep(onChainCell, "code"),
    ],
    headerDeps: [
      resource.mockHeader(
        createHeaderViewTemplate(),
        "0x00",
        [],
      ),
      resource.mockHeader(
        createHeaderViewTemplate(),
        "0x0000",
        [inputCell],
      ),
    ],
    inputs: [
      Resource.createCellInput(inputCell),
    ],
    outputs: [
      Resource.createCellOutput(alwaysSuccessScript),
    ],
    outputsData: [
      hexFrom("0x0001020304050607"),
    ],
    witnesses: [
      hexFrom("0x0001020304050607"),
    ]
  });

  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(true);
}

describe("file system", () => {
  test("syscalls success", () => {
    main("src/syscalls/index.js");
  });
});
