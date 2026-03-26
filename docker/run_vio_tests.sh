#!/bin/bash

# Скрипт для запуска тестов на верифицированных датасетах
set -e

echo "🚀 VIO-lite Benchmarking System"
echo "==============================="

DATASETS_ROOT="./datasets"
CALIBRATION_ZIP_PATH="./data/calibration_datasets.zip"
RESULTS_DIR="./test_results"
mkdir -p "$RESULTS_DIR"

# Проверка наличия утилиты 'unzip' на хост-системе
if ! command -v unzip &> /dev/null
then
    echo "❌ Утилита 'unzip' не найдена на вашей хост-системе."
    echo "Пожалуйста, установите её (например, 'sudo apt-get install unzip' на Ubuntu/Debian)."
    exit 1
fi

# Проверка и распаковка калибровочных датасетов
if [ -f "$CALIBRATION_ZIP_PATH" ]; then
    echo "📦 Найден архив с калибровочными датасетами: $CALIBRATION_ZIP_PATH"
    if [ -z "$(ls -A $DATASETS_ROOT 2>/dev/null)" ]; then
        echo "📂 Директория '$DATASETS_ROOT' пуста. Распаковываем датасеты..."
        mkdir -p "$DATASETS_ROOT" # Убедимся, что целевая директория существует
        unzip -q "$CALIBRATION_ZIP_PATH" -d "$DATASETS_ROOT"
        echo "✅ Датасеты успешно распакованы в '$DATASETS_ROOT'."
    else
        echo "⚠️ Директория '$DATASETS_ROOT' не пуста. Пропускаем автоматическую распаковку."
    fi
else
    echo "ℹ️ Архив '$CALIBRATION_ZIP_PATH' не найден. Пропускаем автоматическую распаковку."
    echo "Пожалуйста, скачайте и поместите его в '$CALIBRATION_ZIP_PATH' или распакуйте датасеты вручную в '$DATASETS_ROOT'."
fi

run_test() {
    local NAME=$1
    local PATH_IN_CONTAINER="/workspace/docker/datasets/$2"
    local CONFIG=$3

    echo "------------------------------------------------"
    echo "🧪 Запуск теста: $NAME"
    echo "📂 Путь: $PATH_IN_CONTAINER"
    echo "⚙️ Конфиг: $CONFIG"
    
    # Формируем аргументы для нашего C++ приложения
    # Если в папке 'data' лежат изображения, OpenCV может открыть их через маску (например, %06d.png)
    # Для простоты передаем путь к папке или файлу
    export DEMO_ARGS="--config /workspace/docker/test_configs/$CONFIG --input $PATH_IN_CONTAINER"
    
    # Запуск через docker-compose и сохранение логов
    docker-compose --profile demo up 2>&1 | tee "$RESULTS_DIR/${NAME// /_}.log"
    
    echo "✅ Тест $NAME завершен."
}

# Автоматический обход всех датасетов в распакованной папке
echo "🔍 Поиск доступных датасетов в $DATASETS_ROOT..."

for d in "$DATASETS_ROOT"/*/; do
    # Пропускаем, если папок нет
    [ -e "$d" ] || continue
    
    DIR_NAME=$(basename "$d")
    
    # 1. Определяем источник данных (предпочтение папке с кадрами 'data')
    if [ -d "${d}data" ]; then
        SOURCE_PATH="${DIR_NAME}/data"
    else
        # Ищем первый попавшийся .bag файл
        BAG_FILE=$(find "$d" -maxdepth 1 -name "*.bag" -print -quit)
        if [ -n "$BAG_FILE" ]; then
            SOURCE_PATH="${DIR_NAME}/$(basename "$BAG_FILE")"
        else
            echo "⚠️ В папке $DIR_NAME не найдено ни папки 'data', ни .bag файла. Пропускаем."
            continue
        fi
    fi

    # 2. Ищем специфичный конфиг или используем дефолтный
    CONFIG_FILE="default_config.txt"
    if [ -f "./test_configs/${DIR_NAME}.txt" ]; then
        CONFIG_FILE="${DIR_NAME}.txt"
    elif [ -f "./test_configs/config_vio.txt" ]; then
        CONFIG_FILE="config_vio.txt"
    fi

    run_test "$DIR_NAME" "$SOURCE_PATH" "$CONFIG_FILE"
done

echo ""
echo "📊 Все тесты завершены. Проверь логи в $RESULTS_DIR для оценки дрифта и FPS."
```

### Как это использовать:
1. **EuRoC:** Скачай ZIP, распакуй его в `docker/datasets/EuRoC/MH_01_easy/`.
2. **Конфиг:** Создай в `docker/test_configs/euroc_config.txt` параметры, соответствующие калибровке из датасета (фокусное расстояние, параметры шума IMU).
3. **Запуск:**
  ```bash
  chmod +x run_vio_tests.sh
  ./run_vio_tests.sh
