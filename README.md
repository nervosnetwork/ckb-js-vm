# ckb-js-vm

A JavaScript/TypeScript runtime environment for CKB-VM, built by adapting [QuickJS](https://bellard.org/quickjs/). This project consists of two main components:

1. **ckb-js-vm**: An on-chain script runtime engine that executes JavaScript code or bytecode
2. **ckb-js-std**: TypeScript packages providing helper utilities for writing on-chain script

## Prerequisites

Ensure you have the following installed:

- [pnpm](https://pnpm.io/)
- [ckb-debugger](https://github.com/nervosnetwork/ckb-standalone-debugger)
- clang-18


## Quick Start with create-ckb-js-vm-app (Recommended)

```bash
pnpm create ckb-js-vm-app
```


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
