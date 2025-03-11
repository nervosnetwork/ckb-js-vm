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
