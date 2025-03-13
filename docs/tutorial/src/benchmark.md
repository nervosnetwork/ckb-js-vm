# Performance and Bytecode Size Benchmarks

The performance and bytecode size are critical factors for on-chain scripts in the CKB ecosystem.
Understanding these constraints helps developers build efficient and deployable solutions:

- **Execution Cycles**: Each transaction has a limited cycle budget (execution time). If a script
  exceeds this limit, the transaction cannot be processed by the network.

- **Bytecode Size**: On-chain storage is expensive and limited. Each cell has a size limit of
  approximately 500KB, making code optimization essential.

Below are benchmark examples of on-chain scripts with their respective sizes and cycle consumption:

| Script | Size | Cycles |
| ------ | ---- | ------ |
| [secp256k1/blake2b](https://github.com/nervosnetwork/ckb-js-vm/blob/main/packages/examples/src/secp256k1_blake160_lock.ts) | 26KB | 14M cycles |
| [simple_udt](https://github.com/nervosnetwork/ckb-js-vm/blob/main/packages/examples/src/simple_udt.ts) | 26KB | 12M cycles |
| [silent berry AccountBook](https://github.com/ksleifjsslsls/silent-berry2/tree/re-js/ts) | 70KB | 20M-40M cycles |

These benchmarks can help you gauge the resource requirements when developing your own ckb-js-vm scripts.

## Bytecode Size Impacts Performance

Unlike languages such as C and Rust where binary size has minimal impact on runtime performance, in
ckb-js-vm the bytecode size directly affects execution efficiency. Our testing reveals that larger
bytecode significantly increases ckb-js-vm boot time, potentially consuming 20-30M cycles.

This insight provides a valuable optimization strategy: reducing your overall bundle size delivers dual
benefits of smaller bytecode footprint and improved performance. When developing on-chain scripts for ckb-js-vm,
code size optimization should be considered a performance optimization as well.

