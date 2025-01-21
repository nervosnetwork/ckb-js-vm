import * as secp256k1 from '@ckb-js-std/bindings';
import * as ckb from "@ckb-js-std/bindings";
import * as misc from "@ckb-js-std/bindings";

function test_recovery(failure) {
    const recid = 1;
    const msg = misc.hex.decode(
        '6a0024347e28905e2587c4c7598332a39' +
        'ba6684bb6b74653511656a02bd20edb');
    const sig = misc.hex.decode(
        '76e6d0e5ea61b46fe10443fe5b4d1bc6' +
        'ce2d0d49d55e810312f7c22702e0548a' +
        '3969ce72940a34632f93ebd1b8d591c3' +
        '775428f035c6577e4adf8068b04819f0');
    const s = ckb.currentCycles();
    const expected_pubkey = misc.hex.decode(
        'aca98c5822b997c15f8c974386a11b14' +
        'a0d009a4d5156e145644573e82ef7e7b' +
        '226b9eb6173d6b4504606eb8d9558bde' +
        '98d12100836e92d306a40f337ed8a0f3');
    const e = ckb.currentCycles();
    console.log(`misc.hex.decode: ${e - s}`);

    // Verify the signature
    if (failure) {
        let success = false;
        const wrong_sig = misc.hex.decode(
            '00000000000000000000000000000000' +
            '00000000000000000000000000000000' +
            '00000000000000000000000000000000' +
            '00000000000000000000000000000000');
        try {
            success = secp256k1.recover(wrong_sig, recid, msg);
        } catch (e) {
            success = true;
        }
        console.assert(success, 'Signature recovery should fail');
    } else {
        const start = ckb.currentCycles();
        const pubkey = secp256k1.recover(sig, recid, msg);
        const end = ckb.currentCycles();
        console.log(`recover cycles: ${end - start}`);
        console.assert(
            misc.hex.encode(pubkey) === misc.hex.encode(expected_pubkey),
            'Signature recovery failed');
    }
    console.log('test_recovery ok');
}

function test_recovery_failure() {
    const recid = 1;
    const wrong_msg = misc.hex.decode(
        '00000000000000000000000000000000' +
        '00000000000000000000000000000000');
    const sig = misc.hex.decode(
        '76e6d0e5ea61b46fe10443fe5b4d1bc6' +
        'ce2d0d49d55e810312f7c22702e0548a' +
        '3969ce72940a34632f93ebd1b8d591c3' +
        '775428f035c6577e4adf8068b04819f0');
    const expected_pubkey = misc.hex.decode(
        'aca98c5822b997c15f8c974386a11b14' +
        'a0d009a4d5156e145644573e82ef7e7b' +
        '226b9eb6173d6b4504606eb8d9558bde' +
        '98d12100836e92d306a40f337ed8a0f3');

    const pubkey = secp256k1.recover(sig, recid, wrong_msg);
    console.assert(
        misc.hex.encode(pubkey) !== misc.hex.encode(expected_pubkey),
        'Signature recovery should fail');
    console.log('test_recovery_failure ok');
}

function test_verify() {
    const sig = misc.hex.decode(
        '76e6d0e5ea61b46fe10443fe5b4d1bc6' +
        'ce2d0d49d55e810312f7c22702e0548a' +
        '3969ce72940a34632f93ebd1b8d591c3' +
        '775428f035c6577e4adf8068b04819f0');
    const msg = misc.hex.decode(
        '6a0024347e28905e2587c4c7598332a39' +
        'ba6684bb6b74653511656a02bd20edb');
    const pubkey = misc.hex.decode(
        'aca98c5822b997c15f8c974386a11b14' +
        'a0d009a4d5156e145644573e82ef7e7b' +
        '226b9eb6173d6b4504606eb8d9558bde' +
        '98d12100836e92d306a40f337ed8a0f3');
    const start = ckb.currentCycles();
    const success = secp256k1.verify(sig, msg, pubkey);
    const end = ckb.currentCycles();
    console.log(`verify cycles: ${end - start}`);
    console.assert(success, 'test_verify failed');

    console.log('test_verify ok');
}

function test_parse_pubkey() {
    const pubkey = misc.hex.decode(
        '0375fbccbf29be9408ed96ca232fb941' +
        'b358e6158ace9fbfe8214c994d38bd9ff9');
    const start = ckb.currentCycles();
    const out_pubkey = secp256k1.parsePubkey(pubkey);
    const end = ckb.currentCycles();
    console.log(`parsePubkey cycles: ${end - start}`);
    console.assert(
        misc.hex.encode(out_pubkey) ===
            'f99fbd384d994c21e8bf9fce8a15e658' +
                'b341b92f23ca96ed0894be29bfccfb75' +
                '3d797a1b2ce723964030b3ef1e31656b' +
                '04a9c3fadcf100a613b385fec85620d1',
        'parsePubkey failed');
    console.log('test_parse_pubkey ok');
}

function test_serialize_pubkey() {
    const pubkey = misc.hex.decode(
        'f99fbd384d994c21e8bf9fce8a15e658' +
        'b341b92f23ca96ed0894be29bfccfb75' +
        '3d797a1b2ce723964030b3ef1e31656b' +
        '04a9c3fadcf100a613b385fec85620d1');
    const start = ckb.currentCycles();
    const out_pubkey = secp256k1.serializePubkey(pubkey, true);
    const end = ckb.currentCycles();
    console.log(`serializePubkey: ${end - start}`);
    console.assert(
        misc.hex.encode(out_pubkey) ===
            '0375fbccbf29be9408ed96ca232fb941' +
                'b358e6158ace9fbfe8214c994d38bd9ff9',
        'serializePubkey failed');

    const pubkey2 = misc.hex.decode(
        '11dae3a18c58627d4564aff118f9b49f' +
        'c1dc48992e0f615cef19732b7d8842d4' +
        '95a0eb41da0b71a3e18dd063e9097d40' +
        '944936ee93f498b26188faaa02276bde');
    const out_pubkey2 = secp256k1.serializePubkey(pubkey2, false);
    console.assert(
        misc.hex.encode(out_pubkey2) ===
            '04d442887d2b7319ef5c610f2e9948dc' +
                'c19fb4f918f1af64457d62588ca1e3da' +
                '11de6b2702aafa8861b298f493ee3649' +
                '94407d09e963d08de1a3710bda41eba095',
        'serializePubkey failed');

    console.log('test_serialize_pubkey ok');
}

function test_func_not_found() {
    let success = false;
    try {
        secp256k1.recover_not_found(1, 2, 3);
    } catch (e) {
        success = true;
    }
    console.assert(success, 'test_func_not_found failed');
    console.log('test_func_not_found ok');
}

console.log('test_secp256k1.js ...');
test_recovery(true);
test_recovery(false);
test_recovery_failure();
test_verify();
test_parse_pubkey();
test_serialize_pubkey();
test_func_not_found();
console.log('test_secp256k1.js ok');
