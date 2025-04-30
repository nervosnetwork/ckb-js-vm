import * as ckb from "@ckb-js-std/bindings";

const ARRAY8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07];
function expect_array(a, b) {
  if (a.byteLength != b.length) {
    console.assert(
      false,
      `expect_array failed: length mismatched, ${a} VS ${b}`,
    );
  }
  for (let i = 0; i < a.length; i++) {
    console.assert(a[i] === b[i], `expect_array failed at index ${i}`);
  }
}

function must_throw_exception(f) {
  let has_exception = false;
  let error_code = 0;
  try {
    f();
  } catch (e) {
    has_exception = true;
    error_code = e.errorCode;
  }
  console.assert(has_exception, "Error, no exception found");
  return error_code;
}

function test_partial_loading(load_func) {
  console.log("test_partial_loading ...");
  let data = load_func(0, ckb.SOURCE_OUTPUT);
  expect_array(data, ARRAY8);
  data = load_func(0, ckb.SOURCE_OUTPUT, 0);
  expect_array(data, ARRAY8);
  let length = load_func(0, ckb.SOURCE_OUTPUT, 1, 0);
  console.assert(length === 7, "length != 7");
  data = load_func(0, ckb.SOURCE_OUTPUT, 0, 7);
  expect_array(data, ARRAY8.slice(0, 7));
  data = load_func(0, ckb.SOURCE_OUTPUT, 1, 7);
  expect_array(data, ARRAY8.slice(1, 8));

  let error_code = must_throw_exception(() => {
    load_func(1001, ckb.SOURCE_OUTPUT);
  });
  // CKB_INDEX_OUT_OF_BOUND
  console.log(error_code === 1);
  error_code = must_throw_exception(() => {
    load_func(0, ckb.SOURCE_OUTPUT + 1000n);
  });
  console.log("test_partial_loading done");
}

function test_partial_loading_without_comparing(load_func) {
  console.log("test_partial_loading_without_comparing ...");
  let data = load_func(0, ckb.SOURCE_OUTPUT);
  console.assert(data);
  let length = load_func(0, ckb.SOURCE_OUTPUT, 0, 0);
  console.assert(length == data.byteLength, "length != data.byteLength");
  length = load_func(0, ckb.SOURCE_OUTPUT, 1, 0);
  console.assert(
    length == data.byteLength - 1,
    "length != data.byteLength - 1",
  );
  data = load_func(0, ckb.SOURCE_OUTPUT, 0, 7);
  console.assert(data);
  data = load_func(0, ckb.SOURCE_OUTPUT, 1, 7);
  console.assert(data);

  must_throw_exception(() => {
    load_func(1001n, ckb.SOURCE_OUTPUT);
  });
  must_throw_exception(() => {
    load_func(0, ckb.SOURCE_OUTPUT + 1000n);
  });
  console.log("test_partial_loading done");
}

function test_partial_loading_field_without_comparing(load_func, field) {
  console.log("test_partial_loading_field_without_comparing ...");
  let data = load_func(0, ckb.SOURCE_INPUT, field);
  console.assert(data);
  let length = load_func(0, ckb.SOURCE_INPUT, field, 0, 0);
  console.assert(length == data.byteLength, "length != data.byteLength");
  length = load_func(0, ckb.SOURCE_INPUT, field, 1, 0);
  console.assert(
    length == data.byteLength - 1,
    "length != data.byteLength - 1",
  );
  data = load_func(0, ckb.SOURCE_INPUT, field, 0, 7);
  console.assert(data);
  data = load_func(0, ckb.SOURCE_INPUT, field, 1, 7);
  console.assert(data);

  must_throw_exception(() => {
    load_func(1001n, ckb.SOURCE_INPUT, field);
  });
  must_throw_exception(() => {
    load_func(0, ckb.SOURCE_INPUT + 1000n, field);
  });
  console.log("test_partial_loading_field_without_comparing done");
}

