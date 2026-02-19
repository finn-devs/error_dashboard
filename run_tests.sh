#!/bin/bash
# Test runner script for error-dashboard-qt

set -e

PROJECT_DIR="$HOME/error-dashboard-qt"
TEST_DIR="$PROJECT_DIR/tests"
BUILD_DIR="$PROJECT_DIR/build-tests"

echo "================================="
echo "Error Surface Qt - Test Suite"
echo "================================="
echo ""

# Ensure project structure exists
if [ ! -d "$PROJECT_DIR/src" ]; then
    echo "ERROR: $PROJECT_DIR/src not found."
    echo "Expected project layout:"
    echo "  ~/error-dashboard-qt/src/*.h *.cpp"
    echo "  ~/error-dashboard-qt/tests/test_errordashboard.cpp"
    exit 1
fi

# Check for Qt6::Sql SQLITE driver (required for persistence)
if ! qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null | grep -q ""; then
    echo "Note: Ensure libqt6sql6-sqlite (or equivalent) is installed for persistence tests."
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring tests..."
cmake -DCMAKE_BUILD_TYPE=Debug "$TEST_DIR"

echo ""
echo "Building tests..."
make -j"$(nproc)"

echo ""
echo "================================="
echo "Running Test Suite..."
echo "================================="
echo ""

# Run with verbose output
./test_errordashboard -v2

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
