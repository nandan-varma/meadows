import { spawn } from 'child_process';
import { tmpdir } from 'os';
import { writeFile, unlink } from 'fs/promises';
import { join } from 'path';
import { Range } from 'vscode-languageserver/node';

export interface CompilerDiagnostic {
  range: Range;
  severity: 1 | 2 | 3 | 4; // Error, Warning, Info, Hint
  message: string;
  source: string;
}

interface LSPResponse {
  file: string;
  diagnostics: Array<{
    range: {
      start: { line: number; character: number };
      end: { line: number; character: number };
    };
    severity: number;
    message: string;
    source: string;
  }>;
}

export class CompilerBridge {
  private compilerPath: string;

  constructor(compilerPath: string = 'meadows') {
    this.compilerPath = compilerPath;
  }

  async getDiagnostics(uri: string, content: string): Promise<CompilerDiagnostic[]> {
    // Create a temporary file for the compiler
    const tempFile = join(tmpdir(), `meadows-lsp-${Date.now()}.ms`);
    
    try {
      // Write content to temp file
      await writeFile(tempFile, content, 'utf8');
      
      // Run compiler with LSP diagnostics flag
      const result = await this.runCompiler(tempFile);
      
      // Parse the JSON response
      return this.parseDiagnostics(result, uri);
    } finally {
      // Clean up temp file
      try {
        await unlink(tempFile);
      } catch {
        // Ignore cleanup errors
      }
    }
  }

  private runCompiler(filePath: string): Promise<string> {
    return new Promise((resolve, reject) => {
      const proc = spawn(this.compilerPath, ['--lsp-diagnostics', filePath], {
        stdio: ['ignore', 'pipe', 'pipe']
      });

      let stdout = '';
      let stderr = '';

      proc.stdout.on('data', (data: Buffer) => {
        stdout += data.toString();
      });

      proc.stderr.on('data', (data: Buffer) => {
        stderr += data.toString();
      });

      proc.on('close', (code: number | null) => {
        // The compiler outputs JSON to stdout in LSP mode
        // Exit code 0 means success (even if there are diagnostics)
        resolve(stdout || stderr);
      });

      proc.on('error', (err: Error) => {
        reject(new Error(`Failed to run compiler: ${err.message}`));
      });
    });
  }

  private parseDiagnostics(jsonOutput: string, uri: string): CompilerDiagnostic[] {
    try {
      // Try to parse the JSON output
      const response: LSPResponse = JSON.parse(jsonOutput);
      
      return response.diagnostics.map(d => ({
        range: {
          start: d.range.start,
          end: d.range.end
        },
        severity: d.severity as 1 | 2 | 3 | 4,
        message: d.message,
        source: d.source
      }));
    } catch (error) {
      // If JSON parsing fails, create a generic error diagnostic
      return [{
        range: {
          start: { line: 0, character: 0 },
          end: { line: 0, character: 0 }
        },
        severity: 1,
        message: `Compiler error: ${jsonOutput || 'Unknown error'}`,
        source: 'meadows-lsp'
      }];
    }
  }
}
