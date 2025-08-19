# Writing Effective Unit Tests

The mission of the ckb-js-vm project is to enable developers to write on-chain scripts using a single language:
TypeScript. In the previous chapter, we learned how to write on-chain scripts in TypeScript. This chapter will
demonstrate how you can also write unit tests in TypeScript, allowing you to use just one language for your entire
development workflow.

## ckb-testtool

While Rust developers have been using [ckb-testtool](https://github.com/nervosnetwork/ckb-testtool) for testing, we now
have a [TypeScript version of ckb-testtool](https://www.npmjs.com/package/ckb-testtool) available. This tool leverages
two important components:

1. [ccc](https://github.com/ckb-devrel/ccc) - A transaction assembler written in TypeScript
2. [ckb-debugger](https://github.com/nervosnetwork/ckb-standalone-debugger) - A debugger and execution environment

The workflow is straightforward:
- Use ccc to assemble transactions in TypeScript, outputting them in JSON format
- Use ckb-debugger to execute and validate these transactions
- Write assertions to verify the expected behavior

This combination provides a complete unit testing framework for CKB scripts written in TypeScript.

## Examples

```typescript
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
  });

  test("alwaysFailure", () => {
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
  });
});
```

In the example above, we're testing on-chain script with two test cases:
1. A test case which succeeds
2. A test case which fails

This pattern allows you to verify both the positive and negative cases for your script's validation logic, ensuring
robust behavior in all scenarios.

## Pre-compiled Test Binaries

To simplify testing, the ckb-js-vm project provides several pre-compiled binaries that you can use in your test cases:

1. **Always Success Script** - A script that always returns success (exit code 0)
   - Access via `DEFAULT_SCRIPT_ALWAYS_SUCCESS`

2. **Always Failure Script** - A script that always returns failure (exit code -1)
   - Access via `DEFAULT_SCRIPT_ALWAYS_FAILURE`

3. **ckb-js-vm Script** - The main ckb-js-vm runtime for testing TypeScript scripts
   - Access via `DEFAULT_SCRIPT_CKB_JS_VM`

These binaries can be imported directly in your tests without needing to compile them yourself, making it easier to
create test fixtures and validation scenarios.

Example usage:
```typescript
// Import the binary
const alwaysSuccessScript = hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS));

// Deploy it in your test
const lockScript = resource.deployCell(alwaysSuccessScript, tx, false);
```

> ⚠️ **SECURITY WARNING**: These pre-compiled binaries are intended for testing purposes only. Never deploy them in a
> production environment. For production use, always compile your scripts from source code to ensure security and
> integrity.
