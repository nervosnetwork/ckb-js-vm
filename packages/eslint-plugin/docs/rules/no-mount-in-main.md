# Disallow `bindings.mount` calls in main entry files (`@ckb-js-std/no-mount-in-main`)

<!-- end auto-generated rule header -->

This rule discourages `bindings.mount` calls in main entry files (e.g., `index.js`, `index.ts`) to prevent issues related to JavaScript module hoisting. It recommends placing such calls in `init.js` or `init.bc` files instead.

## Rule Details

In ckb-js-vm, JavaScript's [hoisting behavior](https://developer.mozilla.org/en-US/docs/Glossary/Hoisting) processes `import` statements before other code in the same module. If `bindings.mount` is called in the main entry file alongside imports that depend on the mounted filesystem, those imports may fail because the filesystem isn't available yet.

The [Simple File System and Modules documentation for ckb-js-vm](https://github.com/nervosnetwork/ckb-js-vm/tree/main/docs/tutorial/src/file-system.md#using-initbcinitjs-files) explains:

> Due to JavaScript's hoisting behavior, import statements are processed before other code executes. [...] This will fail because the import attempts to access files before the file system is mounted. To solve this problem, place the `bindings.mount` statement in an `init.bc` or `init.js` file, which will execute before any imports are processed in the main file.

This rule enforces this pattern by disallowing `bindings.mount` in main entry files.

### ❌ Incorrect

If `index.js` contains:

```js
// In index.js
import * as bindings from "@ckb-js-std/bindings";
import { someFunction } from "./my-module.js"; // Depends on a mounted filesystem

// Attempt to mount the filesystem
bindings.mount(2, bindings.SOURCE_CELL_DEP, "/");

// Use the imported function
someFunction();

// Rest of the code...
```

Due to hoisting, the code is effectively processed as:

```js
// Hoisted imports first
import * as bindings from "@ckb-js-std/bindings";
import { someFunction } from "./my-module.js"; // This will likely fail because the filesystem isn't mounted yet at the time of import resolution.

// Then the mount operation
bindings.mount(2, bindings.SOURCE_CELL_DEP, "/");

someFunction();
// Rest of the code...
```

### ✅ Correct

Separate the mount operation into an `init.js` (or `init.bc`) file:

```js
// In init.js
import * as bindings from "@ckb-js-std/bindings";

bindings.mount(2, bindings.SOURCE_CELL_DEP, "/");
console.log("Filesystem mounted from init.js");
```

```js
// In index.js
import * as bindings from "@ckb-js-std/bindings";
// This import will now succeed as init.js (and thus the mount) executes first.
import { someFunction } from "./my-module.js";

function main() {
  console.log("Executing main function from index.js");
  someFunction();
  return 0;
}

bindings.exit(main());
```

**Note:** Ensure your build process and deployment include the `init.js` or `init.bc` file and that ckb-js-vm is configured to load it.

## When Not To Use It

While generally recommended, you might consider disabling this rule if:

- Your main entry file has no imports that depend on the mounted filesystem.
- You have a different mechanism to ensure filesystems are mounted before any dependent modules are resolved (though using `init.js`/`init.bc` is the standard approach).

However, for most scenarios, adhering to this rule by using an initialization file leads to more predictable and reliable behavior in ckb-js-vm.
