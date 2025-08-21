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
// deploy a cell with risc-v binary, return a cell.
const lockCell = resource.mockCell(
  resource.createScriptUnused(),
  resource.createScriptTypeID(),
  hexFrom(readFileSync(DEFAULT_SCRIPT_ALWAYS_SUCCESS)),
);
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
```
