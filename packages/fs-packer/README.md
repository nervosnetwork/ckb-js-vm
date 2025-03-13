In addition to executing individual JavaScript files, ckb-js-vm also supports JavaScript modules through its Simple File System. Files within this file system are made available for JavaScript to read, import, and execute, enabling module imports like import { * } from "./module.js".

A file system is represented as a binary file with a specific format. You can use the ckb-fs-packer tool to create a file system from your source files or to unpack an existing file system.

See more [Simple File System and Modules](https://github.com/nervosnetwork/ckb-js-vm/blob/main/docs/tutorial/src/file-system.md).
