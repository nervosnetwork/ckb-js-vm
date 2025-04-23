#!/usr/bin/env node

// TODO: update ckb-debugger's syscall to support writing file in WASM
import { compileQjsBytecode } from "./wasm-debugger";

async function main() {
  const args = process.argv.slice(2);
  if (args.length !== 2) {
    console.error("Usage: compile <input.js> <output.bytecode>");
    process.exit(1);
  }

  const [inputPath, outputPath] = args;

  try {
    await compileQjsBytecode(inputPath, outputPath);
    console.log(`Successfully compiled ${inputPath} to ${outputPath}`);
  } catch (error) {
    console.error("Compilation failed:", error);
    process.exit(1);
  }
}

main().catch((error) => {
  console.error("Unexpected error:", error);
  process.exit(1);
});
