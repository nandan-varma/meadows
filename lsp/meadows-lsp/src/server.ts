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
  InitializeResult,
  SemanticTokensBuilder,
  SemanticTokensLegend,
  SemanticTokensParams,
  HoverParams,
  Hover,
  MarkupKind,
  DocumentSymbolParams,
  DocumentSymbol,
  SymbolKind
} from 'vscode-languageserver/node';

import { TextDocument } from 'vscode-languageserver-textdocument';
import { CompilerBridge } from './compiler-bridge';
import { Logger } from './logger';
import { SemanticTokenProvider } from './semantic-tokens';
import { HoverProvider } from './hover-provider';

// Create a connection for the server
const connection = createConnection(ProposedFeatures.all);

// Create a text document manager
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

// Initialize components
const logger = new Logger(connection);
const compilerBridge = new CompilerBridge('Meadows', logger);
const tokenProvider = new SemanticTokenProvider(logger);
const hoverProvider = new HoverProvider(logger);

// Track capabilities
let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
let hasSemanticTokensCapability = false;
let hasHoverCapability = false;

// Performance tracking
const performanceMetrics = {
  totalValidations: 0,
  totalValidationTime: 0,
  avgValidationTime: 0
};

connection.onInitialize((params: InitializeParams) => {
  const capabilities = params.capabilities;
  
  logger.info('Initializing Meadows Language Server...');
  logger.debug(`Client capabilities: ${JSON.stringify(capabilities.textDocument)}`);

  // Check client capabilities
  hasConfigurationCapability = !!(
    capabilities.workspace && !!capabilities.workspace.configuration
  );

  hasWorkspaceFolderCapability = !!(
    capabilities.workspace && !!capabilities.workspace.workspaceFolders
  );
  
  hasSemanticTokensCapability = !!(
    capabilities.textDocument && capabilities.textDocument.semanticTokens
  );
  
  hasHoverCapability = !!(
    capabilities.textDocument && capabilities.textDocument.hover
  );

  const result: InitializeResult = {
    capabilities: {
      textDocumentSync: TextDocumentSyncKind.Incremental,
      
      // Semantic Tokens Provider
      semanticTokensProvider: hasSemanticTokensCapability ? {
        legend: {
          tokenTypes: [
            'namespace', 'type', 'class', 'enum', 'interface',
            'struct', 'typeParameter', 'parameter', 'variable', 'property',
            'enumMember', 'event', 'function', 'method', 'macro',
            'keyword', 'modifier', 'comment', 'string', 'number',
            'regexp', 'operator'
          ],
          tokenModifiers: [
            'declaration', 'definition', 'readonly', 'static',
            'deprecated', 'abstract', 'async', 'modification',
            'documentation', 'defaultLibrary'
          ]
        },
        full: {
          delta: false
        },
        range: true
      } : undefined,
      
      // Hover Provider
      hoverProvider: hasHoverCapability,
      
      // Document Symbol Provider (Outline)
      documentSymbolProvider: true
    }
  };

  if (hasWorkspaceFolderCapability) {
    result.capabilities.workspace = {
      workspaceFolders: {
        supported: true
      }
    };
  }
  
  logger.info(`Server initialized with capabilities:`);
  logger.info(`  - Semantic Tokens: ${hasSemanticTokensCapability}`);
  logger.info(`  - Hover: ${hasHoverCapability}`);
  logger.info(`  - Configuration: ${hasConfigurationCapability}`);

  return result;
});

connection.onInitialized(() => {
  logger.info('Meadows Language Server initialized successfully');
  
  if (hasConfigurationCapability) {
    connection.client.register(DidChangeConfigurationNotification.type, undefined);
  }
});

// The content of a text document has changed
documents.onDidChangeContent(change => {
  validateDocument(change.document);
});

