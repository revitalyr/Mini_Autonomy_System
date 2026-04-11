# Tests CMakeLists.txt

# Create test_core executable (doesn't require OpenCV)
add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE
    Catch2::Catch2WithMain
    perception_modules
    Threads::Threads
)
target_include_directories(test_core PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Register test_core with CTest
include(CTest)
include(Catch)
catch_discover_tests(test_core)

# Create test_vision executable
add_executable(test_vision test_vision.cpp)
target_link_libraries(test_vision PRIVATE
    Catch2::Catch2WithMain
    perception_modules
    ${OpenCV_LIBS}
    Threads::Threads
)
target_include_directories(test_vision PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_vision PRIVATE ${OpenCV_INCLUDE_DIRS})

# Register test_vision with CTest
catch_discover_tests(test_vision)

# Create test_ros_provider executable
add_executable(test_ros_provider test_ros_provider.cpp)
target_link_libraries(test_ros_provider PRIVATE
    Catch2::Catch2WithMain
    perception_modules
    ${OpenCV_LIBS}
    ${catkin_LIBRARIES}
    Threads::Threads
)
target_include_directories(test_ros_provider PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_ros_provider PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(test_ros_provider PRIVATE ${catkin_INCLUDE_DIRS})

# Register test_ros_provider with CTest
catch_discover_tests(test_ros_provider)
