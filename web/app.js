// Meadows Compiler Web IDE - JavaScript Application with Monaco Editor

class MeadowsIDE {
    constructor() {
        this.compiler = null;
        this.isLoading = false;
        this.codeEditor = null;
        this.outputEditors = {};
        this.examples = {
            hello: `# Hello World in Meadows
print("Hello, World!")
print("Welcome to Meadows!")`,
            
            fibonacci: `# Fibonacci sequence generator
def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n - 1) + fibonacci(n - 2)

# Calculate first 10 Fibonacci numbers
i = 0
while i < 10:
    print(fibonacci(i))
    i = i + 1`,
    
            factorial: `# Factorial calculation
def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

# Test factorial function
print("Factorial of 5:")
print(factorial(5))
print("Factorial of 10:")
print(factorial(10))`,
    
            calculator: `# Simple calculator
def add(a, b):
    return a + b

def multiply(a, b):
    return a * b

def power(base, exp):
    if exp == 0:
        return 1
    else:
        return base * power(base, exp - 1)

# Perform calculations
x = 10
y = 5

print("Addition:")
print(add(x, y))
print("Multiplication:")
print(multiply(x, y))
print("Power:")
print(power(x, 3))`
        };
        
        try {
            this.initializeElements();
            this.setupEventListeners();
            this.initializeMonaco();
            
            // Delay compiler loading to ensure DOM is ready
            setTimeout(() => {
                this.loadCompiler();
            }, 100);
            
        } catch (error) {
            console.error('Failed to initialize MeadowsIDE:', error);
            this.updateStatus('error', 'Initialization Failed');
            
            // Try to at least show a basic error state
            try {
                const runBtn = document.getElementById('runBtn');
                if (runBtn) {
                    runBtn.disabled = true;
                    runBtn.textContent = 'IDE Error';
                }
            } catch (btnError) {
                console.error('Could not update run button:', btnError);
            }
        }
    }

    initializeElements() {
        this.elements = {
            // Buttons and controls
            runBtn: document.getElementById('runBtn'),
            clearBtn: document.getElementById('clearBtn'),
            clearOutputBtn: document.getElementById('clearOutputBtn'),
            exampleSelect: document.getElementById('exampleSelect'),
            
            // Editor containers
            codeEditorContainer: document.getElementById('codeEditor'),
            executionOutputContainer: document.getElementById('executionMonaco'),
            astOutputContainer: document.getElementById('astMonaco'),
            tokensOutputContainer: document.getElementById('tokensMonaco'),
            irOutputContainer: document.getElementById('irMonaco'),
            
            // Output sections and placeholders
            executionOutput: document.getElementById('executionOutput'),
            astOutput: document.getElementById('astOutput'),
            tokensOutput: document.getElementById('tokensOutput'),
            irOutput: document.getElementById('irOutput'),
            
            executionPlaceholder: document.getElementById('executionPlaceholder'),
            astPlaceholder: document.getElementById('astPlaceholder'),
            tokensPlaceholder: document.getElementById('tokensPlaceholder'),
            irPlaceholder: document.getElementById('irPlaceholder'),
            
            // Status and modals
            statusIndicator: document.getElementById('statusIndicator'),
            loadingModal: document.getElementById('loadingModal'),
            errorModal: document.getElementById('errorModal'),
            aboutModal: document.getElementById('aboutModal'),
            aboutBtn: document.getElementById('aboutBtn'),
            closeAboutModal: document.getElementById('closeAboutModal'),
            closeErrorModal: document.getElementById('closeErrorModal'),
            buildDate: document.getElementById('buildDate'),
            lineCount: document.getElementById('lineCount'),
            
            // Tab buttons
            tabButtons: document.querySelectorAll('.tab-btn'),
            outputSections: document.querySelectorAll('.output-section')
        };
    }

