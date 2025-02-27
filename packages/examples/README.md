# Example on-chain scripts written in TypeScript

## Examples

- `src/index.ts`: Various usages of `@ckb-js-std/bindings` and `@ckb-js-std/core`
- `src/simple_udt.ts`: Simple UDT implementation
- `src/secp256k1_blake160_lock.ts`: Secp256k1/blake160 lock script implementation

## How to Run

To run and test src/index.ts:

```bash
pnpm build
pnpm start
```

To run and test src/simple_udt.ts and src/secp256k1_blake160_lock.ts:

```bash
pnpm run build:lock
pnpm run build:simple_udt
pnpm test
```
