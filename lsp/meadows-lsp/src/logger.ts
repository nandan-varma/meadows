import { Connection } from 'vscode-languageserver/node';

export class Logger {
  private connection: Connection;
  private logLevel: 'debug' | 'info' | 'warn' | 'error' = 'info';

  constructor(connection: Connection) {
    this.connection = connection;
  }

  setLogLevel(level: 'debug' | 'info' | 'warn' | 'error') {
    this.logLevel = level;
  }

  debug(message: string) {
    if (this.shouldLog('debug')) {
      this.connection.console.log(`[DEBUG] ${message}`);
    }
  }

  info(message: string) {
    if (this.shouldLog('info')) {
      this.connection.console.log(`[INFO] ${message}`);
    }
  }

  warn(message: string) {
    if (this.shouldLog('warn')) {
      this.connection.console.warn(`[WARN] ${message}`);
    }
  }

  error(message: string) {
    if (this.shouldLog('error')) {
      this.connection.console.error(`[ERROR] ${message}`);
    }
  }

  private shouldLog(level: 'debug' | 'info' | 'warn' | 'error'): boolean {
    const levels = { debug: 0, info: 1, warn: 2, error: 3 };
    return levels[level] >= levels[this.logLevel];
  }
}
