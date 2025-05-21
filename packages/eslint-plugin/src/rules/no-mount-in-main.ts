import type { TSESTree } from "@typescript-eslint/utils";
import { AST_NODE_TYPES } from "@typescript-eslint/utils";
import { createRule } from "../utils";

export type MessageId = "noMountInMain";

export const rule = createRule<[], MessageId>({
  name: "no-mount-in-main",
  meta: {
    type: "problem",
    docs: {
      description:
        "Disallow `bindings.mount` calls in main entry files (e.g., `index.js`, `index.bc`). Suggest using `init.js` or `init.bc` instead.",
      recommended: true,
    },
    messages: {
      noMountInMain:
        "`bindings.mount` in main entry files can cause issues. Use `init.js` or `init.bc` instead.",
    },
    schema: [],
  },
  defaultOptions: [],

  create(context) {
    const filename = context.filename;
    if (!/index\.(ts|js)$/i.test(filename)) {
      return {};
    }

    return {
      CallExpression(node: TSESTree.CallExpression): void {
        if (
          node.callee.type === AST_NODE_TYPES.MemberExpression &&
          node.callee.object.type === AST_NODE_TYPES.Identifier &&
          node.callee.object.name === "bindings" &&
          node.callee.property.type === AST_NODE_TYPES.Identifier &&
          node.callee.property.name === "mount"
        ) {
          context.report({
            node,
            messageId: "noMountInMain",
            data: {
              filename: filename.split(/[\\/]/).pop() || "",
            },
          });
        }
      },
    };
  },
});
