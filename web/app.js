// Meadows Compiler Web IDE - JavaScript Application

class MeadowsIDE {
    constructor() {
        this.compiler = null;
        this.isLoading = false;
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
        this.initializeEditor();
        this.loadCompiler();
    }

    initializeElements() {
        this.elements = {
            // Buttons and controls
            runBtn: document.getElementById('runBtn'),
            clearBtn: document.getElementById('clearBtn'),
            clearOutputBtn: document.getElementById('clearOutputBtn'),
            exampleSelect: document.getElementById('exampleSelect'),
            
            // Editor
            codeEditor: document.getElementById('codeEditor'),
            editorGutter: document.getElementById('editorGutter'),
            lineCount: document.getElementById('lineCount'),
            
            // Output sections
            executionOutput: document.getElementById('executionOutput'),
            astOutput: document.getElementById('astOutput'),
            tokensOutput: document.getElementById('tokensOutput'),
            irOutput: document.getElementById('irOutput'),
            
            // Status and modals
            statusIndicator: document.getElementById('statusIndicator'),
            loadingModal: document.getElementById('loadingModal'),
            errorModal: document.getElementById('errorModal'),
            aboutModal: document.getElementById('aboutModal'),
            aboutBtn: document.getElementById('aboutBtn'),
            closeAboutModal: document.getElementById('closeAboutModal'),
            closeErrorModal: document.getElementById('closeErrorModal'),
            buildDate: document.getElementById('buildDate'),
            
            // Tab buttons
            tabButtons: document.querySelectorAll('.tab-btn'),
            outputSections: document.querySelectorAll('.output-section')
        };
    }

    setupEventListeners() {
        // Button events
        this.elements.runBtn.addEventListener('click', () => this.runCode());
        this.elements.clearBtn.addEventListener('click', () => this.clearEditor());
        this.elements.clearOutputBtn.addEventListener('click', () => this.clearOutput());
        
        // Example selection
        this.elements.exampleSelect.addEventListener('change', (e) => {
            if (e.target.value) {
                this.loadExample(e.target.value);
                e.target.value = '';
            }
        });
        
        // Editor events
        this.elements.codeEditor.addEventListener('input', () => this.updateEditor());
        this.elements.codeEditor.addEventListener('scroll', () => this.syncGutter());
        this.elements.codeEditor.addEventListener('keydown', () => {
            // Use setTimeout to ensure the change is processed first
            setTimeout(() => this.updateEditor(), 0);
        });
        this.elements.codeEditor.addEventListener('paste', () => {
            // Use setTimeout to ensure the paste is processed first
            setTimeout(() => this.updateEditor(), 0);
        });
        
        // Tab switching
        this.elements.tabButtons.forEach(btn => {
            btn.addEventListener('click', () => this.switchTab(btn.dataset.tab));
        });
        
        // Modal events
        this.elements.aboutBtn.addEventListener('click', () => this.showModal('about'));
        this.elements.closeAboutModal.addEventListener('click', () => this.hideModal('about'));
        this.elements.closeErrorModal.addEventListener('click', () => this.hideModal('error'));
        
        // Click outside modal to close
        this.elements.aboutModal.addEventListener('click', (e) => {
            if (e.target === this.elements.aboutModal) {
                this.hideModal('about');
            }
        });
        
        this.elements.errorModal.addEventListener('click', (e) => {
            if (e.target === this.elements.errorModal) {
                this.hideModal('error');
            }
        });
        
        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => this.handleKeyboard(e));
        
