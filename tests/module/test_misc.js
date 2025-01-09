import * as misc from 'misc';
import * as ckb from 'ckb';

function test_ckb_smt_verify1() {
    // Test vectors from the Rust example
    const keyHex =
        '381dc5391dab099da5e28acd1ad859a051cf18ace804d037f12819c6fbc0e18b';
    const valueHex =
        '9158ce9b0e11dd150ba2ae5d55c1db04b1c5986ec626f2e38a93fe8ad0b2923b';
    const rootHashHex =
        'ebe0fab376cd802d364eeb44af20c67a74d6183a33928fead163120ef12e6e06';
    const proofHex =
        '4c4fff51ff322de8a89fe589987f97220cfcb6820bd798b31a0b56ffea221093d35f909e580b00000000000000000000000000000000000000000000000000000000000000';

    // Update the conversions to use misc.encoding.decodeHex
    const key = misc.encoding.decodeHex(keyHex);
    const value = misc.encoding.decodeHex(valueHex);
    const rootHash = misc.encoding.decodeHex(rootHashHex);
    const proof = misc.encoding.decodeHex(proofHex);

    // Create new SMT instance
    const smt = new misc.Smt();

    // Insert key-value pair and measure cycles
    const startInsert = ckb.current_cycles();
    smt.insert(key, value);
    const endInsert = ckb.current_cycles();
    console.log(`SMT insert cycles: ${endInsert - startInsert}`);

    // Verify proof and measure cycles
    const startVerify = ckb.current_cycles();
    const isValid = smt.verify(rootHash, proof);
    const endVerify = ckb.current_cycles();
    console.log(`SMT verify cycles: ${endVerify - startVerify}`);

    console.assert(isValid === true, 'SMT verification failed');
    console.log('test_ckb_smt_verify1 ok');
}

function test_ckb_smt_verify2() {
    const keyHex =
        'a9bb945be71f0bd2757d33d2465b6387383da42f321072e47472f0c9c7428a8a';
    const valueHex =
        'a939a47335f777eac4c40fbc0970e25f832a24e1d55adc45a7b76d63fe364e82';
    const rootHashHex =
        '6e5c722644cd55cef8c4ed886cd8b44027ae9ed129e70a4b67d87be1c6857842';
    const proofHex =
        '4c4fff51fa8aaa2aece17b92ec3f202a40a09f7286522bae1e5581a2a49195ab6781b1b8090000000000000000000000000000000000000000000000000000000000000000';

    const key = misc.encoding.decodeHex(keyHex);
    const value = misc.encoding.decodeHex(valueHex);
    const rootHash = misc.encoding.decodeHex(rootHashHex);
    const proof = misc.encoding.decodeHex(proofHex);

    const smt = new misc.Smt();

    const startInsert = ckb.current_cycles();
    smt.insert(key, value);
    const endInsert = ckb.current_cycles();
    console.log(`SMT verify2 insert cycles: ${endInsert - startInsert}`);

    const startVerify = ckb.current_cycles();
    const isValid = smt.verify(rootHash, proof);
    const endVerify = ckb.current_cycles();
    console.log(`SMT verify2 verify cycles: ${endVerify - startVerify}`);

    console.assert(isValid === true, 'SMT verification2 failed');
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

    const key = misc.encoding.decodeHex(keyHex);
    const value = misc.encoding.decodeHex(valueHex);
    const rootHash = misc.encoding.decodeHex(rootHashHex);
    const proof = misc.encoding.decodeHex(proofHex);

    const smt = new misc.Smt();

    const startInsert = ckb.current_cycles();
    smt.insert(key, value);
    const endInsert = ckb.current_cycles();
    console.log(`SMT verify3 insert cycles: ${endInsert - startInsert}`);

    const startVerify = ckb.current_cycles();
    const isValid = smt.verify(rootHash, proof);
    const endVerify = ckb.current_cycles();
    console.log(`SMT verify3 verify cycles: ${endVerify - startVerify}`);

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

    const key = misc.encoding.decodeHex(keyHex);
    const value = misc.encoding.decodeHex(valueHex);
    const rootHash = misc.encoding.decodeHex(rootHashHex);
    const proof = misc.encoding.decodeHex(proofHex);

    const smt = new misc.Smt();

    const startInsert = ckb.current_cycles();
    smt.insert(key, value);
    const endInsert = ckb.current_cycles();
    console.log(`SMT verify invalid insert cycles: ${endInsert - startInsert}`);

    const startVerify = ckb.current_cycles();
    const isValid = smt.verify(rootHash, proof);
    const endVerify = ckb.current_cycles();
    console.log(`SMT verify invalid verify cycles: ${endVerify - startVerify}`);

    console.assert(isValid === false, 'SMT invalid verification should fail');
    console.log('test_ckb_smt_verify_invalid ok');
}

function test_base64_encode() {
    const inputHex = '48656c6c6f20576f726c6421'; // "Hello World!" in hex
    const expectedBase64 = 'SGVsbG8gV29ybGQh';

    const input = misc.encoding.decodeHex(inputHex);
    const encoded = misc.encoding.encodeBase64(input);

    console.assert(encoded === expectedBase64, 'Base64 encoding failed');
    console.log('test_base64_encode ok');
}

function test_base64_decode() {
    const base64Input = 'SGVsbG8gV29ybGQh'; // "Hello World!" in base64
    const expectedHex = '48656c6c6f20576f726c6421';

    const decoded = misc.encoding.decodeBase64(base64Input);
    const result = misc.encoding.encodeHex(decoded);

    console.assert(result === expectedHex, 'Base64 decoding failed');
    console.log('test_base64_decode ok');
}

// Add the new test cases to the main execution
console.log('test_misc.js ...');
test_ckb_smt_verify1();
test_ckb_smt_verify2();
test_ckb_smt_verify3();
test_ckb_smt_verify_invalid();
test_base64_encode();
test_base64_decode();
console.log('test_misc.js ok');
