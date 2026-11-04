// Exercise native bindings and VM exit status through ckb-debugger.
const assert = require("node:assert/strict");
const { spawnSync } = require("node:child_process");
const { mkdtempSync, readFileSync, rmSync, writeFileSync } = require("node:fs");
const { tmpdir } = require("node:os");
const { join, resolve } = require("node:path");
const { test } = require("node:test");

const ROOT = resolve(__dirname, "../..");
const VM = join(ROOT, "build/ckb-js-vm");
const DEBUGGER = process.env.CKB_DEBUGGER || "ckb-debugger";

function runDebugger(args, success = true) {
  const result = spawnSync(DEBUGGER, ["--max-cycles", "2000000000", ...args], {
    encoding: "utf8",
    timeout: 60_000,
    maxBuffer: 4 * 1024 * 1024,
  });
  if (result.error) throw result.error;
  const output = result.stdout + result.stderr;
  assert.equal(result.signal, null, output);
  const vmResult = output.match(/^Run result: (-?\d+)$/m);
  assert.ok(vmResult, output);
  if (success) {
    assert.equal(result.status, 0, output);
    assert.equal(Number(vmResult[1]), 0, output);
  } else {
    assert.notEqual(result.status, 0, output);
    assert.notEqual(Number(vmResult[1]), 0, output);
  }
  return output;
}

function runScript(source, success = true) {
  return runDebugger(["--bin", VM, "--", "-e", source], success);
}

function packFiles(files) {
  const header = Buffer.alloc(4 + files.length * 16);
  header.writeUInt32LE(files.length);
  const payload = [];
  let offset = 0;
  files.forEach(([name, content], index) => {
    const filename = Buffer.from(name + "\0");
    const source = Buffer.from(content);
    const entry = 4 + index * 16;
    header.writeUInt32LE(offset, entry);
    header.writeUInt32LE(filename.length, entry + 4);
    header.writeUInt32LE(offset + filename.length, entry + 8);
    header.writeUInt32LE(source.length, entry + 12);
    payload.push(filename, source, Buffer.from([0]));
    offset += filename.length + source.length + 1;
  });
  return Buffer.concat([header, ...payload]);
}

test("filesystem survives queued imports", async (t) => {
  const source = `
    Promise.resolve().then(() => {
      globalThis.buffers = Array.from({length: 64},
        () => new Uint8Array(1024).fill(0x58));
      return import('./late.js');
    }).then(m => {
      if (m.value !== 42) throw new Error('wrong module');
      console.log('late import OK');
    });
  `;
  const archive = packFiles([
    ["index.js", source],
    ["padding", " ".repeat(65536)],
    ["late.js", "export const value = 42;"],
  ]);

  // CKB hashes of the Molecule-serialized type scripts below. Using fixed
  // type hashes avoids hashing the changing VM binary or needing npm packages.
  // Both scripts have a zero code_hash and hash_type=data; args are empty / 01.
  const vmType = {
    code_hash: "0x" + "00".repeat(32),
    hash_type: "data",
    args: "0x",
  };
  const fsType = { ...vmType, args: "0x01" };
  const vmTypeHash =
    "77c93b0632b5b6c3ef922c5b7cea208fb0a7c427a13d50e13d3fefad17e0c590";
  const fsTypeHash =
    "67951b34bce20cb71b7e235c1f8cda259628d99d94825bffe549c23b4dd2930f";
  const script = {
    code_hash: "0x" + vmTypeHash,
    hash_type: "type",
    args: "0x0100" + fsTypeHash + "01",
  };
  const output = { capacity: "0x2540be400", lock: script, type: null };
  const outPoint = (index) => ({
    tx_hash: "0x" + index.toString(16).padStart(64, "0"),
    index: "0x0",
  });
  const deps = [1, 2].map((index) => ({
    out_point: outPoint(index),
    dep_type: "code",
  }));
  const txInput = { since: "0x0", previous_output: outPoint(3) };
  const tx = {
    mock_info: {
      inputs: [{ input: txInput, output, data: "0x" }],
      cell_deps: [readFileSync(VM), archive].map((buffer, index) => ({
        cell_dep: deps[index],
        output: { ...output, type: [vmType, fsType][index] },
        data: "0x" + buffer.toString("hex"),
      })),
      header_deps: [],
      extensions: [],
    },
    tx: {
      version: "0x0",
      cell_deps: deps,
      header_deps: [],
      inputs: [txInput],
      outputs: [],
      outputs_data: [],
      witnesses: [],
    },
  };
  const directory = mkdtempSync(join(tmpdir(), "ckb-native-regressions-"));
  try {
    const txPath = join(directory, "tx.json");
    const fsPath = join(directory, "archive.fs");
    writeFileSync(txPath, JSON.stringify(tx));
    writeFileSync(fsPath, archive);
    for (const [name, args] of [
      ["cell data", ["--tx-file", txPath]],
      [
        "explicit target",
        ["--tx-file", txPath, "--", "-t", fsTypeHash + "01", "-f"],
      ],
      ["local archive", ["--read-file", fsPath, "--bin", VM, "--", "-r", "-f"]],
    ]) {
      await t.test(name, () => {
        assert.match(runDebugger(args), /late import OK/);
      });
    }
  } finally {
    rmSync(directory, { recursive: true, force: true });
  }
});

