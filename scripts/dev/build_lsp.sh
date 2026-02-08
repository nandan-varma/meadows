#!/bin/bash
# Build Meadows LSP and VS Code Extension
# Creates: lsp/meadows-vscode/meadows-vscode.vsix

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LSP_DIR="$PROJECT_ROOT/lsp"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Meadows LSP Build${NC}"
echo -e "${BLUE}========================================${NC}"
echo

# Check prerequisites
echo -e "${YELLOW}Checking prerequisites...${NC}"

# Check Node.js
if ! command -v node &> /dev/null; then
    echo -e "${RED}Error: Node.js is required but not installed.${NC}"
    exit 1
fi

# Check npm
if ! command -v npm &> /dev/null; then
    echo -e "${RED}Error: npm is required but not installed.${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Node.js and npm found${NC}"
echo

# Step 1: Build compiler
echo -e "${YELLOW}Step 1: Building Meadows compiler...${NC}"
cd "$PROJECT_ROOT"
if [ ! -f "build.sh" ]; then
    echo -e "${RED}Error: build.sh not found in project root${NC}"
    exit 1
fi

./build.sh release

if [ ! -f "build-release/meadows" ]; then
    echo -e "${RED}Error: Compiler build failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Compiler built successfully${NC}"
echo

# Add build directory to PATH for LSP server
export PATH="$PROJECT_ROOT/build-release:$PATH"

# Step 2: Build LSP Server
echo -e "${YELLOW}Step 2: Building LSP server...${NC}"
cd "$LSP_DIR/meadows-lsp"

if [ ! -f "package.json" ]; then
    echo -e "${RED}Error: package.json not found in meadows-lsp${NC}"
    exit 1
fi

echo "  Installing dependencies..."
npm install

echo "  Compiling TypeScript..."
npm run build

if [ ! -f "dist/server.js" ]; then
    echo -e "${RED}Error: LSP server build failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ LSP server built successfully${NC}"
echo

# Step 3: Build VS Code Extension
echo -e "${YELLOW}Step 3: Building VS Code extension...${NC}"
cd "$LSP_DIR/meadows-vscode"

if [ ! -f "package.json" ]; then
    echo -e "${RED}Error: package.json not found in meadows-vscode${NC}"
    exit 1
fi

echo "  Installing dependencies..."
npm install

echo "  Compiling TypeScript..."
npm run build

if [ ! -f "dist/extension.js" ]; then
    echo -e "${RED}Error: VS Code extension build failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ VS Code extension built successfully${NC}"
echo

# Step 4: Bundle extension
echo -e "${YELLOW}Step 4: Bundling extension...${NC}"

# Check if vsce is installed
if ! command -v vsce &> /dev/null; then
    echo "  Installing vsce..."
    npm install -g vsce
fi

# Create the .vsix package
echo "  Creating .vsix package..."
vsce package --out meadows-vscode.vsix --no-dependencies

if [ ! -f "meadows-vscode.vsix" ]; then
    echo -e "${RED}Error: Extension bundling failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Extension bundled successfully${NC}"
echo

# Summary
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}   Build Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo
echo "Extension bundle: lsp/meadows-vscode/meadows-vscode.vsix"
echo
echo "To install in VS Code:"
echo "  code --install-extension lsp/meadows-vscode/meadows-vscode.vsix"
echo
echo "Or manually:"
echo "  1. Open VS Code"
echo "  2. Go to Extensions view (Ctrl+Shift+X)"
echo "  3. Click '...' menu → 'Install from VSIX...'"
echo "  4. Select: $(cd "$LSP_DIR/meadows-vscode" && pwd)/meadows-vscode.vsix"
echo
