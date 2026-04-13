cmake_minimum_required(VERSION 3.28)

project(MiniAutonomySystem LANGUAGES CXX VERSION 1.0.0)



set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)



# Включаем экспериментальную поддержку модулей C++20/23 (после поиска пакетов)
# Только для Clang, так как GCC не поддерживает CMAKE_CXX_SCAN_FOR_MODULES
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28" AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
    set(MODULES_SUPPORT ON)
    message(STATUS "C++20 modules enabled")
else()
    set(MODULES_SUPPORT OFF)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(WARNING "GCC detected - C++ modules not supported with CMAKE_CXX_SCAN_FOR_MODULES")
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



# Поиск необходимых пакетов (OpenCV, ROS, потоки)

find_package(OpenCV REQUIRED CONFIG)

find_package(Threads REQUIRED)



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

# Add include directories for OpenCV (needed for C++ modules)
target_include_directories(perception_modules PUBLIC ${OpenCV_INCLUDE_DIRS})

if(MODULES_SUPPORT)

    target_sources(perception_modules PUBLIC

        FILE_SET CXX_MODULES TYPE CXX_MODULES

        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"

        FILES

            include/perception/types.ixx

            include/perception/concepts.ixx

            include/perception/result.ixx

            include/perception/async.ixx

            include/perception/queue.ixx

            include/perception/metrics.ixx

            include/perception/image_loader.ixx

            include/perception/detector.ixx

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



# Создание исполняемого файла

add_executable(perception_demo src/main.cpp)



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

endif()



if(ROS_FOUND)

    target_include_directories(rosbag_demo PRIVATE include)

    target_include_directories(rosbag_demo PRIVATE ${rclcpp_INCLUDE_DIRS} ${cv_bridge_INCLUDE_DIRS} ${sensor_msgs_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS})

    target_include_directories(rosbag_viz_demo PRIVATE include)

    target_include_directories(rosbag_viz_demo PRIVATE ${rclcpp_INCLUDE_DIRS} ${cv_bridge_INCLUDE_DIRS} ${sensor_msgs_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS})

    target_include_directories(rosbag_viz_demo_headless PRIVATE include)

    target_include_directories(rosbag_viz_demo_headless PRIVATE ${rclcpp_INCLUDE_DIRS} ${cv_bridge_INCLUDE_DIRS} ${sensor_msgs_INCLUDE_DIRS} ${std_msgs_INCLUDE_DIRS})

endif()



# Современные настройки компилятора

if(MSVC)

    # MSVC настройки с C++20

    target_compile_options(perception_modules PRIVATE

        /W4                    # Высокий уровень предупреждений

        /EHsc                  # Обработка исключений C++

        /permissive-           # Строгий стандарт C++

        /std:c++latest         # C++ стандарт

        /Zc:__cplusplus        # Правильный __cplusplus

        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot>   # Максимальная оптимизация в Release

        $<$<CONFIG:Debug>:/Zi /Od>             # Отладочная информация в Debug

    )

    

    target_compile_options(perception_demo PRIVATE

        /W4

        /EHsc

        /permissive-

        /std:c++latest

        /Zc:__cplusplus

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