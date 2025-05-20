import dedent from "dedent";
import path from "node:path";
import tseslint from "typescript-eslint";
import { RuleTester } from "@typescript-eslint/rule-tester";
import { AST_NODE_TYPES } from "@typescript-eslint/utils";
import * as vitest from "vitest";

import { rule } from "./no-commonjs-modules";

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

ruleTester.run("no-commonjs-modules-no-shadow-check", rule, {
  valid: [
    { code: "import foo from 'bar';" },
    { code: "import * as foo from 'bar';" },
    { code: "export const foo = 1;" },
    { code: "export default function foo() {}" },
    { code: "export { name1, name2 };" },
    { code: "console.log(module.id);" },
    { code: "const x = { exports: {} }; x.exports.foo = 1;" },
    { code: "const myRequire = () => {}; myRequire();" },
    { code: "const myModule = { exports: {} }; myModule.exports = {};" },
    { code: "const myExports = {}; myExports.foo = 1;" },
  ],
  invalid: [
    {
      code: "const foo = require('bar');",
      errors: [
        {
          messageId: "noRequire",
          type: AST_NODE_TYPES.CallExpression,
          line: 1,
          column: 13,
        },
      ],
    },
    {
      code: "require('bar');",
      errors: [
        {
          messageId: "noRequire",
          type: AST_NODE_TYPES.CallExpression,
          line: 1,
          column: 1,
        },
      ],
    },
    {
      code: "module.exports = { foo: 'bar' };",
      errors: [
        {
          messageId: "noModuleExports",
          type: AST_NODE_TYPES.MemberExpression,
          line: 1,
          column: 1,
        },
      ],
    },
    {
      code: "module.exports.foo = 'bar';",
      errors: [
        {
          messageId: "noExportsAssignment",
          type: AST_NODE_TYPES.MemberExpression,
          line: 1,
          column: 1,
        },
      ],
    },
    {
      code: "exports.foo = 'bar';",
      errors: [
        {
          messageId: "noExportsAssignment",
          type: AST_NODE_TYPES.MemberExpression,
          line: 1,
          column: 1,
        },
      ],
    },
    {
      code: "exports['foo'] = 'bar';",
      errors: [
        {
          messageId: "noExportsAssignment",
          type: AST_NODE_TYPES.MemberExpression,
          line: 1,
          column: 1,
        },
      ],
    },
    {
      code: "exports = { foo: 'bar' };",
      errors: [
        {
          messageId: "noModuleExports",
          type: AST_NODE_TYPES.Identifier,
          line: 1,
          column: 1,
        },
      ],
    },
    {
      code: dedent`
        function myFunc() {
          const require = (path) => console.log(path);
          require('./foo');
        }
      `,
      errors: [
        {
          messageId: "noRequire",
          type: AST_NODE_TYPES.CallExpression,
          line: 3,
          column: 3,
        },
      ],
    },
    {
      code: dedent`
        const module = { exports: {} };
        module.exports = { foo: 'bar' };
      `,
      errors: [
        {
          messageId: "noModuleExports",
          type: AST_NODE_TYPES.MemberExpression,
          line: 2,
          column: 1,
        },
      ],
    },
    {
      code: dedent`
        let exports = {};
        exports.foo = 'bar';
        exports = { bar: 'baz' };
      `,
      errors: [
        {
          messageId: "noExportsAssignment",
          type: AST_NODE_TYPES.MemberExpression,
          line: 2,
          column: 1,
        },
        {
          messageId: "noModuleExports",
          type: AST_NODE_TYPES.Identifier,
          line: 3,
          column: 1,
        },
      ],
    },
  ],
});
