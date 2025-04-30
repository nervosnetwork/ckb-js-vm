import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  MockInfoHeaderDep,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
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

  const headerTemp: MockInfoHeaderDep = {
    compact_target: "0x0",
    dao: "0x0000000000000000000000000000000000000000000000000000000000000000",
    epoch: "0x0",
    extra_hash:
      "0x0000000000000000000000000000000000000000000000000000000000000000",
    hash: "0x00000000000000000000000000000000",
    nonce: "0x0",
    number: "0x0",
    parent_hash:
      "0x0000000000000000000000000000000000000000000000000000000000000000",
    proposals_hash:
      "0x0000000000000000000000000000000000000000000000000000000000000000",
    timestamp: "0x0",
    transactions_root:
      "0x0000000000000000000000000000000000000000000000000000000000000000",
    version: "0x0",
  };
  const headerHashByHeaderDep = resource.mockHeader(
    JSON.parse(JSON.stringify(headerTemp)),
  );
  resource.mockExtension(headerHashByHeaderDep, "0x00");
  tx.headerDeps.push(headerHashByHeaderDep);
  const headerHashByInput = resource.mockHeader(
    JSON.parse(JSON.stringify(headerTemp)),
  );
  resource.mockExtension(headerHashByInput, "0x0000");
  resource.bindCellWithHeader(inputCell, headerHashByInput);
  tx.headerDeps.push(headerHashByInput);

  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(true);
}

describe("file system", () => {
  test("syscalls success", () => {
    main("src/syscalls/index.js");
  });
});
