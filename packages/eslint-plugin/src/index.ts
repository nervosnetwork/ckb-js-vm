import { rules } from "./rules/index.js";

const { name, version } =
  require("../package.json") as typeof import("../package.json");

const plugin = {
  configs: {
    get recommended() {
      return recommended;
    },
  },
  meta: { name, version },
  rules,
};

const recommended = {
  plugins: {
    "ckb-js-std": plugin,
  },
  rules,
};

export = plugin;
