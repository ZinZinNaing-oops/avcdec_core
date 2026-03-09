cmake_minimum_required(VERSION 3.16)

project(avc_project LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt installation path on macOS
if(APPLE)
    # Try to find Qt using Homebrew path
    if(NOT CMAKE_PREFIX_PATH)
        # Check for Qt6 first
        if(EXISTS "/opt/homebrew/opt/qt")
            set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" ${CMAKE_PREFIX_PATH})
        elseif(EXISTS "/usr/local/opt/qt")
            set(CMAKE_PREFIX_PATH "/usr/local/opt/qt" ${CMAKE_PREFIX_PATH})
        # Fallback to Qt5
        elseif(EXISTS "/opt/homebrew/opt/qt@5")
            set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@5" ${CMAKE_PREFIX_PATH})
        elseif(EXISTS "/usr/local/opt/qt@5")
            set(CMAKE_PREFIX_PATH "/usr/local/opt/qt@5" ${CMAKE_PREFIX_PATH})
        endif()
    endif()
endif()

# Add Qt support (do this BEFORE subdirectories)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 or Qt5
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)

if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found, trying Qt5...")
    find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
    if(Qt5_FOUND)
        message(STATUS "Found Qt5 version ${Qt5_VERSION}")
    endif()
else()
    message(STATUS "Found Qt6 version ${Qt6_VERSION}")
endif()

# Disable narrowing warning for JM compatibility
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-c++11-narrowing)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-narrowing)
endif()

add_subdirectory(JM)
add_subdirectory(avcdec)
add_subdirectory(player_mdi)
