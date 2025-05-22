import type { TSESTree } from "@typescript-eslint/utils";
import { AST_NODE_TYPES } from "@typescript-eslint/utils";
import { createRule } from "../utils";

export type MessageId = "noRequire" | "noModuleExports" | "noExportsAssignment";

export const rule = createRule<[], MessageId>({
  name: "no-commonjs-modules",
  meta: {
    type: "problem",
    docs: {
      description:
        "Disallow CommonJS module patterns (`require`, `module.exports`, `exports.xxx`) as ckb-js-vm exclusively supports ECMAScript Modules (ESM). This version flags all occurrences without checking for shadowing.",
      recommended: true,
    },
    messages: {
      noRequire:
        "Use ES6 imports instead of `require()`. ckb-js-vm supports ESM only.",
      noModuleExports:
        "Use ES6 exports instead of `module.exports` or `exports = ...`. ckb-js-vm supports ESM only.",
      noExportsAssignment:
        "Use ES6 named exports instead of `exports.xxx`. ckb-js-vm supports ESM only.",
    },
    schema: [],
  },
  defaultOptions: [],

  create(context) {
    return {
      // require(...)
      CallExpression(node: TSESTree.CallExpression): void {
        if (
          node.callee.type === AST_NODE_TYPES.Identifier &&
          node.callee.name === "require"
        ) {
          context.report({ node, messageId: "noRequire" });
        }
      },

      AssignmentExpression(node: TSESTree.AssignmentExpression): void {
        const { left } = node;

        if (left.type === AST_NODE_TYPES.MemberExpression) {
          // module.exports = ...
          if (
            left.object.type === AST_NODE_TYPES.Identifier &&
            left.object.name === "module" &&
            left.property.type === AST_NODE_TYPES.Identifier &&
            left.property.name === "exports"
          ) {
            context.report({ node: left, messageId: "noModuleExports" });
          }
          // exports.foo = ... OR module.exports.foo = ...
          else if (
            (left.object.type === AST_NODE_TYPES.Identifier &&
              left.object.name === "exports") ||
            (left.object.type === AST_NODE_TYPES.MemberExpression &&
              left.object.object.type === AST_NODE_TYPES.Identifier &&
              left.object.object.name === "module" &&
              left.object.property.type === AST_NODE_TYPES.Identifier &&
              left.object.property.name === "exports")
          ) {
            context.report({ node: left, messageId: "noExportsAssignment" });
          }
        }
        // exports = ...
        else if (
          left.type === AST_NODE_TYPES.Identifier &&
          left.name === "exports"
        ) {
          context.report({ node: left, messageId: "noModuleExports" });
        }
      },
    };
  },
});
