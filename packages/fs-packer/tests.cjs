#!/usr/bin/env node

const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

// Configuration
const testFiles = ["dist/index.js", "src/index.ts"];
const pakFile = "test.pak";
const outputDir = "test-output";
// Add source:destination format test files
const mappedFiles = [
  "dist/index.js:custom/path/index.js",
  "src/index.ts:another/location/index.ts",
];
const mappedPakFile = "test-mapped.pak";
// Binary path configuration
const binaryPath = process.env.BINARY_PATH || "node dist.commonjs/index.js";

// Ensure the output directory exists
if (!fs.existsSync(outputDir)) {
  fs.mkdirSync(outputDir, { recursive: true });
}

// Helper function to run commands and handle errors
function runCommand(command) {
  console.log(`Running: ${command}`);
  try {
    execSync(command, { stdio: "inherit" });
    return true;
  } catch (error) {
    console.error(`Command failed: ${command}`);
    console.error(error.message);
    return false;
  }
}

// Main test function
async function runTests() {
  console.log("Starting tests...");

  // Step 1: Build the project
  if (!runCommand("npm run build")) {
    process.exit(1);
  }

  // Step 2: Pack files
  const packCommand = `${binaryPath} pack ${pakFile} ${testFiles.join(" ")}`;
  if (!runCommand(packCommand)) {
    process.exit(1);
  }

  // Step 3: Unpack files
  const unpackCommand = `${binaryPath} unpack ${pakFile} ${outputDir}`;
  if (!runCommand(unpackCommand)) {
    process.exit(1);
  }

  // Step 4: Pack files with source:destination format
  console.log("\nTesting source:destination format...");
  const packMappedCommand = `${binaryPath} pack ${mappedPakFile} ${mappedFiles.join(" ")}`;
  if (!runCommand(packMappedCommand)) {
    process.exit(1);
  }

  // Step 5: Unpack mapped files
  const unpackMappedCommand = `${binaryPath} unpack ${mappedPakFile} ${outputDir}`;
  if (!runCommand(unpackMappedCommand)) {
    process.exit(1);
  }

  // Step 6: Compare files
  console.log("Comparing files...");
  let allPassed = true;

  for (const file of testFiles) {
    const originalFile = file;
    const unpackedFile = path.join(outputDir, file);

    try {
      const originalContent = fs.readFileSync(originalFile);
      const unpackedContent = fs.readFileSync(unpackedFile);

      if (Buffer.compare(originalContent, unpackedContent) === 0) {
        console.log(`✅ ${file}: Files match`);
      } else {
        console.error(`❌ ${file}: Files do not match`);
        allPassed = false;
      }
    } catch (error) {
      console.error(`❌ Error comparing ${file}: ${error.message}`);
      allPassed = false;
    }
  }

  // Step 7: Compare mapped files
  console.log("\nComparing mapped files...");
  for (const mappedFile of mappedFiles) {
    const [sourceFile, destFile] = mappedFile.split(":");
    const originalFile = sourceFile;
    const unpackedFile = path.join(outputDir, destFile);

    try {
      const originalContent = fs.readFileSync(originalFile);
      const unpackedContent = fs.readFileSync(unpackedFile);

      if (Buffer.compare(originalContent, unpackedContent) === 0) {
        console.log(`✅ ${mappedFile}: Files match`);
      } else {
        console.error(`❌ ${mappedFile}: Files do not match`);
        allPassed = false;
      }
    } catch (error) {
      console.error(`❌ Error comparing ${mappedFile}: ${error.message}`);
      allPassed = false;
    }
  }

  if (allPassed) {
    console.log("All tests passed! 🎉");
    process.exit(0);
  } else {
    console.error("Some tests failed ❌");
    process.exit(1);
  }
}

// Run the tests
runTests().catch((error) => {
  console.error("Test execution failed:", error);
  process.exit(1);
});
