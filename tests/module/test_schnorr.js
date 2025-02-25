import { schnorr } from '@ckb-js-std/bindings';
import * as ckb from "@ckb-js-std/bindings";
import * as misc from "@ckb-js-std/bindings";

function test_verify() {
    const sig = misc.hex.decode(
        '52420fa2807eac7336c15ec7db76b41c3247f8457a1533bd783378e563cb33c43e5a8b17e91badaa290c02d3f8ce50df130c9d09c90c288d9d2be0e5976a5354');
    const msg = misc.hex.decode(
        '1bd69c075dd7b78c4f20a698b22a3fb9d7461525c39827d6aaf7a1628be0a283');
    // secret key: 0x7a1190ccfd7db3742080a56654f91066613df61f9d0ddd3b4d803ca5041c0678
    const pubkey = misc.hex.decode(
        '2504ea5763b6d7a51b50dbf5871e50f195b3e0297fe6272334be555d3e5231a6');
    const start = ckb.currentCycles();
    const xonlyPubkey = schnorr.parseXonlyPubkey(pubkey);
    const success = schnorr.verify(sig, msg, xonlyPubkey);
    const end = ckb.currentCycles();
    console.log(`verify cycles: ${end - start}`);
    console.assert(success, 'test_verify failed');

    console.log('test_verify ok');
}

function test_pubkey() {
    const pubkey = misc.hex.decode(
        '2504ea5763b6d7a51b50dbf5871e50f195b3e0297fe6272334be555d3e5231a6');
    const xonlyPubkey = schnorr.parseXonlyPubkey(pubkey);
    const pubkey2 = schnorr.serializeXonlyPubkey(xonlyPubkey);
    console.assert(misc.hex.encode(pubkey) === misc.hex.encode(pubkey2), 'test_pubkey failed');
}

function test_tagged_sha256() {
    const tag = misc.hex.decode('6d795f66616e63795f70726f746f636f6c');
    const msg = misc.hex.decode('48656c6c6f20576f726c6421');
    const hash = schnorr.taggedSha256(tag, msg);
    const expected = misc.hex.decode(
        '1bd69c075dd7b78c4f20a698b22a3fb9d7461525c39827d6aaf7a1628be0a283');
    console.assert(misc.hex.encode(hash) === misc.hex.encode(expected), 'test_tagged_sha256 failed');
}


console.log('test_schnorr.js ...');
test_verify();
test_pubkey();
test_tagged_sha256();
console.log('test_schnorr.js ok');
