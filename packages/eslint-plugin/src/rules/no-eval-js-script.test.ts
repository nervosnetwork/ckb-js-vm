import path from "node:path";
import tseslint from "typescript-eslint";
import { RuleTester } from "@typescript-eslint/rule-tester";
import { AST_NODE_TYPES } from "@typescript-eslint/utils";
import * as vitest from "vitest";

import { rule, MessageId } from "./no-eval-js-script";

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

const noEvalJsScriptId: MessageId = "noEvalJsScript";

ruleTester.run("no-eval-js-script", rule, {
  valid: [
    { code: "console.log('hello');" },
    { code: "bindings.someOtherFunction();" },
    { code: "otherObject.evalJsScript('code');" },
    {
      code: "const localBindings = { evalJsScript: () => {} }; localBindings.evalJsScript('code');",
    },
  ],
  invalid: [
    {
      code: "bindings.evalJsScript('console.log(1)');",
      errors: [
        {
          messageId: noEvalJsScriptId,
          type: AST_NODE_TYPES.MemberExpression,
          line: 1,
          column: 1,
        },
      ],
    },
    {
      code: "const code = 'dangerous'; bindings.evalJsScript(code);",
      errors: [
        {
          messageId: noEvalJsScriptId,
          type: AST_NODE_TYPES.MemberExpression,
          line: 1,
          column: 27,
        },
      ],
    },
    {
      code: "function X(){ return bindings.evalJsScript('test'); }",
      errors: [
        {
          messageId: noEvalJsScriptId,
          type: AST_NODE_TYPES.MemberExpression,
          line: 1,
          column: 22,
        },
      ],
    },
  ],
});
