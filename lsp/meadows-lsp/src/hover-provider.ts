import { TextDocument, Hover, Position, MarkupKind } from 'vscode-languageserver/node';
import { Logger } from './logger';

export class HoverProvider {
  private logger: Logger;

  constructor(logger: Logger) {
    this.logger = logger;
  }

  /**
   * Provide hover information at a position
   */
  provideHover(document: TextDocument, position: Position): Hover | null {
    const text = document.getText();
    const lines = text.split('\n');
    
    if (position.line >= lines.length) {
      return null;
    }

    const line = lines[position.line];
    
    // Get the word at the position
    const word = this.getWordAtPosition(line, position.character);
    
    if (!word) {
      return null;
    }

    this.logger.debug(`Hover requested for word: "${word}"`);

    // Check what kind of symbol this is
    const hoverInfo = this.getHoverInfo(word, text, position.line);
    
    if (hoverInfo) {
      return {
        contents: {
          kind: MarkupKind.Markdown,
          value: hoverInfo
        }
      };
    }

    return null;
  }

  private getWordAtPosition(line: string, character: number): string | null {
    // Find word boundaries
    let start = character;
    let end = character;

    // Expand backwards
    while (start > 0 && /[a-zA-Z0-9_]/.test(line[start - 1])) {
      start--;
    }

    // Expand forwards
    while (end < line.length && /[a-zA-Z0-9_]/.test(line[end])) {
      end++;
    }

    if (start === end) {
      return null;
    }

    return line.substring(start, end);
  }

  private getHoverInfo(word: string, text: string, currentLine: number): string | null {
    // Check if it's a keyword
    const keywords = ['let', 'func', 'if', 'else', 'for', 'while', 'return', 'in', 'break', 'continue'];
    if (keywords.includes(word)) {
      return this.getKeywordDocumentation(word);
    }

    // Check if it's a built-in function
    if (word === 'print') {
      return [
        '```meadows',
        'print(value)',
        '```',
        '',
        'Prints a value to stdout with a newline.',
        '',
        '**Parameters:**',
        '- `value`: The value to print (i32 or string)',
        '',
        '**Example:**',
        '```meadows',
        'print("Hello, World!");',
        'print(42);',
        '```'
      ].join('\n');
    }

    if (word === 'range') {
      return [
        '```meadows',
        'range(start, end)',
        '```',
        '',
        'Creates a range iterator for use in for loops.',
        '',
        '**Parameters:**',
        '- `start`: The starting number (inclusive)',
        '- `end`: The ending number (exclusive)',
        '',
        '**Returns:**',
        'An iterator that yields numbers from start to end-1',
        '',
        '**Example:**',
        '```meadows',
        'for (i in range(0, 10)) {',
        '  print(i);  // Prints 0 through 9',
        '}',
        '```'
      ].join('\n');
    }

    // Check if it's a function
    const funcRegex = new RegExp(`func\\s+${word}\\s*\\(([^)]*)\\)`, 'g');
    const funcMatch = funcRegex.exec(text);
    if (funcMatch) {
      const params = funcMatch[1].trim();
      return [
        '```meadows',
        `func ${word}(${params})`,
        '```',
        '',
        `Function **${word}**`,
        '',
        '**Parameters:**',
        params ? `- ${params}` : 'None',
        '',
        '**Returns:**',
        'i32'
      ].join('\n');
    }

    // Check if it's a variable
    const lines = text.split('\n');
    for (let i = 0; i <= currentLine; i++) {
      const line = lines[i];
      
      // Check for let declaration
      const letRegex = new RegExp(`let\\s+${word}\\s*=\\s*(.+?)(?:;|$)`);
      const letMatch = line.match(letRegex);
      if (letMatch) {
        const value = letMatch[1].trim();
        const type = this.inferType(value);
        return [
          '```meadows',
          `let ${word} = ${value};`,
          '```',
          '',
          `Variable **${word}**: \`${type}\``,
          i < currentLine ? `Declared on line ${i + 1}` : 'Declared here'
        ].join('\n');
      }

      // Check for parameter declaration
      const paramRegex = new RegExp(`func\\s+\\w+\\s*\\(([^)]*\\b${word}\\b[^)]*)\\)`);
      const paramMatch = line.match(paramRegex);
      if (paramMatch) {
        return [
          '```meadows',
          `func ...(${paramMatch[1]})`,
          '```',
          '',
          `Parameter **${word}**: \`i32\``
        ].join('\n');
      }
    }

    return null;
  }

  private getKeywordDocumentation(keyword: string): string {
    const docs: Record<string, string> = {
      'let': [
        '**let** - Variable declaration',
        '',
        'Declares a new variable with an initial value.',
        '',
        '**Syntax:**',
        '```meadows',
        'let name = value;',
        '```',
        '',
        '**Example:**',
        '```meadows',
        'let x = 10;',
        'let message = "Hello";',
        '```'
      ].join('\n'),
      
      'func': [
        '**func** - Function declaration',
        '',
        'Declares a new function.',
        '',
        '**Syntax:**',
        '```meadows',
        'func name(param1, param2) {',
        '  // body',
        '}',
        '```',
        '',
        '**Example:**',
        '```meadows',
        'func add(a, b) {',
        '  return a + b;',
        '}',
        '```'
      ].join('\n'),
      
      'if': [
        '**if** - Conditional statement',
        '',
        'Executes code conditionally.',
        '',
        '**Syntax:**',
        '```meadows',
        'if (condition) {',
        '  // then branch',
        '} else {',
        '  // else branch',
        '}',
        '```'
      ].join('\n'),
      
      'for': [
        '**for** - Loop statement',
        '',
        'Iterates over a range.',
        '',
        '**Syntax:**',
        '```meadows',
        'for (variable in range(start, end)) {',
        '  // body',
        '}',
        '```',
        '',
        '**Example:**',
        '```meadows',
        'for (i in range(0, 10)) {',
        '  print(i);',
        '}',
        '```'
      ].join('\n'),
      
      'while': [
        '**while** - Loop statement',
        '',
        'Loops while condition is true.',
        '',
        '**Syntax:**',
        '```meadows',
        'while (condition) {',
        '  // body',
        '}',
        '```'
      ].join('\n'),
      
      'return': [
        '**return** - Return statement',
        '',
        'Returns a value from a function.',
        '',
        '**Syntax:**',
        '```meadows',
        'return value;',
        '```'
      ].join('\n')
    };

    return docs[keyword] || `**${keyword}** - Meadows keyword`;
  }

  private inferType(value: string): string {
    value = value.trim();
    
    // Check for string literal
    if (value.startsWith('"') && value.endsWith('"')) {
      return 'string';
    }
    
    // Check for array
    if (value.startsWith('[') && value.endsWith(']')) {
      return 'i32[]';
    }
    
    // Check for object
    if (value.startsWith('{') && value.endsWith('}')) {
      return 'object';
    }
    
    // Check for number
    if (/^-?\d+$/.test(value)) {
      return 'i32';
    }
    
    // Default
    return 'unknown';
  }
}
