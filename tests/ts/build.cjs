const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");
const CKB_FS_PACKER = path.resolve(
  __dirname,
  "./node_modules/.bin/ckb-fs-packer",
);

function check() {
  if (!fs.existsSync(CKB_FS_PACKER)) {
    throw new Error(`ckb-fs-packer binary not found at $ { CKB_FS_PACKER }`);
  }
}

function compileBc(jsFile) {
  let bcFile = jsFile.replace(".js", ".bc");
  bcFile = bcFile.replace("src", "dist");

  const bcDir = path.dirname(bcFile);
  if (!fs.existsSync(bcDir)) {
    fs.mkdirSync(bcDir, { recursive: true });
  }

  const command = `ckb-debugger --read-file ${jsFile} --bin ../../build/ckb-js-vm -- -c ${bcFile}`;

  try {
    execSync(command, { stdio: "inherit" });
  } catch (error) {
    console.error("Error building BC file:", error.message);
    process.exit(1);
  }
}

function packFileSystem(files, outputPath) {
  try {
    check();
    const command = `"${CKB_FS_PACKER}" pack ${outputPath} ${files.join(" ")}`;
    execSync(command, { stdio: "inherit" });
  } catch (error) {
    console.error("Error building file system:", error.message);
    process.exit(1);
  }
}

module.exports = { compileBc, check, packFileSystem };
