cmake_minimum_required(VERSION 3.16)

# Default: cross-compile the firmware with avr-gcc.
# -DLAMPTEST_TESTS=ON: build the host unit tests instead (run them with ctest).
option(LAMPTEST_TESTS "Build host unit tests instead of the firmware" OFF)

if(NOT LAMPTEST_TESTS AND NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/avr-gcc.cmake)
endif()

if(NOT CMAKE_BUILD_TYPE)
    if(LAMPTEST_TESTS)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    else()
        set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
    endif()
endif()

project(lamptest LANGUAGES C)

if(LAMPTEST_TESTS)
    enable_testing()
    add_subdirectory(avt5529/tests)
else()
    add_subdirectory(avt5529)
endif()
