const { execSync } = require('child_process');
const path = require('path');
const fs = require('fs');

const CKB_FS_PACKER = path.resolve(__dirname, '../../node_modules/.bin/ckb-fs-packer');
const OUTPUT_FS = "dist/file-system/fs.bin";
const OUTPUT_FS_BC = "dist/file-system/fs-bc.bin";

module.exports = {
  OUTPUT_FS,
  OUTPUT_FS_BC
};

function check() {
    if (!fs.existsSync(CKB_FS_PACKER)) {
        throw new Error(`ckb-fs-pack binary not found at $ { CKB_FS_PACKER }`);
    }
}

function compileBc(jsFile) {
    let bcFile = jsFile.replace('.js', '.bc');
    bcFile = bcFile.replace('src', 'dist');

    const bcDir = path.dirname(bcFile);
    if (!fs.existsSync(bcDir)) {
        fs.mkdirSync(bcDir, { recursive: true });
    }

    const command = `ckb-debugger --read-file ${jsFile} --bin ../../build/ckb-js-vm -- -c ${bcFile}`;

    try {
        execSync(command, {stdio : 'inherit'});
    } catch (error) {
        console.error('Error building BC file:', error.message);
        process.exit(1);
    }
}

function buildFileSystem() {
  try {
    const command = `"${CKB_FS_PACKER}" pack ${OUTPUT_FS} src/file-system/data/fib_module.js:fib_module.js src/file-system/data/index.js:index.js src/file-system/data/init.js:init.js`;
    execSync(command, { stdio: 'inherit' });
  } catch (error) {
    console.error('Error building file system:', error.message);
    process.exit(1);
  }
}


function buildFileSystemBc() {
  try {
    compileBc('src/file-system/data/fib_module.js');
    compileBc('src/file-system/data/index_bc.js');
    compileBc('src/file-system/data/init.js');

    const command = `"${CKB_FS_PACKER}" pack ${OUTPUT_FS_BC} dist/file-system/data/fib_module.bc:fib_module.bc dist/file-system/data/index_bc.bc:index.bc dist/file-system/data/init.bc:init.bc`;
    execSync(command, {stdio : 'inherit'});
  } catch (error) {
    console.error('Error building file system:', error.message);
    process.exit(1);
  }
}

check();
buildFileSystem();
buildFileSystemBc();
