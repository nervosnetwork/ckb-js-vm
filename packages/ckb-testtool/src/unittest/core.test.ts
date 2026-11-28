import { hexFrom, Transaction } from "@ckb-ccc/core";
import assert from "assert";
import { readFileSync } from "fs";
import {
  DEFAULT_SCRIPT_ALWAYS_FAILURE,
  DEFAULT_SCRIPT_ALWAYS_SUCCESS,
  parseAllCycles,
  parseRunResult,
  Resource,
  UnitTestClient,
  Verifier,
} from "./index";

describe("example", () => {
  test("alwaysSuccess", async () => {
    const resource = Resource.default();
    // deploy a cell with risc-v binary, return a cell.
    const lockCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
    const lockScript = resource.createScriptByType(lockCell, "0xEEFF");
    // deploy a cell with always success lock.
    const inputCell = resource.mockCell(lockScript);

    const tx = Transaction.from({
      inputs: [Resource.createCellInput(inputCell)],
      cellDeps: [Resource.createCellDep(lockCell, "code")],
    });

    // verify the transaction
    const verifier = Verifier.from(resource, tx);
    await verifier.verifySuccess();
    // turn off console.log
    jest.spyOn(console, "log").mockImplementation(() => {});
    await expect(verifier.verifyFailure()).rejects.toThrow(
      "Transaction verification should fail. No verification failure occurred.",
    );
    jest.spyOn(console, "log").mockRestore();
  });
  test("should run script specified by code hash", async () => {
    const resource = Resource.default();
    const lockCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
    const lockScript = resource.createScriptByType(lockCell, "0xEEFF");
    const inputCell = resource.mockCell(lockScript);

    const tx = Transaction.from({
      inputs: [Resource.createCellInput(inputCell)],
      cellDeps: [Resource.createCellDep(lockCell, "code")],
    });

    // verify the transaction
    const verifier = Verifier.from(resource, tx);
    await verifier.verifySuccess(false, { codeHash: lockScript.hash() });
    await expect(
      verifier.verifyFailure(undefined, false, { codeHash: lockScript.hash() }),
    ).rejects.toThrow(
      "Transaction verification should fail. No verification failure occurred.",
    );
    await expect(
      verifier.verifySuccess(false, { codeHash: "0x00000" }),
    ).rejects.toThrow(
      "No scripts found to verify. Please check your configuration parameters and ensure scripts are present in the transaction.",
    );
  });

  test("alwaysFailure", async () => {
    const resource = Resource.default();
    const lockCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_FAILURE)));
    const lockScript = resource.createScriptByType(lockCell, "0xEEFF");
    const inputCell = resource.mockCell(lockScript);

    const tx = Transaction.from({
      inputs: [Resource.createCellInput(inputCell)],
      cellDeps: [Resource.createCellDep(lockCell, "code")],
    });

    const verifier = Verifier.from(resource, tx);
    await verifier.verifyFailure();
    await verifier.verifyFailure(-1);
    // turn off console.log
    jest.spyOn(console, "log").mockImplementation(() => {});
    await expect(verifier.verifySuccess()).rejects.toThrow(
      "Transaction verification failed. See details above.",
    );
    await expect(verifier.verifyFailure(2)).rejects.toThrow(
      "Transaction verification failed with unexpected error code: expected 2, got -1. See details above.",
    );
    jest.spyOn(console, "log").mockRestore();
  });

  test("parse", async () => {
    const resource = Resource.default();
    const lockCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_FAILURE)));
    const lockScript = resource.createScriptByType(lockCell, "0xEEFF");
    const inputCell = resource.mockCell(lockScript);

    const tx = Transaction.from({
      inputs: [Resource.createCellInput(inputCell)],
      cellDeps: [Resource.createCellDep(lockCell, "code")],
    });

    const verifier = Verifier.from(resource, tx);
    const result = await verifier.verify();
    expect(parseRunResult(result[0].stdout.toString())).toBe(-1);
    expect(parseAllCycles(result[0].stdout.toString())).toBe(539);
  });
  test("signHashInfo", async () => {
    const resource = Resource.default();
    const client = new UnitTestClient(resource);
    const lockCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
    const lockScript = resource.createScriptByType(lockCell, "0xEEFF");
    const inputCell = resource.mockCell(lockScript);

    const tx = Transaction.from({
      inputs: [Resource.createCellInput(inputCell)],
      cellDeps: [Resource.createCellDep(lockCell, "code")],
      witnesses: [
        // the format of witness should follow WitnessArgs
        hexFrom(
          "0x5500000010000000550000005500000041000000725e20eeee617616f881e65773fdb8d0f2d91619a71cfe18121f3fef67f9cfcb0c019c66ebf67ef2f41123443a786c554a5287ff2a3e92725fa14634c4f1550f01",
        ),
        // extra witness, anything is OK
        hexFrom("0x00112233445566778899aabbccddeeff"),
      ],
    });

    await tx.prepareSighashAllWitness(lockScript, 0, client);
    const sigHashAll = await tx.getSignHashInfo(lockScript, client);
    assert(sigHashAll?.message.length == 66);
    const verifier = Verifier.from(resource, tx);
    await verifier.verifySuccess();
  });

  test("alwaysSuccessWasmDebugger", async () => {
    const resource = Resource.default();
    const lockCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
    const lockScript = resource.createScriptByType(lockCell, "0xEEFF");
    const inputCell = resource.mockCell(lockScript);

    const tx = Transaction.from({
      inputs: [Resource.createCellInput(inputCell)],
      cellDeps: [Resource.createCellDep(lockCell, "code")],
    });

    const verifier = Verifier.from(resource, tx);
    verifier.setWasmDebuggerEnabled(true);
    await verifier.verifySuccess();
  });
});

