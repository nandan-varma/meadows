import { spawn } from 'child_process';
import { fileURLToPath } from 'url';
import { Range } from 'vscode-languageserver/node';
import { Logger } from './logger';

export interface CompilerDiagnostic {
  range: Range;
  severity: 1 | 2 | 3 | 4; // Error, Warning, Info, Hint
  message: string;
  source: string;
  code?: string;
}

interface LSPResponse {
  diagnostics: Array<{
    file: string;
    range: {
      start: { line: number; character: number };
      end: { line: number; character: number };
    };
    severity: number;
    message: string;
    source: string;
    code?: string;
  }>;
}

export class CompilerBridge {
  private compilerPath: string;
  private logger: Logger | undefined;

  constructor(compilerPath: string = 'Meadows', logger?: Logger) {
    this.compilerPath = compilerPath;
    this.logger = logger;
  }

  async getDiagnostics(uri: string, content: string): Promise<CompilerDiagnostic[]> {
    const filePath = uri.startsWith('file:') ? fileURLToPath(uri) : uri;
    const output = await this.runCompiler(filePath, content);
    return this.parseDiagnostics(output, filePath);
  }

  private runCompiler(filePath: string, content: string): Promise<string> {
    return new Promise((resolve, reject) => {
      this.logger?.debug(`Running compiler: ${this.compilerPath} --lsp-diagnostics --stdin ${filePath}`);

      const proc = spawn(this.compilerPath, ['--lsp-diagnostics', '--stdin', filePath], {
        stdio: ['pipe', 'pipe', 'pipe']
      });

      let stdout = '';
      let stderr = '';
      proc.stdout.on('data', (data: Buffer) => {
        stdout += data.toString();
      });
      proc.stderr.on('data', (data: Buffer) => {
        stderr += data.toString();
      });
      proc.on('close', () => resolve(stdout || stderr));
      proc.on('error', (err: Error) => {
        this.logger?.error(`Compiler error: ${err.message}`);
        reject(new Error(`Failed to run compiler: ${err.message}`));
      });
      proc.stdin.on('error', () => undefined);
      proc.stdin.end(content);
    });
  }

  private parseDiagnostics(jsonOutput: string, filePath: string): CompilerDiagnostic[] {
    try {
      const response: LSPResponse = JSON.parse(jsonOutput);
      return response.diagnostics
        .filter(d => d.file === filePath)
        .map(d => ({
          range: d.range,
          severity: d.severity as 1 | 2 | 3 | 4,
          message: d.message,
          source: d.source,
          code: d.code
        }));
    } catch (error) {
      this.logger?.error(`Failed to parse diagnostics: ${error}`);
      return [{
        range: { start: { line: 0, character: 0 }, end: { line: 0, character: 0 } },
        severity: 1,
        message: `Compiler error: ${jsonOutput || 'Unknown error'}`,
        source: 'meadows-lsp'
      }];
    }
  }
}
