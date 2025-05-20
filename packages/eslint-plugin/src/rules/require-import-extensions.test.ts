import path from "node:path";
import tseslint from "typescript-eslint";
import { RuleTester } from "@typescript-eslint/rule-tester";
import { AST_NODE_TYPES } from "@typescript-eslint/utils";
import * as vitest from "vitest";

import { rule, MessageId } from "./require-import-extensions";

RuleTester.afterAll = vitest.afterAll;
RuleTester.it = vitest.it;
RuleTester.itOnly = vitest.it.only;
RuleTester.describe = vitest.describe;

const ruleTester = new RuleTester({
  languageOptions: {
    parser: tseslint.parser,
    parserOptions: {
      ecmaVersion: "latest",
      sourceType: "module",
      projectService: {
        allowDefaultProject: ["*.ts*", "*.js*"],
        defaultProject: "tsconfig.json",
      },
      tsconfigRootDir: path.join(__dirname, "../.."),
    },
  },
});

const missingExtensionId: MessageId = "missingExtension";

ruleTester.run("require-import-extensions", rule, {
  valid: [
    { code: "import foo from './bar.js';" },
    { code: "import * as foo from '../bar.bc';" },
    { code: "import { name } from './baz/qux.js';" },
  ],
  invalid: [
    {
      code: "import foo from './bar';",
      errors: [
        {
          messageId: missingExtensionId,
          data: { path: "./bar" },
          type: AST_NODE_TYPES.Literal,
          line: 1,
          column: 17,
        },
      ],
    },
    {
      code: "import * as foo from './bar/baz';",
      errors: [
        {
          messageId: missingExtensionId,
          data: { path: "./bar/baz" },
          type: AST_NODE_TYPES.Literal,
          line: 1,
          column: 22,
        },
      ],
    },
    {
      code: "import { name } from './bar';",
      errors: [
        {
          messageId: missingExtensionId,
          data: { path: "./bar" },
          type: AST_NODE_TYPES.Literal,
          line: 1,
          column: 22,
        },
      ],
    },
  ],
});
