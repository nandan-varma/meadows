import { TextDocument, SemanticTokens } from 'vscode-languageserver/node';
import { Logger } from './logger';

/**
 * Token types (must match the legend in server.ts)
 */
const enum TokenType {
  NAMESPACE = 0,
  TYPE = 1,
  CLASS = 2,
  ENUM = 3,
  INTERFACE = 4,
  STRUCT = 5,
  TYPE_PARAMETER = 6,
  PARAMETER = 7,
  VARIABLE = 8,
  PROPERTY = 9,
  ENUM_MEMBER = 10,
  EVENT = 11,
  FUNCTION = 12,
  METHOD = 13,
  MACRO = 14,
  KEYWORD = 15,
  MODIFIER = 16,
  COMMENT = 17,
  STRING = 18,
  NUMBER = 19,
  REGEXP = 20,
  OPERATOR = 21
}

/**
 * Token modifiers (must match the legend in server.ts)
 */
const enum TokenModifier {
  DECLARATION = 1 << 0,
  DEFINITION = 1 << 1,
  READONLY = 1 << 2,
  STATIC = 1 << 3,
  DEPRECATED = 1 << 4,
  ABSTRACT = 1 << 5,
  ASYNC = 1 << 6,
  MODIFICATION = 1 << 7,
  DOCUMENTATION = 1 << 8,
  DEFAULT_LIBRARY = 1 << 9
}

export class SemanticTokenProvider {
  private logger: Logger;

  constructor(logger: Logger) {
    this.logger = logger;
  }

  /**
   * Provide semantic tokens for a document
   * Uses regex-based tokenization for simplicity
   */
  provideSemanticTokens(document: TextDocument): SemanticTokens {
    const text = document.getText();
    const lines = text.split('\n');
    const data: number[] = [];

    let previousLine = 0;
    let previousCharacter = 0;

    for (let lineIndex = 0; lineIndex < lines.length; lineIndex++) {
      const line = lines[lineIndex];
      
      // Process each line for tokens
      const tokens = this.tokenizeLine(line, lineIndex);
      
      for (const token of tokens) {
        // Encode token as 5 integers: line, char, length, type, modifiers
        const lineDelta = token.line - previousLine;
        const charDelta = lineDelta === 0 ? token.character - previousCharacter : token.character;
        
        data.push(lineDelta);
        data.push(charDelta);
        data.push(token.length);
        data.push(token.tokenType);
        data.push(token.modifiers);
        
        previousLine = token.line;
        previousCharacter = token.character;
      }
    }

    return { data };
  }

  private tokenizeLine(line: string, lineIndex: number): TokenInfo[] {
    const tokens: TokenInfo[] = [];
    
    // Keywords
    const keywords = /\b(let|func|if|else|for|while|return|print|in|break|continue)\b/g;
    let match;
    while ((match = keywords.exec(line)) !== null) {
      tokens.push({
        line: lineIndex,
        character: match.index,
        length: match[0].length,
        tokenType: TokenType.KEYWORD,
        modifiers: 0
      });
    }

    // Built-in functions
    const builtins = /\b(print|range)\b/g;
    while ((match = builtins.exec(line)) !== null) {
      tokens.push({
        line: lineIndex,
        character: match.index,
        length: match[0].length,
        tokenType: TokenType.FUNCTION,
        modifiers: TokenModifier.DEFAULT_LIBRARY
      });
    }

    // Function declarations: func name(
    const funcDecl = /func\s+(\w+)/g;
    while ((match = funcDecl.exec(line)) !== null) {
      const nameMatch = match[1];
      const startIndex = match.index + 5; // After "func "
      tokens.push({
        line: lineIndex,
        character: startIndex,
        length: nameMatch.length,
        tokenType: TokenType.FUNCTION,
        modifiers: TokenModifier.DECLARATION | TokenModifier.DEFINITION
      });
    }

    // Variable declarations: let name =
    const varDecl = /let\s+(\w+)/g;
    while ((match = varDecl.exec(line)) !== null) {
      const nameMatch = match[1];
      const startIndex = match.index + 4; // After "let "
      tokens.push({
        line: lineIndex,
        character: startIndex,
        length: nameMatch.length,
        tokenType: TokenType.VARIABLE,
        modifiers: TokenModifier.DECLARATION | TokenModifier.DEFINITION
      });
    }

    // Parameters: func name(param, param)
    const params = /func\s+\w+\s*\(([^)]*)\)/g;
    while ((match = params.exec(line)) !== null) {
      const paramList = match[1];
      const paramNames = paramList.match(/\b\w+\b/g);
      if (paramNames) {
        let searchIndex = match.index + match[0].indexOf('(') + 1;
        for (const param of paramNames) {
          const paramIndex = line.indexOf(param, searchIndex);
          if (paramIndex !== -1) {
            tokens.push({
              line: lineIndex,
              character: paramIndex,
              length: param.length,
              tokenType: TokenType.PARAMETER,
              modifiers: TokenModifier.DECLARATION
            });
            searchIndex = paramIndex + param.length;
          }
        }
      }
    }

    // Numbers
    const numbers = /\b\d+\b/g;
    while ((match = numbers.exec(line)) !== null) {
      tokens.push({
        line: lineIndex,
        character: match.index,
        length: match[0].length,
        tokenType: TokenType.NUMBER,
        modifiers: 0
      });
    }

    // Strings (simple handling)
    const strings = /"[^"]*"/g;
    while ((match = strings.exec(line)) !== null) {
      tokens.push({
        line: lineIndex,
        character: match.index,
        length: match[0].length,
        tokenType: TokenType.STRING,
        modifiers: 0
      });
    }

    // Comments
    const comments = /(#|\/\/).*$/g;
    while ((match = comments.exec(line)) !== null) {
      tokens.push({
        line: lineIndex,
        character: match.index,
        length: match[0].length,
        tokenType: TokenType.COMMENT,
        modifiers: 0
      });
    }

    // Operators
    const operators = /(\+|-|\*|\/|==|!=|<=|>=|<|>|&&|\|\|!|=)/g;
    while ((match = operators.exec(line)) !== null) {
      tokens.push({
        line: lineIndex,
        character: match.index,
        length: match[0].length,
        tokenType: TokenType.OPERATOR,
        modifiers: 0
      });
    }

    // Sort tokens by position
    tokens.sort((a, b) => {
      if (a.line !== b.line) return a.line - b.line;
      return a.character - b.character;
    });

    return tokens;
  }
}

interface TokenInfo {
  line: number;
  character: number;
  length: number;
  tokenType: number;
  modifiers: number;
}
