#!/bin/bash

# Meadows Compiler Release Helper Script
# This script helps create a local release build for testing

set -e

VERSION=${1:-"dev-$(date +%Y%m%d)"}
BUILD_DIR="build-release"
RELEASE_DIR="release-local"

echo "Creating local release build v${VERSION}..."

# Clean up previous builds
rm -rf "${BUILD_DIR}" "${RELEASE_DIR}"

# Create release build
echo "Configuring CMake..."
cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build "${BUILD_DIR}" --parallel

echo "Running tests..."
./"${BUILD_DIR}"/meadows_test -m

# Create release directory structure
mkdir -p "${RELEASE_DIR}"

echo "Preparing release artifacts..."

# Copy binaries
cp "${BUILD_DIR}/meadows" "${RELEASE_DIR}/"
cp "${BUILD_DIR}/meadows_test" "${RELEASE_DIR}/"

# Copy documentation and scripts
cp README.md "${RELEASE_DIR}/"
cp run_tests.sh "${RELEASE_DIR}/"
cp format.sh "${RELEASE_DIR}/"

# Create install script
cat > "${RELEASE_DIR}/install.sh" << 'EOF'
#!/bin/bash
# Meadows Compiler Installation Script

INSTALL_DIR="${HOME}/.local/bin"

echo "Installing Meadows compiler to ${INSTALL_DIR}..."

# Create install directory if it doesn't exist
mkdir -p "${INSTALL_DIR}"

# Copy binaries
cp meadows "${INSTALL_DIR}/"
cp meadows_test "${INSTALL_DIR}/"

# Make sure binaries are executable
chmod +x "${INSTALL_DIR}/meadows"
chmod +x "${INSTALL_DIR}/meadows_test"

echo "Installation complete!"
echo "Make sure ${INSTALL_DIR} is in your PATH:"
echo "  export PATH=\"${INSTALL_DIR}:\$PATH\""
echo ""
echo "You can now run:"
echo "  meadows --help"
echo "  meadows_test -m"
EOF

chmod +x "${RELEASE_DIR}/install.sh"

# Create version info file
cat > "${RELEASE_DIR}/VERSION" << EOF
Meadows Compiler ${VERSION}
Built on: $(date)
Platform: $(uname -s)-$(uname -m)
Commit: $(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
EOF

# Create archive
ARCHIVE_NAME="meadows-${VERSION}-$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m)"

if command -v tar &> /dev/null; then
    echo "Creating tar.gz archive..."
    tar -czf "${ARCHIVE_NAME}.tar.gz" -C "${RELEASE_DIR}" .
    echo "Created: ${ARCHIVE_NAME}.tar.gz"
fi

if command -v zip &> /dev/null; then
    echo "Creating zip archive..."
    (cd "${RELEASE_DIR}" && zip -r "../${ARCHIVE_NAME}.zip" .)
    echo "Created: ${ARCHIVE_NAME}.zip"
fi

echo ""
echo "Release build complete!"
echo "Files created:"
echo "  Directory: ${RELEASE_DIR}/"
ls -la "${RELEASE_DIR}/"
echo ""
echo "Archives:"
ls -la "${ARCHIVE_NAME}".* 2>/dev/null || echo "  No archives created"

echo ""
echo "To test the release:"
echo "  cd ${RELEASE_DIR}"
echo "  ./meadows --help"
echo "  ./meadows_test -m"
echo ""
echo "To install locally:"
echo "  cd ${RELEASE_DIR}"
echo "  ./install.sh"
