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
  const mainCell = resource.mockCell(
    resource.createScriptUnused(),
    resource.createScriptTypeID(),
    hexFrom(readFileSync("../../build/ckb-js-vm")),
  );
  const alwaysSuccessCell = resource.mockCell(
    resource.createScriptUnused(),
    undefined,
    hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)),
  );
  const alwaysSuccessScript = resource.createScriptByData(alwaysSuccessCell, "0x");
  const clientCell = resource.mockCell(
    resource.createScriptUnused(),
    resource.createScriptTypeID(),
    hexFrom(readFileSync(CLIENT_BYTECODE_PATH)),
  );
  const clientCellScript = resource.createScriptByType(clientCell, "0x");
  const serverCell = resource.mockCell(
    resource.createScriptUnused(),
    resource.createScriptTypeID(),
    hexFrom(readFileSync(SERVER_BYTECODE_PATH)),
  );
  const mainScript = resource.createScriptByType(mainCell, hexFrom(
    "0x0000" +
    clientCellScript.codeHash.slice(2) +
    hexFrom(hashTypeToBytes(clientCellScript.hashType)).slice(2),
  ));

  const inputCell = resource.mockCell(mainScript, undefined, "0x");

  const tx = Transaction.from({
    cellDeps: [
      Resource.createCellDep(mainCell, "code"),
      Resource.createCellDep(alwaysSuccessCell, "code"),
      Resource.createCellDep(clientCell, "code"),
      Resource.createCellDep(serverCell, "code"),
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
  test("ipc", () => {
    main();
  });
});
