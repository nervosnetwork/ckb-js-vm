import { rule as enforceBindingsExitMain } from "./enforce-bindings-exit-main";
import { rule as noMountInMain } from "./no-mount-in-main";

export const rules = {
  "enforce-bindings-exit-main": enforceBindingsExitMain,
  "no-mount-in-main": noMountInMain,
};
