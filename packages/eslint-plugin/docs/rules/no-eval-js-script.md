# Disallow the use of `bindings.evalJsScript()` (`@ckb-js-std/no-eval-js-script`)

<!-- end auto-generated rule header -->

This rule prevents the use of `bindings.evalJsScript()` due to the significant security risks associated with evaluating arbitrary JavaScript code, especially from untrusted sources.

## Rule Details

Using `bindings.evalJsScript()` to evaluate dynamic JavaScript code can introduce severe security vulnerabilities, such as code injection. The [Security Best Practices for ckb-js-vm](https://github.com/nervosnetwork/ckb-js-vm/tree/main/docs/tutorial/src/security.md#dynamic-loading) documentation warns:

> JavaScript provides the ability to load modules dynamically at runtime through the `evalJsScript` function. [...] However, it comes with significant security implications. When modules are loaded from untrusted sources (such as other cells on-chain), they may contain malicious code. A simple `exit(0)` statement could cause your entire script to exit with a success status, bypassing your validation logic.

This rule helps mitigate such risks by flagging all uses of `bindings.evalJsScript()`.

### ❌ Incorrect

```js
import * as bindings from "@ckb-js-std/bindings";

// Evaluating code directly from a string
bindings.evalJsScript('console.log("Hello World")');

// Potentially dangerous evaluation of unvalidated user input
const userCode = getUntrustedInput();
bindings.evalJsScript(userCode);
```

### ✅ Correct

Avoid dynamic code evaluation where possible. If it's absolutely necessary, use safer alternatives or apply strict security measures:

```js
import * as bindings from "@ckb-js-std/bindings";

// Example of a safer pattern: using a sandboxed environment (conceptual)
function loadModuleSafely(moduleSource, trustedApis) {
  // This is a simplified example. Real sandboxing is complex.
  const wrappedSource = `
    (function(restrictedScope) {
      // Ensure 'this' is bound to the restricted scope
      // and global variables are not accessible.
      ${moduleSource}
    }).call({}, trustedApis); // Pass only whitelisted APIs
  `;
  return bindings.evalJsScript(wrappedSource);
}

// Usage with strict validation and limited APIs:
const moduleSource = getValidatedAndTrustedCode();
if (moduleSource) {
  const restrictedAPIs = {
    log: console.log, // Only allow specific, safe functions
    // Do NOT pass the full 'bindings' object or other powerful APIs
  };
  try {
    loadModuleSafely(moduleSource, restrictedAPIs);
  } catch (error) {
    console.error("Error evaluating sandboxed module:", error);
    // Handle error appropriately
  }
}
```

## Security Recommendations

If you must use dynamic code evaluation:

1.  **Only load code from trusted sources** you control.
2.  **Implement strict permission restrictions** for the loaded code. Consider sandboxing techniques.
3.  **Validate module integrity** using cryptographic signatures if possible.
4.  **Minimize exposed APIs** to the evaluated code. Avoid passing the full `bindings` object or other powerful interfaces.

Refer to the [Security Best Practices for ckb-js-vm](https://github.com/nervosnetwork/ckb-js-vm/tree/main/docs/tutorial/src/security.md#dynamic-loading) for more details.

## When Not To Use It

This rule should generally not be disabled due to the inherent security risks. If you have a specific, controlled use case where `bindings.evalJsScript()` is absolutely necessary and you have implemented robust input validation, sandboxing, and other security measures, you might consider selectively disabling this rule for those specific, audited code sections using ESLint configuration comments.
