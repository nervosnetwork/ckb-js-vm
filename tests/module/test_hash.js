import * as hash from "@ckb-js-std/bindings";
import * as ckb from "@ckb-js-std/bindings";

// Add global constant
const CKB_DEFAULT_HASH = 'ckb-default-hash';

function hexStringToUint8Array(hexString) {
    hexString = hexString.replace(/[^0-9A-Fa-f]/g, '');
    const bytes = new Uint8Array(hexString.length / 2);
    for (let i = 0; i < hexString.length; i += 2) {
        bytes[i / 2] = parseInt(hexString.substr(i, 2), 16);
    }
    return bytes;
}

function arrayBufferToHexString(buffer) {
    return Array.from(new Uint8Array(buffer))
        .map(b => b.toString(16).padStart(2, '0'))
        .join('');
}

function stringToUint8Array(str) {
    const arr = new Uint8Array(str.length);
    for (let i = 0; i < str.length; i++) {
        arr[i] = str.charCodeAt(i);
    }
    return arr;
}

function test_sha2_sha256_empty_string() {
    // SHA256 of empty string
    const expected =
        'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855';

    const sha256 = new hash.Sha256();
    const start = ckb.currentCycles();
    sha256.update(new Uint8Array(0).buffer);
    const result = sha256.finalize();
    const end = ckb.currentCycles();
    console.log(`empty string hash cycles: ${end - start}`);

    console.assert(
        arrayBufferToHexString(result) === expected,
        'Empty string hash failed');
    console.log('test_sha2_sha256_empty_string ok');
}

function test_keccak256_empty_string() {
    const expected =
        'c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470';

    const keccak256 = new hash.Keccak256();
    const start = ckb.currentCycles();
    keccak256.update(new Uint8Array(0).buffer);
    const result = keccak256.finalize();
    const end = ckb.currentCycles();
    console.log(`empty string hash cycles: ${end - start}`);

    console.assert(
        arrayBufferToHexString(result) === expected,
        'Empty string hash failed');
    console.log('test_keccak256_empty_string ok');
}


function test_sha2_sha256_basic_string() {
    // SHA256 of "hello"
    const input = hexStringToUint8Array('68656c6c6f');  // "hello" in hex
    const expected =
        '2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824';

    const sha256 = new hash.Sha256();
    const start = ckb.currentCycles();
    sha256.update(input.buffer);
    const result = sha256.finalize();
    const end = ckb.currentCycles();
    console.log(`basic string hash cycles: ${end - start}`);

    console.assert(
        arrayBufferToHexString(result) === expected,
        'Basic string hash failed');
    console.log('test_sha2_sha256_basic_string ok');
}

function test_sha2_sha256_multiple_updates() {
    // Testing multiple write operations
    const input1 = hexStringToUint8Array('68656c');  // "hel"
    const input2 = hexStringToUint8Array('6c6f');    // "lo"
    const expected =
        '2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824';

    const sha256 = new hash.Sha256();
    const start = ckb.currentCycles();
    sha256.update(input1.buffer);
    sha256.update(input2.buffer);
    const result = sha256.finalize();
    const end = ckb.currentCycles();
    console.log(`multiple updates hash cycles: ${end - start}`);

    console.assert(
        arrayBufferToHexString(result) === expected,
        'Multiple updates hash failed');
    console.log('test_sha2_sha256_multiple_updates ok');
}

