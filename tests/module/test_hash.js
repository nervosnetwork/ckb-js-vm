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
        '8e5e657ab293b4f6146feed495bf87c4c3c5e0cfca6aef78f924311866ea277bf359afae4a763af955e23abdad3f9c941c9e4a0a795c73d8b205679ab68eb294';

    const blake2b = new hash.Blake2b(CKB_DEFAULT_HASH);
    const start = ckb.currentCycles();
    blake2b.update(new Uint8Array(0).buffer);
    const result = blake2b.finalize();
    const end = ckb.currentCycles();
    console.log(`empty string hash cycles: ${end - start}`);
    console.assert(
        arrayBufferToHexString(result) === expected,
        'Empty string hash failed');
    console.log('test_blake2b_empty_string ok');
}

function test_blake2b_basic_string() {
    // Blake2b of "hello"
    const input = hexStringToUint8Array('68656c6c6f');  // "hello" in hex
    const expected =
        'a1e60e2fbb09f4f071f4e3cc30791fcdd694bfda60223c5b3912ae3d762a6ba59c9e90e9fd185c10eb545a4ca86a9bdc72539d5160576707a43760f4b50013ba';

    const blake2b = new hash.Blake2b(CKB_DEFAULT_HASH);
    const start = ckb.currentCycles();
    blake2b.update(input.buffer);
    const result = blake2b.finalize();
    const end = ckb.currentCycles();
    console.log(`basic string hash cycles: ${end - start}`);
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
        'a1e60e2fbb09f4f071f4e3cc30791fcdd694bfda60223c5b3912ae3d762a6ba59c9e90e9fd185c10eb545a4ca86a9bdc72539d5160576707a43760f4b50013ba';

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
        'e2bc748623468948e5483c45f6250557a672288edf3677535502e83f574f4d90aa296599678a010b7d5f1b0eb7249083f7294e6b80fea1351ca042dd10ddd7d4';

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
    console.log("result: ", arrayBufferToHexString(result));
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

    console.log("result: ", arrayBufferToHexString(result));
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

    console.log("result: ", arrayBufferToHexString(result));
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
    console.log("result: ", arrayBufferToHexString(result));
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
