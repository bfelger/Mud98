import type { Monaco } from "@monaco-editor/react";

const languageId = "lox";
let registered = false;

const keywords = [
  "and",
  "or",
  "if",
  "else",
  "while",
  "for",
  "break",
  "continue",
  "return",
  "fun",
  "class",
  "this",
  "var",
  "const",
  "enum"
];

const literals = ["true", "false", "nil"];

const typeKeywords = ["Race", "DamageType"];

const operators = [
  "=",
  "+",
  "-",
  "*",
  "/",
  "%",
  "==",
  "!=",
  "<=",
  ">=",
  "<",
  ">",
  "+=",
  "-=",
  "*=",
  "/=",
  "!",
  "->"
];

export function registerLoxLanguage(monaco: Monaco) {
  if (registered) {
    return;
  }
  registered = true;

  monaco.languages.register({ id: languageId });
  monaco.languages.setLanguageConfiguration(languageId, {
    comments: { lineComment: "//" },
    brackets: [
      ["{", "}"],
      ["[", "]"],
      ["(", ")"]
    ],
    autoClosingPairs: [
      { open: "{", close: "}" },
      { open: "[", close: "]" },
      { open: "(", close: ")" },
      { open: "\"", close: "\"" }
    ],
    surroundingPairs: [
      { open: "{", close: "}" },
      { open: "[", close: "]" },
      { open: "(", close: ")" },
      { open: "\"", close: "\"" }
    ]
  });

  monaco.languages.setMonarchTokensProvider(languageId, {
    tokenPostfix: ".lox",
    keywords,
    typeKeywords,
    literals,
    operators,
    symbols: /[=><!~?:&|+\-*/^%]+/,
    escapes: /\\(?:[nrt"\\$]|x[0-9A-Fa-f]{2})/,
    tokenizer: {
      root: [
        { include: "@whitespace" },
        [
          /[a-zA-Z_][\w]*/,
          {
            cases: {
              "@keywords": "keyword",
              "@typeKeywords": "type.identifier",
              "@literals": "constant.language",
              "@default": "identifier"
            }
          }
        ],
        [/\d+\.\d+([eE][\-+]?\d+)?/, "number.float"],
        [/\d+/, "number"],
        [/[{}()[\]]/, "@brackets"],
        [/[;,.]/, "delimiter"],
        [
          /@symbols/,
          { cases: { "@operators": "operator", "@default": "delimiter" } }
        ],
        [/"/, { token: "string.quote", next: "@string" }]
      ],
      whitespace: [[/[ \t\r\n]+/, "white"], [/\/\/.*$/, "comment"]],
      string: [
        [/[^\\$"]+/, "string"],
        [/@escapes/, "string.escape"],
        [/\$\$/, "string.escape"],
        [/\$\{/, { token: "delimiter.bracket", next: "@stringEmbedded" }],
        [/"/, { token: "string.quote", next: "@pop" }]
      ],
      stringEmbedded: [
        [/\}/, { token: "delimiter.bracket", next: "@pop" }],
        { include: "@root" }
      ]
    }
  });
}
