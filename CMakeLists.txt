cmake_minimum_required(VERSION 3.16)

# ── Version ──────────────────────────────────────────────────────────────────
# Version is sourced exclusively from the Git tag on HEAD.
# Expected format: v1.0.0.0 — build will fail if no matching tag exists.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GitVersion)

project(ErrorSurface
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
)

# ── C++ standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Sql)

# ── Executable ───────────────────────────────────────────────────────────────
add_executable(error-dashboard
    src/main.cpp
    src/logcollector.cpp
    src/threatdetector.cpp
    src/statstab.cpp
    src/mainwindow.cpp
    src/persistencemanager.cpp
    src/settingsdrawer.cpp
)

# ── Version compile-time constant ────────────────────────────────────────────
# Exposes APP_VERSION as a string in source code.
# Usage in C++:
#   #include "version.h"
#   qDebug() << APP_VERSION;   // "1.0.0.0"
#   qDebug() << APP_VERSION_FULL; // "v1.0.0.0"
target_compile_definitions(error-dashboard PRIVATE
    APP_VERSION="${GIT_TAG_CLEAN}"
    APP_VERSION_FULL="${GIT_TAG_FULL}"
)

# ── Link libraries ───────────────────────────────────────────────────────────
target_link_libraries(error-dashboard
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Sql
    systemd
)

# ── Include directories ───────────────────────────────────────────────────────
target_include_directories(error-dashboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
