import eslintReact from "@eslint-react/eslint-plugin";
import js from "@eslint/js";
import stylistic from "@stylistic/eslint-plugin";
import { defineConfig } from "eslint/config";
import promise from "eslint-plugin-promise";
import reactHooks from "eslint-plugin-react-hooks";
import globals from "globals";
import tseslint from "typescript-eslint";

const controlFlow = [
  "if",
  "for",
  "while",
  "do",
  "switch",
  "try",
  "return",
  "throw",
  "break",
  "continue",
];

export default defineConfig(
  { ignores: ["dist/"] },
  {
    files: ["**/*.{ts,tsx}"],
    extends: [
      js.configs.recommended,
      tseslint.configs.strictTypeChecked,
      tseslint.configs.stylisticTypeChecked,
      eslintReact.configs["recommended-type-checked"],
      reactHooks.configs.flat.recommended,
    ],
    plugins: { "@stylistic": stylistic, promise },
    languageOptions: {
      globals: globals.browser,
      parserOptions: {
        projectService: true,
        tsconfigRootDir: import.meta.dirname,
      },
    },
    settings: {
      // Preact 10 matches the React 18 component API.
      "react-x": { importSource: "preact", version: "18.3.1" },
    },
    rules: {
      curly: ["error", "all"],
      eqeqeq: ["error", "always", { null: "ignore" }],
      "no-var": "error",
      "object-shorthand": "error",
      "prefer-arrow-callback": "error",
      "prefer-const": "error",
      "prefer-template": "error",
      // `onClick={() => setOpen(true)}` returns the setter's void on purpose.
      "@typescript-eslint/no-confusing-void-expression": ["error", { ignoreArrowShorthand: true }],
      "@typescript-eslint/restrict-template-expressions": ["error", { allowNumber: true }],
      // `text || fallback` treats an empty string as missing on purpose.
      "@typescript-eslint/prefer-nullish-coalescing": [
        "error",
        { ignorePrimitives: { string: true } },
      ],
      // Event handlers may be async; Preact ignores what they return.
      "@typescript-eslint/no-misused-promises": [
        "error",
        { checksVoidReturn: { attributes: false } },
      ],
      // try/catch around await instead of .then/.catch chains.
      "promise/prefer-await-to-then": ["error", { strict: true }],
      "@stylistic/padding-line-between-statements": [
        "error",
        { blankLine: "always", prev: "*", next: controlFlow },
        { blankLine: "always", prev: "block-like", next: "*" },
        { blankLine: "always", prev: ["const", "let"], next: "*" },
        { blankLine: "any", prev: ["const", "let"], next: ["const", "let"] },
      ],
    },
  },
  {
    files: ["*.config.{js,ts}"],
    languageOptions: { globals: globals.node },
  },
  {
    files: ["eslint.config.js"],
    extends: [js.configs.recommended],
  },
);
