cmake_minimum_required(VERSION 3.20)
project(desktop-watcher VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Dependencies ────────────────────────────────────────────────────────────
find_package(fmt            CONFIG REQUIRED)
find_package(SQLite3               REQUIRED)  # CMake built-in FindSQLite3
find_package(tomlplusplus   CONFIG REQUIRED)
find_package(CLI11          CONFIG REQUIRED)
find_package(X11                   REQUIRED)
find_package(PkgConfig             REQUIRED)

find_library(XSCRNSAVER_LIB Xss REQUIRED)

pkg_check_modules(ATSPI  REQUIRED atspi-2)
pkg_check_modules(GIO    REQUIRED gio-2.0)

# ── libdwatcher ──────────────────────────────────────────────────────────────
add_library(libdwatcher STATIC
    src/core/config.cpp
    src/core/db.cpp
    src/core/categorizer.cpp
    src/platform/x11_window.cpp
    src/platform/gnome_wayland.cpp
)

set_target_properties(libdwatcher PROPERTIES OUTPUT_NAME dwatcher)

target_include_directories(libdwatcher PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${X11_INCLUDE_DIR}
    ${ATSPI_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
)

target_link_libraries(libdwatcher PUBLIC
    fmt::fmt
    SQLite::SQLite3
    tomlplusplus::tomlplusplus
    ${X11_LIBRARIES}
    ${XSCRNSAVER_LIB}
    ${ATSPI_LIBRARIES}
    ${GIO_LIBRARIES}
)

# ── desktop-watcherd ─────────────────────────────────────────────────────────
add_executable(desktop-watcherd
    src/daemon/main.cpp
    src/daemon/poller.cpp
    src/daemon/idle.cpp
)

target_link_libraries(desktop-watcherd PRIVATE libdwatcher)

add_custom_command(TARGET desktop-watcherd POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink desktop-watcherd dwd
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating symlink dwd -> desktop-watcherd"
)

# ── desktop-watcher (CLI) ────────────────────────────────────────────────────
add_executable(desktop-watcher
    src/cli/main.cpp
)

target_link_libraries(desktop-watcher PRIVATE libdwatcher CLI11::CLI11)

add_custom_command(TARGET desktop-watcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink desktop-watcher dw
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating symlink dw -> desktop-watcher"
)

# ── Tests ────────────────────────────────────────────────────────────────────
enable_testing()

foreach(TEST_NAME test_db test_categorizer test_config)
    add_executable(${TEST_NAME} tests/${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE libdwatcher)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
