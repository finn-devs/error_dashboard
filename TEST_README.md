# Error Surface Qt - Test Suite Documentation

## Overview
Comprehensive test suite covering all components of the error-surface-qt application using Qt Test framework.

## Test Coverage

### 1. LogEntry Tests (4 tests)
- **testLogEntrySeverityLabels**: Validates severity label generation (EMRG, ALRT, CRIT, ERR, WARN)
- **testLogEntrySeverityColors**: Verifies color assignments for each severity level
- **testLogEntryThreatBadge**: Tests threat badge formatting

### 2. ThreatDetector Tests (10 tests)
- **testThreatDetectorAuthentication**: SSH failures, authentication errors
- **testThreatDetectorPrivilege**: Sudo violations, privilege escalation attempts
- **testThreatDetectorNetwork**: Firewall blocks, suspicious connections
- **testThreatDetectorFilesystem**: Permission denied on sensitive files
- **testThreatDetectorStability**: Segfaults, kernel panics, crashes
- **testThreatDetectorResources**: OOM kills, resource exhaustion
- **testThreatDetectorSELinux**: SELinux/AppArmor violations
- **testThreatDetectorMalware**: Suspicious process detection
- **testThreatDetectorMultipleThreats**: Messages with multiple threat indicators
- **testThreatDetectorNoThreats**: Normal messages with no threats

### 3. LogCollector Tests (4 tests)
- **testLogCollectorJournaldOpen**: Validates systemd journal access
- **testLogCollectorSeverityFiltering**: Ensures only priority 0-4 entries collected
- **testLogCollectorTimeFiltering**: Verifies time window filtering
- **testLogCollectorDmesgFallback**: Tests dmesg collection when available

### 4. StatsTab Tests (8 tests)
- **testStatsTabDataLoading**: Data ingestion without crashes
- **testStatsTabStatCounts**: Stat card value updates
- **testStatsTabFiltering**: Severity filter functionality
- **testStatsTabSearchFilter**: Text search filtering
- **testStatsTabUnitFilter**: Unit dropdown filtering
- **testStatsTabClickableCards**: Card click event handling
- **testStatsTabChartGeneration**: Chart creation from data
- **testStatsTabExportCSV**: CSV export functionality

### 5. MainWindow Tests (3 tests)
- **testMainWindowInitialization**: Window creation and setup
- **testMainWindowTabSwitching**: Scan/Live tab navigation
- **testMainWindowLivePolling**: Background collection threads

### 6. Integration Tests (3 tests)
- **testEndToEndDataFlow**: Collection → Processing → Display pipeline
- **testThreatDetectionPipeline**: End-to-end threat detection
- **testUIResponsiveness**: Performance with 10,000 entries (< 5s)

## Running Tests

### Quick Start
```bash
cd ~/Projects/error-surface-qt
chmod +x run_tests.sh
./run_tests.sh
```

### Manual Execution
```bash
# Create test directory
mkdir -p ~/Projects/error-surface-qt/tests
cd ~/Projects/error-surface-qt/tests

# Copy test files
cp test_errorsurface.cpp .
cp CMakeLists_tests.txt CMakeLists.txt

# Build
mkdir -p ../build-tests
cd ../build-tests
cmake ../tests
make -j$(nproc)

# Run all tests
./test_errorsurface

# Run with verbose output
./test_errorsurface -v2

# Run specific test
./test_errorsurface testThreatDetectorAuthentication
```

### Using CTest
```bash
cd ~/Projects/error-surface-qt/build-tests
ctest --verbose
ctest --output-on-failure
```

## Test Data Generation

The test suite includes helper functions for generating test data:

```cpp
// Generate diverse test dataset
QVector<LogEntry> entries = createTestEntries();
// Returns: 1 critical, 5 errors, 3 warnings

// Create specific entry
LogEntry entry = createTestEntry("error", "Service failed", "nginx.service");
```

## Dependencies

Required Qt modules:
- Qt6::Test
- Qt6::Core
- Qt6::Gui
- Qt6::Widgets
- Qt6::Charts

System libraries:
- libsystemd (for journal access)

## Expected Results

All 32 tests should PASS on a properly configured system:

```
********* Start testing of TestErrorSurface *********
Config: Using QtTest library 6.x.x
PASS   : TestErrorSurface::initTestCase()
PASS   : TestErrorSurface::testLogEntrySeverityLabels()
PASS   : TestErrorSurface::testLogEntrySeverityColors()
...
PASS   : TestErrorSurface::cleanupTestCase()
Totals: 32 passed, 0 failed, 0 skipped, 0 blacklisted
********* Finished testing of TestErrorSurface *********
```

## Troubleshooting

### Test Failures

**Journal Access Failed**
```
FAIL! : TestErrorSurface::testLogCollectorJournaldOpen()
```
Solution: Ensure user is in `adm` group or running with proper permissions

**Chart Tests Fail**
```
FAIL! : TestErrorSurface::testStatsTabChartGeneration()
```
Solution: Verify Qt6Charts is installed: `sudo apt install qt6-charts-dev`

**Build Errors**
```
fatal error: QtTest/QtTest: No such file or directory
```
Solution: Install Qt Test module: `sudo apt install qt6-base-dev`

### Performance Benchmarks

The `testUIResponsiveness` test validates that 10,000 log entries can be processed in under 5 seconds. On typical hardware:
- Expected: 1-2 seconds
- Warning threshold: 3-4 seconds
- Failure threshold: 5+ seconds

## Continuous Integration

To integrate with CI/CD:

```yaml
# .github/workflows/test.yml
name: Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y qt6-base-dev qt6-charts-dev libsystemd-dev
      - name: Run tests
        run: ./run_tests.sh
```

## Coverage Analysis

To generate coverage report:

```bash
cd ~/Projects/error-surface-qt/build-tests
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" ../tests
make
./test_errorsurface
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-report
```

View report: `firefox coverage-report/index.html`

## Writing New Tests

Template for adding new test cases:

```cpp
void TestErrorSurface::testNewFeature() {
    // Setup
    StatsTab tab("scan");
    auto entries = createTestEntries();
    
    // Execute
    tab.setData(entries);
    
    // Verify
    QVERIFY(someCondition);
    QCOMPARE(actual, expected);
}
```

Add to test class in `test_errorsurface.cpp`:
```cpp
private slots:
    void testNewFeature();
```

## Test Philosophy

- **Unit tests**: Test individual components in isolation
- **Integration tests**: Verify component interactions
- **Performance tests**: Ensure acceptable response times
- **Regression tests**: Prevent reintroduction of fixed bugs

## Maintenance

Run tests before each commit:
```bash
git commit --no-verify  # Skip if needed (not recommended)
```

Add to git hooks:
```bash
echo "./run_tests.sh" > .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```
