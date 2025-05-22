# Disallow CommonJS module patterns (`@ckb-js-std/no-commonjs-modules`)

<!-- end auto-generated rule header -->

This rule enforces the use of ECMAScript Modules (ESM) because ckb-js-vm exclusively supports ESM.

## Rule Details

The ckb-js-vm runtime environment exclusively supports ECMAScript Modules (ESM) for modern module handling and compatibility. As stated in the [Working with ckb-js-vm documentation](https://github.com/nervosnetwork/ckb-js-vm/tree/main/docs/tutorial/src/ckb-js-vm.md#javascript-module-system):

> ckb-js-vm exclusively supports ECMAScript Modules (ESM) and does not support CommonJS. This means you must use the modern ES import syntax for all module operations.

This rule prevents the use of CommonJS module patterns such as `require()`, `module.exports`, and `exports.xxx`.

### ❌ Incorrect

```js
// Using require()
const bindings = require("@ckb-js-std/bindings");

// Using module.exports
module.exports = { foo, bar };

// Using exports assignments
exports.foo = function () {};
```

### ✅ Correct

```js
// Using import statement
import * as bindings from "@ckb-js-std/bindings";

// Using named imports
import { hex } from "@ckb-js-std/bindings";

// Using export statement
export { foo, bar };

// Using named exports
export function foo() {}
```

## When Not To Use It

This rule should not be disabled if your code is intended to run in the ckb-js-vm environment, as CommonJS modules are not supported.

If you have specific files that are not intended for the ckb-js-vm environment (e.g., build scripts or test helpers running in Node.js), you can disable this rule for those files using ESLint's [configuration comments](https://eslint.org/docs/latest/use/configure/rules#disabling-rules) or by excluding them in your ESLint configuration.
