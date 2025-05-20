import { rule as enforceBindingsExitMain } from "./enforce-bindings-exit-main";
import { rule as noMountInMain } from "./no-mount-in-main";
import { rule as noCommonjsModules } from "./no-commonjs-modules";
import { rule as requireImportExtensions } from "./require-import-extensions";
import { rule as noEvalJsScript } from "./no-eval-js-script";

export const rules = {
  "enforce-bindings-exit-main": enforceBindingsExitMain,
  "no-mount-in-main": noMountInMain,
  "no-commonjs-modules": noCommonjsModules,
  "require-import-extensions": requireImportExtensions,
  "no-eval-js-script": noEvalJsScript,
};
