cmake_minimum_required(VERSION 3.28) # Требуется для стабильной поддержки модулей в MSVC 
project(MAV0ModulesDemo VERSION 1.0.0 LANGUAGES CXX)

# Настройка стандарта C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включаем автоматическое сканирование зависимостей модулей
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Поиск OpenCV
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
else()
    message(FATAL_ERROR "OpenCV not found")
endif()

# Создаем исполняемый файл
add_executable(mav0_modules_demo ../src/mav0_modules_demo.cpp)

# Сначала добавляем обычные файлы исходного кода
target_sources(mav0_modules_demo PRIVATE 
    ../src/mav0_modules_demo.cpp
)

# Затем отдельным вызовом (или после нового спецификатора) добавляем модули
target_sources(mav0_modules_demo PRIVATE 
    FILE_SET CXX_MODULES 
    TYPE CXX_MODULES 
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/.." 
    FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/core/benchmark.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/core/thread_safe_queue.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/vision/detector.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/vision/tracker.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/vision/fusion.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/io/imu_simulator.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/core/metrics.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/../modules/vision_detector.cppm"
        "${CMAKE_CURRENT_SOURCE_DIR}/../modules/vision_tracker.cppm"
        "${CMAKE_CURRENT_SOURCE_DIR}/../modules/vision_fusion.cppm"
)

# Директории включения
target_include_directories(mav0_modules_demo PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../modules
) 

# Линковка библиотек
target_link_libraries(mav0_modules_demo PRIVATE 
    ${OpenCV_LIBS}
)

# Специфичные настройки для компиляторов
if(MSVC)
    target_compile_options(mav0_modules_demo PRIVATE 
        /W4                     # Высокий уровень предупреждений
        /MD                     # Использование многопоточной DLL
        /O2                     # Оптимизация скорости
        /EHsc                   # Модель обработки исключений
        /D_CRT_SECURE_NO_WARNINGS
    )
    
    # Макросы для Windows
    target_compile_definitions(mav0_modules_demo PRIVATE 
        _WIN32_WINNT=0x0A00     # Windows 10+
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
else()
    # Настройки для GCC/Clang
    target_compile_options(mav0_modules_demo PRIVATE 
        -Wall -Wextra -O2 -pthread
    )
endif()

message(STATUS "=== Configuration Complete ===")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")