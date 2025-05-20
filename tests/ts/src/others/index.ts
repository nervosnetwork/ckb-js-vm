import * as bindings from "@ckb-js-std/bindings";
import { hex } from "@ckb-js-std/bindings";
import {
  log,
  mol,
  numFromBytes,
  numToBytes,
  HighLevel,
} from "@ckb-js-std/core";

function testMoleculeBigInt() {
  const num128 = 2n ** 64n + 3n;
  let encode128 = mol.Uint128.encode(num128);
  console.assert(
    hex.encode(encode128) === "0300000000000000" + "0100000000000000",
    "Uint128",
  );

  const num256 = 2n ** 128n + 3n;
  let encode256 = mol.Uint256.encode(num256);
  console.assert(
    hex.encode(encode256) ===
      "0300000000000000" +
        "0000000000000000" +
        "0100000000000000" +
        "0000000000000000",
    "Uint256",
  );

  const num512 = 2n ** 256n + 3n;
  let encode512 = mol.Uint512.encode(num512);
  console.assert(
    hex.encode(encode512) ===
      "0300000000000000" +
        "0000000000000000" +
        "0000000000000000" +
        "0000000000000000" +
        "0100000000000000" +
        "0000000000000000" +
        "0000000000000000" +
        "0000000000000000",
    "Uint512",
  );

  let value = mol.Uint64.encode(18446744073709551615n);
  console.assert(hex.encode(value) === "ff".repeat(8), "mol.Uint64");
}

function testMoleculeString() {
  let emptyStr = mol.String.encode("");
  console.assert(hex.encode(emptyStr) === "00000000", "String(empty)");

  let str = "hello, world";
  let encodeStr = mol.String.encode(str);
  console.log(`${hex.encode(encodeStr)}`);
  console.assert(
    hex.encode(encodeStr) === "0c00000068656c6c6f2c20776f726c64",
    "String",
  );
}

function testText() {
  log.setLevel(log.LogLevel.Debug);
  let str = "hello, world";
  let encoder = new bindings.TextEncoder();
  let encoded = encoder.encode(str);
  let decoder = new bindings.TextDecoder();
  let decoded = decoder.decode(encoded);

  console.assert(decoded === str);
}

function testMoleculeOption() {
  let ret = mol.Uint8Opt.encode(0);
  console.assert(hex.encode(ret) === "00", "Uint8Opt");
}

function testNum() {
  let part1 = "0100000000000000";
  let part2 = "0200000000000000";
  let part3 = "0300000000000000";
  let part4 = "0400000000000000";
  function checkNum(s: string) {
    let bytes = hex.decode(s);
    let num = numFromBytes(bytes);
    let bytes2 = numToBytes(num, bytes.byteLength);
    console.assert(bytes.toString() == bytes2.toString(), "checkNum on " + s);
  }
  checkNum(part1);
  checkNum(part1 + part2);
  checkNum(part1 + part2 + part3 + part4);
  checkNum(part1 + part2 + part3 + part4);

  // Test uint256 (32 bytes) with random combinations
  checkNum(part2 + part1 + part4 + part3);
  checkNum(part3 + part4 + part1 + part2);
  checkNum(part4 + part3 + part2 + part1);

  // Test uint512 (64 bytes) with random combinations
  checkNum(part1 + part3 + part2 + part4 + part2 + part4 + part1 + part3);
  checkNum(part4 + part2 + part3 + part1 + part3 + part1 + part4 + part2);
  checkNum(part2 + part4 + part1 + part3 + part4 + part2 + part3 + part1);
}

function testHighLevel() {
  let header = HighLevel.loadHeader(0, bindings.SOURCE_HEADER_DEP);
  console.assert(header.rawHeader.version == 0n);
  console.assert(header.rawHeader.compact_target == 1n);
  console.assert(header.rawHeader.timestamp == 2n);
  console.assert(header.rawHeader.number == 3n);
  console.assert(header.rawHeader.epoch == 4n);
  console.assert(
    hex.encode(header.rawHeader.parent_hash) ==
      "0000000000000000000000000000000000000000000000000000000000000005",
  );
  console.assert(
    hex.encode(header.rawHeader.transactions_root) ==
      "0000000000000000000000000000000000000000000000000000000000000006",
  );
  console.assert(
    hex.encode(header.rawHeader.proposals_hash) ==
      "0000000000000000000000000000000000000000000000000000000000000007",
  );
  console.assert(
    hex.encode(header.rawHeader.extra_hash) ==
      "0000000000000000000000000000000000000000000000000000000000000008",
  );
  console.assert(
    hex.encode(header.rawHeader.dao) ==
      "0000000000000000000000000000000000000000000000000000000000000009",
  );
  console.assert(header.nonce.toString() == "10");
}

function main() {
  testText();
  testMoleculeBigInt();
  testMoleculeString();
  testMoleculeOption();
  testNum();
  testHighLevel();
}

main();
