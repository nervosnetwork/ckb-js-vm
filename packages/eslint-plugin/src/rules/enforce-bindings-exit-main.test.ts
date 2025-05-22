import dedent from "dedent";
import path from "node:path";
import tseslint from "typescript-eslint";
import { RuleTester } from "@typescript-eslint/rule-tester";
import * as vitest from "vitest";

import { rule } from "./enforce-bindings-exit-main";

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

ruleTester.run("enforce-bindings-exit-main", rule, {
  valid: [
    {
      filename: "index.ts",
      code: dedent`
        import { bindings } from "ckb-js-vm";
        function main() {}
        bindings.exit(main());
      `,
    },
  ],
  invalid: [
    {
      filename: "index.ts",
      code: dedent`
        import { bindings } from "ckb-js-vm";
        function main() {}
      `,
      errors: [
        {
          messageId: "missingBindingsExitMain",
        },
      ],
    },
    {
      filename: "index.ts",
      code: dedent`
        import { bindings } from "ckb-js-vm";
        function main() {
          bindings.exit(main());
        }
      `,
      errors: [
        {
          line: 3,
          column: 3,
          messageId: "notTopLevel",
        },
      ],
    },
  ],
});
