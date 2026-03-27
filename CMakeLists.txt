cmake_minimum_required(VERSION 3.28)
project(MiniAutonomySystem LANGUAGES CXX)

# Using C++20 with modules support for modern features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable C++20 modules support (required for this project)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
    if(CMAKE_GENERATOR MATCHES "Ninja|Ninja Multi-Config|Visual Studio")
        set(CMAKE_CXX_SCAN_FOR_MODULES ON)
        set(CMAKE_CXX_MODULE_STD ON)  # Enable C++23 modules support
        set(MODULES_SUPPORT ON)
    else()
        # Unix Makefiles does not support module-enabled scan mode
        set(MODULES_SUPPORT OFF)
    endif()
else()
    set(MODULES_SUPPORT OFF)
endif()

# Add modules directory
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/modules ${CMAKE_MODULE_PATH})

# Project include directories
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/io
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vision
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Find OpenCV for computer vision (disabled for testing)
# find_package(OpenCV COMPONENTS core highgui imgproc imgcodecs videoio)
set(OpenCV_FOUND FALSE)

# Find GStreamer for video pipeline
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GSTREAMER QUIET IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0)
endif()

# Optional: Find TensorRT for high-performance inference
find_package(TensorRT QUIET)

# C++20 Modules for modern perception system (required)
if(MODULES_SUPPORT)
    add_library(perception_modules STATIC
        modules/perception.concepts.cppm
        modules/perception.queue.cppm
        modules/perception.pipeline.cppm
        modules/perception.result.cppm
        modules/perception.metrics.cppm
        modules/perception.detector.simple.cppm
        modules/perception.async.cppm
        modules/perception.image_loader.simple.cppm
    )
    target_compile_features(perception_modules PUBLIC cxx_std_20)
    
    # Add module file set for proper CMake module handling
    target_sources(perception_modules PRIVATE 
        FILE_SET CXX_MODULES 
        FILES
            modules/perception.concepts.cppm
            modules/perception.queue.cppm
            modules/perception.pipeline.cppm
            modules/perception.result.cppm
            modules/perception.metrics.cppm
            modules/perception.detector.simple.cppm
            modules/perception.async.cppm
            modules/perception.image_loader.simple.cppm
    )
endif()

# Main demo application using only modules
if(MODULES_SUPPORT)
    add_executable(perception_demo
        src/main.cpp
    )
    
    target_link_libraries(perception_demo PRIVATE 
        perception_modules
        Threads::Threads
    )
    
    target_compile_features(perception_demo PRIVATE cxx_std_20)
endif()

# Modern C++23 demo with modules (disabled for testing)
# if(MODULES_SUPPORT)
#     add_executable(perception_modern_demo
#         src/main_modern.cpp
#     )
#     
#     target_link_libraries(perception_modern_demo PRIVATE 
#         perception_modules
#         Threads::Threads
#     )
#     
#     target_compile_features(perception_modern_demo PRIVATE cxx_std_23)
# endif()

# Benchmark application (disabled for testing)
# add_executable(perception_benchmark
#     apps/benchmark.cpp
# )
# 
# target_include_directories(perception_benchmark PRIVATE 
#     ${PROJECT_INCLUDE_DIRS}
# )
# 
# target_link_libraries(perception_benchmark PRIVATE 
#     perception_core
#     perception_io
#     perception_vision
#     Threads::Threads
# )

# Compiler-specific options for modules only
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(MODULES_SUPPORT)
        target_compile_options(perception_modules PRIVATE -Wall -Wextra -O3)
        target_compile_options(perception_demo PRIVATE -Wall -Wextra -O3)
    endif()
endif()

if(MSVC)
    if(MODULES_SUPPORT)
        target_compile_options(perception_modules PRIVATE /W4 /O2)
        target_compile_options(perception_demo PRIVATE /W4 /O2)
    endif()
endif()

# Optional: Enable testing
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    # Find Google Test
    find_package(GTest QUIET)
    if(GTest_FOUND)
        enable_testing()
        
        # Test executables
        add_executable(test_core tests/test_core.cpp)
        target_link_libraries(test_core PRIVATE 
            perception_core
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
        )
        
        add_executable(test_vision tests/test_vision.cpp)
        target_link_libraries(test_vision PRIVATE 
            perception_core
            perception_io
            perception_vision
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
            ${OpenCV_LIBS}
        )
        
        # Register tests
        add_test(NAME CoreTests COMMAND test_core)
        add_test(NAME VisionTests COMMAND test_vision)
    else()
        message(WARNING "GTest not found. Tests will not be built.")
    endif()
endif()
