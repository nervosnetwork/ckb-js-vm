import type { TSESTree } from "@typescript-eslint/utils";
import { AST_NODE_TYPES } from "@typescript-eslint/utils";
import { createRule } from "../utils";

export type MessageId = "noEvalJsScript";

export const rule = createRule<[], MessageId>({
  name: "no-eval-js-script",
  meta: {
    type: "problem",
    docs: {
      description:
        "Disallow the use of `bindings.evalJsScript()` due to significant security risks when loading code from untrusted sources. Consider safer alternatives or restrict usage to trusted, validated inputs only.",
      recommended: true,
    },
    messages: {
      noEvalJsScript:
        "`bindings.evalJsScript()` is insecure. Avoid use or ensure inputs are trusted and validated.",
    },
    schema: [],
  },
  defaultOptions: [],

  create(context) {
    return {
      CallExpression(node: TSESTree.CallExpression): void {
        if (
          node.callee.type === AST_NODE_TYPES.MemberExpression &&
          node.callee.object.type === AST_NODE_TYPES.Identifier &&
          node.callee.object.name === "bindings" &&
          node.callee.property.type === AST_NODE_TYPES.Identifier &&
          node.callee.property.name === "evalJsScript"
        ) {
          context.report({
            node: node.callee,
            messageId: "noEvalJsScript",
          });
        }
      },
    };
  },
});
