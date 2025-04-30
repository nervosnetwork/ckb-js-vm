import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
  createMockInfoHeaderDepTemplate,
} from "ckb-testtool";

async function main(path: string) {
  const resource = Resource.default();
  const tx = Transaction.default();

  const mainScript = resource.deployCell(
    hexFrom(readFileSync("../../build/ckb-js-vm")),
    tx,
    true,
  );
  const alwaysSuccessScript = resource.deployCell(
    hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)),
    tx,
    false,
  );
  const onChainScript = resource.deployCell(
    hexFrom(readFileSync(path)),
    tx,
    false,
  );
  // flag: NO file system
  mainScript.args = hexFrom(
    "0x0000" +
      onChainScript.codeHash.slice(2) +
      hexFrom(hashTypeToBytes(onChainScript.hashType)).slice(2),
  );
  // 1 input cell
  const inputCell = resource.mockCell(mainScript, undefined, "0x");
  tx.inputs.push(Resource.createCellInput(inputCell));

  // 1 output cell
  tx.outputs.push(Resource.createCellOutput(alwaysSuccessScript));
  tx.outputsData.push(hexFrom("0x0001020304050607"));

  tx.witnesses.push(hexFrom("0x0001020304050607"));

  const headerHashByHeaderDep = resource.mockHeader(
    createMockInfoHeaderDepTemplate(),
    "0x00",
    [],
  );
  tx.headerDeps.push(headerHashByHeaderDep);
  const headerHashByInput = resource.mockHeader(
    createMockInfoHeaderDepTemplate(),
    "0x0000",
    [inputCell],
  );
  tx.headerDeps.push(headerHashByInput);

  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(true);
}

describe("file system", () => {
  test("syscalls success", () => {
    main("src/syscalls/index.js");
  });
});
