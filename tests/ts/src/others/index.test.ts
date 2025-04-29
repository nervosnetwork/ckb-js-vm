import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
} from "ckb-testtool";

import { BYTECODE_PATH } from "./build.cjs";

async function run(path: string) {
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
  const script = resource.deployCell(hexFrom(readFileSync(path)), tx, false);

  // flag: enable file system
  mainScript.args = hexFrom(
    "0x0000" +
      script.codeHash.slice(2) +
      hexFrom(hashTypeToBytes(script.hashType)).slice(2),
  );
  // 1 input cell
  const inputCell = resource.mockCell(mainScript, undefined, "0x");
  tx.inputs.push(Resource.createCellInput(inputCell));

  // 1 output cell
  tx.outputs.push(Resource.createCellOutput(alwaysSuccessScript));
  tx.outputsData.push(hexFrom("0x"));

  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(false);
}

describe("file system", () => {
  test("javascript success", () => {
    run(BYTECODE_PATH);
  });
});
