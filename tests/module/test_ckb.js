import * as ckb from 'ckb';

function test_eval_script() {
    const script = `
        import * as ckb from 'ckb';
        globalThis.test_eval_script = 100;
    `;
    ckb.evalScript(script);
    console.assert(globalThis.test_eval_script === 100, "evalScript failed");
}

function test_eval_script_with_exception() {
    const script = `
        import * as ckb from 'ckb';
        abcdef.init();
    `;
    let success = false;
    try {
        ckb.evalScript(script);
    } catch (e) {
        success = true;
    }
    console.assert(success, "evalScript with exception failed");
}

console.log("test_ckb.js...");
test_eval_script();
test_eval_script_with_exception();
console.log("test_ckb.js ok");

