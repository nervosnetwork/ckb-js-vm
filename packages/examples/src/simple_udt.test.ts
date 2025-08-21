import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
} from "ckb-testtool";

async function main() {
  const resource = Resource.default();
  const alwaysSuccessCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
  const alwaysSuccessScript = resource.createScriptByData(alwaysSuccessCell, "0x");
  const sudtCell = resource.mockCellAsCellDep(hexFrom(readFileSync("./dist/simple_udt.bc")));
  const sudtScript = resource.createScriptByData(sudtCell, "0x");
  const mainCell = resource.mockCell(
    resource.createScriptUnused(),
    undefined,
    hexFrom(readFileSync("../../build/ckb-js-vm"))
  );
  const mainScript = resource.createScriptByData(mainCell, hexFrom(
    "0x0000" +
    sudtScript.codeHash.slice(2) +
    hexFrom(hashTypeToBytes(sudtScript.hashType)).slice(2) +
    "0000000000000000000000000000000000000000000000000000000000000000",
  ))
  const inputCell = resource.mockCell(alwaysSuccessScript, mainScript, "0xFF000000000000000000000000000000");

  const tx = Transaction.from({
    cellDeps: [
      Resource.createCellDep(alwaysSuccessCell, "code"),
      Resource.createCellDep(sudtCell, "code"),
      Resource.createCellDep(mainCell, "code"),
    ],
    inputs: [
      Resource.createCellInput(inputCell),
    ],
    outputs: [
      Resource.createCellOutput(alwaysSuccessScript, mainScript),
      Resource.createCellOutput(alwaysSuccessScript, mainScript),
    ],
    outputsData: [
      hexFrom("0xFE000000000000000000000000000000"),
      hexFrom("0x01000000000000000000000000000000"),
    ],
  })

  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(true);
}

describe("simple_udt", () => {
  test("success", () => {
    main();
  });
});
