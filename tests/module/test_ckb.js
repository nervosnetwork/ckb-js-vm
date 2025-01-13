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

console.log("test_ckb.js...");
test_eval_script();
test_eval_script_with_exception();
test_eval_script_no_module();
test_eval_script_no_module_with_exception();
console.log("test_ckb.js ok");

