const { compileBc, typeCheck, bundleCode } = require("../../build.cjs");
const path = require("path");

function R(p) {
  return path.resolve(__dirname, p);
}

typeCheck([R("./index.ts")]);
bundleCode(R("./index.ts"), R("../../dist/index.js"));

const BYTECODE_PATH = R("../../dist/index.bc");
compileBc(R("../../dist/index.js"), BYTECODE_PATH);

module.exports = {
  BYTECODE_PATH,
};
