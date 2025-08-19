import { hexFrom, Transaction, hashTypeToBytes } from "@ckb-ccc/core";
import { readFileSync } from "fs";
import {
  Resource,
  Verifier,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
  createHeaderViewTemplate,
} from "ckb-testtool";

import { BYTECODE_PATH } from "./build.cjs";

async function run(path: string) {
  const resource = Resource.default();
  const alwaysSuccessCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
  const alwaysSuccessScript = resource.createScriptByData(alwaysSuccessCell, "0x");
  const lockCell = resource.mockCellAsCellDep(hexFrom(readFileSync(path)));
  const lockScript = resource.createScriptByData(lockCell, "0x");
  const mainCell = resource.mockCell(
    resource.createScriptUnused(),
    undefined,
    hexFrom(readFileSync("../../build/ckb-js-vm")),
  );
  const mainScript = resource.createScriptByData(mainCell, hexFrom(
    "0x0000" +
    lockScript.codeHash.slice(2) +
    hexFrom(hashTypeToBytes(lockScript.hashType)).slice(2),
  ));
  const inputCell = resource.mockCell(
    mainScript,
    undefined,
    "0x",
  );

  let header = createHeaderViewTemplate();
  header.version = "0x0";
  header.compact_target = "0x1";
  header.timestamp = "0x2";
  header.number = "0x3";
  header.epoch = "0x4";
  header.parent_hash =
    "0x0000000000000000000000000000000000000000000000000000000000000005";
  header.transactions_root =
    "0x0000000000000000000000000000000000000000000000000000000000000006";
  header.proposals_hash =
    "0x0000000000000000000000000000000000000000000000000000000000000007";
  header.extra_hash =
    "0x0000000000000000000000000000000000000000000000000000000000000008";
  header.dao =
    "0x0000000000000000000000000000000000000000000000000000000000000009";
  header.nonce = "0xa";

  const tx = Transaction.from({
    cellDeps: [
      Resource.createCellDep(alwaysSuccessCell, "code"),
      Resource.createCellDep(lockCell, "code"),
      Resource.createCellDep(mainCell, "code"),
    ],
    headerDeps: [
      resource.mockHeader(header, "0x00", []),
    ],
    inputs: [
      Resource.createCellInput(inputCell),
    ],
    outputs: [
      Resource.createCellOutput(alwaysSuccessScript),
    ],
    outputsData: [
      hexFrom("0x"),
    ],
  })

  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(false);
}

describe("file system", () => {
  test("javascript success", () => {
    run(BYTECODE_PATH);
  });
});
