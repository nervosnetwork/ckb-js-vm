const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");
const CKB_FS_PACKER = path.resolve(
  __dirname,
  "../../packages/fs-packer/dist.commonjs/index.js",
);

function check() {
  if (!fs.existsSync(CKB_FS_PACKER)) {
    throw new Error(`ckb-fs-packer binary not found at ${CKB_FS_PACKER}`);
  }
}

function compileBc(jsFile, bcFile) {
  if (!bcFile) {
    bcFile = jsFile.replace(".js", ".bc");
    bcFile = bcFile.replace("src", "dist");
  }

  const bcDir = path.dirname(bcFile);
  if (!fs.existsSync(bcDir)) {
    fs.mkdirSync(bcDir, { recursive: true });
  }

  const jsVmPath = path.resolve(__dirname, "../../build/ckb-js-vm");

  const command = `ckb-debugger --read-file ${jsFile} --bin ${jsVmPath} -- -c ${bcFile}`;

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
    const dir = path.dirname(outputPath);
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
    const command = `node "${CKB_FS_PACKER}" pack ${outputPath} ${files.join(" ")}`;
    execSync(command, { stdio: "inherit" });
  } catch (error) {
    console.error("Error building file system:", error.message);
    process.exit(1);
  }
}

/**
 * Runs TypeScript type checking on specified TypeScript files
 * @param {string|string[]} filePaths - Path(s) to TypeScript file(s) to check
 * @returns {boolean} true if compilation succeeds, false otherwise
 */
function typeCheck(filePaths) {
  // Convert single path to array
  const files = Array.isArray(filePaths) ? filePaths : [filePaths];

  // Check if files exist
  for (const file of files) {
    if (!fs.existsSync(file)) {
      console.error(`Error: File not found: ${file}`);
      return false;
    }
  }

  try {
    // Run tsc --noEmit on the files
    execSync(`tsc --noEmit ${files.join(" ")}`, {
      stdio: "inherit",
      cwd: path.resolve(__dirname, "../../"),
    });
    return true;
  } catch (error) {
    console.error("Type checking failed:");
    console.error(error.message);
    return false;
  }
}

function bundleCode(inputFile, outputFile) {
  try {
    const dir = path.dirname(outputFile);
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }

    const command = `npx esbuild ${inputFile} \
      --platform=neutral \
      --minify \
      --bundle \
      --external:@ckb-js-std/bindings \
      --target=es2022 \
      --outfile=${outputFile}`;

    execSync(command, { stdio: "inherit" });
  } catch (error) {
    console.error("Error bundling code:", error.message);
    process.exit(1);
  }
}

module.exports = { compileBc, check, packFileSystem, typeCheck, bundleCode };
