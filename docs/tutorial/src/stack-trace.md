# Stack Traces

# Stack Traces

`ckb-js-vm` does not currently support breakpoint debugging. In production, contracts are typically shipped as **bytecode** compiled from **minified JavaScript**, which makes it hard to map runtime errors back to source lines.
This guide shows how to print and recover stack traces when a contract aborts, so you can quickly pinpoint the root cause.

## How it works

When `ckb-js-vm` executes JavaScript, you can print **function names** and **line numbers** with `console.log`. However, because production contracts are minified and converted to bytecode, the raw output is noisy and not source-accurate.
For debugging you should:

1. Produce a **non-minified** JS bundle that **preserves function names**; and
2. Generate a **source map** (`.map`) so the runtime stack can be **mapped back to your TypeScript sources**.

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

Compared with the **release** build, these flags favor debuggability. The output includes `*.debug.js` and `*.debug.js.map`.
(The configuration above is already included in the `create-app` scaffold—ready to use.)

> Notes
>
> * `--keep-names` preserves function names for readable stacks.
> * `--sourcemap=external` plus `--sources-content` emits a separate map and embeds source content for accurate mapping.
> * `--minify=false` avoids line/column drift introduced by minification.

## Catch and print errors

```ts
import logError from "@ckb-js-std/core";

try {
  main();
} catch (e) {
  logError(e);
}
```

This captures **uncaught** exceptions from `main` and prints them via `logError`.
The raw stack originates from `*.debug.js`; to map exact **TypeScript** file/line/column, keep the colocated `*.debug.js.map`.

## Emit the error stack

1. **Create a debug contract:**

   ```ts
   const lockCell = resource.mockDebugCellAsCellDep("./dist/script.debug.js");
   ```

   You **must** use `mockDebugCellAsCellDep`. It records the **contract path** in `resource`, so the transformer can find the **sibling** `.map` file later. Ensure you pass the **debug** build.

2. **Assemble the transaction and run verification:**

   ```ts
   // ...
   const verifier = Verifier.from(resource, tx);
   verifier.verifySuccess();
   // or
   verifier.verifyFailure(-2);
   ```

   Whether `verifySuccess` or `verifyFailure` is used, if the script returns a non-zero value (CKB convention: non-zero means failure), the stack trace will be printed.

### Sample output

```text
  console.log
    
    ╔════════════════════════════════════════════════════════
    ║ Script Verification Summary
    ║ Type: input lock script
    ║ Index: 0
    ╠════════════════════════════════════════════════════════
    ║ STDOUT:
    ║ Script log:     at main (<run_from_file>:30)
    ║     at <anonymous> (<run_from_file>:45)
    ║ 
    ║ Caused by:     at step2 (<run_from_file>:39)
    ║     at step1 (<run_from_file>:35)
    ║     at main (<run_from_file>:28)
    ║     at <anonymous> (<run_from_file>:45)
    ║ Run result: -2
    ║ All cycles: 4829837(4.6M)
    ╠════════════════════════════════════════════════════════
    ║ STDERR:
    ║ 
    ╚════════════════════════════════════════════════════════

      at ScriptVerificationResult.reportSummary (../ckb-testtool/dist.commonjs/unittest/core.js:106:17)

  console.log
    ------------------------------------------------------------
    Script log:     at main (packages/src/simple_stack_trace.ts:8:54)
        at <anonymous> (packages/src/simple_stack_trace.ts:20:7)
    
    Caused by:     at step2 (packages/src/simple_stack_trace.ts:14:45)
        at step1 (packages/src/simple_stack_trace.ts:12:25)
        at main (packages/src/simple_stack_trace.ts:6:10)
        at <anonymous> (packages/src/simple_stack_trace.ts:20:7)
    Run result: -2
    All cycles: 4829837(4.6M)
    ------------------------------------------------------------

      at printCrashStack (../ckb-testtool/dist.commonjs/unittest/crashStack.js:29:13)
```

The output contains two parts:

* The first block is the **raw script output** (with `<run_from_file>` locations) to confirm runtime flow.
* The second block is the **source-mapped stack** (now pointing to your TypeScript files and exact positions) and is what you’ll typically use to fix the issue.
