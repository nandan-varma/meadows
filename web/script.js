// Global variables
let editor = null;
let meadowsModule = null;
let compiler = null;

// Default sample code
const defaultCode = `// Welcome to Meadows Language Playground!
// Try running this sample code:

print("Hello, World!")

// Define a function
fun greet(name) {
    print("Hello, " + name + "!")
}

// Call the function
greet("Meadows")

// Variables and arithmetic
let x = 10
let y = 20
let sum = x + y
print("Sum: " + sum)`;

// Initialize the application
async function initializeApp() {
    try {
        updateStatus('Initializing Monaco Editor...');
        await initializeMonacoEditor();
        
        updateStatus('Loading WebAssembly module...');
        await initializeWASM();
        
        updateStatus('✅ Ready');
        setupEventListeners();
        updateInfo();
        
    } catch (error) {
        console.error('Initialization failed:', error);
        updateStatus('❌ Failed to initialize: ' + error.message);
    }
}

// Initialize Monaco Editor
async function initializeMonacoEditor() {
    return new Promise((resolve, reject) => {
        require.config({ paths: { vs: 'https://unpkg.com/monaco-editor@0.44.0/min/vs' } });
        
        require(['vs/editor/editor.main'], function () {
            try {
                // Define custom language for Meadows
                monaco.languages.register({ id: 'meadows' });
                
                // Set language configuration
                monaco.languages.setLanguageConfiguration('meadows', {
                    comments: {
                        lineComment: '//',
                        blockComment: ['/*', '*/']
                    },
                    brackets: [
                        ['{', '}'],
                        ['[', ']'],
                        ['(', ')']
                    ],
                    autoClosingPairs: [
                        { open: '{', close: '}' },
                        { open: '[', close: ']' },
                        { open: '(', close: ')' },
                        { open: '"', close: '"' },
                        { open: "'", close: "'" }
                    ],
                    surroundingPairs: [
                        { open: '{', close: '}' },
                        { open: '[', close: ']' },
                        { open: '(', close: ')' },
                        { open: '"', close: '"' },
                        { open: "'", close: "'" }
                    ]
                });

                // Set syntax highlighting
                monaco.languages.setMonarchTokensProvider('meadows', {
                    tokenizer: {
                        root: [
                            [/\b(fun|let|if|else|while|for|return|print|class|new|this)\b/, 'keyword'],
                            [/\b(true|false|null)\b/, 'constant'],
                            [/\b\d+(\.\d+)?\b/, 'number'],
                            [/"([^"\\]|\\.)*"/, 'string'],
                            [/'([^'\\]|\\.)*'/, 'string'],
                            [/\/\/.*$/, 'comment'],
                            [/\/\*[\s\S]*?\*\//, 'comment'],
                            [/[{}()\[\]]/, 'bracket'],
                            [/[;,.]/, 'delimiter'],
                            [/[+\-*/%=<>!&|]/, 'operator'],
                            [/[a-zA-Z_]\w*/, 'identifier']
                        ]
                    }
                });

                // Create editor
                editor = monaco.editor.create(document.getElementById('editor'), {
                    value: defaultCode,
                    language: 'meadows',
                    theme: 'vs-dark',
                    fontSize: 14,
                    fontFamily: 'Monaco, Menlo, "Ubuntu Mono", monospace',
                    minimap: { enabled: false },
                    scrollBeyondLastLine: false,
                    automaticLayout: true,
                    lineNumbers: 'on',
                    wordWrap: 'on',
                    folding: true,
                    renderWhitespace: 'selection',
                    cursorBlinking: 'smooth',
                    smoothScrolling: true
                });

                // Update cursor position
                editor.onDidChangeCursorPosition((e) => {
                    updateCursorPosition(e.position.lineNumber, e.position.column);
                });

                // Update info on content change
                editor.onDidChangeModelContent(() => {
                    updateInfo();
                });

                // Auto-compile on change (debounced)
                let compileTimeout;
                editor.onDidChangeModelContent(() => {
                    clearTimeout(compileTimeout);
                    compileTimeout = setTimeout(() => {
                        if (compiler) {
                            compileCode(false); // Silent compilation for live feedback
                        }
                    }, 1000);
                });

                resolve();
            } catch (error) {
                reject(error);
            }
        });
    });
}

