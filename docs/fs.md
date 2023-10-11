# Simple File System and JavaScript Module

In addition to executing individual JavaScript files, ckb-js-vm also supports
JavaScript modules by Simple File System, files within the file system may be
made available for Javascript to read, import, and execute, e.g. import modules
by `import { * } from "./module.js";`. Each Simple File System contains at least
one entry file named `main.js`, and ckb-js-vm will load this file system from
any cell and execute `main.js` in it.

A file system is represented as a binary file in the format described below. We
may use the script [fs.lua](../tools/fs.lua) to create a file system from given
files or unpack the file system into files.

## How to create a Simple File System

Consider the following two files:

```js
// File main.js
import { fib } from "./fib_module.js";
console.log("fib(10)=", fib(10));
```

```js
// File fib_module.js
export function fib(n) {
    if (n <= 0)
        return 0;
    else if (n == 1)
        return 1;
    else
        return fib(n - 1) + fib(n - 2);
}
```

If we want ckb-js-vm to execute this code smoothly, we must package them into a
file system first. To pack them within the current directory into `fib.fs`, you
may run 
```shell
find . -name *.js -type f | lua tools/fs.lua pack fib.fs
```

```
// Output
packing file ./fib_module.js to fib_module.js
packing file ./main.js to main.js
```

Note that all file paths piped into the `fs.lua` must be in the relative path
format. The absolute path of a file in the current system is usually meaningless
in the Simple File System.

## How to deploy and use Simple File System

In most cases, it is more resource-efficient to write all JavaScript code in one
file. To enable file system support, we cannot directly use ckb-js-vm as a lock
script, ckb-js-vm must be used as an exec Or the spawn target passes the "-f"
parameter to it.

We wrote an example that uses `spawn` syscall to call ckb-js-vm to demonstrate
how to use the file system.

```sh
$ cd tests/ckb_js_tests
$ make module
```

The key is `spawn_caller`, in this example, `spawn_caller` is the real lock
script, which then calls ckb-js-vm using `spawn` and passes it the `-f`
parameter: ckb-js-vm will then run in file system mode.

## Using JavaScript bytecode to improve performance

When ckb-js-vm executes JavaScript codes, it will first compile them into
bytecode, and then interpret and execute the bytecode. To improve the
performance of ckb-js-vm, we can also choose to directly let ckb-js-vm execute
bytecode.

We define all JavaScript bytecode files to have a `.bc` extension. When
ckb-js-vm obtains a file system, it will first look for the `main.js` file; if
not, it continues to look for the `main.bc` file. When importing a module in
JavaScript codes, e.g. `import { * } from "./module.js`, the steps are similar,
ckb-js-vm will look for `./module.js` or `./module.bc`.

In general, we only need to compile all JavaScript files into corresponding
bytecode files, and then package the bytecode files just like packaging
JavaScript files.

## Unpack Simple File System to Files

To unpack the files contained within a fs, you may run `lua tools/fs.lua unpack fib.fs .`.

## Simple File System On-disk Representation

The on-disk representation of a Simple File System consists of three parts, a
number to represent the number of files contained in this file system, an array
of metadata to store file metadata and an array of binary objects (also called
blob) to store the actual file contents.

A metadata is simply an offset from the start of the blob array and a datum
length. Each file name and file content has a metadata. For each file stored in
the fs, there is four `uint32_t` number in the metadata, i.e. the offset of the
file name in the blob array, the length of the file name, the offset of the file
content in the blob array, and the length of the file content.

We represent the above structures using c struct-like syntax as follows.
```c
struct Blob {
    uint32_t offset;
    uint32_t length;
}

struct Metadata {
    struct Blob file_name;
    struct Blob file_content;
}

struct SimpleFileSystem {
    uint32_t file_count;
    struct Metadata metadata[..];
    uint8_t payload[..];
}
```

When serializing the file system into a file, all integers are encoded as a
32-bit little-endian number. The file names are stored as null-terminated
strings.

Below is a binary dump of the file system created from a simple file called
`main.js` with content `console.log('hello world!')`.

```text
00000000  01 00 00 00 00 00 00 00  08 00 00 00 08 00 00 00  |................|
00000010  1c 00 00 00 6d 61 69 6e  2e 6a 73 00 63 6f 6e 73  |....main.js.cons|
00000020  6f 6c 65 2e 6c 6f 67 28  27 68 65 6c 6c 6f 20 77  |ole.log('hello w|
00000030  6f 72 6c 64 21 27 29 0a  00                       |orld!')..|
```
