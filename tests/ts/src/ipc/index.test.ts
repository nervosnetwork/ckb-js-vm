import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
} from "ckb-testtool";

import { SERVER_BYTECODE_PATH, CLIENT_BYTECODE_PATH } from "./build.cjs";

function main() {
  const resource = Resource.default();
  const tx = Transaction.default();

  const mainScript = resource.deployCell(
    hexFrom(readFileSync("../../build/ckb-js-vm")),
    tx,
    true,
  );
  // console.log("ckb-js-vm script:", JSON.stringify(mainScript));
  const alwaysSuccessScript = resource.deployCell(
    hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)),
    tx,
    false,
  );
  const clientCellScript = resource.deployCell(
    hexFrom(readFileSync(CLIENT_BYTECODE_PATH)),
    tx,
    true,
  );
  const serverCellScript = resource.deployCell(
    hexFrom(readFileSync(SERVER_BYTECODE_PATH)),
    tx,
    true,
  );
  // console.log("server script:", JSON.stringify(serverCellScript));
  mainScript.args = hexFrom(
    "0x0000" +
      clientCellScript.codeHash.slice(2) +
      hexFrom(hashTypeToBytes(clientCellScript.hashType)).slice(2),
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
  test("ipc", () => {
    main();
  });
});