// Initialize WASM module
async function initializeWASM() {
    try {
        // Check if MeadowsModule is available
        if (typeof MeadowsModule === 'undefined') {
            throw new Error('MeadowsModule not found. Make sure meadows.js is loaded.');
        }

        // Pre-load the WASM binary
        const wasmResponse = await fetch('meadows.wasm');
        if (!wasmResponse.ok) {
            throw new Error(`Failed to fetch WASM file: ${wasmResponse.status} ${wasmResponse.statusText}`);
        }
        const wasmBinary = await wasmResponse.arrayBuffer();

        // Configure the module
        const moduleConfig = {
            wasmBinary: wasmBinary,
            locateFile: (path, prefix) => {
                if (path.endsWith('.wasm')) {
                    return 'meadows.wasm';
                }
                return prefix + path;
            },
            onAbort: (what) => {
                console.error('WASM Module aborted:', what);
            },
            onRuntimeInitialized: () => {
                console.log('WASM Runtime initialized successfully');
            },
            print: (text) => {
                console.log('WASM stdout:', text);
            },
            printErr: (text) => {
                console.error('WASM stderr:', text);
            }
        };

        // Initialize the module
        meadowsModule = await MeadowsModule(moduleConfig);
        
        // Check for WebCompiler
        if (!meadowsModule.WebCompiler) {
            throw new Error('WebCompiler class not found in WASM module');
        }

        // Create compiler instance
        compiler = new meadowsModule.WebCompiler();
        console.log('WebCompiler initialized successfully');

    } catch (error) {
        console.error('WASM initialization failed:', error);
        throw error;
    }
}

// Setup event listeners
function setupEventListeners() {
    // Run button
    document.getElementById('runBtn').addEventListener('click', () => {
        compileCode(true);
    });

    // Clear button
    document.getElementById('clearBtn').addEventListener('click', () => {
        clearOutput();
        clearTokens();
        clearAST();
    });

    // Tab switching
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            switchTab(e.target.dataset.tab);
        });
    });

    // Traffic light buttons (just for show)
    document.querySelector('.traffic-light.close').addEventListener('click', () => {
        if (confirm('Close Meadows Playground?')) {
            window.close();
        }
    });

    document.querySelector('.traffic-light.minimize').addEventListener('click', () => {
        alert('Minimize not implemented in web version');
    });

    document.querySelector('.traffic-light.maximize').addEventListener('click', () => {
        document.documentElement.requestFullscreen?.();
    });
}

// Compile code
async function compileCode(showOutput = true) {
    if (!compiler) {
        showError('Compiler not initialized');
        return;
    }

    const code = editor.getValue();
    const startTime = performance.now();

    try {
        if (showOutput) {
            updateOutputStatus('Compiling...');
        }

        // Compile the code
        const resultJson = compiler.compile(code);
        const result = JSON.parse(resultJson);
        
        const endTime = performance.now();
        const compileTime = Math.round(endTime - startTime);

        // Update compilation info
        document.getElementById('compile-time').textContent = `${compileTime}ms`;
        document.getElementById('compile-status').textContent = result.success ? 'Success' : 'Error';

        if (result.success) {
            if (showOutput) {
                showOutput(result.output || 'Program executed successfully');
                updateOutputStatus('Success');
            }
            
            // Show tokens if available
            if (result.tokens) {
                showTokens(result.tokens);
            }

            // Show AST if available  
            if (result.ast) {
                showAST(result.ast);
            }

        } else {
            if (showOutput) {
                showError(result.error || 'Compilation failed');
                updateOutputStatus('Error');
            }
        }

    } catch (error) {
        console.error('Compilation error:', error);
        if (showOutput) {
            showError('Compilation failed: ' + error.message);
            updateOutputStatus('Error');
        }
    }
}

// Show output
function showOutput(output) {
    const content = document.getElementById('output-content');
    content.innerHTML = '';
    
    const lines = output.split('\n');
    lines.forEach(line => {
        const div = document.createElement('div');
        div.className = 'output-line fade-in';
        div.textContent = line;
        content.appendChild(div);
    });

    switchTab('output');
}

