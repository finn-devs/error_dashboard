cmake_minimum_required(VERSION 3.16)
project(error-surface VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)

add_executable(error-surface
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/logcollector.cpp
    src/logcollector.h
    src/threatdetector.cpp
    src/threatdetector.h
    src/logentry.h
    src/statstab.cpp
    src/statstab.h
)

target_include_directories(error-surface PRIVATE ${SYSTEMD_INCLUDE_DIRS})
target_link_libraries(error-surface PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    ${SYSTEMD_LIBRARIES}
)

install(TARGETS error-surface DESTINATION bin)
