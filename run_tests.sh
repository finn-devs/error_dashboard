#!/bin/bash
# Test runner script for error-surface-qt

set -e

PROJECT_DIR="$HOME/error-surface-qt"
TEST_DIR="$PROJECT_DIR/tests"
BUILD_DIR="$PROJECT_DIR/build-tests"

echo "================================="
echo "Error Surface Qt - Test Suite"
echo "================================="
echo ""

# Create test directory structure
mkdir -p "$TEST_DIR"
cd "$PROJECT_DIR"

# Copy test files if they don't exist
if [ ! -f "$TEST_DIR/test_errorsurface.cpp" ]; then
    echo "Setting up test files..."
    cp test_errorsurface.cpp "$TEST_DIR/" 2>/dev/null || true
    cp CMakeLists_tests.txt "$TEST_DIR/CMakeLists.txt" 2>/dev/null || true
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring tests..."
cmake -DCMAKE_BUILD_TYPE=Debug "$TEST_DIR"

echo ""
echo "Building tests..."
make -j$(nproc)

echo ""
echo "================================="
echo "Running Test Suite..."
echo "================================="
echo ""

# Run tests with verbose output
./test_errorsurface -v2

TEST_RESULT=$?

echo ""
echo "================================="
if [ $TEST_RESULT -eq 0 ]; then
    echo "✓ All tests PASSED"
else
    echo "✗ Some tests FAILED"
fi
echo "================================="

exit $TEST_RESULT
