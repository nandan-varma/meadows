#!/bin/bash
# Code formatting script using clang-format

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Parse arguments
CHECK_MODE=0
SILENT=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --check)
            CHECK_MODE=1
            shift
            ;;
        --silent)
            SILENT=1
            shift
            ;;
        *)
            shift
            ;;
    esac
done

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    if [ $SILENT -eq 0 ]; then
        echo -e "${YELLOW}Warning: clang-format not found. Install with: brew install clang-format${NC}"
    fi
    exit 0
fi

# Find all source files
SOURCE_FILES=$(find "$PROJECT_ROOT/src" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null)
TEST_FILES=$(find "$PROJECT_ROOT/tests/unit" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null)

ALL_FILES="$SOURCE_FILES $TEST_FILES"

if [ -z "$ALL_FILES" ]; then
    if [ $SILENT -eq 0 ]; then
        echo -e "${YELLOW}No source files found${NC}"
    fi
    exit 0
fi

FILE_COUNT=$(echo "$ALL_FILES" | wc -w)

if [ $CHECK_MODE -eq 1 ]; then
    # Check mode - report files that need formatting
    NEEDS_FORMAT=0
    for file in $ALL_FILES; do
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            if [ $SILENT -eq 0 ]; then
                echo -e "${RED}Would format:${NC} $(basename "$file")"
            fi
            NEEDS_FORMAT=1
        fi
    done

    if [ $NEEDS_FORMAT -eq 1 ]; then
        if [ $SILENT -eq 0 ]; then
            echo
            echo -e "${RED}Some files need formatting. Run: ./scripts/dev/format.sh${NC}"
        fi
        exit 1
    else
        if [ $SILENT -eq 0 ]; then
            echo -e "${GREEN}All files are properly formatted!${NC}"
        fi
        exit 0
    fi
else
    # Format mode - actually format the files
    if [ $SILENT -eq 0 ]; then
        echo -e "${BLUE}========================================${NC}"
        echo -e "${BLUE}   Meadows Code Formatter${NC}"
        echo -e "${BLUE}========================================${NC}"
        echo
        echo -e "${YELLOW}Found $FILE_COUNT files to format${NC}"
        echo
        echo -e "${YELLOW}Formatting files...${NC}"
    fi

    for file in $ALL_FILES; do
        clang-format -i "$file" 2>/dev/null
        if [ $SILENT -eq 0 ]; then
            echo "  $(basename "$file")"
        fi
    done

    if [ $SILENT -eq 0 ]; then
        echo
        echo -e "${GREEN}Formatting complete!${NC}"
    fi
    exit 0
fi
