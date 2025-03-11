# Working with ckb-js-vm

The `ckb-js-vm` is the binary name of an on-chain script that integrates QuickJS with additional glue code. It
functions similarly to the `node` binary as a JavaScript engine and runtime. However, compared to `node`, it has more
limited capabilities and is designed to run specifically in the CKB-VM environment. During development, you can run it
on your local machine using `ckb-debugger`.

## ckb-js-vm Command Line Options

When an on-chain script is invoked by `exec` or `spawn` syscalls, it can accept command line arguments. The
ckb-js-vm supports the following options to control its execution behavior:

- `-c <filename>`: Compile JavaScript source code to bytecode, making it more efficient for on-chain execution
- `-e <code>`: Execute JavaScript code directly from the command line string
- `-r <filename>`: Read and execute JavaScript code from the specified file
- `-t <target>`: Specify the target resource cell's code_hash and hash_type in hexadecimal format
- `-f`: Enable file system mode, which provides support for JavaScript modules and imports

Note, the `-c` and `-r` options can only work with `ckb-debugger`.  The `-c` option is particularly useful for preparing
optimized bytecode as described in the previous chapter. When no options are specified, ckb-js-vm runs in its default
mode. These command line options provide valuable debugging capabilities during development.

## ckb-js-vm `args` Explanation

The `ckb-js-vm` script structure in molecule is below:
```
code_hash: <code hash of ckb-js-vm, 32 bytes>
hash_type: <hash type of ckb-js-vm, 1 byte>
args: <ckb-js-vm flags, 2 bytes> <code hash of resource cell, 32 bytes> <hash type of resource cell, 1 byte>
```

The first 2 bytes are parsed into an `int16_t` in C using little-endian format (referred to as ckb-js-vm flags). If
the lowest bit of these flags is set (`v & 0x01 == 1`), the file system is enabled. File system functionality will be
described in another chapter.

The subsequent `code_hash` and `hash_type` point to a resource cell which may contain:
1. A file system
2. JavaScript source code
3. QuickJS bytecode

When the file system flag is enabled, the resource cell contains a file system that can also include JavaScript code.
For most scenarios, QuickJS bytecode is stored in the resource cell. When an on-chain script requires extra `args`,
they can be stored beginning at offset 35 (2 + 32 + 1). Compared to normal on-chain scripts in other languages,
ckb-js-vm requires these extra 35 bytes.

## QuickJS Integration

ckb-js-vm is built on [QuickJS](https://bellard.org/quickjs/), a small and embeddable JavaScript engine developed by
Fabrice Bellard. QuickJS features:

- Fast and lightweight JavaScript interpreter
- Support for ES2022 features
- Small footprint suitable for embedded systems
- Efficient memory management

ckb-js-vm leverages QuickJS to provide a JavaScript runtime environment within the CKB. This integration enables:

- Running JavaScript code directly on CKB-VM
- Compiling JavaScript to bytecode for more efficient execution
- Calling syscalls

## Bindings

ckb-js-vm provides bindings that allow JavaScript code to interact with the CKB blockchain through the
`@ckb-js-std/bindings` module. These bindings expose CKB syscalls and other functionality to JavaScript:

- Syscalls defined in the [RFC](https://github.com/nervosnetwork/rfcs)
- Hashing functions: SHA2-256, Keccak256, Blake2b, RIPEMD-160
- Cryptographic algorithms: secp256k1, Schnorr
- Miscellaneous functions: hex, base64, and [SMT](https://github.com/nervosnetwork/sparse-merkle-tree) (Sparse Merkle Tree)