function test_misc() {
  console.log("test_misc ....");
  let hash = ckb.loadTxHash();
  console.assert(hash.byteLength == 32);

  // script
  let script = ckb.loadScript();
  console.assert(script.byteLength == 88, "script.byteLength != 88");
  let script1 = ckb.loadScript(1);
  console.assert(script1.byteLength == 87, "script1.byteLength != 87");
  let script_length = ckb.loadScript(0, 0);
  console.assert(script_length == 88, "script_length != 88");
  let script2 = ckb.loadScript(1, 10);
  console.assert(script2.byteLength == 10, "script2.byteLength != 10");
  // transaction
  let tx = ckb.loadTransaction();
  console.assert(tx.byteLength > 100, "tx.byteLength > 100");
  let tx1 = ckb.loadTransaction(1);
  console.assert(tx1.byteLength > 100, "tx1.byteLength > 100");
  let tx_length = ckb.loadTransaction(0, 0);
  console.assert(tx_length > 100, "tx_length > 100");
  let tx2 = ckb.loadTransaction(1, 10);
  console.assert(tx2.byteLength == 10, "tx2.byteLength != 10");

  let blockExtensionByHeaderDep = ckb.loadBlockExtension(
    0,
    ckb.SOURCE_HEADER_DEP,
  );
  console.assert(
    blockExtensionByHeaderDep.byteLength == 1,
    "blockExtensionByHeaderDep.byteLength != 1",
  );
  let blockExtensionByInput = ckb.loadBlockExtension(0, ckb.SOURCE_INPUT);
  console.assert(
    blockExtensionByInput.byteLength == 2,
    "blockExtensionByInput.byteLength != 2",
  );

  let headerByHeaderDep = ckb.loadHeader(0, ckb.SOURCE_HEADER_DEP);
  console.assert(
    headerByHeaderDep.byteLength == 208,
    "headerByHeaderDep.byteLength == 208",
  );
  let headerByInput = ckb.loadHeader(0, ckb.SOURCE_INPUT);
  console.assert(
    headerByInput.byteLength == 208,
    "headerByInput.byteLength == 208",
  );

  hash = ckb.loadScriptHash();
  console.assert(hash.byteLength == 32);
  let version = ckb.vmVersion();
  console.assert(version >= 0);
  let cycles = ckb.currentCycles();
  console.assert(cycles > 0);
  let cycles2 = ckb.currentCycles();
  console.assert(cycles2 > cycles);
  try {
    // wrong index type
    ckb.loadCellData("hello");
  } catch (e) {
    let s = e.toString();
    console.assert(
      s.includes("Invalid argument: expected integer at index 0"),
      "failed in test_misc",
    );
  }
  try {
    ckb.loadCellData({});
  } catch (e) {
    let s = e.toString();
    console.assert(
      s.includes("Invalid argument: expected integer at index 0"),
      "failed in test_misc",
    );
  }
  console.log("test_misc done");
}

function test_spawn() {
  const js_code = `
    import * as ckb from "@ckb-js-std/bindings";
    let fds = ckb.inheritedFds();
    ckb.write(fds[0], new Uint8Array([0, 1, 2, 3]).buffer);
    ckb.close(fds[0]);
    ckb.exit(42);
    `;
  let code_hash = new Uint8Array([
    0xb9, 0x51, 0x23, 0xc7, 0x1a, 0x87, 0x0e, 0x3f, 0x0f, 0x74, 0xa7, 0xee,
    0x1d, 0xab, 0x82, 0x68, 0xdb, 0xfb, 0xc1, 0x40, 0x7b, 0x46, 0x73, 0x3e,
    0xbd, 0x1b, 0x41, 0xf8, 0x54, 0xb4, 0x32, 0x4a,
  ]);
  function test(via_code_hash) {
    let fds = ckb.pipe();
    // Unlike the C version, we only need to pass in two parameters: argv and inherited_fds.
    // * There is no need to use the argc parameter.
    // * There is no need to add 0 to the end of inherited_fds as a terminator.
    // * There is no need to pass in the pid address.
    let spawn_args = {
      argv: ["-e", js_code],
      inheritedFds: [fds[1]],
    };
    let pid = 0;
    if (via_code_hash) {
      pid = ckb.spawnCell(
        code_hash.buffer,
        ckb.SCRIPT_HASH_TYPE_TYPE,
        0,
        0,
        spawn_args,
      );
    } else {
      pid = ckb.spawn(0, ckb.SOURCE_CELL_DEP, 0, 0, spawn_args);
    }
    let txt = new Uint8Array(ckb.read(fds[0], 4));
    console.assert(txt[0] == 0);
    console.assert(txt[1] == 1);
    console.assert(txt[2] == 2);
    console.assert(txt[3] == 3);
    let ret = ckb.wait(pid);
    console.assert(ret == 42);
  }
  test(true);
  test(false);
  console.log("test_spawn done");
}

function test_exec() {
  const js_code = `
    import * as ckb from "@ckb-js-std/bindings";
    console.log("from exec");
    ckb.exit(0);
    `;
  let code_hash = new Uint8Array([
    0xb9, 0x51, 0x23, 0xc7, 0x1a, 0x87, 0x0e, 0x3f, 0x0f, 0x74, 0xa7, 0xee,
    0x1d, 0xab, 0x82, 0x68, 0xdb, 0xfb, 0xc1, 0x40, 0x7b, 0x46, 0x73, 0x3e,
    0xbd, 0x1b, 0x41, 0xf8, 0x54, 0xb4, 0x32, 0x4a,
  ]);
  ckb.execCell(
    code_hash.buffer,
    ckb.SCRIPT_HASH_TYPE_TYPE,
    0,
    0,
    "-e",
    js_code,
  );
  // can't be invoked
  ckb.exit(-10);
}

test_misc();
test_partial_loading(ckb.loadWitness);
test_partial_loading(ckb.loadCellData);
test_partial_loading_without_comparing(ckb.loadWitness);
test_partial_loading_without_comparing(ckb.loadCellData);
test_partial_loading_without_comparing(ckb.loadCell);
test_partial_loading_field_without_comparing(
  ckb.loadCellByField,
  ckb.CELL_FIELD_CAPACITY,
);
test_partial_loading_field_without_comparing(
  ckb.loadInputByField,
  ckb.INPUT_FIELD_OUT_POINT,
);
test_spawn();
// this test must be at the end
test_exec();
