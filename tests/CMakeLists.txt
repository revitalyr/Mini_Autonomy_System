# Tests CMakeLists.txt

# Old tests that use .hpp files (disabled after module migration)
# TODO: Update these tests to use modules
# add_executable(test_core test_core.cpp)
# target_link_libraries(test_core PRIVATE
#     Catch2::Catch2WithMain
#     perception_modules
#     Threads::Threads
# )
# target_include_directories(test_core PRIVATE ${CMAKE_SOURCE_DIR}/include)
# catch_discover_tests(test_core)

# add_executable(test_vision test_vision.cpp)
# target_link_libraries(test_vision PRIVATE
#     Catch2::Catch2WithMain
#     perception_modules
#     ${OpenCV_LIBS}
#     Threads::Threads
# )
# target_include_directories(test_vision PRIVATE ${CMAKE_SOURCE_DIR}/include)
# target_include_directories(test_vision PRIVATE ${OpenCV_INCLUDE_DIRS})
# catch_discover_tests(test_vision)

# add_executable(test_ros_provider test_ros_provider.cpp)
# target_link_libraries(test_ros_provider PRIVATE
#     Catch2::Catch2WithMain
#     perception_modules
#     ${OpenCV_LIBS}
#     ${catkin_LIBRARIES}
#     Threads::Threads
# )
# target_include_directories(test_ros_provider PRIVATE ${CMAKE_SOURCE_DIR}/include)
# target_include_directories(test_ros_provider PRIVATE ${OpenCV_INCLUDE_DIRS})
# target_include_directories(test_ros_provider PRIVATE ${catkin_INCLUDE_DIRS})
# catch_discover_tests(test_ros_provider)

# Create test_detector executable
add_executable(test_detector test_detector.cpp)
target_link_libraries(test_detector PRIVATE
    Catch2::Catch2WithMain
    perception_modules
    ${OpenCV_LIBS}
    Threads::Threads
)
target_include_directories(test_detector PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_detector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_compile_options(test_detector PRIVATE -std=c++23)

# Register test_detector with CTest
include(CTest)
include(Catch)
catch_discover_tests(test_detector)
