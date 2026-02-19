# Building error-dashboard from Source

error-dashboard is a native C++/Qt6 Linux system monitoring and log analysis tool.
It interfaces directly with systemd journal and dmesg and is therefore
**Linux-only**. It will not build or run on Windows or macOS.

---

## Requirements

| Dependency | Minimum Version | Notes |
|---|---|---|
| GCC or Clang | GCC 10+ / Clang 12+ | Must support C++17 |
| CMake | 3.16+ | |
| Qt6 | 6.2+ | Core, Gui, Widgets, Charts, Sql |
| libsystemd | Any modern version | From systemd 245+ recommended |
| SQLite | 3.x | Pulled in via Qt6::Sql |
| Ninja | Any | Recommended; Make also works |

---

## Installing Dependencies

### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    ninja-build \
    gcc \
    g++ \
    libsystemd-dev \
    qt6-base-dev \
    qt6-charts-dev \
    libqt6sql6-sqlite \
    libgl1-mesa-dev \
    libglib2.0-dev \
    libxkbcommon-dev \
    libvulkan-dev
```

### Fedora / RHEL / Rocky Linux

```bash
sudo dnf install -y \
    cmake \
    ninja-build \
    gcc \
    gcc-c++ \
    systemd-devel \
    qt6-qtbase-devel \
    qt6-qtcharts-devel \
    qt6-qtbase-sqlite \
    mesa-libGL-devel \
    libxkbcommon-devel \
    vulkan-headers
```

> **Note:** Qt6 availability varies across RHEL-based distros. If `qt6-qtcharts-devel`
> is not available via dnf, you may need to enable additional repositories or
> install Qt6 manually from [qt.io](https://www.qt.io/download-qt-installer).

### Arch Linux

```bash
sudo pacman -S \
    cmake \
    ninja \
    gcc \
    systemd \
    qt6-base \
    qt6-charts \
    mesa \
    libxkbcommon \
    vulkan-headers
```

### openSUSE

```bash
sudo zypper install -y \
    cmake \
    ninja \
    gcc \
    gcc-c++ \
    systemd-devel \
    qt6-base-devel \
    qt6-charts-devel \
    libQt6Sql6-sqlite \
    Mesa-libGL-devel \
    libxkbcommon-devel \
    vulkan-devel
```

---

## Building

```bash
# Clone the repository
git clone https://github.com/[your-github-handle]/error-dashboard.git
cd error-dashboard

# Configure
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -G Ninja

# Build
cmake --build build --parallel
```

The compiled binary will be at `build/error-dashboard`.

---

## Running

```bash
./build/error-dashboard
```

error-dashboard reads from the systemd journal and dmesg. Reading journal entries
may require your user to be a member of the `systemd-journal` group:

```bash
sudo usermod -aG systemd-journal $USER
# Log out and back in for the group change to take effect
```

---

## Running the Test Suite

```bash
cd build
QT_QPA_PLATFORM=offscreen ctest --output-on-failure --parallel 4
```

`QT_QPA_PLATFORM=offscreen` is required for tests that instantiate Qt widgets
in a headless environment.

---

## Installing System-Wide (Optional)

There is no install target defined by default. To make the binary available
system-wide, copy it manually:

```bash
sudo cp build/error-dashboard /usr/local/bin/
```

---

## Troubleshooting

**`Qt6Charts` not found during configure**
Install `qt6-charts-dev` (Debian/Ubuntu) or `qt6-qtcharts-devel` (Fedora).
On distros where Qt6 Charts is not packaged, install Qt6 via the
[Qt online installer](https://www.qt.io/download-qt-installer).

**`libsystemd` not found**
Install `libsystemd-dev` (Debian/Ubuntu) or `systemd-devel` (Fedora/RHEL).

**Tests fail with `cannot connect to X server`**
Set `QT_QPA_PLATFORM=offscreen` before running ctest as shown above.

**Binary runs but journal is empty**
Add your user to the `systemd-journal` group as described in the Running section.

---

## License

error-dashboard is source-available under the
[error-dashboard Source-Available License](LICENSE). Commercial use requires a
separate written license from Frederick Finn / Finn Devs. See [LICENSE](LICENSE)
for full terms.

Contributions are subject to the [Contributor License Agreement](CLA.md).
