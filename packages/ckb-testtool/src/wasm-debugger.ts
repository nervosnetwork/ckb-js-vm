import { randomBytes } from "node:crypto";
import {
  closeSync,
  existsSync,
  openSync,
  readFileSync,
  realpathSync,
  unlinkSync,
  writeFileSync,
} from "node:fs";
import path from "node:path";
import { WASI } from "node:wasi";
import { DEFAULT_SCRIPT_CKB_JS_VM } from "./unittest";

const STDOUT_FILENAME = "stdout.txt";
const STDERR_FILENAME = "stderr.txt";

function getNextFilename(basename: string): string {
  const timestamp = Date.now();
  const random = randomBytes(4).toString("hex");
  return `${timestamp}-${random}-${basename}`;
}

export interface Result {
  status: number;
  stdout: string;
  stderr: string;
}

/**
 * Executes the CKB debugger in a WebAssembly environment.
 *
 * This function provides a WASI-compatible wrapper around the `ckb-debugger` CLI tool,
 * enabling its execution within a WebAssembly context. It handles file system access
 * through preopens and manages standard I/O streams.
 *
 * @param preopens_path - Array of file paths that must be accessible to the WASM module.
 *                        These paths are required for file operations like reading
 *                        binary files or transaction data.
 *                        Example: ["--bin", "file.bin"] or ["--tx", "file.json"]
 * @param args - Additional command line arguments to pass to the debugger
 *
 * @returns Promise<Result> - A promise that resolves to an object containing:
 *                           - status: The exit code from the debugger
 *                           - stdout: Standard output content
 *                           - stderr: Standard error content
 */
export async function run(
  preopens_path: string[],
  args: string[],
): Promise<Result> {
  let stdoutContent = "";
  let stderrContent = "";

  const real_preopens_path = preopens_path.map((p) => realpathSync(p));

  for (const p of real_preopens_path) {
    if (!existsSync(p)) {
      throw new Error(`Preopen path not found: ${p}`);
    }
  }

  const preopens: Record<string, string> = {};
  for (let i = 0; i < real_preopens_path.length; i++) {
    const dir = path.dirname(real_preopens_path[i]);
    preopens[dir] = dir;
  }

  const wasm_debugger_path = path.join(
    __dirname,
    "../src/wasm/ckb-debugger.wasm",
  );
  if (!existsSync(wasm_debugger_path)) {
    throw new Error(`WASM binary not found: ${wasm_debugger_path}`);
  }
  // temporary files for stdout and stderr.
  // will be removed after use.
  const stdoutPath = path.join(".", getNextFilename(STDOUT_FILENAME));
  const stderrPath = path.join(".", getNextFilename(STDERR_FILENAME));

  const stdoutFd = openSync(stdoutPath, "w");
  const stderrFd = openSync(stderrPath, "w");
  const wasi_host = new WASI({
    version: "preview1",
    args: ["ckb-debugger", ...args],
    preopens: preopens,
    stdout: stdoutFd,
    stderr: stderrFd,
  });

  const mod = await WebAssembly.compile(readFileSync(wasm_debugger_path));

  const instance = new WebAssembly.Instance(
    mod,
    wasi_host.getImportObject() as WebAssembly.Imports,
  );

  const exitCode = wasi_host.start(instance);

  closeSync(stdoutFd);
  closeSync(stderrFd);
  // Read the contents of stdout and stderr files
  stdoutContent = readFileSync(stdoutPath, "utf8");
  stderrContent = readFileSync(stderrPath, "utf8");
  // clean up
  unlinkSync(stdoutPath);
  unlinkSync(stderrPath);

  return {
    status: exitCode,
    stdout: stdoutContent,
    stderr: stderrContent,
  };
}

/**
 * Compiles JavaScript code into QuickJS bytecode
 * @param jsPath - Path to the JavaScript source file
 * @param bcPath - Path where the compiled bytecode will be saved
 * @throws {Error} If compilation fails
 */
export async function compileQjsBytecode(
  jsPath: string,
  bcPath: string,
): Promise<void> {
  let ckbJsVmPath = DEFAULT_SCRIPT_CKB_JS_VM;
  if (!existsSync(ckbJsVmPath)) {
    throw new Error(`ckb-js-vm not found: ${ckbJsVmPath}`);
  }
  ckbJsVmPath = realpathSync(ckbJsVmPath);

  writeFileSync(bcPath, "");
  bcPath = realpathSync(bcPath);

  if (!existsSync(jsPath)) {
    throw new Error(`jsPath not found: ${jsPath}`);
  }
  jsPath = realpathSync(jsPath);
  // identical to
  // `ckb-debugger --read-file $jsPath --bin ../src/unittest/defaultScript/ckb-js-vm -- -c $bcPath`
  const result = await run(
    [jsPath, ckbJsVmPath, bcPath],
    ["--read-file", jsPath, "--bin", ckbJsVmPath, "--", "-c", bcPath],
  );
  if (result.status !== 0) {
    throw new Error(`Failed to compile QuickJS bytecode: ${result.stdout}`);
  }
}