async function validateDocument(textDocument: TextDocument): Promise<void> {
  const startTime = Date.now();
  const uri = textDocument.uri;
  const content = textDocument.getText();

  logger.debug(`Validating document: ${uri}`);

  try {
    const compilerDiagnostics = await compilerBridge.getDiagnostics(uri, content);
    
    const diagnostics: Diagnostic[] = compilerDiagnostics.map(d => ({
      severity: mapSeverity(d.severity),
      range: d.range,
      message: d.message,
      source: d.source,
      code: d.code
    }));

    // Send diagnostics to client
    connection.sendDiagnostics({ uri: textDocument.uri, diagnostics });
    
    // Update performance metrics
    const duration = Date.now() - startTime;
    performanceMetrics.totalValidations++;
    performanceMetrics.totalValidationTime += duration;
    performanceMetrics.avgValidationTime = 
      performanceMetrics.totalValidationTime / performanceMetrics.totalValidations;
    
    logger.debug(`Validation completed in ${duration}ms (${diagnostics.length} diagnostics)`);
    logger.debug(`Average validation time: ${performanceMetrics.avgValidationTime.toFixed(2)}ms`);
    
  } catch (error) {
    logger.error(`Error validating document: ${error}`);
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

// Semantic Tokens Provider
connection.languages.semanticTokens.on((params: SemanticTokensParams) => {
  const document = documents.get(params.textDocument.uri);
  if (!document) {
    logger.warn(`Document not found for semantic tokens: ${params.textDocument.uri}`);
    return { data: [] };
  }
  
  logger.debug(`Computing semantic tokens for: ${params.textDocument.uri}`);
  const startTime = Date.now();
  
  const tokens = tokenProvider.provideSemanticTokens(document);
  
  const duration = Date.now() - startTime;
  logger.debug(`Semantic tokens computed in ${duration}ms`);
  
  return tokens;
});

// Hover Provider
connection.onHover((params: HoverParams): Hover | null => {
  const document = documents.get(params.textDocument.uri);
  if (!document) {
    return null;
  }
  
  logger.debug(`Hover requested at ${params.position.line}:${params.position.character}`);
  
  const hoverInfo = hoverProvider.provideHover(document, params.position);
  
  if (hoverInfo) {
    logger.debug(`Hover info found: ${hoverInfo.contents}`);
  }
  
  return hoverInfo;
});

// Document Symbols (Outline)
connection.onDocumentSymbol((params: DocumentSymbolParams): DocumentSymbol[] => {
  const document = documents.get(params.textDocument.uri);
  if (!document) {
    return [];
  }
  
  logger.debug(`Document symbols requested for: ${params.textDocument.uri}`);
  
  // Simple regex-based symbol extraction
  const symbols: DocumentSymbol[] = [];
  const text = document.getText();
  const lines = text.split('\n');
  
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    
    // Match function declarations
    const funcMatch = line.match(/func\s+(\w+)\s*\(/);
    if (funcMatch) {
      symbols.push({
        name: funcMatch[1],
        kind: SymbolKind.Function,
        range: {
          start: { line: i, character: 0 },
          end: { line: i, character: line.length }
        },
        selectionRange: {
          start: { line: i, character: line.indexOf(funcMatch[1]) },
          end: { line: i, character: line.indexOf(funcMatch[1]) + funcMatch[1].length }
        }
      });
    }
    
    // Match variable declarations
    const varMatch = line.match(/let\s+(\w+)\s*=/);
    if (varMatch) {
      symbols.push({
        name: varMatch[1],
        kind: SymbolKind.Variable,
        range: {
          start: { line: i, character: 0 },
          end: { line: i, character: line.length }
        },
        selectionRange: {
          start: { line: i, character: line.indexOf(varMatch[1]) },
          end: { line: i, character: line.indexOf(varMatch[1]) + varMatch[1].length }
        }
      });
    }
  }
  
  logger.debug(`Found ${symbols.length} symbols`);
  return symbols;
});

// Make the text document manager listen on the connection
documents.listen(connection);

// Listen on the connection
connection.listen();

logger.info('Meadows Language Server is running');
