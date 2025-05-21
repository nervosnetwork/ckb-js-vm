# Require file extensions in import/export statements (`@ckb-js-std/require-import-extensions`)

<!-- end auto-generated rule header -->

This rule enforces that relative module paths in `import` and `export` statements include a valid file extension (e.g., `.js`, `.bc`).

## Rule Details

In the ckb-js-vm environment, file extensions are mandatory for resolving relative module imports. The [Working with ckb-js-vm documentation](https://github.com/nervosnetwork/ckb-js-vm/tree/main/docs/tutorial/src/ckb-js-vm.md#module-resolution-rules) states:

> When using file system mode, make sure your module structure follows ESM conventions with `.js` or `.bc` file extensions explicitly included in import statements.

This rule applies to relative paths (those starting with `./` or `../`) and ensures they end with an allowed extension, typically `.js` for JavaScript source files or `.bc` for precompiled bytecode files.

### ❌ Incorrect

```js
// Missing extension for a relative path
import { something } from './utils';
```

### ✅ Correct

```js
// With proper .js extensions
import { something } from './utils.js';

// .bc extension is also valid for bytecode files
import { bytecodeMain } from './compiled-script.bc';

// Built-in modules typically don't require extensions
import * as bindings from "@ckb-js-std/bindings";
```

## Module Resolution in ckb-js-vm

ckb-js-vm follows specific rules for module resolution:

1.  **Built-in modules** (e.g., `@ckb-js-std/bindings`) are resolved automatically without needing an extension.
2.  **Relative imports** (e.g., `./module.js`) *must* include the file extension.

## When Not To Use It

This rule should generally not be disabled for code intended to run in the ckb-js-vm environment, as explicit file extensions are crucial for correct module resolution.

If you have a specific build setup for a non-ckb-js-vm environment (e.g., a web project using a bundler that resolves extensions automatically) where this rule is not desired, you can disable it for those specific files or directories in your ESLint configuration. 
