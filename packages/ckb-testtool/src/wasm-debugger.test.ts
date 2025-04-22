import path from "path";
import { wasmDebugger } from "./index";

describe("wasm-debugger", () => {
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
    const riscv_path = path.join(__dirname, "unittest/defaultScript/ckb-js-vm");
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
