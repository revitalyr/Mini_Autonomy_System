# Tests CMakeLists.txt

# Create test_core executable (doesn't require OpenCV)
add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)
target_include_directories(test_core PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_core PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Register test_core with CTest
include(GoogleTest)
gtest_discover_tests(test_core)

# Create test_vision executable
add_executable(test_vision test_vision.cpp)
target_link_libraries(test_vision PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${OpenCV_LIBS}
    Threads::Threads
)
target_include_directories(test_vision PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_vision PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(test_vision PRIVATE ${OpenCV_INCLUDE_DIRS})

# Register test_vision with CTest
gtest_discover_tests(test_vision)
