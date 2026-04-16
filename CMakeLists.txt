cmake_minimum_required(VERSION 3.28)

project(MiniAutonomySystem LANGUAGES CXX VERSION 1.0.0)



set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)



# Включаем экспериментальную поддержку модулей C++20/23 (после поиска пакетов)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28" AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC))
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
    set(MODULES_SUPPORT ON)
    message(STATUS "C++20/23 modules enabled for ${CMAKE_CXX_COMPILER_ID}")
else()
    set(MODULES_SUPPORT OFF)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(WARNING "GCC detected - C++ modules support in CMake is still limited for GCC")
    else()
        message(WARNING "C++ modules not supported - using traditional includes")
    endif()
endif()



# Проверяем, задана ли переменная окружения VCPKG_ROOT

if(DEFINED ENV{VCPKG_ROOT})
    # Устанавливаем пути, используя переменную окружения
    # Переменная триплета (x64-windows) также может быть вынесена для гибкости

    set(VCPKG_INSTALLED_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows")
    set(TIFF_INCLUDE_DIR "${VCPKG_INSTALLED_PATH}/include")
    set(TIFF_LIBRARY     "${VCPKG_INSTALLED_PATH}/lib/tiff.lib")
    # Сообщаем пользователю, что пути подхвачены из vcpkg
    message(STATUS "Using TIFF from VCPKG_ROOT: ${VCPKG_INSTALLED_PATH}")

else()

    message(WARNING "VCPKG_ROOT environment variable is not set!")

endif()

# Check for downloaded OpenCV first (faster than building from vcpkg)
if(EXISTS "D:/opencv/opencv/build/x64/vc16/lib/OpenCVConfig.cmake")
    set(OpenCV_DIR "D:/opencv/opencv/build/x64/vc16/lib")
    message(STATUS "Using downloaded OpenCV: ${OpenCV_DIR}")
elseif(EXISTS "D:/opencv/build/x64/vc16/lib/OpenCVConfig.cmake")
    set(OpenCV_DIR "D:/opencv/build/x64/vc16/lib")
    message(STATUS "Using downloaded OpenCV: ${OpenCV_DIR}")
elseif(DEFINED VCPKG_INSTALLED_PATH AND EXISTS "${VCPKG_INSTALLED_PATH}/share/opencv4/OpenCVConfig.cmake")
    # Fallback to vcpkg if available
    set(OpenCV_DIR "${VCPKG_INSTALLED_PATH}/share/opencv4")
    message(STATUS "Using OpenCV from vcpkg: ${OpenCV_DIR}")
else()
    message(STATUS "Searching for OpenCV in standard locations")
endif()

# Search for packages - use world if available
find_package(OpenCV REQUIRED CONFIG HINTS "${OpenCV_DIR}" COMPONENTS world)
find_package(Threads REQUIRED)

# Create executable
add_executable(perception_demo src/main.cpp)

# Copy OpenCV world DLL to build directory
if(OpenCV_FOUND)
    add_custom_command(TARGET perception_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "D:/opencv/opencv/build/x64/vc16/bin/opencv_world4100.dll"
        $<TARGET_FILE_DIR:perception_demo>
        COMMENT "Copying OpenCV world DLL to build directory"
    )
endif()

# Поиск ROS 2 (опционально)
find_package(ament_cmake QUIET)
find_package(rclcpp QUIET)
find_package(rosbag2_cpp QUIET)
find_package(cv_bridge QUIET)
find_package(sensor_msgs QUIET)
find_package(std_msgs QUIET)

if(ament_cmake_FOUND AND rclcpp_FOUND)
    message(STATUS "ROS 2 found - building with ROS 2 support")
    set(ROS_FOUND TRUE)

else()

    message(STATUS "ROS 2 not found - building without ROS support")
    set(ROS_FOUND FALSE)
    add_definitions(-DNO_ROS_SUPPORT)

endif()


# Создание библиотеки модулей
add_library(perception_modules src/modules_dummy.cpp)

# Add include directories for OpenCV and project headers (needed for C++ modules)
target_include_directories(perception_modules PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(perception_modules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(perception_modules PUBLIC ${OpenCV_LIBS})

if(MODULES_SUPPORT)
    target_sources(perception_modules PUBLIC

        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"

        FILES
            include/perception/types.ixx
            include/perception/geom.ixx
            include/perception/concepts.ixx
            include/perception/result.ixx
            include/perception/async.ixx
            include/perception/queue.ixx
            include/perception/metrics.ixx
            include/perception/image_loader.ixx
            include/perception/detector.ixx
            include/perception/tracking.ixx
            include/perception/calibration.ixx
            include/perception/stereo_matching.ixx
            include/perception/viz.ixx
            include/perception/pipeline.ixx
    )

    # Add ROS module only if ROS is available
    if(ROS_FOUND)
        target_sources(perception_modules PUBLIC

            FILE_SET CXX_MODULES TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"

            FILES
                include/perception/ros_provider.ixx
        )

    endif()

else()

    # When modules are disabled, .ixx files are treated as headers
    # No need to add them as source files - they're included by implementation files

endif()

# Add implementation files for modules - they should be part of the same library
# but not in the CXX_MODULES file set
if(MODULES_SUPPORT)
    # For MSVC and Clang with module support, implementation files are added separately
    target_sources(perception_modules PRIVATE
        modules/perception.types.cpp
        modules/perception.detector.cpp
        modules/perception.calibration.cpp
        modules/perception.stereo_matching.cpp
        modules/perception.tracking.cpp
        modules/perception.image_loader.cpp
        modules/perception.pipeline.cpp
        modules/perception.viz.cpp
    )
else()
    # For compilers without module support, treat .ixx as headers
    target_sources(perception_modules PRIVATE
        modules/perception.detector.cpp
        modules/perception.tracking.cpp
        modules/perception.image_loader.cpp
        modules/perception.pipeline.cpp
        modules/perception.viz.cpp
    )
endif()

# Создание исполняемого файла для ROSBAG демо только если ROS доступен
if(ROS_FOUND)

    add_executable(rosbag_demo src/rosbag_demo.cpp)
    add_executable(rosbag_viz_demo src/rosbag_viz_demo.cpp)
    add_executable(rosbag_viz_demo_headless src/rosbag_viz_demo_headless.cpp)

endif()

# Линковка

target_link_libraries(perception_demo PRIVATE 

    perception_modules 
    ${OpenCV_LIBS} 
    Threads::Threads

)

if(ROS_FOUND)

    target_link_libraries(rosbag_demo PRIVATE

        perception_modules
        ${OpenCV_LIBS}
        ${rclcpp_LIBRARIES}
        ${cv_bridge_LIBRARIES}
        ${rosbag2_cpp_LIBRARIES}
        Threads::Threads

    )

    target_link_libraries(rosbag_viz_demo PRIVATE

        perception_modules
        ${OpenCV_LIBS}
        ${rclcpp_LIBRARIES}
        ${cv_bridge_LIBRARIES}
        ${rosbag2_cpp_LIBRARIES}
        Threads::Threads

    )

    target_link_libraries(rosbag_viz_demo_headless PRIVATE

        perception_modules
        ${OpenCV_LIBS}
        ${rclcpp_LIBRARIES}
        ${cv_bridge_LIBRARIES}
        ${rosbag2_cpp_LIBRARIES}
        Threads::Threads

    )

endif()

# Настройка путей включения
target_include_directories(perception_demo PRIVATE include)

if(ROS_FOUND)

    target_include_directories(perception_demo PRIVATE ${rclcpp_INCLUDE_DIRS} ${cv_bridge_INCLUDE_DIRS} ${sensor_msgs_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS})
    target_include_directories(rosbag_demo PRIVATE include)
    target_include_directories(rosbag_demo PRIVATE ${rclcpp_INCLUDE_DIRS} ${cv_bridge_INCLUDE_DIRS} ${sensor_msgs_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS})
    target_include_directories(rosbag_viz_demo PRIVATE include)
    target_include_directories(rosbag_viz_demo PRIVATE ${rclcpp_INCLUDE_DIRS} ${cv_bridge_INCLUDE_DIRS} ${sensor_msgs_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS})
    target_include_directories(rosbag_viz_demo_headless PRIVATE include)
    target_include_directories(rosbag_viz_demo_headless PRIVATE ${rclcpp_INCLUDE_DIRS} ${cv_bridge_INCLUDE_DIRS} ${sensor_msgs_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS})

endif()

# Современные настройки компилятора
if(MSVC)

    # MSVC settings with C++20/23 modules
    target_compile_options(perception_modules PRIVATE
        /W4                    # High warning level
        /EHsc                  # C++ exception handling
        /permissive-           # Strict C++ standard
        /std:c++latest         # C++ standard
        /Zc:__cplusplus        # Correct __cplusplus
        /experimental:module   # Enable module support
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot>   # Max optimization in Release
        $<$<CONFIG:Debug>:/Zi /Od>             # Debug info in Debug
    )

    target_compile_options(perception_demo PRIVATE
        /W4
        /EHsc
        /permissive-
        /std:c++latest
        /Zc:__cplusplus
        /experimental:module   # Enable module support
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Zi>
    )

    # Современные определения для MSVC
    target_compile_definitions(perception_demo PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_CXX20_OLD_DEPRECATION_WARNINGS
    )

else()

    # GCC/Clang настройки с C++23
    target_compile_options(perception_modules PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -std=c++23
        # Suppress warnings from OpenCV headers with C++23
        -Wno-deprecated-anon-enum-enum-conversion
        -Wno-c11-extensions
        $<$<CONFIG:Release>:-O3 -march=native -flto>
        $<$<CONFIG:Debug>:-g -O0>
    )

    target_compile_definitions(perception_modules PRIVATE
        OPENCV_SKIP_VISIBILITY_CONTROL
    )

    target_compile_options(perception_demo PRIVATE
        -Wall -Wextra -Wpedantic
        -std=c++23
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g>
    )

    if(ROS_FOUND)

        target_compile_options(rosbag_demo PRIVATE
            -Wall -Wextra -Wpedantic
            -std=c++23
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Debug>:-g>
        )

        target_compile_options(rosbag_viz_demo PRIVATE
            -Wall -Wextra -Wpedantic
            -std=c++23
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Debug>:-g>
        )

        target_compile_options(rosbag_viz_demo_headless PRIVATE
            -Wall -Wextra -Wpedantic
            -std=c++23
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Debug>:-g>
        )

    endif()

endif()

# Добавление современных библиотек и инструментов
option(BUILD_TESTS "Build tests with Catch2 v3" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Подключение Catch2 v3 для тестов
if(BUILD_TESTS)

    enable_testing()
    find_package(Catch2 3 QUIET)
    if(Catch2_FOUND)
        add_subdirectory(tests)
    else()
        message(WARNING "Catch2 v3 not found - tests disabled")
    endif()

endif()

# Санитизаторы для отладки

if(ENABLE_SANITIZERS AND NOT MSVC)

    target_compile_options(perception_modules PRIVATE 
        -fsanitize=address -fsanitize=undefined
    )

    target_link_options(perception_modules PRIVATE 
        -fsanitize=address -fsanitize=undefined
    )

    target_compile_options(perception_demo PRIVATE 
        -fsanitize=address -fsanitize=undefined
    )

    target_link_options(perception_demo PRIVATE 
        -fsanitize=address -fsanitize=undefined
    )

endif()

# Code coverage
if(ENABLE_COVERAGE AND NOT MSVC)

    target_compile_options(perception_modules PRIVATE --coverage)
    target_link_options(perception_modules PRIVATE --coverage)

endif()

# Установка целей

install(TARGETS perception_demo perception_modules
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)