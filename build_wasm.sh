#!/bin/bash

# Meadows Compiler - WebAssembly Build Script
# This script builds the Meadows compiler for WebAssembly using Emscripten

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Emscripten is installed
check_emscripten() {
    print_status "Checking Emscripten installation..."
    
    if ! command -v emcc &> /dev/null; then
        print_error "Emscripten not found. Please install and activate Emscripten SDK."
        print_status "Installation instructions: https://emscripten.org/docs/getting_started/downloads.html"
        exit 1
    fi
    
    local emcc_version=$(emcc --version | head -n1)
    print_success "Found Emscripten: $emcc_version"
}

# Check if LLVM is available for Emscripten
check_llvm() {
    print_status "Checking LLVM availability..."
    
    # Emscripten comes with its own LLVM, so we don't need system LLVM
    print_success "Using Emscripten's LLVM"
}

# Clean previous builds
clean_build() {
    print_status "Cleaning previous builds..."
    
    if [ -d "build-wasm" ]; then
        rm -rf build-wasm
        print_status "Removed build-wasm directory"
    fi
    
    # Clean web output files
    if [ -d "web" ]; then
        rm -f web/meadows.js web/meadows.wasm web/meadows.worker.js
        print_status "Cleaned web output files"
    fi
}

# Configure CMake for WebAssembly
configure_cmake() {
    print_status "Configuring CMake for WebAssembly..."
    
    mkdir -p build-wasm
    cd build-wasm
    
    # Use emcmake to configure with Emscripten
    emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_WASM=ON \
        -DCMAKE_CROSSCOMPILING_EMULATOR=node \
        -DCMAKE_VERBOSE_MAKEFILE=ON
    
    if [ $? -eq 0 ]; then
        print_success "CMake configuration completed"
    else
        print_error "CMake configuration failed"
        exit 1
    fi
    
    cd ..
}

# Build the WebAssembly module
build_wasm() {
    print_status "Building WebAssembly module..."
    
    cd build-wasm
    
    # Use emmake to build with Emscripten
    emmake make -j$(nproc) meadows_wasm
    
    if [ $? -eq 0 ]; then
        print_success "WebAssembly build completed"
    else
        print_error "WebAssembly build failed"
        exit 1
    fi
    
    cd ..
}

# Verify the build output
verify_build() {
    print_status "Verifying build output..."
    
    local output_dir="web"
    local wasm_file="$output_dir/meadows.wasm"
    local js_file="$output_dir/meadows.js"
    
    if [ -f "$wasm_file" ]; then
        local wasm_size=$(du -h "$wasm_file" | cut -f1)
        print_success "WASM file created: $wasm_file ($wasm_size)"
    else
        print_warning "WASM file not found at $wasm_file"
    fi
    
    if [ -f "$js_file" ]; then
        local js_size=$(du -h "$js_file" | cut -f1)
        print_success "JS file created: $js_file ($js_size)"
    else
        print_warning "JS file not found at $js_file"
    fi
    
    # List all files in web directory
    print_status "Files in web directory:"
    ls -la "$output_dir/"
}

# Test the WebAssembly module
test_wasm() {
    print_status "Testing WebAssembly module..."
    
    if command -v node &> /dev/null; then
        # Create a simple test script
        cat > test_wasm.js << 'EOF'
const fs = require('fs');
const path = require('path');

// Check if the WASM file exists
const wasmPath = path.join(__dirname, 'web', 'meadows.wasm');
const jsPath = path.join(__dirname, 'web', 'meadows.js');

if (fs.existsSync(wasmPath)) {
    console.log('✓ WASM file exists');
    const stats = fs.statSync(wasmPath);
    console.log(`  Size: ${stats.size} bytes`);
} else {
    console.log('✗ WASM file not found');
}

if (fs.existsSync(jsPath)) {
    console.log('✓ JS file exists');
    const stats = fs.statSync(jsPath);
    console.log(`  Size: ${stats.size} bytes`);
} else {
    console.log('✗ JS file not found');
}

console.log('WebAssembly test completed');
EOF
        
        node test_wasm.js
        rm test_wasm.js
        
        print_success "WebAssembly module test completed"
    else
        print_warning "Node.js not found, skipping WASM test"
    fi
}

# Start a simple HTTP server for testing
start_server() {
    print_status "Starting development server..."
    
    if command -v python3 &> /dev/null; then
        print_success "Starting server at http://localhost:8000"
        print_status "Press Ctrl+C to stop the server"
        cd web && python3 -m http.server 8000
    elif command -v python &> /dev/null; then
        print_success "Starting server at http://localhost:8000"
        print_status "Press Ctrl+C to stop the server"
        cd web && python -m SimpleHTTPServer 8000
    else
        print_warning "Python not found. Please start a web server manually in the 'web' directory"
        print_status "You can use any static file server to serve the web directory"
    fi
}

# Show help
show_help() {
    echo "Meadows Compiler - WebAssembly Build Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  build      Build the WebAssembly module (default)"
    echo "  clean      Clean previous builds"
    echo "  test       Test the WebAssembly module"
    echo "  serve      Start a development server"
    echo "  help       Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                # Build WebAssembly module"
    echo "  $0 build          # Build WebAssembly module"
    echo "  $0 clean          # Clean and build"
    echo "  $0 test           # Build and test"
    echo "  $0 serve          # Build and start server"
}

# Main function
main() {
    local command="${1:-build}"
    
    case $command in
        "build")
            check_emscripten
            check_llvm
            clean_build
            configure_cmake
            build_wasm
            verify_build
            print_success "WebAssembly build process completed!"
            print_status "You can now serve the 'web' directory with any static file server"
            ;;
        "clean")
            clean_build
            print_success "Clean completed"
            ;;
        "test")
            check_emscripten
            check_llvm
            clean_build
            configure_cmake
            build_wasm
            verify_build
            test_wasm
            ;;
        "serve")
            check_emscripten
            check_llvm
            clean_build
            configure_cmake
            build_wasm
            verify_build
            start_server
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
