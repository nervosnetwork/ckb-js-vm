# Security Best Practices

In this chapter, we will introduce some background and useful security tips for ckb-js-vm.

## Stack and Heap Memory

For normal native C programs, there is no method to control the stack size. However, QuickJS provides this capability
through its `JS_SetMaxStackSize` function. This is a critical feature to prevent stack/heap collisions.

Before explaining our memory organization design, let's understand the memory layout of ckb-vm, which follows these rules:
- Total memory is 4M
- From address 0 to the address specified by symbol `_end`, there are ELF sections (.data, .rss, .text, etc.)
- The stack begins at 4M and grows backward toward lower addresses

In ckb-js-vm, we carefully organize memory regions as follows:
- From address 0 to `_end`: ELF sections
- From address `_end` to 3M: Heap memory for malloc
- From address 3M+4K to 4M: Stack

The 4K serves as a margin area. This organization prevents stack/heap collisions when the stack grows too large.

## Exit Code

When bytecode or JavaScript code throws an uncaught exception, ckb-js-vm will exit with error code (-1).
You can write JavaScript code without explicitly checking for exceptions—simply let them throw naturally.

QuickJS treats every file as a module. Since it implements [Top level await](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/await#top_level_await),
the evaluation result of a module is a promise. This means the code below doesn't return -100 as expected:

  ```js
  function main() {
    // ...
    return -100;
  }
  main();
  ```

Instead, it unexpectedly returns zero. To ensure your exit code is properly returned, use this pattern instead:

  ```js
  function main() {
    // ...
    return -100;
  }
  bindings.exit(main());
  ```
