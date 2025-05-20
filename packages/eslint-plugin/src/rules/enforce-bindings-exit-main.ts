import type { TSESTree } from "@typescript-eslint/utils";
import { AST_NODE_TYPES } from "@typescript-eslint/utils";
import { createRule } from "../utils";

export type MessageId = "missingBindingsExitMain" | "notTopLevel";

export const rule = createRule<[], MessageId>({
  name: "enforce-bindings-exit-main",
  meta: {
    type: "problem",
    docs: {
      description:
        "Enforce that `index.ts` (or `index.js`) exits via a top-level `bindings.exit(main());` call.",
      recommended: true,
    },
    messages: {
      missingBindingsExitMain:
        "In `index.ts` (or `index.js`), the script must exit with a top-level call to `bindings.exit(main());`.",
      notTopLevel:
        "The `bindings.exit(main());` call must be at the top level of the module.",
    },
    schema: [],
  },
  defaultOptions: [],

  create(context) {
    const filename = context.filename;
    if (!/index\.(ts|js)$/i.test(filename)) {
      return {};
    }

    let foundTopLevel = false;
    let hasReported = false;

    return {
      CallExpression(node: TSESTree.CallExpression): void {
        if (
          node.callee.type === AST_NODE_TYPES.MemberExpression &&
          node.callee.object.type === AST_NODE_TYPES.Identifier &&
          node.callee.object.name === "bindings" &&
          node.callee.property.type === AST_NODE_TYPES.Identifier &&
          node.callee.property.name === "exit"
        ) {
          if (
            node.parent?.type === AST_NODE_TYPES.ExpressionStatement &&
            node.parent.parent?.type === AST_NODE_TYPES.Program
          ) {
            const arg = node.arguments[0];
            if (
              arg.type === AST_NODE_TYPES.CallExpression &&
              arg.callee.type === AST_NODE_TYPES.Identifier &&
              arg.callee.name === "main"
            ) {
              foundTopLevel = true;
            }
          } else {
            context.report({
              node,
              messageId: "notTopLevel",
            });
            hasReported = true;
          }
        }
      },

      "Program:exit"(programNode: TSESTree.Program): void {
        if (!foundTopLevel && !hasReported) {
          context.report({
            node: programNode,
            loc: {
              line: programNode.loc.end.line,
              column: 0,
            },
            messageId: "missingBindingsExitMain",
          });
          hasReported = true;
        }
      },
    };
  },
});
