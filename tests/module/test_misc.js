import * as misc from "@ckb-js-std/bindings";
import * as ckb from "@ckb-js-std/bindings";

function test_ckb_smt_verify1(failure) {
    // Test vectors from the Rust example
    const keyHex =
        '381dc5391dab099da5e28acd1ad859a051cf18ace804d037f12819c6fbc0e18b';
    const valueHex =
        '9158ce9b0e11dd150ba2ae5d55c1db04b1c5986ec626f2e38a93fe8ad0b2923b';
    const rootHashHex =
        'ebe0fab376cd802d364eeb44af20c67a74d6183a33928fead163120ef12e6e06';
    const proofHex =
        '4c4fff51ff322de8a89fe589987f97220cfcb6820bd798b31a0b56ffea221093d35f909e580b00000000000000000000000000000000000000000000000000000000000000';

    // Update the conversions to use misc.hex.decode
    const key = misc.hex.decode(keyHex);
    const value = misc.hex.decode(valueHex);
    const rootHash = misc.hex.decode(rootHashHex);
    const proof = misc.hex.decode(proofHex);

    // Create new SMT instance
    const smt = new misc.Smt();

    // Insert key-value pair and measure cycles
    const startInsert = ckb.currentCycles();
    smt.insert(key, value);
    const endInsert = ckb.currentCycles();
    // console.log(`SMT insert cycles: ${endInsert - startInsert}`);

    if (failure) {
        // Verify proof and measure cycles
        const startVerify = ckb.currentCycles();
        const wrongRootHash = misc.hex.decode(
            '0000000000000000000000000000000000000000000000000000000000000000');
        const isValid = smt.verify(wrongRootHash, proof);
        const endVerify = ckb.currentCycles();
        // console.log(`SMT verify cycles: ${endVerify - startVerify}`);
        console.assert(isValid === false, 'SMT verification should fail');
    } else {
        // Verify proof and measure cycles
        const startVerify = ckb.currentCycles();
        const isValid = smt.verify(rootHash, proof);
        const endVerify = ckb.currentCycles();
        // console.log(`SMT verify cycles: ${endVerify - startVerify}`);
        console.assert(isValid === true, 'SMT verification failed');
    }

    console.log('test_ckb_smt_verify1 ok');
}

function test_ckb_smt_verify2(failure) {
    const keyHex =
        'a9bb945be71f0bd2757d33d2465b6387383da42f321072e47472f0c9c7428a8a';
    const valueHex =
        'a939a47335f777eac4c40fbc0970e25f832a24e1d55adc45a7b76d63fe364e82';
    const rootHashHex =
        '6e5c722644cd55cef8c4ed886cd8b44027ae9ed129e70a4b67d87be1c6857842';
    const proofHex =
        '4c4fff51fa8aaa2aece17b92ec3f202a40a09f7286522bae1e5581a2a49195ab6781b1b8090000000000000000000000000000000000000000000000000000000000000000';

    const key = misc.hex.decode(keyHex);
    const value = misc.hex.decode(valueHex);
    const rootHash = misc.hex.decode(rootHashHex);
    const proof = misc.hex.decode(proofHex);

    const smt = new misc.Smt();

    const startInsert = ckb.currentCycles();
    smt.insert(key, value);
    const endInsert = ckb.currentCycles();
    // console.log(`SMT verify2 insert cycles: ${endInsert - startInsert}`);

    if (failure) {
        const startVerify = ckb.currentCycles();
        const wrongProof = misc.hex.decode(
            '0000000000000000000000000000000000000000000000000000000000000000');
        const isValid = smt.verify(rootHash, wrongProof);
        const endVerify = ckb.currentCycles();
        // console.log(`SMT verify2 verify cycles: ${endVerify - startVerify}`);
        console.assert(isValid === false, 'SMT verification2 should fail');
    } else {
        const startVerify = ckb.currentCycles();
        const isValid = smt.verify(rootHash, proof);
        const endVerify = ckb.currentCycles();
        // console.log(`SMT verify2 verify cycles: ${endVerify - startVerify}`);
        console.assert(isValid === true, 'SMT verification2 failed');
    }
    console.log('test_ckb_smt_verify2 ok');
}

function test_ckb_smt_verify3() {
    const keyHex =
        'e8c0265680a02b680b6cbc880348f062b825b28e237da7169aded4bcac0a04e5';
    const valueHex =
        '2ca41595841e46ce8e74ad749e5c3f1d17202150f99c3d8631233ebdd19b19eb';
    const rootHashHex =
        'c8f513901e34383bcec57c368628ce66da7496df0a180ee1e021df3d97cb8f7b';
    const proofHex =
        '4c4fff51fa8aaa2aece17b92ec3f202a40a09f7286522bae1e5581a2a49195ab6781b1b8090000000000000000000000000000000000000000000000000000000000000000';

    const key = misc.hex.decode(keyHex);
    const value = misc.hex.decode(valueHex);
    const rootHash = misc.hex.decode(rootHashHex);
    const proof = misc.hex.decode(proofHex);

    const smt = new misc.Smt();

    const startInsert = ckb.currentCycles();
    smt.insert(key, value);
    const endInsert = ckb.currentCycles();
    // console.log(`SMT verify3 insert cycles: ${endInsert - startInsert}`);

    const startVerify = ckb.currentCycles();
    const isValid = smt.verify(rootHash, proof);
    const endVerify = ckb.currentCycles();
    // console.log(`SMT verify3 verify cycles: ${endVerify - startVerify}`);

    console.assert(isValid === true, 'SMT verification3 failed');
    console.log('test_ckb_smt_verify3 ok');
}