function test_sha2_sha256_long_string() {
    // Testing with a longer string
    const input = 'a'.repeat(1000);
    const inputBytes = stringToUint8Array(input);
    const expected =
        '41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3';

    const sha256 = new hash.Sha256();
    const start = ckb.currentCycles();
    sha256.update(inputBytes.buffer);
    const result = sha256.finalize();
    const end = ckb.currentCycles();
    console.log(`long string hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected, 'Long string hash failed');
    console.log('test_sha2_sha256_long_string ok');
}

function test_sha2_sha256_error_handling() {
    let success = false;
    try {
        const sha256 = new hash.Sha256();
        sha256.update('invalid input');  // Should throw type error
    } catch (e) {
        success = true;
    }
    console.assert(success, 'Error handling test failed');
    console.log('test_sha2_sha256_error_handling ok');
}

function test_keccak256_basic_string() {
    // Keccak256 of "hello"
    const input = hexStringToUint8Array('68656c6c6f');  // "hello" in hex
    const expected =
        '1c8aff950685c2ed4bc3174f3472287b56d9517b9c948127319a09a7a36deac8';

    const keccak256 = new hash.Keccak256();
    const start = ckb.currentCycles();
    keccak256.update(input.buffer);
    const result = keccak256.finalize();
    const end = ckb.currentCycles();
    console.log(`basic string hash cycles: ${end - start}`);

    console.assert(
        arrayBufferToHexString(result) === expected,
        'Basic string hash failed');
    console.log('test_keccak256_basic_string ok');
}

function test_keccak256_multiple_updates() {
    // Testing multiple write operations
    const input1 = hexStringToUint8Array('68656c');  // "hel"
    const input2 = hexStringToUint8Array('6c6f');    // "lo"
    const expected =
        '1c8aff950685c2ed4bc3174f3472287b56d9517b9c948127319a09a7a36deac8';

    const keccak256 = new hash.Keccak256();
    const start = ckb.currentCycles();
    keccak256.update(input1.buffer);
    keccak256.update(input2.buffer);
    const result = keccak256.finalize();
    const end = ckb.currentCycles();
    console.log(`multiple updates hash cycles: ${end - start}`);

    console.assert(
        arrayBufferToHexString(result) === expected,
        'Multiple updates hash failed');
    console.log('test_keccak256_multiple_updates ok');
}

function test_keccak256_long_string() {
    // Testing with a longer string
    const input = 'a'.repeat(1000);
    const inputBytes = stringToUint8Array(input);
    const expected =
        'b6a4ac1f51884d71f30fa397a5e155de3099e11fc0edef5d08b646e621e19de9';

    const keccak256 = new hash.Keccak256();
    const start = ckb.currentCycles();
    keccak256.update(inputBytes.buffer);
    const result = keccak256.finalize();
    const end = ckb.currentCycles();
    console.log(`long string hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected, 'Long string hash failed');
    console.log('test_keccak256_long_string ok');
}

function test_blake2b_empty_string() {
    // Blake2b of empty string with ckb-default-hash personalization
    const expected =
        '44f4c69744d5f8c55d642062949dcae49bc4e7ef43d388c5a12f42b5633d163e';

    const blake2b = new hash.Blake2b(CKB_DEFAULT_HASH);
    const start = ckb.currentCycles();
    blake2b.update(new Uint8Array(0).buffer);
    const result = blake2b.finalize();
    const end = ckb.currentCycles();
    console.log(`empty string hash cycles: ${end - start}`);
    console.log(`${arrayBufferToHexString(result)}`)
    console.assert(
        arrayBufferToHexString(result) === expected,
        'Empty string hash failed');
    console.log('test_blake2b_empty_string ok');
}

function test_blake2b_basic_string() {
    // Blake2b of "hello"
    const input = hexStringToUint8Array('68656c6c6f');  // "hello" in hex
    const expected =
        '2da1289373a9f6b7ed21db948f4dc5d942cf4023eaef1d5a2b1a45b9d12d1036';

    const blake2b = new hash.Blake2b(CKB_DEFAULT_HASH);
    const start = ckb.currentCycles();
    blake2b.update(input.buffer);
    const result = blake2b.finalize();
    const end = ckb.currentCycles();
    console.log(`basic string hash cycles: ${end - start}`);
    console.log(`${arrayBufferToHexString(result)}`)
    console.assert(
        arrayBufferToHexString(result) === expected,
        'Basic string hash failed');
    console.log('test_blake2b_basic_string ok');
}

