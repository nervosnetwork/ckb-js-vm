import * as ckb from "ckb";

const ARRAY8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07];
function expect_array(a, b) {
    if (a.byteLength != b.length) {
        console.assert(false, `expect_array failed: length mismatched, ${a} VS ${b}`);
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
    console.assert(has_exception, 'Error, no exception found');
    return error_code;
}

function test_partial_loading(load_func) {
    console.log('test_partial_loading ...');
    let data = load_func(0, ckb.SOURCE_OUTPUT);
    expect_array(data, ARRAY8);
    data = load_func(0, ckb.SOURCE_OUTPUT, 100);
    expect_array(data, ARRAY8);
    let length = load_func(0, ckb.SOURCE_OUTPUT, 0);
    console.assert(length === 8, 'length != 8');
    length = load_func(0, ckb.SOURCE_OUTPUT, 0, 1);
    console.assert(length === 7, 'length != 7');
    data = load_func(0, ckb.SOURCE_OUTPUT, 7);
    expect_array(data, ARRAY8.slice(0, 7));
    data = load_func(0, ckb.SOURCE_OUTPUT, 7, 1);
    expect_array(data, ARRAY8.slice(1, 8));

    let error_code = must_throw_exception(() => {
        load_func(1001, ckb.SOURCE_OUTPUT);
    });
    // CKB_INDEX_OUT_OF_BOUND
    console.log(error_code === 1);
    error_code = must_throw_exception(() => {
        load_func(0, ckb.SOURCE_OUTPUT + 1000n);
    });
    console.log('test_partial_loading done');
}


function test_partial_loading_without_comparing(load_func) {
    console.log('test_partial_loading_without_comparing ...');
    let data = load_func(0, ckb.SOURCE_OUTPUT);
    console.assert(data);
    let length = load_func(0, ckb.SOURCE_OUTPUT, 0);
    console.assert(length > 0);
    length = load_func(0, ckb.SOURCE_OUTPUT, 0, 1);
    console.assert(length > 0);
    data = load_func(0, ckb.SOURCE_OUTPUT, 7);
    console.assert(data);
    data = load_func(0, ckb.SOURCE_OUTPUT, 7, 1);
    console.assert(data);

    must_throw_exception(() => {
        load_func(1001n, ckb.SOURCE_OUTPUT);
    });
    must_throw_exception(() => {
        load_func(0, ckb.SOURCE_OUTPUT + 1000n);
    });
    console.log('test_partial_loading done');
}

function test_partial_loading_field_without_comparing(load_func, field) {
    console.log('test_partial_loading_field_without_comparing ...');
    let data = load_func(0, ckb.SOURCE_INPUT, field);
    console.assert(data);
    let length = load_func(0, ckb.SOURCE_INPUT, field, 0);
    console.assert(length > 0);
    length = load_func(0, ckb.SOURCE_INPUT, field, 0, 1);
    console.assert(length > 0);
    data = load_func(0, ckb.SOURCE_INPUT, field, 7);
    console.assert(data);
    data = load_func(0, ckb.SOURCE_INPUT, field, 7, 1);
    console.assert(data);

    must_throw_exception(() => {
        load_func(1001n, ckb.SOURCE_INPUT, field);
    });
    must_throw_exception(() => {
        load_func(0, ckb.SOURCE_INPUT + 1000n, field);
    });
    console.log('test_partial_loading_field_without_comparing done');
}

function test_misc() {
    console.log('test_misc ....');
    let hash = ckb.loadTxHash();
    console.assert(hash.byteLength == 32);
    hash = ckb.loadScriptHash();
    console.assert(hash.byteLength == 32);
    let version = ckb.vmVersion();
    console.assert(version >= 0);
    let cycles = ckb.currentCycles();
    console.assert(cycles > 0);
    let cycles2 = ckb.currentCycles();
    console.assert(cycles2 > cycles);
    console.log('test_misc done');
}

function test_spawn() {
    console.log('test_spawn ...');
    const js_code = `
    import * as ckb from 'ckb';
    let fds = ckb.inheritedFds();
    ckb.write(fds[0], new Uint8Array([0, 1, 2, 3]));
    ckb.close(fds[0]);
    ckb.exit(42);
    `;
    let code_hash = new Uint8Array([
        0xdf, 0x97, 0x77, 0x78, 0x08, 0x9b, 0xf3, 0x3f, 0xc5, 0x1f, 0x22, 0x45, 0xfa, 0x6d, 0xb7, 0xfa,
        0x18, 0x19, 0xd5, 0x03, 0x11, 0x31, 0xa8, 0x3d, 0x4e, 0xcb, 0xcb, 0x6c, 0xba, 0x07, 0xce, 0x91
    ]);
    let fds = ckb.pipe();
    // Unlike the C version, we only need to pass in two parameters: argv and inherited_fds.
    // * There is no need to use the argc parameter.
    // * There is no need to add 0 to the end of inherited_fds as a terminator.
    // * There is no need to pass in the pid address.
    let spawn_args = {
        argv: ['-e', js_code],
        inherited_fds: [fds[1]],
    };
    let pid = ckb.spawnCell(code_hash, ckb.SCRIPT_HASH_TYPE_TYPE, 0, 0, spawn_args);
    let txt = new Uint8Array(ckb.read(fds[0], 4));
    console.assert(txt[0] == 0);
    console.assert(txt[1] == 1);
    console.assert(txt[2] == 2);
    console.assert(txt[3] == 3);
    let ret = ckb.wait(pid);
    console.assert(ret == 42);
    console.log('test_spawn done');
}

test_misc();
test_partial_loading(ckb.loadWitness);
test_partial_loading(ckb.loadCellData);
test_partial_loading_without_comparing(ckb.loadWitness);
test_partial_loading_without_comparing(ckb.loadCellData);
test_partial_loading_without_comparing(ckb.loadTransaction);
test_partial_loading_without_comparing(ckb.loadScript);
test_partial_loading_without_comparing(ckb.loadCell);
test_partial_loading_field_without_comparing(ckb.loadCellByField, ckb.CELL_FIELD_CAPACITY);
test_partial_loading_field_without_comparing(ckb.loadInputByField, ckb.INPUT_FIELD_OUT_POINT);
test_spawn()

ckb.exit(0);
