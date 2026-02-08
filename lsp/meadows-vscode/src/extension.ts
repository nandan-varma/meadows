import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
  // The server is implemented in Node
  const serverModule = context.asAbsolutePath(
    path.join('..', 'meadows-lsp', 'dist', 'server.js')
  );

  // Server options
  const serverOptions: ServerOptions = {
    run: { 
      module: serverModule, 
      transport: TransportKind.ipc 
    },
    debug: { 
      module: serverModule, 
      transport: TransportKind.ipc 
    }
  };

  // Client options
  const clientOptions: LanguageClientOptions = {
    // Register for Meadows documents
    documentSelector: [
      { scheme: 'file', language: 'meadows' },
      { scheme: 'untitled', language: 'meadows' }
    ],
    synchronize: {
      // Notify server about file changes to .ms files
      fileEvents: workspace.createFileSystemWatcher('**/*.ms')
    }
  };

  // Create and start the client
  client = new LanguageClient(
    'meadowsLanguageServer',
    'Meadows Language Server',
    serverOptions,
    clientOptions
  );

  client.start();
}

export function deactivate(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  return client.stop();
}
