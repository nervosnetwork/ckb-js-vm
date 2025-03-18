# ckb-js-std Library Reference

## Overview

The ckb-js-std ecosystem consists of two TypeScript libraries designed to work with ckb-js-vm:

1. `@ckb-js-std/bindings`
2. `@ckb-js-std/core`


## @ckb-js-std/bindings

This library provides low-level bindings to the C implementation of ckb-js-vm. It serves as the foundation layer
that enables JavaScript/TypeScript to interact with the underlying C code. Key characteristics:

- Contains declarations for binding functions to C implementations
- Has no TypeScript implementation of its own
- Primarily used as a dependency for higher-level libraries

## Errors thrown by bindings functions

It is possible that bindings functions throw exceptions that can be handled gracefully. The CKB VM defines
standard error codes that your code should be prepared to handle:

  ```
  CKB_INDEX_OUT_OF_BOUND 1
  CKB_ITEM_MISSING 2
  ```

Common scenarios where these errors occur:
- `CKB_INDEX_OUT_OF_BOUND (1)`: Occurs when iterating beyond available items, such as when looping over cells,
  scripts, witnesses, etc. This error is expected and should be caught to terminate iteration loops.
- `CKB_ITEM_MISSING (2)`: Occurs when a type script is missing. This can be a valid state in some on-chain scrips.

You can handle these exceptions by checking the `errorCode` property of the thrown exception. Here's an example
of properly handling the out-of-bounds case in an iterator:

  ```typescript
  next(): IteratorResult<T> {
    try {
      const item = this.queryFn(this.index, this.source);
      this.index += 1;
      return { value: item, done: false };
    } catch (err: any) {
      if (err.errorCode === bindings.INDEX_OUT_OF_BOUND) {
        // End iteration gracefully when we've reached the end of available items
        return { value: undefined, done: true };
      }
      // Re-throw any other errors with additional context
      throw new Error(`QueryIter error: ${err.message || err}`);
    }
  }
  ```

## @ckb-js-std/core

Built on top of `@ckb-js-std/bindings`, this library offers a more developer-friendly interface with:

- Enhanced TypeScript types for better code completion and error checking
- Higher-level utility functions that simplify common operations
- Abstractions that make working with ckb-js-vm more intuitive
- Recommended for most application development scenarios

The `@ckb-js-std/core` library contains several important sub-modules that provide specialized functionality:

- **HighLevel**: A convenient wrapper around the "bindings" module that simplifies common operations with an
  easy-to-use API.
- **hasher**: Provides cryptographic hashing functions essential for blockchain operations, including SHA256
  and Blake2b implementations.
- **log**: Contains logging utilities for debugging and monitoring your on-chain script during development and
  production.
- **molecule**: Implements [molecule](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0008-serialization/0008-serialization.md)
  serialization and deserialization, the standard data encoding format used in the CKB ecosystem.
- **num**: Offers utilities for serializing and deserializing numeric values, handling the conversion between
  JavaScript numbers and their binary representations.

We recommend exploring these sub-modules before starting your project to understand the full capabilities
available to you.

## Usage Recommendations

For most projects, we recommend using `@ckb-js-std/core` as it provides a more ergonomic developer experience
while maintaining access to the full capabilities of ckb-js-vm.

Only use `@ckb-js-std/bindings` directly when you need precise control over low-level operations or are
developing custom extensions to the ecosystem.

## CommonJS Modules (require)

For some scenarios, you might need to write code in JavaScript and use the CommonJS `require` syntax to load
modules. This can be done as follows for `@ckb-js-std/bindings`(already embedded in ckb-js-vm):

  ```js
  const bindings = require("@ckb-js-std/bindings");
  ```

However, we generally recommend using ES modules (import/export) instead of CommonJS for the following reasons:

- Better compatibility with modern JavaScript tooling
- Enables tree-shaking in bundling tools like esbuild
- Provides clearer static analysis for IDEs and type checking

For other library, you can do it as follows:
  ```typescript
  import * as core from '@ckb-js-std/core';
  globalThis.__ckb_core = core;
  require = function (name) {
  if (name === '@ckb-js-std/core') {
    return globalThis.__ckb_module_core; }\
      throw new Error('cannot find the module: ' + name);
  }
  ```
The [`globalThis`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/globalThis) global
property contains the global `this` value, which is usually akin to the global object.
