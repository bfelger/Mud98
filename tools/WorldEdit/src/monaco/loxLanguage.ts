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

const typeKeywords = ["Race", "DamageType", "Reputation"];

const builtinGlobals = [
  "global_areas",
  "global_objs",
  "global_mobs",
  "native_cmds",
  "native_mob_cmds"
];

const builtinFunctions = [
  "clock",
  "marshal",
  "floor",
  "string",
  "damage",
  "dice",
  "do",
  "saves_spell",
  "delay"
];

const builtinMethods = [
  "is_area",
  "is_area_data",
  "is_room",
  "is_room_data",
  "is_mob",
  "is_mob_proto",
  "is_obj",
  "is_obj_proto",
  "send",
  "say",
  "echo",
  "can_quest",
  "has_quest",
  "grant_quest",
  "can_finish_quest",
  "finish_quest",
  "get_reputation",
  "adjust_reputation",
  "set_reputation",
  "is_enemy",
  "is_ally",
  "each",
  "count",
  "add",
  "contains"
];

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
    builtinGlobals,
    builtinFunctions,
    builtinMethods,
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
              "@builtinGlobals": "variable.predefined",
              "@builtinFunctions": "function",
              "@builtinMethods": "method",
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
