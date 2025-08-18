import {
  hexFrom,
  Transaction,
  SignerCkbPrivateKey,
  ClientPublicMainnet,
  WitnessArgs,
  hashCkb,
  hashTypeToBytes,
} from "@ckb-ccc/core";
import { readFileSync } from "fs";
import { Resource, UnitTestClient, Verifier } from "ckb-testtool";

async function main() {
  const privateKey = new SignerCkbPrivateKey(
    new ClientPublicMainnet(),
    "0x0000000000000000000000000000000000000000000000000000000000000001",
  );
  const pubkey = privateKey.publicKey;

  const resource = Resource.default();
  const lockCell = resource.mockCell(
    resource.createScriptUnused(),
    undefined,
    hexFrom(readFileSync("./dist/secp256k1_blake160_lock.bc")),
  );
  const lockScript = resource.createScriptByData(lockCell, "0x");
  const mainCell = resource.mockCell(
    resource.createScriptUnused(),
    undefined,
    hexFrom(readFileSync("../../build/ckb-js-vm")),
  );
  const mainScript = resource.createScriptByData(mainCell, hexFrom(
    "0x0000" +
    lockScript.codeHash.slice(2) +
    hexFrom(hashTypeToBytes(lockScript.hashType)).slice(2) +
    hashCkb(pubkey).slice(2, 42),
  ));
  const inputCell = resource.mockCell(mainScript);

  const tx = Transaction.from({
    cellDeps: [
      Resource.createCellDep(lockCell, "code"),
      Resource.createCellDep(mainCell, "code"),
    ],
    inputs: [
      Resource.createCellInput(inputCell),
    ],
    outputs: [
      Resource.createCellOutput(mainScript),
      Resource.createCellOutput(mainScript),
    ],
    outputsData: [
      hexFrom("0x00"),
      hexFrom("0x01"),
    ],
    witnesses: [
      hexFrom(
        new WitnessArgs(
          hexFrom(new Uint8Array(65)),
          undefined,
          undefined,
        ).toBytes(),
      )
    ],
  })

  let mh = await tx.getSignHashInfo(mainScript, new UnitTestClient(resource))!;
  let signature = await privateKey._signMessage(mh!.message);
  tx.witnesses[0] = hexFrom(
    new WitnessArgs(signature, undefined, undefined).toBytes(),
  );
  const verifier = Verifier.from(resource, tx);
  verifier.verifySuccess(true);
}

describe("secp256k1_blake160_lock", () => {
  test("success", () => {
    main();
  });
});
