# Core Concepts Explained

In the last chapter, we learned how to use the tool to create a blank project. In this chapter, we'll explain how it works.

Let's first examine the `pnpm build` command in the `package.json` file:

```bash
tsc --noEmit && esbuild --platform=neutral --minify --bundle --external:@ckb-js-std/bindings --target=es2022 src/index.ts --outfile=dist/index.js && ckb-debugger --read-file dist/index.js --bin ../../build/ckb-js-vm -- -c dist/index.bc
```

This can be split into 3 distinct commands:

```bash
tsc --noEmit
esbuild --platform=neutral --minify --bundle --external:@ckb-js-std/bindings --target=es2022 src/index.ts --outfile=dist/index.js
ckb-debugger --read-file dist/index.js --bin ../../build/ckb-js-vm -- -c dist/index.bc
```

## Build Process Breakdown

### 1. TypeScript Type Checking
The first command performs type checking on TypeScript code. This helps catch syntax errors early in the development process.
At this stage, no code is output.

### 2. JavaScript Bundling
The second command uses esbuild to bundle the code:
- `--minify` minimizes the generated code size, which is critical since larger storage costs more money on ckb-vm
- `--external:@ckb-js-std/bindings` tells esbuild to skip this dependency, as it's just a binding from JavaScript to C with no
  JavaScript implementation
- `--target=es2022` sets the target to ES2022, which QuickJS supports
- The final output is `dist/index.js`, which contains all the code needed to run

While we could run this JavaScript file directly with `ckb-js-vm`, the performance wouldn't be optimal. The next step improves
performance and further minimizes code size.

It's perfectly fine to switch to other bundling tools if you prefer. The only constraint is that the output .js file must be able to run without external dependencies.

### 3. Bytecode Compilation
The third command converts JavaScript code into QuickJS bytecode:
- `ckb-debugger` is a ckb-vm runner and debugger that can read and write local files (note that on a real CKB node, ckb-vm
  cannot do this)
- We've implemented a special feature in the `ckb-js-vm` binary to compile JavaScript code into QuickJS bytecode
- This approach ensures the generated code is always compatible with the on-chain script
- The final output is `dist/index.bc`, which is the binary that will be deployed and used

## Testing

The `pnpm test` command runs Jest for unit testing. During this phase:
- The binary `ckb-js-vm` and `dist/index.bc` are used
- The `.ts` and `.js` files are not involved
- It uses the `ckb-testtool` package, which we'll explain in a later chapter
