#!/bin/bash
# Clean build artifacts for Meadows Compiler

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Cleaning build artifacts...${NC}"

# Remove build directories
if [ -d "$PROJECT_ROOT/build" ]; then
    echo "  Removing build/"
    rm -rf "$PROJECT_ROOT/build"
fi

if [ -d "$PROJECT_ROOT/build-debug" ]; then
    echo "  Removing build-debug/"
    rm -rf "$PROJECT_ROOT/build-debug"
fi

if [ -d "$PROJECT_ROOT/build-release" ]; then
    echo "  Removing build-release/"
    rm -rf "$PROJECT_ROOT/build-release"
fi

# Remove generated test files
echo "  Removing generated test files..."
find "$PROJECT_ROOT" -name "*.ms.ll" -type f -delete 2>/dev/null || true
find "$PROJECT_ROOT" -name "*.ms.out" -type f -delete 2>/dev/null || true

# Remove compile_commands.json if exists
if [ -f "$PROJECT_ROOT/compile_commands.json" ]; then
    echo "  Removing compile_commands.json"
    rm -f "$PROJECT_ROOT/compile_commands.json"
fi

echo -e "${GREEN}✓ Clean complete${NC}"
