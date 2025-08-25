# Get Started

Before we introduce the core concepts and detailed implementation, let's familiarize ourselves with the basic workflow
of developing on-chain scripts with ckb-js-vm.

We can use the scaffolding tool [create-ckb-js-vm-app](https://www.npmjs.com/package/create-ckb-js-vm-app) to quickly
set up a new project. Before we start, make sure the following tools are installed:
- [pnpm](https://pnpm.io/)
- [ckb-debugger](https://github.com/nervosnetwork/ckb-standalone-debugger)

```bash
pnpm create ckb-js-vm-app
```

When prompted, choose the default project name: `my-ckb-script`. After a few moments, your project will be created.

## Project Structure

Let's explore the project structure to understand its components:

- `packages/on-chain-script` - Contains the TypeScript code that will be compiled and deployed on the CKB blockchain
- `packages/on-chain-script-tests` - Contains the off-chain TypeScript code for testing your on-chain script. Note that a
single test project can test multiple output files from different on-chain script projects. That's why there is a
dedicated test project.


## Building and Testing

To build your project, run:

```bash
pnpm build
```

This command compiles your TypeScript code and prepares the on-chain script.

To run the test suite:

```bash
pnpm test
```

The tests will verify that your on-chain script behaves as expected in various scenarios.

## Key Output Files

After building your project, two important files are generated:

- `packages/on-chain-script/dist/index.js` - The bundled JavaScript code compiled from your TypeScript source
- `packages/on-chain-script/dist/index.bc` - The bytecode representation of your script, which is significantly smaller
  and more efficient. This is the file that will be deployed on-chain.

The build process is defined in `packages/on-chain-script/package.json`, which configures [esbuild](https://esbuild.github.io/)
and ckb-debugger to transform your TypeScript code into deployable bytecode. We'll explore these tools in more detail in
later chapters.
