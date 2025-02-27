import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
} from "ckb-testtool";

async function main() {
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
  const sudtScript = resource.deployCell(
    hexFrom(readFileSync("./dist/simple_udt.bc")),
    tx,
    false,
  );
  mainScript.args = hexFrom(
    "0x0000" +
      sudtScript.codeHash.slice(2) +
      hexFrom(hashTypeToBytes(sudtScript.hashType)).slice(2) +
      "0000000000000000000000000000000000000000000000000000000000000000",
  );
  // 1 input cell
  const inputCell = resource.mockCell(
    alwaysSuccessScript,
    mainScript,
    "0xFF000000000000000000000000000000",
  );
  tx.inputs.push(Resource.createCellInput(inputCell));

  // 2 output cells
  tx.outputs.push(Resource.createCellOutput(alwaysSuccessScript, mainScript));
  tx.outputsData.push(hexFrom("0xFE000000000000000000000000000000"));
  tx.outputs.push(Resource.createCellOutput(alwaysSuccessScript, mainScript));
  tx.outputsData.push(hexFrom("0x01000000000000000000000000000000"));

  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(true);
}

describe("simple_udt", () => {
  test("success", () => {
    main();
  });
});