describe("debugger path", () => {
  const savedPath = process.env.CKB_DEBUGGER_PATH;
  afterEach(() => {
    if (savedPath === undefined) {
      delete process.env.CKB_DEBUGGER_PATH;
    } else {
      process.env.CKB_DEBUGGER_PATH = savedPath;
    }
  });

  function alwaysSuccessTx(resource: Resource): Transaction {
    const lockCell = resource.mockCellAsCellDep(hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)));
    const lockScript = resource.createScriptByType(lockCell, "0xEEFF");
    const inputCell = resource.mockCell(lockScript);
    return Transaction.from({
      inputs: [Resource.createCellInput(inputCell)],
      cellDeps: [Resource.createCellDep(lockCell, "code")],
    });
  }

  test("defaults to ckb-debugger on PATH", () => {
    delete process.env.CKB_DEBUGGER_PATH;
    const resource = Resource.default();
    const verifier = Verifier.from(resource, alwaysSuccessTx(resource));
    expect(verifier.debugger).toBe("ckb-debugger");
  });

  test("reads the debugger path from CKB_DEBUGGER_PATH", () => {
    process.env.CKB_DEBUGGER_PATH = "/opt/ckb/bin/ckb-debugger";
    const resource = Resource.default();
    const verifier = Verifier.from(resource, alwaysSuccessTx(resource));
    expect(verifier.debugger).toBe("/opt/ckb/bin/ckb-debugger");
  });

  test("names the path tried when the debugger cannot be spawned", async () => {
    const missing = "/no/such/dir/ckb-debugger-missing";
    process.env.CKB_DEBUGGER_PATH = missing;
    const resource = Resource.default();
    const verifier = Verifier.from(resource, alwaysSuccessTx(resource));
    await expect(verifier.verifySuccess()).rejects.toThrow(
      `Failed to run ckb-debugger at "${missing}"`,
    );
    await expect(verifier.verifySuccess()).rejects.toThrow("ckb-debugger not found");
  });

  test("explains the .cmd shim limitation on Windows", () => {
    const error = Object.assign(new Error("spawnSync C:\\tools\\ckb-debugger.cmd EINVAL"), {
      code: "EINVAL",
    });
    const message = Verifier.spawnErrorMessage(error, "C:\\tools\\ckb-debugger.cmd", "win32");
    expect(message).toContain('Failed to run ckb-debugger at "C:\\tools\\ckb-debugger.cmd"');
    expect(message).toContain("EINVAL");
    expect(message).toContain("CKB_DEBUGGER_PATH");
    expect(message).toContain("ckb-debugger.exe");
    expect(message).not.toContain("not found");
  });

  test("points at CKB_DEBUGGER_PATH on other platforms", () => {
    const error = Object.assign(new Error("spawnSync ckb-debugger ENOENT"), { code: "ENOENT" });
    const message = Verifier.spawnErrorMessage(error, "ckb-debugger", "linux");
    expect(message).toContain("ckb-debugger not found");
    expect(message).toContain("CKB_DEBUGGER_PATH");
    expect(message).not.toContain(".cmd");
  });
});
