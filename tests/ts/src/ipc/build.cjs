const { compileBc, typeCheck, bundleCode } = require("../../build.cjs");
const { hashCkb, bytesFrom } = require("@ckb-ccc/core");
const fs = require("fs");
const path = require("path");

function R(p) {
  return path.resolve(__dirname, p);
}

typeCheck([R("./server.ts"), R("./client.ts")]);
bundleCode(R("./server.ts"), R("../../dist/ipc/server.js"));
bundleCode(R("./client.ts"), R("../../dist/ipc/client.js"));

const SERVER_BYTECODE_PATH = R("../../dist/ipc/server.bc");
const CLIENT_BYTECODE_PATH = R("../../dist/ipc/client.bc");
compileBc(R("../../dist/ipc/server.js"), SERVER_BYTECODE_PATH);
compileBc(R("../../dist/ipc/client.js"), CLIENT_BYTECODE_PATH);

module.exports = {
  SERVER_BYTECODE_PATH,
  CLIENT_BYTECODE_PATH,
};
