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
  const tx = Transaction.default();

  const mainScript = resource.deployCell(
    hexFrom(readFileSync("../../build/ckb-js-vm")),
    tx,
    false,
  );
  const lockScript = resource.deployCell(
    hexFrom(readFileSync("./dist/secp256k1_blake160_lock.bc")),
    tx,
    false,
  );
  mainScript.args = hexFrom(
    "0x0000" +
      lockScript.codeHash.slice(2) +
      hexFrom(hashTypeToBytes(lockScript.hashType)).slice(2) +
      hashCkb(pubkey).slice(2, 42),
  );
  // 1 input cell
  const inputCell = resource.mockCell(mainScript);
  tx.inputs.push(Resource.createCellInput(inputCell));

  // 2 output cells
  tx.outputs.push(Resource.createCellOutput(mainScript));
  tx.outputsData.push(hexFrom("0x00"));
  tx.outputs.push(Resource.createCellOutput(mainScript));
  tx.outputsData.push(hexFrom("0x01"));

  tx.witnesses.push(
    hexFrom(
      new WitnessArgs(
        hexFrom(new Uint8Array(65)),
        undefined,
        undefined,
      ).toBytes(),
    ),
  );

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
