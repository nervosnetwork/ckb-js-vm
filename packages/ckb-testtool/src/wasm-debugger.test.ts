import path from "path";
import { DEFAULT_SCRIPT_CKB_JS_VM, wasmDebugger } from "./index";

describe("wasm-debugger", () => {
  describe("toPosixPath", () => {
    const originalPlatform = process.platform;

    afterEach(() => {
      // Restore the original platform
      Object.defineProperty(process, "platform", {
        value: originalPlatform,
      });
    });

    test("converts Windows path with drive letter to POSIX format", () => {
      Object.defineProperty(process, "platform", {
        value: "win32",
      });

      expect(wasmDebugger.toPosixPath("C:\\Users\\test")).toBe("/c/Users/test");
      expect(wasmDebugger.toPosixPath("D:\\Program Files\\app")).toBe(
        "/d/Program Files/app",
      );
      expect(wasmDebugger.toPosixPath("c:\\windows\\system32")).toBe(
        "/c/windows/system32",
      );
    });

    test("converts Windows path without drive letter", () => {
      Object.defineProperty(process, "platform", {
        value: "win32",
      });

      expect(wasmDebugger.toPosixPath("\\Users\\test")).toBe("/Users/test");
      expect(wasmDebugger.toPosixPath("relative\\path\\file.txt")).toBe(
        "relative/path/file.txt",
      );
    });

    test("leaves Unix paths unchanged on non-Windows platforms", () => {
      Object.defineProperty(process, "platform", {
        value: "linux",
      });

      expect(wasmDebugger.toPosixPath("/home/user/file")).toBe(
        "/home/user/file",
      );
      expect(wasmDebugger.toPosixPath("relative/path/file.txt")).toBe(
        "relative/path/file.txt",
      );
    });

    test("leaves Unix paths unchanged on macOS", () => {
      Object.defineProperty(process, "platform", {
        value: "darwin",
      });

      expect(wasmDebugger.toPosixPath("/Users/test/file")).toBe(
        "/Users/test/file",
      );
    });

    test("handles already converted paths on Windows", () => {
      Object.defineProperty(process, "platform", {
        value: "win32",
      });

      // If the path is already in POSIX format, it should just convert backslashes
      expect(wasmDebugger.toPosixPath("/c/Users/test")).toBe("/c/Users/test");
    });
  });

  test("alwaysSuccess", async () => {
    const riscv_path = path.join(
      __dirname,
      "unittest/defaultScript/alwaysSuccess",
    );
    const result = await wasmDebugger.run([riscv_path], ["--bin", riscv_path]);
    expect(result.status).toBe(0);
    expect(result.stderr).toBe("");
    expect(result.stdout).toContain("Run result: 0");
  });
  test("alwaysFailure", async () => {
    const riscv_path = path.join(
      __dirname,
      "unittest/defaultScript/alwaysFailure",
    );
    const result = await wasmDebugger.run([riscv_path], ["--bin", riscv_path]);
    expect(result.status).toBe(254);
    expect(result.stderr).toBe("");
    expect(result.stdout).toContain("Run result: -1");
  });
  test("ckb-js-vm", async () => {
    const riscv_path = DEFAULT_SCRIPT_CKB_JS_VM;
    const result = await wasmDebugger.run(
      [riscv_path],
      ["--bin", riscv_path, "--", "-e", 'console.log("hello,world")'],
    );
    expect(result.status).toBe(0);
    expect(result.stderr).toBe("");
    expect(result.stdout).toContain("hello,world");
    expect(result.stdout).toContain("Run result: 0");
  });
});
