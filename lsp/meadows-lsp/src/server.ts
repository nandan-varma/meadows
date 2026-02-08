import {
  createConnection,
  TextDocuments,
  Diagnostic,
  DiagnosticSeverity,
  ProposedFeatures,
  InitializeParams,
  DidChangeConfigurationNotification,
  CompletionItem,
  CompletionItemKind,
  TextDocumentPositionParams,
  TextDocumentSyncKind,
  InitializeResult
} from 'vscode-languageserver/node';

import { TextDocument } from 'vscode-languageserver-textdocument';
import { CompilerBridge } from './compiler-bridge';

// Create a connection for the server
const connection = createConnection(ProposedFeatures.all);

// Create a text document manager
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

// Compiler bridge instance
const compilerBridge = new CompilerBridge();

// Track whether the client supports configuration
let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;

connection.onInitialize((params: InitializeParams) => {
  const capabilities = params.capabilities;

  // Does the client support the `workspace/configuration` request?
  hasConfigurationCapability = !!(
    capabilities.workspace && !!capabilities.workspace.configuration
  );

  hasWorkspaceFolderCapability = !!(
    capabilities.workspace && !!capabilities.workspace.workspaceFolders
  );

  const result: InitializeResult = {
    capabilities: {
      textDocumentSync: TextDocumentSyncKind.Incremental,
      // Future: completionProvider, hoverProvider, etc.
    }
  };

  if (hasWorkspaceFolderCapability) {
    result.capabilities.workspace = {
      workspaceFolders: {
        supported: true
      }
    };
  }

  return result;
});

connection.onInitialized(() => {
  if (hasConfigurationCapability) {
    // Register for all configuration changes
    connection.client.register(DidChangeConfigurationNotification.type, undefined);
  }
});

// The content of a text document has changed
documents.onDidChangeContent(change => {
  validateDocument(change.document);
});

async function validateDocument(textDocument: TextDocument): Promise<void> {
  const uri = textDocument.uri;
  const content = textDocument.getText();

  try {
    const compilerDiagnostics = await compilerBridge.getDiagnostics(uri, content);
    
    const diagnostics: Diagnostic[] = compilerDiagnostics.map(d => ({
      severity: mapSeverity(d.severity),
      range: d.range,
      message: d.message,
      source: d.source
    }));

    // Send diagnostics to client
    connection.sendDiagnostics({ uri: textDocument.uri, diagnostics });
  } catch (error) {
    connection.console.error(`Error validating document: ${error}`);
  }
}

function mapSeverity(severity: number): DiagnosticSeverity {
  switch (severity) {
    case 1: return DiagnosticSeverity.Error;
    case 2: return DiagnosticSeverity.Warning;
    case 3: return DiagnosticSeverity.Information;
    case 4: return DiagnosticSeverity.Hint;
    default: return DiagnosticSeverity.Error;
  }
}

// Make the text document manager listen on the connection
documents.listen(connection);

// Listen on the connection
connection.listen();

connection.console.log('Meadows Language Server is running');
