// Debug script to help troubleshoot loading issues
console.log('=== Meadows Web IDE Debug Info ===');
console.log('Page loaded at:', new Date().toISOString());
console.log('User agent:', navigator.userAgent);
console.log('WebAssembly support:', 'WebAssembly' in window);

// Check for required global objects
setTimeout(() => {
    console.log('--- Script Loading Check ---');
    console.log('Monaco Editor:', typeof monaco !== 'undefined' ? 'Loaded' : 'Missing');
    console.log('MeadowsModule:', typeof MeadowsModule !== 'undefined' ? 'Loaded' : 'Missing');
    
    if (typeof MeadowsModule !== 'undefined') {
        console.log('MeadowsModule type:', typeof MeadowsModule);
        console.log('MeadowsModule:', MeadowsModule);
    }
    
    // List all available global variables that might be related
    const potentialModules = Object.keys(window).filter(key => 
        key.toLowerCase().includes('meadows') || 
        key.toLowerCase().includes('module') ||
        key.toLowerCase().includes('wasm')
    );
    console.log('Potential module globals:', potentialModules);
    
    console.log('=== End Debug Info ===');
}, 1000);

// Error handling for uncaught errors
window.addEventListener('error', (event) => {
    console.error('Global error caught:', event.error);
    console.error('Error details:', {
        message: event.message,
        filename: event.filename,
        lineno: event.lineno,
        colno: event.colno
    });
});

// Handle unhandled promise rejections
window.addEventListener('unhandledrejection', (event) => {
    console.error('Unhandled promise rejection:', event.reason);
});
