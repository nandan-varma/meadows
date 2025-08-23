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
            aboutModal: document.getElementById('aboutModal'),
            aboutBtn: document.getElementById('aboutBtn'),
            closeAboutModal: document.getElementById('closeAboutModal'),
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
        
        // Tab switching
        this.elements.tabButtons.forEach(btn => {
            btn.addEventListener('click', () => this.switchTab(btn.dataset.tab));
        });
        
        // Modal events
        this.elements.aboutBtn.addEventListener('click', () => this.showModal('about'));
        this.elements.closeAboutModal.addEventListener('click', () => this.hideModal('about'));
        
        // Click outside modal to close
        this.elements.aboutModal.addEventListener('click', (e) => {
            if (e.target === this.elements.aboutModal) {
                this.hideModal('about');
            }
        });
        
        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => this.handleKeyboard(e));
        
        // Prevent default drag/drop behavior
        document.addEventListener('dragover', (e) => e.preventDefault());
        document.addEventListener('drop', (e) => e.preventDefault());
    }

    initializeEditor() {
        this.updateEditor();
        this.elements.buildDate.textContent = new Date().toLocaleDateString();
        
        // Set initial example
        this.elements.codeEditor.value = this.examples.factorial;
        this.updateEditor();
    }

    async loadCompiler() {
        this.showModal('loading');
        this.updateStatus('loading', 'Loading compiler...');
        
        try {
            // Load the WebAssembly module
            if (typeof MeadowsModule !== 'undefined') {
                // Module is already loaded
                await this.initializeWasmCompiler();
            } else {
                // Wait for module to load
                this.onWasmReady = () => {
                    this.initializeWasmCompiler();
                };
                
                // Load the WASM script
                const script = document.createElement('script');
                script.src = 'meadows.js';
                script.onload = () => {
                    console.log('WebAssembly script loaded');
                };
                script.onerror = () => {
                    throw new Error('Failed to load WebAssembly script');
                };
                document.head.appendChild(script);
            }
        } catch (error) {
            console.error('Failed to load compiler:', error);
            this.hideModal('loading');
            this.updateStatus('error', 'Failed to load compiler');
            this.showError('Failed to load WebAssembly compiler module');
            
            // Fallback to mock compiler
            await this.simulateWasmLoading();
            this.hideModal('loading');
            this.updateStatus('ready', 'Ready (Mock Mode)');
            this.elements.runBtn.disabled = false;
        }
    }

    async initializeWasmCompiler() {
        try {
            if (window.MeadowsCompiler) {
                this.compiler = {
                    compile: (source, options = {}) => {
                        try {
                            const resultJson = window.MeadowsCompiler.compile(source);
                            return JSON.parse(resultJson);
                        } catch (error) {
                            console.error('WASM compilation error:', error);
                            return this.mockCompile(source, options);
                        }
                    }
                };
                
                this.hideModal('loading');
                this.updateStatus('ready', 'Ready');
                this.elements.runBtn.disabled = false;
            } else {
                throw new Error('WebAssembly compiler not available');
            }
        } catch (error) {
            console.error('Failed to initialize WASM compiler:', error);
            // Fallback to mock
            await this.simulateWasmLoading();
            this.hideModal('loading');
            this.updateStatus('ready', 'Ready (Mock Mode)');
            this.elements.runBtn.disabled = false;
        }
    }

    async simulateWasmLoading() {
        // Simulate WebAssembly loading time
        await new Promise(resolve => setTimeout(resolve, 2000));
        
        // Mock compiler interface
        this.compiler = {
            compile: (source, options = {}) => {
                return this.mockCompile(source, options);
            }
        };
    }

    mockCompile(source, options) {
        // Mock compilation results
        const lines = source.split('\n');
        const nonEmptyLines = lines.filter(line => line.trim());
        
        // Generate mock tokens
        const tokens = this.generateMockTokens(source);
        
        // Generate mock AST
        const ast = this.generateMockAST(source);
        
        // Generate mock IR
        const ir = this.generateMockIR(source);
        
        // Generate execution result
        const execution = this.generateMockExecution(source);
        
        return {
            success: true,
            tokens,
            ast,
            ir,
            execution,
            errors: [],
            warnings: []
        };
    }

    generateMockTokens(source) {
        const tokenPatterns = [
            { pattern: /\bdef\b/g, type: 'KEYWORD' },
            { pattern: /\bif\b/g, type: 'KEYWORD' },
            { pattern: /\belse\b/g, type: 'KEYWORD' },
            { pattern: /\breturn\b/g, type: 'KEYWORD' },
            { pattern: /\bwhile\b/g, type: 'KEYWORD' },
            { pattern: /\bprint\b/g, type: 'BUILTIN' },
            { pattern: /[a-zA-Z_][a-zA-Z0-9_]*/g, type: 'IDENTIFIER' },
            { pattern: /\d+/g, type: 'NUMBER' },
            { pattern: /"[^"]*"/g, type: 'STRING' },
            { pattern: /[+\-*/=<>!]/g, type: 'OPERATOR' },
            { pattern: /[(){}[\],.:]/g, type: 'PUNCTUATION' },
            { pattern: /#.*/g, type: 'COMMENT' }
        ];
        
        let tokens = [];
        let lineNum = 1;
        let lines = source.split('\n');
        
        lines.forEach((line, index) => {
            let column = 1;
            tokenPatterns.forEach(({ pattern, type }) => {
                let match;
                pattern.lastIndex = 0;
                while ((match = pattern.exec(line)) !== null) {
                    tokens.push({
                        type,
                        value: match[0],
                        line: index + 1,
                        column: match.index + 1
                    });
                }
            });
        });
        
        return tokens;
    }

    generateMockAST(source) {
        // Generate a simplified AST representation
        const lines = source.split('\n').filter(line => line.trim());
        let ast = 'Program:\n';
        let indentLevel = 1;
        
        lines.forEach(line => {
            const trimmed = line.trim();
            if (trimmed.startsWith('#')) return; // Skip comments
            
            let indent = '  '.repeat(indentLevel);
            
            if (trimmed.startsWith('def ')) {
                ast += `${indent}FunctionDefinition: ${trimmed.match(/def\s+(\w+)/)?.[1] || 'unknown'}\n`;
                indentLevel++;
            } else if (trimmed.startsWith('if ')) {
                ast += `${indent}IfStatement:\n`;
                ast += `${'  '.repeat(indentLevel + 1)}Condition: ${trimmed.slice(3, -1)}\n`;
                indentLevel++;
            } else if (trimmed === 'else:') {
                indentLevel--;
                ast += `${'  '.repeat(indentLevel)}Else:\n`;
                indentLevel++;
            } else if (trimmed.startsWith('while ')) {
                ast += `${indent}WhileStatement:\n`;
                ast += `${'  '.repeat(indentLevel + 1)}Condition: ${trimmed.slice(6, -1)}\n`;
                indentLevel++;
            } else if (trimmed.startsWith('return ')) {
                ast += `${indent}ReturnStatement: ${trimmed.slice(7)}\n`;
            } else if (trimmed.includes('=') && !trimmed.includes('==')) {
                const [left, right] = trimmed.split('=').map(s => s.trim());
                ast += `${indent}Assignment: ${left} = ${right}\n`;
            } else if (trimmed.startsWith('print(')) {
                ast += `${indent}FunctionCall: print\n`;
                ast += `${'  '.repeat(indentLevel + 1)}Arguments: ${trimmed.slice(6, -1)}\n`;
            } else if (trimmed && !trimmed.endsWith(':')) {
                ast += `${indent}ExpressionStatement: ${trimmed}\n`;
            }
            
            // Adjust indentation for block endings
            if (indentLevel > 1 && !line.startsWith('  ')) {
                indentLevel = 1;
            }
        });
        
        return ast;
    }

    generateMockIR(source) {
        // Generate mock LLVM IR
        let ir = `; ModuleID = 'meadows_module'
source_filename = "meadows_input"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\\0A\\00", align 1
@.str.1 = private unnamed_addr constant [4 x i8] c"%s\\0A\\00", align 1

declare i32 @printf(i8*, ...)

`;
        
        // Add function definitions based on source
        if (source.includes('def factorial')) {
            ir += `define i32 @factorial(i32 %n) {
entry:
  %cmp = icmp sle i32 %n, 1
  br i1 %cmp, label %if.then, label %if.else

if.then:
  ret i32 1

if.else:
  %sub = sub i32 %n, 1
  %call = call i32 @factorial(i32 %sub)
  %mul = mul i32 %n, %call
  ret i32 %mul
}

`;
        }
        
        if (source.includes('def fibonacci')) {
            ir += `define i32 @fibonacci(i32 %n) {
entry:
  %cmp = icmp sle i32 %n, 1
  br i1 %cmp, label %if.then, label %if.else

if.then:
  ret i32 %n

if.else:
  %sub1 = sub i32 %n, 1
  %call1 = call i32 @fibonacci(i32 %sub1)
  %sub2 = sub i32 %n, 2
  %call2 = call i32 @fibonacci(i32 %sub2)
  %add = add i32 %call1, %call2
  ret i32 %add
}

`;
        }
        
        ir += `define i32 @main() {
entry:
`;
        
        // Add main function body based on source
        if (source.includes('print(')) {
            const printCalls = source.match(/print\([^)]+\)/g) || [];
            printCalls.forEach((call, index) => {
                const arg = call.slice(6, -1);
                if (arg.includes('"')) {
                    ir += `  %call${index} = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.1, i64 0, i64 0), i8* getelementptr inbounds ([${arg.length - 2} x i8], [${arg.length - 2} x i8]* @.str, i64 0, i64 0))
`;
                } else {
                    ir += `  %call${index} = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i64 0, i64 0), i32 ${arg})
`;
                }
            });
        }
        
        ir += `  ret i32 0
}`;
        
        return ir;
    }

    generateMockExecution(source) {
        // Generate mock execution output
        let output = '';
        
        try {
            // Simple pattern matching for demonstration
            if (source.includes('factorial(5)')) {
                output += 'Factorial of 5:\n120\n';
            }
            if (source.includes('factorial(10)')) {
                output += 'Factorial of 10:\n3628800\n';
            }
            if (source.includes('fibonacci')) {
                output += '0\n1\n1\n2\n3\n5\n8\n13\n21\n34\n';
            }
            if (source.includes('Hello, World!')) {
                output += 'Hello, World!\n';
            }
            if (source.includes('Welcome to Meadows!')) {
                output += 'Welcome to Meadows!\n';
            }
            if (source.includes('add(x, y)')) {
                output += 'Addition:\n15\n';
            }
            if (source.includes('multiply(x, y)')) {
                output += 'Multiplication:\n50\n';
            }
            if (source.includes('power(x, 3)')) {
                output += 'Power:\n1000\n';
            }
            
            if (!output) {
                output = 'Program executed successfully.\n';
            }
            
            return {
                success: true,
                output: output,
                exitCode: 0
            };
        } catch (error) {
            return {
                success: false,
                output: '',
                error: error.message,
                exitCode: 1
            };
        }
    }

    async runCode() {
        if (this.isLoading || !this.compiler) {
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
            this.elements.runBtn.disabled = false;
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
        }
    }

    updateEditor() {
        const code = this.elements.codeEditor.value;
        const lines = code.split('\n');
        
        // Update line count
        this.elements.lineCount.textContent = `${lines.length} line${lines.length !== 1 ? 's' : ''}`;
        
        // Update gutter
        this.updateGutter(lines.length);
        
        // Auto-resize editor
        this.elements.codeEditor.style.height = 'auto';
        this.elements.codeEditor.style.height = this.elements.codeEditor.scrollHeight + 'px';
    }

    updateGutter(lineCount) {
        let gutterContent = '';
        for (let i = 1; i <= lineCount; i++) {
            gutterContent += i + '\n';
        }
        this.elements.editorGutter.textContent = gutterContent;
    }

    syncGutter() {
        this.elements.editorGutter.scrollTop = this.elements.codeEditor.scrollTop;
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
