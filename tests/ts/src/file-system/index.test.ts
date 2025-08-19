import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
} from "ckb-testtool";

import { OUTPUT_FS, OUTPUT_FS_2, OUTPUT_FS_BC } from "./build.cjs";

async function runFileSystem(path: string) {
  const resource = Resource.default();
  const alwaysSuccessCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
  const alwaysSuccessScript = resource.createScriptByData(alwaysSuccessCell, "0x");
  const fileSystemCell = resource.mockCellAsCellDep(hexFrom(readFileSync(path)));
  // the position of the file system should be at index 2
  const fileSystemScript = resource.createScriptByData(fileSystemCell, "0x");
  const mainCell = resource.mockCell(
    resource.createScriptUnused(),
    undefined,
    hexFrom(readFileSync("../../build/ckb-js-vm")),
  );
  // flag: enable file system
  const mainScript = resource.createScriptByData(mainCell, hexFrom(
    "0x0100" +
    fileSystemScript.codeHash.slice(2) +
    hexFrom(hashTypeToBytes(fileSystemScript.hashType)).slice(2),
  ));
  const inputCell = resource.mockCell(mainScript, undefined, "0x");

  const tx = Transaction.from({
    cellDeps: [
      Resource.createCellDep(alwaysSuccessCell, "code"),
      Resource.createCellDep(mainCell, "code"),
      Resource.createCellDep(fileSystemCell, "code"),
    ],
    inputs: [
      Resource.createCellInput(inputCell),
    ],
    outputs: [
      Resource.createCellOutput(alwaysSuccessScript),
    ],
    outputsData: [
      hexFrom("0x"),
    ]
  })

  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(false);
}

describe("file system", () => {
  test("javascript success", () => {
    runFileSystem(OUTPUT_FS);
  });
  test("bytecode success", () => {
    runFileSystem(OUTPUT_FS_BC);
  });
  test("loadJsScript/loadFile success", () => {
    runFileSystem(OUTPUT_FS_2);
  });
});
