import * as ckb from 'ckb';

function test_eval_script() {
    const script = `
        import * as ckb from 'ckb';
        globalThis.test_eval_script = 100;
    `;
    ckb.evalScript(script, true);
    console.assert(globalThis.test_eval_script === 100, "test_eval_script failed");
}

function test_eval_script_no_module() {
    const script = `
        let a = 100;
        let b = 200;
        a + b
    `;
    let result = ckb.evalScript(script);
    console.assert(result === 300, "test_eval_script_no_module failed");
}

function test_eval_script_no_module_with_exception() {
    const script = `
        let a = 100;
        let b = 200;
        abcdefg.abc = 100;
        a + b
    `;
    let success = false;
    try {
        ckb.evalScript(script);
    } catch (e) {
        success = true;
    }
    console.assert(success, "test_eval_script_no_module failed");
}


function test_eval_script_with_exception() {
    const script = `
        import * as ckb from 'ckb';
        abcdef.init();
    `;
    let success = false;
    try {
        ckb.evalScript(script, true);
    } catch (e) {
        success = true;
    }
    console.assert(success, "test_eval_script_with_exception with exception failed");
}

function test_parse_ext_json() {
    const json = `{
        "a": 100,
        "b": 200,
        "c": 0xFFFF,
        "d": 0o777,
        "e": 0b1111
        // comments are supported
    }`;
    let result = ckb.parseExtJSON(json);
    console.assert(result.a === 100, "test_parse_ext_json failed: a");
    console.assert(result.b === 200, "test_parse_ext_json failed: b");
    console.assert(result.c === 0xFFFF, "test_parse_ext_json failed: c");
    console.assert(result.d === 0o777, "test_parse_ext_json failed: d");
    console.assert(result.e === 0b1111, "test_parse_ext_json failed: e");
}

console.log("test_ckb.js...");
test_eval_script();
test_eval_script_with_exception();
test_eval_script_no_module();
test_eval_script_no_module_with_exception();
test_parse_ext_json();
console.log("test_ckb.js ok");

