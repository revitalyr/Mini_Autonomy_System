cmake_minimum_required(VERSION 3.20)
project(MiniAutonomySystem LANGUAGES CXX)

# Using C++17 for broader compatibility (can be upgraded to C++23 when available)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Core library for perception system
add_library(perception_core
    src/core/pipeline.cpp
    src/core/metrics.cpp
    src/core/thread_safe_queue.cpp
    src/core/config.cpp
    src/core/logger.cpp
)

target_include_directories(perception_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(perception_demo PRIVATE 
    perception_core
    perception_io
    perception_vision
    Threads::Threads
    ${OpenCV_LIBS}
)

# Benchmark application
add_executable(perception_benchmark
    apps/benchmark.cpp
)

target_include_directories(perception_benchmark PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
