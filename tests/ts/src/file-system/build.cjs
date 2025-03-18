const { compileBc, packFileSystem } = require("../../build.cjs");

const OUTPUT_FS = "dist/file-system/fs.bin";
const OUTPUT_FS_2 = "dist/file-system/fs2.bin";
const OUTPUT_FS_BC = "dist/file-system/fs-bc.bin";

module.exports = {
  OUTPUT_FS,
  OUTPUT_FS_2,
  OUTPUT_FS_BC,
};

function buildFileSystem() {
  const files = [
    "src/file-system/data/fib_module.js:fib_module.js",
    "src/file-system/data/index.js:index.js",
    "src/file-system/data/init.js:init.js",
  ];
  packFileSystem(files, OUTPUT_FS);
}

function buildFileSystemBc() {
  compileBc("src/file-system/data/fib_module.js");
  compileBc("src/file-system/data/index_bc.js");
  compileBc("src/file-system/data/init.js");

  const files = [
    "dist/file-system/data/fib_module.bc:fib_module.bc",
    "dist/file-system/data/index_bc.bc:index.bc",
    "dist/file-system/data/init.bc:init.bc",
  ];
  packFileSystem(files, OUTPUT_FS_BC);
}

function buildFileSystem2() {
  const files = [
    "src/file-system/data2/fib_module.js:fib_module.js",
    "src/file-system/data2/index.js:index.js",
    "src/file-system/data2/fib.js:fib.js",
  ];
  packFileSystem(files, OUTPUT_FS_2);
}

buildFileSystem();
buildFileSystem2();
buildFileSystemBc();