function test_ckb_smt_verify_invalid() {
    const keyHex =
        'e8c0265680a02b680b6cbc880348f062b825b28e237da7169aded4bcac0a04e5';
    const valueHex =
        '2ca41595841e46ce8e74ad749e5c3f1d17202150f99c3d8631233ebdd19b19eb';
    const rootHashHex =
        'a4cbf1b69a848396ac759f362679e2b185ac87a17cba747d2db1ef6fd929042f';
    const proofHex =
        '4c50fe32845309d34f132cd6f7ac6a7881962401adc35c19a18d4fffeb511b97eabf86';

    const key = misc.hex.decode(keyHex);
    const value = misc.hex.decode(valueHex);
    const rootHash = misc.hex.decode(rootHashHex);
    const proof = misc.hex.decode(proofHex);

    const smt = new misc.Smt();

    const startInsert = ckb.currentCycles();
    smt.insert(key, value);
    const endInsert = ckb.currentCycles();
    // console.log(`SMT verify invalid insert cycles: ${endInsert - startInsert}`);

    const startVerify = ckb.currentCycles();
    const isValid = smt.verify(rootHash, proof);
    const endVerify = ckb.currentCycles();
    // console.log(`SMT verify invalid verify cycles: ${endVerify - startVerify}`);

    console.assert(isValid === false, 'SMT invalid verification should fail');
    console.log('test_ckb_smt_verify_invalid ok');
}

function test_base64_encode() {
    const inputHex = '48656c6c6f20576f726c6421';  // "Hello World!" in hex
    const expectedBase64 = 'SGVsbG8gV29ybGQh';

    const input = misc.hex.decode(inputHex);
    const encoded = misc.base64.encode(input);

    console.assert(encoded === expectedBase64, 'Base64 encoding failed');
    console.log('test_base64_encode ok');
}

function test_base64_decode() {
    const base64Input = 'SGVsbG8gV29ybGQh';  // "Hello World!" in base64
    const expectedHex = '48656c6c6f20576f726c6421';

    const decoded = misc.base64.decode(base64Input);
    const result = misc.hex.encode(decoded);

    console.assert(result === expectedHex, 'Base64 decoding failed');
    console.log('test_base64_decode ok');
}

function test_import_meta() {
    console.assert(import.meta.main, 'import.meta.main should be true');
    console.assert(import.meta.url.length > 0, 'import.meta.url should be true');
    console.log('test_import_meta ok');
}

function test_printf() {
    console.log("require", typeof require);
    const k = 100;
    console.log("hello", "world", 111.1234, {"a": 1, "b": 2}, [1, 2, 3], `k = ${k}`);
    misc.printf('Hello, World: %d', 100);
    const str = misc.sprintf("%s, %d", "Hello, World", 100);
    console.assert(str === "Hello, World, 100", 'sprintf failed');
    console.log('test_printf ok');
}

function test_require() {
    const ckb = require('@ckb-js-std/bindings');
    console.assert(typeof ckb.loadScript === 'function', 'require failed');
    console.assert(ckb.currentCycles() > 0, 'currentCycles failed');
    let success = false;
    try {
        const ckb = require("not existing module");
    } catch (e) {
        success = true;
    }
    console.assert(success, 'require should throw error');
}


function test_text_encoder() {
    const encoder = new misc.TextEncoder();
    const encoded = encoder.encode("你好世界");
    // Each Chinese character takes 3 bytes in UTF-8
    const expected = new Uint8Array([
        0xe4, 0xbd, 0xa0,  // 你
        0xe5, 0xa5, 0xbd,  // 好
        0xe4, 0xb8, 0x96,  // 世
        0xe7, 0x95, 0x8c   // 界
    ]);
    console.assert(
        encoded.byteLength === expected.length, 'Encoded length mismatch');
    for (let i = 0; i < encoded.length; i++) {
        console.assert(encoded[i] === expected[i], `Byte mismatch at position ${i}`);
    }
    console.log('test_text_encoder ok');

    const decoder = new misc.TextDecoder();
    const decoded = decoder.decode(encoded);
    console.assert(decoded === "你好世界", 'TextDecoder failed');
    console.log('test_text_decoder ok');
}

// Add the new test cases to the main execution
console.log('test_misc.js ...');
test_text_encoder();
test_printf();
test_require();
test_ckb_smt_verify1(true);
test_ckb_smt_verify1(false);
test_ckb_smt_verify2(true);
test_ckb_smt_verify2(false);
test_ckb_smt_verify3();
test_ckb_smt_verify_invalid();
test_base64_encode();
test_base64_decode();
test_import_meta();
console.log('test_misc.js ok');
