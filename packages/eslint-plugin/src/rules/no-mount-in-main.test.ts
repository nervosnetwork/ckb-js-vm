import dedent from "dedent";
import path from "node:path";
import tseslint from "typescript-eslint";
import { RuleTester } from "@typescript-eslint/rule-tester";
import * as vitest from "vitest";

import { rule, MessageId } from "./no-mount-in-main";

RuleTester.afterAll = vitest.afterAll;
RuleTester.it = vitest.it;
RuleTester.itOnly = vitest.it.only;
RuleTester.describe = vitest.describe;

const ruleTester = new RuleTester({
  languageOptions: {
    parser: tseslint.parser,
    parserOptions: {
      projectService: {
        allowDefaultProject: ["*.ts*"],
        defaultProject: "tsconfig.json",
      },
      tsconfigRootDir: path.join(__dirname, "../.."),
    },
  },
});

ruleTester.run("no-mount-in-main", rule, {
  valid: [
    {
      filename: "init.ts",
      code: dedent`
        import * as bindings from "@ckb-js-std/bindings";
        bindings.mount(2, bindings.SOURCE_CELL_DEP, "/");
      `,
    },
    {
      filename: "index.ts",
      code: dedent`
        import * as bindings from "@ckb-js-std/bindings";
        bindings.exit(0);
      `,
    },
  ],
  invalid: [
    {
      filename: "index.ts",
      code: dedent`
        import * as bindings from "@ckb-js-std/bindings";
        bindings.mount(2, bindings.SOURCE_CELL_DEP, "/");
        import * as module from './fib_module.js';
      `,
      errors: [
        {
          messageId: "noMountInMain",
          data: { filename: "index.js" },
        },
      ],
    },
    {
      filename: "index.ts",
      code: dedent`
        import * as bindings from "@ckb-js-std/bindings";
        bindings.mount(2, bindings.SOURCE_CELL_DEP, "/");
      `,
      errors: [
        {
          messageId: "noMountInMain",
          data: { filename: "index.bc" },
        },
      ],
    },
  ],
});
