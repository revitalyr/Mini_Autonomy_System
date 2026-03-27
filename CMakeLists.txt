cmake_minimum_required(VERSION 3.28)
project(MiniAutonomySystem LANGUAGES CXX)

# Включаем экспериментальную поддержку модулей C++20
# set(CMAKE_EXPERIMENTAL_CXX_MODULE_CXX23_FLAG ON)

# Установка стандарта C++20 (или 20, в зависимости от ваших нужд)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включаем сканирование модулей
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
    set(MODULES_SUPPORT ON)
else()
    set(MODULES_SUPPORT OFF)
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
find_package(OpenCV REQUIRED)
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

# Линковка
target_link_libraries(perception_demo PRIVATE 
    perception_modules 
    ${OpenCV_LIBS} 
    Threads::Threads
)

# Настройка путей включения
target_include_directories(perception_demo PRIVATE include)

# --- ИСПРАВЛЕНИЕ ОШИБКИ MSVC ---
if(MSVC)
    # Удаляем жесткое прописывание /O2 для всех конфигураций
    # Вместо этого используем генераторные выражения, чтобы флаги не конфликтовали
    target_compile_options(perception_modules PRIVATE 
        /W4 
        /EHsc 
        $<$<CONFIG:Release>:/O2>   # /O2 только в Release 
        $<$<CONFIG:Debug>:/Zi>    # Инфо для отладки в Debug
    )
    
    target_compile_options(perception_demo PRIVATE 
        /W4 
        /EHsc 
        $<$<CONFIG:Release>:/O2>   # /O2 только в Release 
    )

    target_compile_definitions(perception_demo PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(perception_modules PRIVATE -Wall -Wextra -O3)
    target_compile_options(perception_demo PRIVATE -Wall -Wextra -O3)
endif()