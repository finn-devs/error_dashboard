cmake_minimum_required(VERSION 3.16)
project(ErrorSurface VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Sql)

add_executable(error-surface
    src/main.cpp
    src/logcollector.cpp
    src/threatdetector.cpp
    src/statstab.cpp
    src/mainwindow.cpp
    src/persistencemanager.cpp
    src/settingsdrawer.cpp
)

target_link_libraries(error-surface
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Sql
    systemd
)

target_include_directories(error-surface PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)