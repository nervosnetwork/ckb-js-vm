import * as secp256k1 from 'secp256k1';

function hexStringToUint8Array(hexString) {
    // Remove any non-hex characters (like spaces and commas)
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

function test_recovery() {
    const recid = 1;
    const msg = hexStringToUint8Array('6a0024347e28905e2587c4c7598332a39ba6684bb6b74653511656a02bd20edb');
    const sig = hexStringToUint8Array('76e6d0e5ea61b46fe10443fe5b4d1bc6ce2d0d49d55e810312f7c22702e0548a3969ce72940a34632f93ebd1b8d591c3775428f035c6577e4adf8068b04819f0');
    const expected_pubkey = hexStringToUint8Array('aca98c5822b997c15f8c974386a11b14a0d009a4d5156e145644573e82ef7e7b226b9eb6173d6b4504606eb8d9558bde98d12100836e92d306a40f337ed8a0f3');

    // Verify the signature
    const pubkey = secp256k1.recover(sig.buffer, recid, msg.buffer);
    console.assert(arrayBufferToHexString(pubkey) === arrayBufferToHexString(expected_pubkey), 'Signature recovery failed');
    console.log("test_recovery ok");
}

// TODO
// this can't work due to bug: https://github.com/bellard/quickjs/issues/232
function test_func_not_found() {
    console.log("throwing error now ...");
    throw new Error("test_func_not_found");
    secp256k1.recover_not_found(1, 2, 3);
    console.assert(false, "test_func_not_found failed");
}

console.log("test_secp256k1.js ...");
test_recovery();
test_func_not_found();
console.log("test_secp256k1.js ok");
