cmake_minimum_required(VERSION 3.16)
project(whispr-clone VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Optimization flags for low latency
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(WHISPER_BUILD_EXAMPLES "Build whisper examples" OFF)
option(WHISPER_BUILD_TESTS "Build whisper tests" OFF)

# Platform detection
if(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_definitions(-DPLATFORM_MACOS)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX)
endif()

# whisper.cpp
add_subdirectory(external/whisper.cpp)

# PortAudio for cross-platform audio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# Source files
set(SOURCES
    src/main.cpp
    src/audio_capture.cpp
    src/transcriber.cpp
    src/hotkey_manager.cpp
    src/clipboard.cpp
    src/app.cpp
    src/text_processor.cpp
)

set(HEADERS
    include/audio_capture.hpp
    include/transcriber.hpp
    include/hotkey_manager.hpp
    include/clipboard.hpp
    include/app.hpp
    include/config.hpp
    include/text_processor.hpp
)

# Main executable
add_executable(whispr ${SOURCES} ${HEADERS})

target_include_directories(whispr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/whisper.cpp
    ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_libraries(whispr PRIVATE
    whisper
    ${PORTAUDIO_LINK_LIBRARIES}
)

# Add portaudio library directory
target_link_directories(whispr PRIVATE ${PORTAUDIO_LIBRARY_DIRS})

# Platform-specific libraries
if(PLATFORM_MACOS)
    target_sources(whispr PRIVATE
        src/platform/macos/hotkey_macos.mm
        src/platform/macos/clipboard_macos.mm
        src/platform/macos/tray_macos.mm
    )
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(CARBON_FRAMEWORK Carbon REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    target_link_libraries(whispr PRIVATE
        ${COCOA_FRAMEWORK}
        ${CARBON_FRAMEWORK}
        ${APPKIT_FRAMEWORK}
        ${COREGRAPHICS_FRAMEWORK}
    )
elseif(PLATFORM_LINUX)
    target_sources(whispr PRIVATE
        src/platform/linux/hotkey_linux.cpp
        src/platform/linux/clipboard_linux.cpp
        src/platform/linux/tray_linux.cpp
    )
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(XTST REQUIRED xtst)
    pkg_check_modules(LIBEVDEV libevdev)
    target_include_directories(whispr PRIVATE ${X11_INCLUDE_DIRS} ${XTST_INCLUDE_DIRS})
    target_link_libraries(whispr PRIVATE ${X11_LIBRARIES} ${XTST_LIBRARIES})
    if(LIBEVDEV_FOUND)
        target_include_directories(whispr PRIVATE ${LIBEVDEV_INCLUDE_DIRS})
        target_link_libraries(whispr PRIVATE ${LIBEVDEV_LIBRARIES})
        add_definitions(-DHAS_LIBEVDEV)
    endif()
endif()

# Install
install(TARGETS whispr RUNTIME DESTINATION bin)