    setupEventListeners() {
        // Button event listeners
        this.elements.runBtn.addEventListener('click', () => this.runCode());
        this.elements.clearBtn.addEventListener('click', () => this.clearEditor());
        this.elements.clearOutputBtn.addEventListener('click', () => this.clearOutput());
        
        // Example selector
        this.elements.exampleSelect.addEventListener('change', (e) => {
            if (e.target.value) {
                this.loadExample(e.target.value);
                e.target.value = '';
            }
        });
        
        // Tab switching
        this.elements.tabButtons.forEach(btn => {
            btn.addEventListener('click', () => {
                this.switchTab(btn.dataset.tab);
            });
        });
        
        // Modal controls
        this.elements.aboutBtn.addEventListener('click', () => this.showModal('about'));
        this.elements.closeAboutModal.addEventListener('click', () => this.hideModal('about'));
        this.elements.closeErrorModal.addEventListener('click', () => this.hideModal('error'));
        
        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => this.handleKeyboard(e));
        
        // Build date
        this.elements.buildDate.textContent = new Date().toLocaleDateString();
    }

    async initializeMonaco() {
        try {
            // Configure Monaco Editor
            require.config({ 
                paths: { 
                    'vs': 'https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.44.0/min/vs' 
                }
            });
            
            require(['vs/editor/editor.main'], () => {
                this.setupMonacoLanguage();
                this.createEditors();
                this.updateStatus('success', 'Editor Ready');
            });
        } catch (error) {
            console.error('Failed to initialize Monaco Editor:', error);
            this.updateStatus('error', 'Editor Failed to Load');
        }
    }

    setupMonacoLanguage() {
        // Define Meadows language syntax highlighting
        monaco.languages.register({ id: 'meadows' });
        
        monaco.languages.setMonarchTokensProvider('meadows', {
            tokenizer: {
                root: [
                    // Keywords
                    [/\b(def|if|else|elif|while|for|return|print|class|import|from|as|try|except|finally|with|lambda|and|or|not|in|is|True|False|None)\b/, 'keyword'],
                    
                    // Numbers
                    [/\d+/, 'number'],
                    [/\d+\.\d+/, 'number.float'],
                    
                    // Strings
                    [/"([^"\\]|\\.)*"/, 'string'],
                    [/'([^'\\]|\\.)*'/, 'string'],
                    
                    // Comments
                    [/#.*$/, 'comment'],
                    
                    // Operators
                    [/[+\-*/%=<>!&|^~]/, 'operator'],
                    
                    // Delimiters
                    [/[{}[\]()]/, 'delimiter'],
                    [/[,;.]/, 'delimiter'],
                    
                    // Variables and identifiers
                    [/[a-zA-Z_]\w*/, 'identifier'],
                    
                    // Whitespace
                    [/\s+/, 'white']
                ]
            }
        });

        // Set language configuration
        monaco.languages.setLanguageConfiguration('meadows', {
            comments: {
                lineComment: '#'
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

        // Define theme
        monaco.editor.defineTheme('meadows-theme', {
            base: 'vs',
            inherit: true,
            rules: [
                { token: 'keyword', foreground: '007aff', fontStyle: 'bold' },
                { token: 'comment', foreground: '8e8e93', fontStyle: 'italic' },
                { token: 'string', foreground: '34c759' },
                { token: 'number', foreground: 'ff9500' },
                { token: 'operator', foreground: '000000' },
                { token: 'identifier', foreground: '000000' }
            ],
            colors: {
                'editor.background': '#ffffff',
                'editor.foreground': '#000000',
                'editor.lineHighlightBackground': '#f2f2f7',
                'editor.selectionBackground': '#007aff40'
            }
        });
    }

    createEditors() {
        // Create main code editor
        this.codeEditor = monaco.editor.create(this.elements.codeEditorContainer, {
            value: `# Welcome to Meadows Compiler
# Write your Python-like code here

def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

print(factorial(5))`,
            language: 'meadows',
            theme: 'meadows-theme',
            fontSize: 14,
            lineNumbers: 'on',
            minimap: { enabled: false },
            scrollBeyondLastLine: false,
            automaticLayout: true,
            tabSize: 4,
            insertSpaces: true,
            wordWrap: 'on',
            folding: true,
            showFoldingControls: 'always'
        });

        // Create output editors
        this.outputEditors.execution = this.createOutputEditor(this.elements.executionOutputContainer, 'plaintext');
        this.outputEditors.ast = this.createOutputEditor(this.elements.astOutputContainer, 'json');
        this.outputEditors.tokens = this.createOutputEditor(this.elements.tokensOutputContainer, 'json');
        this.outputEditors.ir = this.createOutputEditor(this.elements.irOutputContainer, 'plaintext');

        // Add event listeners for the main editor
        this.codeEditor.onDidChangeModelContent(() => {
            this.updateLineCount();
        });

        this.updateLineCount();
    }

    createOutputEditor(container, language) {
        return monaco.editor.create(container, {
            value: '',
            language: language,
            theme: 'meadows-theme',
            fontSize: 13,
            lineNumbers: 'off',
            minimap: { enabled: false },
            scrollBeyondLastLine: false,
            automaticLayout: true,
            readOnly: true,
            wordWrap: 'on',
            folding: false,
            glyphMargin: false,
            lineDecorationsWidth: 0,
            lineNumbersMinChars: 0,
            overviewRulerLanes: 0,
            overviewRulerBorder: false,
            hideCursorInOverviewRuler: true,
            scrollbar: {
                vertical: 'auto',
                horizontal: 'auto'
            }
        });
    }

    updateLineCount() {
        if (this.codeEditor) {
            const lineCount = this.codeEditor.getModel().getLineCount();
            this.elements.lineCount.textContent = `${lineCount} line${lineCount !== 1 ? 's' : ''}`;
        }
    }

    async loadCompiler() {
        this.showModal('loading');
        this.updateStatus('loading', 'Loading Compiler...');
        
        try {
            // Check if MeadowsModule is available
            if (typeof MeadowsModule === 'undefined') {
                throw new Error('MeadowsModule not available. Please ensure meadows.js loaded correctly.');
            }
            
            console.log('MeadowsModule found, initializing...');
            this.updateStatus('loading', 'Loading WASM binary...');
            
            // Pre-load the WASM binary to avoid path issues
            let wasmBinary;
            try {
                const wasmResponse = await fetch('meadows.wasm');
                if (!wasmResponse.ok) {
                    throw new Error(`Failed to fetch WASM file: ${wasmResponse.status} ${wasmResponse.statusText}`);
                }
                wasmBinary = await wasmResponse.arrayBuffer();
                console.log('WASM binary loaded, size:', wasmBinary.byteLength);
            } catch (fetchError) {
                console.error('Failed to fetch WASM binary:', fetchError);
                throw new Error(`Could not load meadows.wasm: ${fetchError.message}`);
            }
            
            this.updateStatus('loading', 'Initializing WebAssembly...');
            
            // Configure the module with the pre-loaded WASM binary
            const moduleConfig = {
                wasmBinary: wasmBinary,  // Provide the WASM binary directly
                locateFile: (path, prefix) => {
                    console.log('Locating file:', path, 'with prefix:', prefix);
                    if (path.endsWith('.wasm')) {
                        return 'meadows.wasm';
                    }
                    return prefix + path;
                },
                onAbort: (what) => {
                    console.error('WebAssembly module aborted:', what);
                },
                onRuntimeInitialized: () => {
                    console.log('WebAssembly runtime initialized successfully');
                },
                print: (text) => {
                    console.log('WASM stdout:', text);
                },
                printErr: (text) => {
                    console.error('WASM stderr:', text);
                }
            };
            
            // Initialize the WebAssembly module with configuration
            const module = await MeadowsModule(moduleConfig);
            console.log('WebAssembly module loaded:', module);
            console.log('Available exports:', Object.keys(module));
            
            // Check if the module has the WebCompiler class
            if (module && module.WebCompiler) {
                console.log('WebCompiler class found, creating instance...');
                this.updateStatus('loading', 'Creating compiler instance...');
                
                const webCompiler = new module.WebCompiler();
                
                // Test the compiler with a simple example
                this.updateStatus('loading', 'Testing compiler...');
                const testResult = webCompiler.compile('# Test');
                console.log('Test compilation result:', testResult);
                
                this.compiler = {
                    compile: (source) => {
                        try {
                            console.log('Compiling source:', source);
                            const resultJson = webCompiler.compile(source);
                            console.log('Raw result from WASM:', resultJson);
                            const result = JSON.parse(resultJson);
                            console.log('Parsed result:', result);
                            return result;
                        } catch (error) {
                            console.error('WASM compilation error:', error);
                            return {
                                success: false,
                                message: 'WebAssembly compilation failed: ' + error.message,
                                errors: [error.message]
                            };
                        }
                    }
                };
                
                this.hideModal('loading');
                this.updateStatus('success', 'Ready (WebAssembly)');
                console.log('WebAssembly compiler initialized successfully');
                
            } else {
                throw new Error('WebCompiler class not found in WASM module. Available exports: ' + Object.keys(module || {}));
            }
            
        } catch (error) {
            console.error('Failed to load compiler:', error);
            this.hideModal('loading');
            
            // Show a more user-friendly error message
            this.updateStatus('error', 'Compiler unavailable - check console');
            
            if (this.elements.runBtn) {
                this.elements.runBtn.disabled = true;
                this.elements.runBtn.textContent = 'Compiler Unavailable';
                this.elements.runBtn.title = 'WebAssembly compiler failed to load. Check browser console for details.';
            }
            
            // Display detailed error information in console only
            console.error('Detailed error information:', {
                message: error.message,
                stack: error.stack,
                moduleAvailable: typeof MeadowsModule !== 'undefined',
                wasmSupported: 'WebAssembly' in window,
                currentURL: window.location.href
            });
            
            // Show a toast-like notification instead of a modal
            this.showToastError('Compiler could not be loaded. See console for details.');
        }
    }

    async runCode() {
        if (!this.codeEditor) {
            this.showError('Editor not initialized');
            return;
        }

        const code = this.codeEditor.getValue().trim();
        if (!code) {
            this.showWarning('Please enter some code to run');
            return;
        }

        if (!this.compiler) {
            this.showError('Compiler not available. The WebAssembly module failed to load. Please refresh the page or check the browser console for details.');
            return;
        }

        this.updateStatus('loading', 'Compiling...');
        this.elements.runBtn.disabled = true;

        try {
            // Clear previous outputs
            this.clearOutput();
            
            console.log('Running code:', code);
            
            // Compile using the WebAssembly compiler
            const result = this.compiler.compile(code);
            console.log('Compilation result:', result);

            // Display results based on the actual response structure
            if (result.success) {
                // Show execution output
                if (result.execution && result.execution.output) {
                    // The output comes with escape sequences, so we need to process them
                    const processedOutput = result.execution.output
                        .replace(/\\n/g, '\n')
                        .replace(/\\t/g, '\t')
                        .replace(/\\r/g, '\r');
                    this.showOutput('execution', processedOutput);
                } else {
                    this.showOutput('execution', 'Program executed successfully (no output)');
                }
                
                // Show tokens if available
                if (result.tokens && Array.isArray(result.tokens)) {
                    this.showOutput('tokens', JSON.stringify(result.tokens, null, 2));
                } else if (result.tokens) {
                    this.showOutput('tokens', JSON.stringify(result.tokens, null, 2));
                }
                
                // Show AST if available
                if (result.ast) {
                    if (typeof result.ast === 'string') {
                        this.showOutput('ast', result.ast);
                    } else {
                        this.showOutput('ast', JSON.stringify(result.ast, null, 2));
                    }
                }
                
                // Show IR if available
                if (result.ir) {
                    this.showOutput('ir', result.ir);
                }
                
                this.updateStatus('success', 'Execution Complete');
                
                // Switch to execution tab to show results
                this.switchTab('execution');
                
            } else {
                // Handle compilation errors
                let errorMessage = result.message || 'Compilation failed';
                let errorDetails = [];
                
                if (result.errors && Array.isArray(result.errors)) {
                    errorDetails = result.errors;
                }
                
                if (result.warnings && Array.isArray(result.warnings) && result.warnings.length > 0) {
                    errorMessage += '\n\nWarnings:\n' + result.warnings.join('\n');
                }
                
                this.showError(errorMessage, errorDetails);
                this.updateStatus('error', 'Compilation Failed');
            }
        } catch (error) {
            console.error('Execution error:', error);
            this.showError('Runtime error: ' + error.message);
            this.updateStatus('error', 'Runtime Error');
        } finally {
            this.elements.runBtn.disabled = false;
        }
    }

    showOutput(type, content) {
        const editor = this.outputEditors[type];
        const placeholder = this.elements[type + 'Placeholder'];
        const container = this.elements[type + 'OutputContainer'];
        
        if (editor && content) {
            editor.getModel().setValue(content);
            placeholder.style.display = 'none';
            container.classList.add('active');
            
            // If this is the current tab, make sure it's visible
            const activeTab = document.querySelector('.tab-btn.active');
            if (activeTab && activeTab.dataset.tab === type) {
                editor.layout();
            }
        }
    }

    clearOutput() {
        Object.keys(this.outputEditors).forEach(type => {
            const editor = this.outputEditors[type];
            const placeholder = this.elements[type + 'Placeholder'];
            const container = this.elements[type + 'OutputContainer'];
            
            if (editor) {
                editor.getModel().setValue('');
                container.classList.remove('active');
                placeholder.style.display = 'flex';
            }
        });
        
        // Switch to execution tab
        this.switchTab('execution');
    }

    clearEditor() {
        if (this.codeEditor) {
            this.codeEditor.getModel().setValue('');
            this.codeEditor.focus();
        }
    }

    loadExample(exampleName) {
        if (this.examples[exampleName] && this.codeEditor) {
            this.codeEditor.getModel().setValue(this.examples[exampleName]);
            this.codeEditor.focus();
        }
    }

    switchTab(tabName) {
        // Update tab buttons
        this.elements.tabButtons.forEach(btn => {
            btn.classList.toggle('active', btn.dataset.tab === tabName);
        });
        
        // Show/hide output sections
        this.elements.outputSections.forEach(section => {
            section.classList.toggle('active', section.id === tabName + 'Output');
        });

        // Show/hide Monaco containers and placeholders
        const editors = ['execution', 'ast', 'tokens', 'ir'];
        editors.forEach(type => {
            const container = this.elements[type + 'OutputContainer'];
            const placeholder = this.elements[type + 'Placeholder'];
            const editor = this.outputEditors[type];
            
            if (type === tabName) {
                const hasContent = editor && editor.getModel().getValue().trim() !== '';
                container.classList.toggle('active', hasContent);
                placeholder.style.display = hasContent ? 'none' : 'flex';
                
                if (editor && hasContent) {
                    editor.layout();
                }
            } else {
                container.classList.remove('active');
            }
        });
    }

    showModal(modalName) {
        try {
            const modal = this.elements[modalName + 'Modal'];
            if (modal) {
                modal.classList.add('active');
                document.body.style.overflow = 'hidden';
            }
        } catch (error) {
            console.log(`Could not show modal: ${modalName}`);
        }
    }

    hideModal(modalName) {
        try {
            const modal = this.elements[modalName + 'Modal'];
            if (modal) {
                modal.classList.remove('active');
                document.body.style.overflow = '';
            }
        } catch (error) {
            console.log(`Could not hide modal: ${modalName}`);
        }
    }

    updateStatus(type, message) {
        try {
            const statusDot = this.elements?.statusIndicator?.querySelector('.status-dot');
            const statusText = this.elements?.statusIndicator?.querySelector('.status-text');
            
            if (statusDot && statusText) {
                statusDot.className = `status-dot ${type}`;
                statusText.textContent = message;
            } else {
                console.log(`Status update: ${type} - ${message}`);
            }
        } catch (error) {
            console.log(`Status update: ${type} - ${message}`);
        }
    }

    showToastError(message) {
        // Create a simple toast notification instead of a modal
        const toast = document.createElement('div');
        toast.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            background: #ff4444;
            color: white;
            padding: 12px 20px;
            border-radius: 4px;
            font-size: 14px;
            z-index: 10000;
            max-width: 300px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.3);
            opacity: 0;
            transition: opacity 0.3s ease;
        `;
        toast.textContent = message;
        
        document.body.appendChild(toast);
        
        // Fade in
        setTimeout(() => {
            toast.style.opacity = '1';
        }, 10);
        
        // Auto-remove after 5 seconds
        setTimeout(() => {
            toast.style.opacity = '0';
            setTimeout(() => {
                if (toast.parentNode) {
                    toast.parentNode.removeChild(toast);
                }
            }, 300);
        }, 5000);
        
        // Click to dismiss
        toast.addEventListener('click', () => {
            toast.style.opacity = '0';
            setTimeout(() => {
                if (toast.parentNode) {
                    toast.parentNode.removeChild(toast);
                }
            }, 300);
        });
    }

    showError(message, details = []) {
        this.showOutput('execution', `Error: ${message}${details.length ? '\n\nDetails:\n' + details.join('\n') : ''}`);
        this.switchTab('execution');
    }

    showWarning(message) {
        this.showOutput('execution', `Warning: ${message}`);
        this.switchTab('execution');
    }

    handleKeyboard(event) {
        // Ctrl/Cmd + Enter to run code
        if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
            event.preventDefault();
            this.runCode();
        }
        
        // Ctrl/Cmd + L to clear editor
        if ((event.ctrlKey || event.metaKey) && event.key === 'l') {
            event.preventDefault();
            this.clearEditor();
        }
        
        // Escape to close modals
        if (event.key === 'Escape') {
            this.hideModal('about');
            this.hideModal('loading');
            this.hideModal('error');
        }
    }
}

// Initialize the IDE when the page loads
document.addEventListener('DOMContentLoaded', async () => {
    // Wait a bit for all scripts to load, especially the WebAssembly module
    await new Promise(resolve => setTimeout(resolve, 500));
    
    console.log('DOM loaded, initializing IDE...');
    console.log('MeadowsModule available:', typeof MeadowsModule !== 'undefined');
    
    if (typeof MeadowsModule === 'undefined') {
        console.error('MeadowsModule not found! Make sure meadows.js loaded correctly.');
        console.log('Available global variables:', Object.keys(window).filter(key => 
            key.toLowerCase().includes('meadows') || key.toLowerCase().includes('module')
        ));
    }
    
    try {
        window.meadowsIDE = new MeadowsIDE();
    } catch (error) {
        console.error('Failed to initialize IDE:', error);
        
        // Show a fallback error message
        const statusIndicator = document.getElementById('statusIndicator');
        if (statusIndicator) {
            const statusText = statusIndicator.querySelector('.status-text');
            const statusDot = statusIndicator.querySelector('.status-dot');
            if (statusText && statusDot) {
                statusDot.className = 'status-dot error';
                statusText.textContent = 'Initialization Failed';
            }
        }
        
        const runBtn = document.getElementById('runBtn');
        if (runBtn) {
            runBtn.disabled = true;
            runBtn.textContent = 'IDE Unavailable';
            runBtn.title = 'IDE failed to initialize. Check console for details.';
        }
    }
});