function test_blake2b_multiple_updates() {
    // Testing multiple write operations
    const input1 = hexStringToUint8Array('68656c');  // "hel"
    const input2 = hexStringToUint8Array('6c6f');    // "lo"
    const expected =
        '2da1289373a9f6b7ed21db948f4dc5d942cf4023eaef1d5a2b1a45b9d12d1036';

    const blake2b = new hash.Blake2b(CKB_DEFAULT_HASH);
    const start = ckb.currentCycles();
    blake2b.update(input1.buffer);
    blake2b.update(input2.buffer);
    const result = blake2b.finalize();
    const end = ckb.currentCycles();
    console.log(`multiple updates hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected,
        'Multiple updates hash failed');
    console.log('test_blake2b_multiple_updates ok');
}

function test_blake2b_long_string() {
    // Testing with a longer string
    const input = 'a'.repeat(1000);
    const inputBytes = stringToUint8Array(input);
    const expected =
        '0f3a195c4c9504134731fbec2e3762342abe9a70b333d2d14ead1c2787cdb937';

    const blake2b = new hash.Blake2b(CKB_DEFAULT_HASH);
    const start = ckb.currentCycles();
    blake2b.update(inputBytes.buffer);
    const result = blake2b.finalize();
    const end = ckb.currentCycles();
    console.log(`long string hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected, 'Long string hash failed');
    console.log('test_blake2b_long_string ok');
}

function test_ripemd160_empty_string() {
    // RIPEMD160 of empty string
    const expected = '9c1185a5c5e9fc54612808977ee8f548b2258d31';

    const ripemd160 = new hash.Ripemd160();
    const start = ckb.currentCycles();
    ripemd160.update(new Uint8Array(0).buffer);
    const result = ripemd160.finalize();
    const end = ckb.currentCycles();
    console.log(`empty string hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected,
        'Empty string hash failed');
    console.log('test_ripemd160_empty_string ok');
}

function test_ripemd160_basic_string() {
    // RIPEMD160 of "hello"
    const input = hexStringToUint8Array('68656c6c6f');  // "hello" in hex
    const expected = '108f07b8382412612c048d07d13f814118445acd';

    const ripemd160 = new hash.Ripemd160();
    const start = ckb.currentCycles();
    ripemd160.update(input.buffer);
    const result = ripemd160.finalize();
    const end = ckb.currentCycles();
    console.log(`basic string hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected,
        'Basic string hash failed');
    console.log('test_ripemd160_basic_string ok');
}

function test_ripemd160_multiple_updates() {
    // Testing multiple write operations
    const input1 = hexStringToUint8Array('68656c');  // "hel"
    const input2 = hexStringToUint8Array('6c6f');    // "lo"
    const expected = '108f07b8382412612c048d07d13f814118445acd';

    const ripemd160 = new hash.Ripemd160();
    const start = ckb.currentCycles();
    ripemd160.update(input1.buffer);
    ripemd160.update(input2.buffer);
    const result = ripemd160.finalize();
    const end = ckb.currentCycles();
    console.log(`multiple updates hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected,
        'Multiple updates hash failed');
    console.log('test_ripemd160_multiple_updates ok');
}

function test_ripemd160_long_string() {
    // Testing with a longer string
    const input = 'a'.repeat(1000);
    const inputBytes = stringToUint8Array(input);
    const expected = 'aa69deee9a8922e92f8105e007f76110f381e9cf';

    const ripemd160 = new hash.Ripemd160();
    const start = ckb.currentCycles();
    ripemd160.update(inputBytes.buffer);
    const result = ripemd160.finalize();
    const end = ckb.currentCycles();
    console.log(`long string hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected, 'Long string hash failed');
    console.log('test_ripemd160_long_string ok');
}

console.log('test_hash.js ...');
test_sha2_sha256_empty_string();
test_sha2_sha256_basic_string();
test_sha2_sha256_multiple_updates();
test_sha2_sha256_long_string();
test_sha2_sha256_error_handling();
test_keccak256_empty_string();
test_keccak256_basic_string();
test_keccak256_multiple_updates();
test_keccak256_long_string();
test_blake2b_empty_string();
test_blake2b_basic_string();
test_blake2b_multiple_updates();
test_blake2b_long_string();
test_ripemd160_empty_string();
test_ripemd160_basic_string();
test_ripemd160_multiple_updates();
test_ripemd160_long_string();
console.log('test_hash.js ok');