        // Prevent default drag/drop behavior
        document.addEventListener('dragover', (e) => e.preventDefault());
        document.addEventListener('drop', (e) => e.preventDefault());
    }

    initializeEditor() {
        // Set initial example first
        this.elements.codeEditor.value = this.examples.factorial;
        this.updateEditor();
        this.elements.buildDate.textContent = new Date().toLocaleDateString();
        
        // Ensure initial sync
        this.syncGutter();
    }

    async loadCompiler() {
        this.showModal('loading');
        this.updateStatus('loading', 'Loading compiler...');
        
        // Add a timeout to prevent infinite loading
        const loadingTimeout = setTimeout(() => {
            console.error('WebAssembly loading timeout');
            this.showCompilerError();
        }, 5000); // 5 second timeout
        
        try {
            // Try to initialize WebAssembly compiler
            await this.initializeWasmCompiler();
            clearTimeout(loadingTimeout);
        } catch (error) {
            clearTimeout(loadingTimeout);
            console.error('Failed to load compiler:', error);
            this.showCompilerError();
        }
    }
    
    showCompilerError() {
        this.hideModal('loading');
        this.showModal('error');
        this.updateStatus('error', 'Compiler not available');
        this.elements.runBtn.disabled = true;
    }

    async initializeWasmCompiler() {
        try {
            // Check if WebAssembly module is available
            if (typeof MeadowsModule === 'undefined') {
                // Wait a bit for the script to load
                await new Promise(resolve => setTimeout(resolve, 100));
                if (typeof MeadowsModule === 'undefined') {
                    throw new Error('MeadowsModule not available');
                }
            }
            
            // Wait for the WebAssembly module to load
            const module = await MeadowsModule();
            if (module && module.WebCompiler) {
                const wasmCompiler = new module.WebCompiler();
                this.compiler = {
                    compile: (source, options = {}) => {
                        try {
                            const resultJson = wasmCompiler.compile(source);
                            return JSON.parse(resultJson);
                        } catch (error) {
                            console.error('WASM compilation error:', error);
                            throw error;
                        }
                    }
                };
                
                this.hideModal('loading');
                this.updateStatus('ready', 'Ready (WebAssembly)');
                this.elements.runBtn.disabled = false;
                console.log('WebAssembly compiler initialized successfully');
                return;
            } else {
                throw new Error('WebCompiler not found in WASM module');
            }
        } catch (error) {
            throw error; // Re-throw to be caught by the caller
        }
    }

    async runCode() {
        if (this.isLoading) {
            return;
        }
        
        if (!this.compiler) {
            this.showCompilerError();
            return;
        }
        
        const source = this.elements.codeEditor.value.trim();
        if (!source) {
            this.showWarning('Please enter some code to run');
            return;
        }
        
        this.isLoading = true;
        this.elements.runBtn.disabled = true;
        this.updateStatus('loading', 'Compiling...');
        
        try {
            // Clear previous output
            this.clearOutput(false);
            
            // Simulate compilation time
            await new Promise(resolve => setTimeout(resolve, 500));
            
            const result = this.compiler.compile(source);
            
            if (result.success) {
                this.displayResults(result);
                this.updateStatus('ready', 'Compilation successful');
            } else {
                this.showError('Compilation failed', result.errors);
                this.updateStatus('error', 'Compilation failed');
            }
        } catch (error) {
            console.error('Compilation error:', error);
            this.showError('Compilation error: ' + error.message);
            this.updateStatus('error', 'Compilation error');
        } finally {
            this.isLoading = false;
            if (this.compiler) {
                this.elements.runBtn.disabled = false;
            }
        }
    }

    displayResults(result) {
        // Display execution output
        this.elements.executionOutput.innerHTML = result.execution.success
            ? `<div class="output-text output-success">${this.escapeHtml(result.execution.output)}</div>`
            : `<div class="output-error">Execution failed: ${this.escapeHtml(result.execution.error || 'Unknown error')}</div>`;
        
        // Display AST
        this.elements.astOutput.innerHTML = `<div class="output-text">${this.escapeHtml(result.ast)}</div>`;
        
        // Display tokens
        const tokensHtml = result.tokens.map(token => 
            `<div class="token-line">
                <span class="token-type">${token.type}</span>
                <span class="token-value">"${this.escapeHtml(token.value)}"</span>
                <span class="token-location">(${token.line}:${token.column})</span>
            </div>`
        ).join('');
        this.elements.tokensOutput.innerHTML = `<div class="tokens-container">${tokensHtml}</div>`;
        
        // Display LLVM IR
        this.elements.irOutput.innerHTML = `<div class="output-text">${this.escapeHtml(result.ir)}</div>`;
    }

    clearEditor() {
        this.elements.codeEditor.value = '';
        this.updateEditor();
        this.clearOutput();
        this.elements.codeEditor.focus();
    }

    clearOutput(updateStatus = true) {
        this.elements.executionOutput.innerHTML = '<div class="output-placeholder">Run your code to see the output here...</div>';
        this.elements.astOutput.innerHTML = '<div class="output-placeholder">Abstract Syntax Tree will appear here...</div>';
        this.elements.tokensOutput.innerHTML = '<div class="output-placeholder">Token analysis will appear here...</div>';
        this.elements.irOutput.innerHTML = '<div class="output-placeholder">LLVM IR code will appear here...</div>';
        
        if (updateStatus) {
            this.updateStatus('ready', 'Ready');
        }
    }

    loadExample(exampleName) {
        if (this.examples[exampleName]) {
            this.elements.codeEditor.value = this.examples[exampleName];
            this.updateEditor();
            this.clearOutput();
            // Ensure proper focus and cursor position
            this.elements.codeEditor.focus();
            this.elements.codeEditor.setSelectionRange(0, 0);
        }
    }

    updateEditor() {
        const code = this.elements.codeEditor.value;
        const lines = code.split('\n');
        
        // Update line count
        this.elements.lineCount.textContent = `${lines.length} line${lines.length !== 1 ? 's' : ''}`;
        
        // Update gutter
        this.updateGutter(lines.length);
        
        // Sync scroll positions
        this.syncGutter();
        
        // Auto-resize editor
        this.elements.codeEditor.style.height = 'auto';
        this.elements.codeEditor.style.height = this.elements.codeEditor.scrollHeight + 'px';
        
        // Update gutter height to match editor
        this.elements.editorGutter.style.height = this.elements.codeEditor.style.height;
    }

    updateGutter(lineCount) {
        let gutterContent = '';
        for (let i = 1; i <= lineCount; i++) {
            gutterContent += i;
            if (i < lineCount) {
                gutterContent += '\n';
            }
        }
        this.elements.editorGutter.textContent = gutterContent;
    }

    syncGutter() {
        // Sync scroll positions
        this.elements.editorGutter.scrollTop = this.elements.codeEditor.scrollTop;
        
        // Ensure heights match
        const editorHeight = this.elements.codeEditor.scrollHeight;
        this.elements.editorGutter.style.minHeight = editorHeight + 'px';
    }

    switchTab(tabName) {
        // Update tab buttons
        this.elements.tabButtons.forEach(btn => {
            btn.classList.toggle('active', btn.dataset.tab === tabName);
        });
        
        // Update output sections
        this.elements.outputSections.forEach(section => {
            section.classList.toggle('active', section.id === tabName + 'Output');
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
        const errorHtml = `
            <div class="output-error">
                <strong>Error:</strong> ${this.escapeHtml(message)}
                ${details.length ? '<br><br><strong>Details:</strong><br>' + details.map(d => this.escapeHtml(d)).join('<br>') : ''}
            </div>
        `;
        this.elements.executionOutput.innerHTML = errorHtml;
        this.switchTab('execution');
    }

    showWarning(message) {
        const warningHtml = `
            <div class="output-warning">
                <strong>Warning:</strong> ${this.escapeHtml(message)}
            </div>
        `;
        this.elements.executionOutput.innerHTML = warningHtml;
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

    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
}

// Initialize the IDE when the page loads
document.addEventListener('DOMContentLoaded', () => {
    window.meadowsIDE = new MeadowsIDE();
});

// Add some additional CSS for tokens display
const tokenStyles = `
<style>
.tokens-container {
    font-family: var(--font-mono);
    font-size: 0.875rem;
}

.token-line {
    display: flex;
    gap: 1rem;
    padding: 0.25rem 0;
    border-bottom: 1px solid var(--border-light);
}

.token-type {
    color: var(--primary-color);
    font-weight: 600;
    min-width: 100px;
}

.token-value {
    color: var(--text-primary);
    flex: 1;
}

.token-location {
    color: var(--text-muted);
    font-size: 0.75rem;
    min-width: 60px;
}
</style>
`;

document.head.insertAdjacentHTML('beforeend', tokenStyles);
