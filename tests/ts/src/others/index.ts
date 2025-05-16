import * as bindings from "@ckb-js-std/bindings";
import { hex } from "@ckb-js-std/bindings";
import { log, mol } from "@ckb-js-std/core";

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

function main() {
  testText();
  testMoleculeBigInt();
  testMoleculeString();
  testMoleculeOption();
}

main();