// Show error
function showError(error) {
    const content = document.getElementById('output-content');
    content.innerHTML = '';
    
    const div = document.createElement('div');
    div.className = 'output-error fade-in';
    div.textContent = error;
    content.appendChild(div);

    switchTab('output');
}

// Show tokens
function showTokens(tokens) {
    const content = document.getElementById('tokens-content');
    content.innerHTML = '';

    if (!tokens || tokens.length === 0) {
        content.innerHTML = '<div class="placeholder">No tokens found</div>';
        document.getElementById('tokens-count').textContent = '0 tokens';
        return;
    }

    tokens.forEach(token => {
        const div = document.createElement('div');
        div.className = 'token-item fade-in';
        
        const type = document.createElement('span');
        type.className = 'token-type';
        type.textContent = token.type || 'UNKNOWN';
        
        const value = document.createElement('span');
        value.className = 'token-value';
        value.textContent = token.value || token.lexeme || '';
        
        div.appendChild(type);
        div.appendChild(value);
        content.appendChild(div);
    });

    document.getElementById('tokens-count').textContent = `${tokens.length} tokens`;
}

// Show AST
function showAST(ast) {
    const content = document.getElementById('ast-content');
    content.innerHTML = '';

    if (!ast) {
        content.innerHTML = '<div class="placeholder">No AST data available</div>';
        return;
    }

    const tree = document.createElement('div');
    tree.className = 'ast-tree fade-in';
    tree.innerHTML = '<pre>' + JSON.stringify(ast, null, 2) + '</pre>';
    content.appendChild(tree);
}

// Clear output
function clearOutput() {
    document.getElementById('output-content').innerHTML = '<div class="placeholder">Run your code to see output here...</div>';
    updateOutputStatus('Ready');
}

// Clear tokens
function clearTokens() {
    document.getElementById('tokens-content').innerHTML = '<div class="placeholder">Tokens will appear here after compilation...</div>';
    document.getElementById('tokens-count').textContent = '0 tokens';
}

// Clear AST
function clearAST() {
    document.getElementById('ast-content').innerHTML = '<div class="placeholder">AST will appear here after parsing...</div>';
}

// Switch tab
function switchTab(tabName) {
    // Update tab buttons
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.remove('active');
    });
    document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');

    // Update tab panels
    document.querySelectorAll('.tab-panel').forEach(panel => {
        panel.classList.remove('active');
    });
    document.getElementById(`${tabName}-tab`).classList.add('active');
}

// Update status
function updateStatus(message) {
    document.getElementById('wasm-status').textContent = message;
}

// Update output status
function updateOutputStatus(status) {
    const statusEl = document.getElementById('output-status');
    statusEl.textContent = status;
    
    // Update status color based on result
    statusEl.className = 'status-indicator';
    if (status === 'Success') {
        statusEl.style.background = '#28ca42';
    } else if (status === 'Error') {
        statusEl.style.background = '#ff5f57';
    } else {
        statusEl.style.background = '#007acc';
    }
}

// Update cursor position
function updateCursorPosition(line, column) {
    document.getElementById('cursor-position').textContent = `Ln ${line}, Col ${column}`;
}

// Update info panel
function updateInfo() {
    if (!editor) return;

    const content = editor.getValue();
    const lines = content.split('\n').length;
    const chars = content.length;

    document.getElementById('line-count').textContent = lines;
    document.getElementById('char-count').textContent = chars;
}

// Initialize app when page loads
window.addEventListener('load', initializeApp);

// Handle window resize
window.addEventListener('resize', () => {
    if (editor) {
        editor.layout();
    }
});

// Keyboard shortcuts
document.addEventListener('keydown', (e) => {
    // Cmd/Ctrl + Enter to run
    if ((e.metaKey || e.ctrlKey) && e.key === 'Enter') {
        e.preventDefault();
        compileCode(true);
    }
    
    // Cmd/Ctrl + K to clear
    if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
        e.preventDefault();
        clearOutput();
        clearTokens();
        clearAST();
    }
});