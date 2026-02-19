# Error Dashboard - Qt6 Native Edition

Native C++/Qt6 Error Dashabord log analyzer.

## Features

- **Native Performance**: ~5MB memory, instant startup, zero browser overhead
- **Two Tabs**: Scan (7-day historical) and Live (60-minute rolling, polls every 5s)
- **Security Threat Detection**: 8 threat categories with pattern matching
- **Dark Terminal Aesthetic**: Exact match to the original Dash design
- **Full Filtering**: Severity groups, unit filter, search, threats-only view
- **Detail Panel**: Click any row to expand full event details with threat breakdown
- **CSV Export**: Export filtered results with timestamps

## Dependencies

### Ubuntu/Debian
```bash
sudo apt install build-essential cmake qt6-base-dev qt6-charts-dev libsystemd-dev
```

### Fedora/RHEL
```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qtcharts-devel systemd-devel
```

### Arch
```bash
sudo pacman -S cmake qt6-base qt6-charts systemd
```

## Build

```bash
cd error-surface-qt
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Install

```bash
sudo make install
# Installs to /usr/local/bin/error-surface
```

## Usage

```bash
# Run with defaults (7-day scan, 60min live window, 5s poll)
./error-surface

# Custom lookback periods
./error-surface --days 14 --live-window 120 --live-poll 10

# Short flags
./error-surface -d 3 -w 30 -p 15
```

### Permissions

For dmesg access without sudo, add yourself to the `adm` group:

```bash
sudo usermod -aG adm $USER
newgrp adm
```

Or configure passwordless sudo for dmesg only:
```bash
echo "$USER ALL=(root) NOPASSWD: /usr/bin/dmesg" | sudo tee /etc/sudoers.d/dmesg-access
sudo chmod 0440 /etc/sudoers.d/dmesg-access
```

## Architecture

- **LogCollector** - Uses libsystemd journal API for journald, QProcess for dmesg
- **ThreatDetector** - Pattern-based security threat detection (8 categories)
- **StatsTab** - Complete UI for one tab (scan or live)
- **MainWindow** - Tabbed interface coordinator with background threads

## Comparison to Python/Dash Version

| Feature | Python/Dash | C++/Qt6 |
|---------|-------------|---------|
| Memory | ~200MB | ~5MB |
| Startup | 2-3s | <100ms |
| Dependencies | Flask, Dash, Plotly, pandas | Qt6, libsystemd |
| Distribution | Script + venv | Single binary |
| Platform | Web browser required | Native Linux app |
| Look & Feel | Identical | Identical |

## Security Threat Categories

1. **Authentication** - Failed logins, invalid users, preauth disconnects
2. **Privilege Escalation** - Sudo abuse, su failures, unauthorized access
3. **Suspicious Network** - Port scans, firewall blocks, connection spam
4. **Filesystem Tampering** - /etc/passwd, /etc/shadow modifications
5. **Service Crashes** - Segfaults, core dumps, kernel panics
6. **Resource Exhaustion** - OOM killer, disk full, file limit hits
7. **SELinux Violations** - AVC denials, policy violations
8. **Malware Indicators** - Rootkit signatures, suspicious binaries

## License

MIT License - See source files for details

## Development

Code structure:
```
src/
├── main.cpp           - Entry point with CLI parsing
├── mainwindow.h/cpp   - Main window with scan/live tabs
├── statstab.h/cpp     - Tab UI (stats, charts, table, filters)
├── logcollector.h/cpp - Journald + dmesg collection with libsystemd
├── threatdetector.h/cpp - Security pattern matching
└── logentry.h         - Data structures
```

Charts use QtCharts (QBarSeries, QPieSeries). Styling uses Qt stylesheets (CSS-like).

Threading: Scan runs once on startup in background thread. Live polls every N seconds in separate thread. Both use Qt's signal/slot mechanism for thread-safe UI updates.
