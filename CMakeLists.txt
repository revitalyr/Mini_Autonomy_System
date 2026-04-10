cmake_minimum_required(VERSION 3.28)
project(MiniAutonomySystem LANGUAGES CXX VERSION 1.0.0)

# Настройка C++23 стандарта с максимальными современными фичами
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включаем экспериментальную поддержку модулей C++20/23
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
    set(CMAKE_EXPERIMENTAL_CXX_MODULE_CXX23_FLAG ON)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
    set(MODULES_SUPPORT ON)
    message(STATUS "C++26 modules support enabled")
else()
    set(MODULES_SUPPORT OFF)
    message(WARNING "C++ modules not supported - using traditional includes")
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

# Поиск необходимых пакетов (OpenCV и потоки)
find_package(OpenCV REQUIRED CONFIG)
find_package(Threads REQUIRED)

# Создание библиотеки модулей
add_library(perception_modules)

# Добавляем OpenCV include directories
target_include_directories(perception_modules PUBLIC ${OpenCV_INCLUDE_DIRS})

# Линкуем OpenCV к модулям
target_link_libraries(perception_modules PUBLIC ${OpenCV_LIBS})

target_sources(perception_modules PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES 
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/modules"
    FILES
        modules/perception.concepts.cppm
        modules/perception.result.cppm
        modules/perception.async.cppm
        modules/perception.queue.cppm
        modules/perception.metrics.cppm
        modules/perception.image_loader.simple.cppm
        modules/perception.detector.simple.cppm
        modules/perception.pipeline.cppm
)

# Создание исполняемого файла
add_executable(perception_demo src/main.cpp)

# Создание минимального теста
add_executable(minimal_test src/main_minimal.cpp)

# Линковка
target_link_libraries(perception_demo PRIVATE 
    perception_modules 
    ${OpenCV_LIBS} 
    Threads::Threads
)

# Настройка путей включения
target_include_directories(perception_demo PRIVATE include)

# Современные настройки компилятора
if(MSVC)
    # MSVC настройки с C++23
    target_compile_options(perception_modules PRIVATE
        /W4                    # Высокий уровень предупреждений
        /EHsc                  # Обработка исключений C++
        /permissive-           # Строгий стандарт C++
        /std:c++23           # C++23 стандарт
        /Zc:__cplusplus        # Правильный __cplusplus
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot>   # Максимальная оптимизация в Release
        $<$<CONFIG:Debug>:/Zi /Od>             # Отладочная информация в Debug
    )
    
    target_compile_options(perception_demo PRIVATE
        /W4
        /EHsc
        /permissive-
        /std:c++23
        /Zc:__cplusplus
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Zi>
    )
    
    # Современные определения для MSVC
    target_compile_definitions(perception_demo PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_CXX23_OLD_DEPRECATION_WARNINGS
    )
else()
    # GCC/Clang настройки с C++23
    target_compile_options(perception_modules PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -std=c++23
        -fmodules-ts
        $<$<CONFIG:Release>:-O3 -march=native -flto>
        $<$<CONFIG:Debug>:-g -O0>
    )
    
    target_compile_options(perception_demo PRIVATE
        -Wall -Wextra -Wpedantic
        -std=c++23
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g>
    )
endif()

# Добавление современных библиотек и инструментов
option(BUILD_TESTS "Build tests with Google Test" ON)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Подключение Google Test для тестов
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)
    if(GTest_FOUND)
        add_subdirectory(tests)
    else()
        message(WARNING "Google Test not found - tests disabled")
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