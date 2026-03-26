cmake_minimum_required(VERSION 3.28)
project(MiniAutonomySystem LANGUAGES CXX)

# Using C++23 with modules support for modern features
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable C++20 modules support (only when generator supports it)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
    if(CMAKE_GENERATOR MATCHES "Ninja|Ninja Multi-Config|Visual Studio")
        set(CMAKE_CXX_SCAN_FOR_MODULES ON)
        set(CMAKE_CXX_MODULE_STD ON)
        set(MODULES_SUPPORT ON)
    else()
        # Unix Makefiles does not support module-enabled scan mode
        set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
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

# Find OpenCV for computer vision
find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc imgcodecs videoio)

# Find GStreamer for video pipeline
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GSTREAMER QUIET IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0)
endif()

# Optional: Find TensorRT for high-performance inference
find_package(TensorRT QUIET)

# C++20 Modules for modern perception system
if(MODULES_SUPPORT)
    add_library(perception_modules STATIC
        modules/perception.concepts.cppm
        modules/perception.queue.cppm
        modules/perception.pipeline.cppm
        modules/perception.result.cppm
        modules/perception.metrics.cppm
        modules/perception.detector.cppm
        modules/perception.async.cppm
        modules/perception.image_loader.cppm
    )
    target_compile_features(perception_modules PUBLIC cxx_std_23)
endif()

# Core library for perception system
add_library(perception_core
    src/core/pipeline.cpp
    src/core/metrics.cpp
    src/core/thread_safe_queue.cpp
    src/core/config.cpp
    src/core/logger.cpp
)

target_include_directories(perception_core PUBLIC 
    ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(perception_core PUBLIC 
    Threads::Threads
    ${OpenCV_LIBS}
)

if(GSTREAMER_FOUND)
    target_link_libraries(perception_core PUBLIC PkgConfig::GSTREAMER)
    target_compile_definitions(perception_core PUBLIC HAVE_GSTREAMER)
endif()

if(TensorRT_FOUND)
    target_link_libraries(perception_core PUBLIC ${TensorRT_LIBRARIES})
    target_include_directories(perception_core PUBLIC ${TensorRT_INCLUDE_DIRS})
    target_compile_definitions(perception_core PUBLIC HAVE_TENSORRT)
endif()

# IO library for video capture and IMU simulation
add_library(perception_io
    src/io/gstreamer_capture.cpp
    src/io/imu_simulator.cpp
)

target_include_directories(perception_io PUBLIC 
    ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(perception_io PUBLIC 
    perception_core
    Threads::Threads
    ${OpenCV_LIBS}
)

if(GSTREAMER_FOUND)
    target_link_libraries(perception_io PUBLIC PkgConfig::GSTREAMER)
endif()

# Vision library for detection, tracking, and fusion
add_library(perception_vision
    src/vision/detector.cpp
    src/vision/tracker.cpp
    src/vision/fusion.cpp
)

target_include_directories(perception_vision PUBLIC 
    ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(perception_vision PUBLIC 
    perception_core
    perception_io
    Threads::Threads
    ${OpenCV_LIBS}
)

if(TensorRT_FOUND)
    target_link_libraries(perception_vision PUBLIC ${TensorRT_LIBRARIES})
endif()

# Main demo application
add_executable(perception_demo
    src/main.cpp
)

target_include_directories(perception_demo PRIVATE 
    ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(perception_demo PRIVATE 
    perception_core
    perception_io
    perception_vision
    Threads::Threads
    ${OpenCV_LIBS}
)

# Modern C++23 demo with modules
if(MODULES_SUPPORT)
    add_executable(perception_modern_demo
        src/main_modern.cpp
    )
    
    target_link_libraries(perception_modern_demo PRIVATE 
        perception_modules
        Threads::Threads
        ${OpenCV_LIBS}
    )
    
    target_compile_features(perception_modern_demo PRIVATE cxx_std_23)
endif()

# Benchmark application
add_executable(perception_benchmark
    apps/benchmark.cpp
)

target_include_directories(perception_benchmark PRIVATE 
    ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(perception_benchmark PRIVATE 
    perception_core
    perception_io
    perception_vision
    Threads::Threads
    ${OpenCV_LIBS}
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(perception_core PRIVATE -Wall -Wextra -O3)
    target_compile_options(perception_io PRIVATE -Wall -Wextra -O3)
    target_compile_options(perception_vision PRIVATE -Wall -Wextra -O3)
    target_compile_options(perception_demo PRIVATE -Wall -Wextra -O3)
    target_compile_options(perception_benchmark PRIVATE -Wall -Wextra -O3)
endif()

if(MSVC)
    target_compile_options(perception_core PRIVATE /W4 /O2)
    target_compile_options(perception_io PRIVATE /W4 /O2)
    target_compile_options(perception_vision PRIVATE /W4 /O2)
    target_compile_options(perception_demo PRIVATE /W4 /O2)
    target_compile_options(perception_benchmark PRIVATE /W4 /O2)
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
