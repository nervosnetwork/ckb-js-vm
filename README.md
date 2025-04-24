# ckb-js-vm

A JavaScript runtime environment for CKB-VM, built by adapting [QuickJS](https://bellard.org/quickjs/). This project consists of two main components:

1. **ckb-js-vm**: An on-chain script runtime engine that executes JavaScript code or bytecode
2. **ckb-js-std**: TypeScript packages providing helper utilities for writing on-chain script

## Prerequisites

Ensure you have the following installed:

- [pnpm](https://pnpm.io/), >= 10.4
- [ckb-debugger](https://github.com/nervosnetwork/ckb-standalone-debugger), >= v0.200.1
- clang-18


## Quick Start with create-ckb-js-vm-app (Recommended)

```bash
pnpm create ckb-js-vm-app
```


## Documentation
  - [The Little Book of ckb-js-vm](./docs/tutorial/src/SUMMARY.md) - Step-by-step instructions for beginners
  - Run `pnpm run docs` to generate the latest API documentation locally (output will be available in the `typedoc` folder)
  - [Examples](./packages/examples/README.md) - Sample projects demonstrating key features

## Install Manually

### Building ckb-js-vm (On-chain Script)

```shell
git submodule update --init
make all
```

### Building ckb-js-std (TypeScript Packages)

```shell
pnpm install
pnpm build
```

## Getting Started

To run the example project:

```shell
cd packages/examples
pnpm run start
```
