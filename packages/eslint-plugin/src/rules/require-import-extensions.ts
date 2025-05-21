import type { TSESTree } from "@typescript-eslint/utils";
import { AST_NODE_TYPES } from "@typescript-eslint/utils";
import { createRule } from "../utils";
import path from "node:path";

export type MessageId = "missingExtension";

const ALLOWED_EXTENSIONS = [".js", ".bc"];
const RELATIVE_PATH_REGEX = /^\.\.?\//;

export const rule = createRule<[], MessageId>({
  name: "require-import-extensions",
  meta: {
    type: "problem",
    docs: {
      description: `Ensure relative module paths in import/export statements include a valid extension (${ALLOWED_EXTENSIONS.join(" or ")}).`,
      recommended: true,
    },
    messages: {
      missingExtension: `Relative import '{{path}}' must include a .js or .bc extension.`,
    },
    schema: [],
  },
  defaultOptions: [],

  create(context) {
    function checkPathValue(
      nodeToReport: TSESTree.Node,
      pathValue: string | null | undefined,
    ): void {
      if (
        typeof pathValue === "string" &&
        RELATIVE_PATH_REGEX.test(pathValue)
      ) {
        const ext = path.extname(pathValue);
        if (!ALLOWED_EXTENSIONS.includes(ext)) {
          context.report({
            node: nodeToReport,
            messageId: "missingExtension",
            data: { path: pathValue },
          });
        }
      }
    }

    return {
      ImportDeclaration(node: TSESTree.ImportDeclaration): void {
        if (node.source && typeof node.source.value === "string") {
          checkPathValue(node.source, node.source.value);
        }
      },
      ImportExpression(node: TSESTree.ImportExpression): void {
        if (
          node.source.type === AST_NODE_TYPES.Literal &&
          typeof node.source.value === "string"
        ) {
          checkPathValue(node.source, node.source.value);
        } else if (
          node.source.type === AST_NODE_TYPES.TemplateLiteral &&
          node.source.quasis.length === 1 &&
          node.source.expressions.length === 0
        ) {
          const cookedValue = node.source.quasis[0].value.cooked;
          if (typeof cookedValue === "string") {
            checkPathValue(node.source, cookedValue);
          }
        }
      },
    };
  },
});
