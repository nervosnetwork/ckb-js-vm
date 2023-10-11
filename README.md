# ckb-js-vm
The objective of this project is to develop scripts in JavaScript for CKB-VM, by
adapting [quickjs](https://bellard.org/quickjs/).


## Build
The clang version 16 is required.

```shell
git submodule update --init
make all
```

## Documents
* [Introduction](./docs/intro.md)
* [CKB Syscall Bindings](./docs/syscalls.md)
* [Simple File System and JavaScript Module](./docs/fs.md)


## Examples

* [Fibonacci Number](./tests/examples/fib.js)
* [Calculate PI](./tests/examples/pi_bigint.js)

More [tests and examples](./tests/).
