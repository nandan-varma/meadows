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
        
        this.initializeElements();
        this.setupEventListeners();
        this.initializeMonaco();
        this.loadCompiler();
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
            if (typeof MeadowsModule !== 'undefined') {
                // Initialize the WebAssembly module
                const module = await MeadowsModule();
                
                // Check if the module has the WebCompiler class
                if (module && module.WebCompiler) {
                    const webCompiler = new module.WebCompiler();
                    this.compiler = {
                        compile: (source) => {
                            try {
                                const resultJson = webCompiler.compile(source);
                                return JSON.parse(resultJson);
                            } catch (error) {
                                console.error('WASM compilation error:', error);
                                throw error;
                            }
                        }
                    };
                    
                    this.hideModal('loading');
                    this.updateStatus('success', 'Ready (WebAssembly)');
                    console.log('WebAssembly compiler initialized successfully');
                    return;
                } else {
                    throw new Error('WebCompiler not found in WASM module');
                }
            } else {
                throw new Error('MeadowsModule not available');
            }
        } catch (error) {
            console.error('Failed to load compiler:', error);
            this.hideModal('loading');
            this.showModal('error');
            this.updateStatus('error', 'Compiler Not Available');
            this.elements.runBtn.disabled = true;
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
            this.showError('Compiler not available. Please ensure WebAssembly module is loaded.');
            return;
        }

        this.updateStatus('loading', 'Compiling...');
        this.elements.runBtn.disabled = true;

        try {
            // Clear previous outputs
            this.clearOutput();
            
            // Compile and execute using the WebAssembly compiler only
            const result = this.compiler.compile(code);

            // Display results
            if (result.success) {
                this.showOutput('execution', result.output || 'Program executed successfully');
                if (result.ast) this.showOutput('ast', JSON.stringify(result.ast, null, 2));
                if (result.tokens) this.showOutput('tokens', JSON.stringify(result.tokens, null, 2));
                if (result.ir) this.showOutput('ir', result.ir);
                
                this.updateStatus('success', 'Execution Complete');
            } else {
                this.showError(result.error || 'Compilation failed', result.details || []);
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
        const modal = this.elements[modalName + 'Modal'];
        if (modal) {
            modal.classList.add('active');
            document.body.style.overflow = 'hidden';
        }
    }

    hideModal(modalName) {
        const modal = this.elements[modalName + 'Modal'];
        if (modal) {
            modal.classList.remove('active');
            document.body.style.overflow = '';
        }
    }

    updateStatus(type, message) {
        const statusDot = this.elements.statusIndicator.querySelector('.status-dot');
        const statusText = this.elements.statusIndicator.querySelector('.status-text');
        
        statusDot.className = `status-dot ${type}`;
        statusText.textContent = message;
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
document.addEventListener('DOMContentLoaded', () => {
    window.meadowsIDE = new MeadowsIDE();
});
