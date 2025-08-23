// Pre-load script for Meadows WebAssembly module
// This script runs before the main module is loaded

// Module configuration
var Module = Module || {};

// Set up module defaults
Module.preRun = Module.preRun || [];
Module.postRun = Module.postRun || [];

// Configure memory and performance
Module.TOTAL_MEMORY = Module.TOTAL_MEMORY || 16777216; // 16MB
Module.ALLOW_MEMORY_GROWTH = true;

// Set up error handling
Module.onRuntimeInitialized = function() {
    console.log('Meadows WebAssembly module initialized');
    
    // Create global compiler instance
    if (typeof window !== 'undefined') {
        window.MeadowsCompiler = new Module.WebCompiler();
        
        // Notify the main application
        if (window.meadowsIDE && window.meadowsIDE.onWasmReady) {
            window.meadowsIDE.onWasmReady();
        }
    }
};

// Error handling
Module.onAbort = function(what) {
    console.error('Meadows WebAssembly module aborted:', what);
};

// Print function override for capturing output
Module.print = function(text) {
    console.log('WASM Output:', text);
};

Module.printErr = function(text) {
    console.error('WASM Error:', text);
};
