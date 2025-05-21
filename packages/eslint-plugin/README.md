# `@ckb-js-std/eslint-plugin`

ESLint plugin providing rules for the CKB JavaScript VM environment. This plugin helps enforce best practices when developing on-chain scripts for the CKB blockchain using the ckb-js-vm runtime.

## Installation

```bash
npm install --save-dev @ckb-js-std/eslint-plugin
```

## Usage

Add `@ckb-js-std` to the plugins section of your ESLint configuration:

```js
// eslint.config.mjs
import { defineConfig } from "eslint/config";
import ckbJsStd from "@ckb-js-std/eslint-plugin";

export default defineConfig([
  {
    files: ["**/*.{js,ts}"],
    plugins: { ckbJsStd },
    extends: ["ckbJsStd/recommended"],
  },
]);
```

Or manually configure rules:

```js
// eslint.config.mjs
import ckbJsStd from "@ckb-js-std/eslint-plugin";

export default [
  {
    plugins: {
      "@ckb-js-std": ckbJsStd,
    },
    rules: {
      "@ckb-js-std/enforce-bindings-exit-main": "error",
      "@ckb-js-std/no-mount-in-main": "error",
      "@ckb-js-std/no-commonjs-modules": "error",
      "@ckb-js-std/require-import-extensions": "error",
      "@ckb-js-std/no-eval-js-script": "error",
    },
  },
];
```

## Rules

<!-- begin auto-generated rules list -->

| Name                                                                   | Description                                                                                                                                |
| :--------------------------------------------------------------------- | :----------------------------------------------------------------------------------------------------------------------------------------- |
| [enforce-bindings-exit-main](docs/rules/enforce-bindings-exit-main.md) | Enforce that `index.ts` (or `index.js`) exits via a top-level `bindings.exit(main());` call.                                               |
| [no-mount-in-main](docs/rules/no-mount-in-main.md)                     | Disallow `bindings.mount` calls in main entry files (e.g., `index.js`, `index.bc`). Suggest using `init.js` or `init.bc` instead.          |
| [no-commonjs-modules](docs/rules/no-commonjs-modules.md)               | Disallow CommonJS module patterns (`require`, `module.exports`, `exports.xxx`) as ckb-js-vm exclusively supports ECMAScript Modules (ESM). |
| [require-import-extensions](docs/rules/require-import-extensions.md)   | Ensure relative module paths in import/export statements include a valid extension (.js or .bc).                                           |
| [no-eval-js-script](docs/rules/no-eval-js-script.md)                   | Disallow the use of `bindings.evalJsScript()` due to significant security risks when loading code from untrusted sources.                  |

<!-- end auto-generated rules list -->

## Why these rules matter

The ckb-js-vm environment has specific requirements that differ from traditional JavaScript applications:

1. **Module System**: ckb-js-vm exclusively supports ECMAScript Modules (ESM) and does not support CommonJS
2. **File System Management**: Mounting operations must be performed in initialization files to avoid race conditions
3. **Exit Handling**: Scripts need to properly terminate with the correct exit code using `bindings.exit(main())`
4. **Security Concerns**: Dynamically loading code with `evalJsScript()` introduces significant security risks
5. **Import Resolution**: File extensions are mandatory in import statements for proper resolution

Following these rules ensures that your on-chain scripts will run reliably in the CKB-VM environment.
