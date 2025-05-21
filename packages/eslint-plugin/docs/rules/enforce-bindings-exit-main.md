# Enforce that `index.ts` (or `index.js`) exits via a top-level `bindings.exit(main());` call (`@ckb-js-std/enforce-bindings-exit-main`)

<!-- end auto-generated rule header -->

This rule ensures that main entry files properly exit using the `bindings.exit(main());` pattern, which is crucial for correct behavior in the ckb-js-vm environment.

## Rule Details

In the ckb-js-vm environment, scripts must use a specific exit pattern to ensure proper program termination and to correctly return values. As detailed in the [Security Best Practices for ckb-js-vm](https://github.com/nervosnetwork/ckb-js-vm/tree/main/docs/tutorial/src/security.md#exit-code):

> QuickJS treats every file as a module. Since it implements Top level await, the evaluation result of a module is a promise. This means the code below doesn't return -100 as expected:
>
> ```js
> function main() {
>   // ...
>   return -100;
> }
> main();
> ```
>
> Instead, it unexpectedly returns zero. To ensure your exit code is properly returned, use this pattern instead:
>
> ```js
> function main() {
>   // ...
>   return -100;
> }
> bindings.exit(main());
> ```

This rule enforces that `index.js` or `index.ts` files (or any file configured as an entry point) conclude with a top-level call to `bindings.exit(main());`.

### ❌ Incorrect

```js
// Missing bindings.exit call
function main() {
  // ...code
  return 0;
}

main();
```

```js
// Using bindings.exit but not with main()
import * as bindings from "@ckb-js-std/bindings";

function main() {
  // ...code
  return 0;
}

// Not using the proper pattern
bindings.exit(0);
```

### ✅ Correct

```js
import * as bindings from "@ckb-js-std/bindings";

function main() {
  // ...code
  return 0;
}

// Proper exit pattern
bindings.exit(main());
```

## Exit Codes

When your script throws an uncaught exception, ckb-js-vm will exit with error code (-1). Using the proper `bindings.exit(main())` pattern ensures that your custom exit codes are correctly propagated. This is essential for distinguishing between script errors and intentional exit statuses.

## When Not To Use It

This rule should not be disabled for code intended to run in the ckb-js-vm environment, as the proper exit pattern is essential for correct operation and reliable error handling.
