# Stack Traces

When a contract execution fails, you rely on **stack traces** to locate the problem.
Because `ckb-js-vm` doesn’t currently support breakpoint debugging—and production bytecode/minified JS cannot be easily mapped back to original source lines—you should build a separate debug artifact for troubleshooting.

## Build

```shell
tsc --noEmit
esbuild src/index.ts \
    --bundle \
    --platform=neutral \
    --target=es2022 \
    --external:@ckb-js-std/bindings \
    --sourcemap=external \
    --sources-content \
    --keep-names \
    --minify=false \
    --define:DEBUG=true \
    --loader:.map=json \
    --outfile=dist/index.debug.js
```

Compared with the **release** build, these flags enable a more debugging-friendly output.
This produces `*.debug.js` and `*.debug.js.map`.
(The `create-app` scaffold already includes this setup—ready to use.)

## Capture and print errors

```ts
import logError from "@ckb-js-std/core";

try {
  main();
} catch (e) {
  logError(e);
}
```

This captures **unhandled** exceptions thrown inside `main` and prints them via `logError`.
The initial stack originates from `*.debug.js`. For precise source line/column mapping, the `*.debug.js.map` file is required.

## Enable source mapping in ckb-testtool

`ckb-testtool` has built-in stack remapping. Use it like this:

1. Enable `Resource` debug mode:

```ts
import Resource from "ckb-testtool";

const resource = Resource.default();
resource.enableDebug = true;
```

2. Create the JS contract with `createJSScript` (**you must** create it this way to enable source mapping and correct path resolution):

```ts
const mainScript = resource.createJSScript(
  "../on-chain-script/dist/index.bc",
  "0x"
);
```

> You pass a `*.bc` path here. When `enableDebug` is `true` and `../on-chain-script/dist/index.debug.js` exists, `*.debug.js` **takes precedence**; otherwise it falls back to `index.bc`.

3. Before execution, call `completeTx` to inject script dependencies into the transaction:

```ts
tx = resource.completeTx(tx);
```

4. Execute and verify:

```ts
import Verifier from "ckb-testtool";

const verifier = Verifier.from(resource, tx);
verifier.verifySuccess(true);
```

When `verifySuccess` reports failure (per CKB convention, **non-zero return code = failure**), the tool will:

* Print the original contract output first;
* Then use `*.debug.js.map` to **remap the stack to real source locations** and print them.
  If the `.map` file is missing, it will print locations from `*.debug.js` only.

> Note: Printed file paths are **relative to the project root**, which makes navigation easier in multi-package/monorepo setups.
