# ckb-js-vm On-Chain Script Project

This is an on-chain script project powered by [ckb-js-vm](https://github.com/nervosnetwork/ckb-js-vm), bootstrapped with [`create-ckb-js-vm-app`](https://github.com/nervosnetwork/ckb-js-vm).

## Getting Started

### Install dependencies

```bash
pnpm install
```

### Build on-chain scripts

```bash
pnpm build
```

**Note:** You can use `npm install` and `npm run build` if you chose npm when creating the project.

### Run tests

```bash
pnpm test
```

## Project Structure

```
├── packages/
│ ├── on-chain-script/ # source code
│ │ └── src/index.ts
│ └── on-chain-script-tests/ # Test suite
├── pnpm-workspace.yaml # pnpm monorepo config
├── package.json
└── README.md
```

## Reference

- [A little book of ckb-js-vm](https://nervosnetwork.github.io/ckb-js-vm/)
- [ckb-js-vm](https://github.com/nervosnetwork/ckb-js-vm)
- [ckb-debugger](https://github.com/nervosnetwork/ckb-standalone-debugger)
- [ccc](https://github.com/ckb-devrel/ccc)
- [@ckb-js-std@core](https://www.npmjs.com/package/@ckb-js-std/core)
- [ckb-js-vm Quick Start](https://docs.nervos.org/docs/script/js/js-quick-start#test)
