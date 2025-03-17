import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
} from "ckb-testtool";

import { OUTPUT_FS, OUTPUT_FS_BC } from "./build.cjs";

async function runFileSystem(path: string) {
  const resource = Resource.default();
  const tx = Transaction.default();

  const mainScript = resource.deployCell(
    hexFrom(readFileSync("../../build/ckb-js-vm")),
    tx,
    false,
  );
  const alwaysSuccessScript = resource.deployCell(
    hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)),
    tx,
    false,
  );
  // the position of the file system should be at index 2
  const fileSystemScript = resource.deployCell(
    hexFrom(readFileSync(path)),
    tx,
    false,
  );

  // flag: enable file system
  mainScript.args = hexFrom(
      "0x0100" +
      fileSystemScript.codeHash.slice(2) +
      hexFrom(hashTypeToBytes(fileSystemScript.hashType)).slice(2),
  );
  // 1 input cell
  const inputCell = resource.mockCell(
    mainScript,
    undefined,
    "0x",
  );
  tx.inputs.push(Resource.createCellInput(inputCell));

  // 1 output cell
  tx.outputs.push(Resource.createCellOutput(alwaysSuccessScript));
  tx.outputsData.push(hexFrom("0x"));

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
});
