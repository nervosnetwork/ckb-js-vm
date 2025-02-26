A TypeScript helper library to write unit tests for on-chain CKB scripts.

## Introduction

ckb-testtool provides a testing framework that allows you to test CKB scripts without deploying them to a real network. This library helps you:

- Mock cells, blocks, and transactions for testing
- Simulate transaction verification
- Create and deploy test scripts
- Verify script success and failure cases

## Prerequisites

You need to install [ckb-debugger](https://github.com/nervosnetwork/ckb-standalone-debugger) which is used by this library to verify transactions.

## Usage

```typescript
import {
    DEFAULT_SCRIPT_ALWAYS_SUCCESS,
    Resource,
    Verifier,
} from "ckb-testtool";

import { hexFrom, Transaction } from "@ckb-ccc/core";
import { readFileSync } from "fs";

const resource = Resource.default();
const tx = Transaction.default();

// deploy a cell with risc-v binary, return a script.
const lockScript = resource.deployCell(
    hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)),
    tx,
    false,
);
// update args
lockScript.args = "0xEEFF";

// mock a input cell with the created script as lock script
const inputCell = resource.mockCell(lockScript);

// add input cell to the transaction
tx.inputs.push(Resource.createCellInput(inputCell));
// add output cell to the transaction
tx.outputs.push(Resource.createCellOutput(lockScript));
// add output data to the transaction
tx.outputsData.push(hexFrom("0x"));

// verify the transaction
const verifier = Verifier.from(resource, tx);
verifier.verifySuccess(true);
```