test("async failures reject the transaction", async (t) => {
  for (const source of [
    'await Promise.resolve(); throw new Error("async failure");',
    'Promise.reject(new Error("async failure"));',
    'Promise.resolve().then(() => { throw new Error("async failure"); });',
    '(async () => { await 0; throw new Error("async failure"); })();',
    "Promise.reject(null);",
    "Promise.reject(undefined);",
    `const p = Promise.reject({toString() {
      p.catch(() => {}); return "late handler";
    }});`,
    "await new Promise(() => {});",
    `import {evalJsScript} from "@ckb-js-std/bindings";
     globalThis.reason = new Error("shared reason");
     Promise.reject(reason);
     try { evalJsScript("throw globalThis.reason", true); } catch (_) {}`,
  ]) {
    await t.test(source, () => runScript(source, false));
  }
});

test("handled async failures succeed", () => {
  runScript(`
    import {evalJsScript} from "@ckb-js-std/bindings";
    await Promise.resolve();
    await Promise.reject("expected").catch(() => {});
    const p = Promise.reject("handled in a later job");
    Promise.resolve().then(() => p.catch(() => {}));
    for (const module of [false, true]) {
      let caught = false;
      try { evalJsScript('throw new Error("expected")', module); }
      catch (e) { caught = e.message === "expected"; }
      if (!caught) throw new Error("lost exception");
    }
  `);
});

test("long module names throw without corrupting memory", async (t) => {
  for (const length of [252, 253, 512, 2048]) {
    await t.test(`${length} characters`, () => {
      const output = runScript(`import "${"a".repeat(length)}.js";`, false);
      assert.match(output, /ReferenceError/);
      assert.doesNotMatch(output, /memory error/);
    });
  }
});

test("execCell checks argument bounds", () => {
  runScript(`
    import {execCell} from "@ckb-js-std/bindings";
    for (const n of [257, 300, 512]) {
      for (const hash of [new ArrayBuffer(32), null]) {
        let caught = false;
        try { execCell(hash, 0, 0, 0, ...Array(n).fill("x")); }
        catch (e) { caught = e instanceof RangeError; }
        if (!caught) throw new Error("missing argument limit");
      }
    }
    for (const n of [0, 256]) {
      try { execCell(new ArrayBuffer(32), 0, 0, 0, ...Array(n).fill("x")); }
      catch (e) {
        if (e instanceof RangeError || typeof e.errorCode !== "number")
          throw new Error("valid argument count rejected");
      }
    }
  `);
});
