// Модуль с определениями всех диаграмм
const Diagrams = {
    // Зависимости модулей
    modules: `graph TD
        Main[<a href="../src/main.cpp" target="_blank" style="color:#4a90e2;text-decoration:none;">main.cpp</a>] --> Pipeline[<a href="../modules/perception.pipeline.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">perception.pipeline</a>]
        Pipeline --> Async[<a href="../modules/perception.async.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">perception.async</a>]
        Pipeline --> Concepts[<a href="../modules/perception.concepts.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">perception.concepts</a>]
        Pipeline --> Result[<a href="../modules/perception.result.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">perception.result</a>]
        Pipeline --> ImageLoader[<a href="../modules/perception.image_loader.simple.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">perception.image_loader</a>]
        Pipeline --> Detector[<a href="../modules/perception.detector.simple.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">perception.detector</a>]
        Pipeline --> Queue[<a href="../modules/perception.queue.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">perception.queue</a>]
        Pipeline --> Metrics[<a href="../modules/perception.metrics.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">perception.metrics</a>]`,

    // Конвейер (Pipeline)
    pipeline: `graph LR
        Input((<a href="../src/main.cpp#L85" target="_blank" style="color:#e65100;text-decoration:none;">Камера</a>)) --> 
        Loader[<a href="../modules/perception.image_loader.simple.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">Image Loader</a>]
        Loader --> 
        Pre[<a href="../modules/perception.detector.simple.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">MockDetector</a>]
        Pre --> 
        Detect[<a href="../modules/perception.detector.simple.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">Detection</a>]
        Detect --> 
        Track[<a href="../modules/perception.detector.simple.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">Result</a>]
        Track --> 
        UI((<a href="../src/main.cpp#L120" target="_blank" style="color:#880e4f;text-decoration:none;">Визуализация</a>))`,

    // Асинхронная модель
    async: `graph TB
        subgraph "Асинхронная архитектура"
            MainThread[<a href="../src/main.cpp" target="_blank" style="color:#ff6f00;text-decoration:none;">Главный поток</a>] --> 
            ThreadPool[<a href="../modules/perception.async.cppm" target="_blank" style="color:#01579b;text-decoration:none;">ThreadPool</a>]
            
            ThreadPool --> 
            Worker1[<a href="../modules/perception.async.cppm" target="_blank" style="color:#01579b;text-decoration:none;">Worker 1</a>]
            ThreadPool --> 
            Worker2[<a href="../modules/perception.async.cppm" target="_blank" style="color:#01579b;text-decoration:none;">Worker 2</a>]
            ThreadPool --> 
            Worker3[<a href="../modules/perception.async.cppm" target="_blank" style="color:#01579b;text-decoration:none;">Worker 3</a>]
            
            Worker1 --> 
            Queue1[<a href="../modules/perception.queue.cppm" target="_blank" style="color:#1b5e20;text-decoration:none;">ThreadSafeQueue</a>]
            Worker2 --> 
            Queue2[<a href="../modules/perception.queue.cppm" target="_blank" style="color:#1b5e20;text-decoration:none;">ThreadSafeQueue</a>]
            Worker3 --> 
            Queue3[<a href="../modules/perception.queue.cppm" target="_blank" style="color:#1b5e20;text-decoration:none;">ThreadSafeQueue</a>]
            
            Queue1 --> 
            Results[<a href="../modules/perception.result.cppm" target="_blank" style="color:#880e4f;text-decoration:none;">Result</a>]
            Queue2 --> Results
            Queue3 --> Results
        end
        
        classDef main fill:#ffecb3,stroke:#ff6f00,stroke-width:2px
        classDef worker fill:#e1f5fe,stroke:#01579b,stroke-width:2px
        classDef queue fill:#e8f5e8,stroke:#1b5e20,stroke-width:2px
        classDef result fill:#fce4ec,stroke:#880e4f,stroke-width:2px
        
        class MainThread main
        class ThreadPool,Worker1,Worker2,Worker3 worker
        class Queue1,Queue2,Queue3 queue
        class Results result`,

    // Диаграмма классов
    classes: `classDiagram
        class PerceptionError {
            <<enumeration>>
            Success
            InvalidInput
            ModelLoadFailed
            InferenceFailed
            QueueEmpty
            QueueShutdown
            ThreadError
            CameraError
        }
        
        class Result {
            -m_value: variant
            +Result()
            +Result(T value)
            +has_value() bool
            +value() T
            +error() PerceptionError
            +operator bool() bool
        }
        
        class Detection {
            +bbox: cv::Rect
            +confidence: float
            +class_id: int
            +class_name: string
            +Detection(cv::Rect, float, int, string)
        }
        
        class MockDetector {
            -m_class_names: vector
            +MockDetector()
            +detect(frame) vector
        }
        
        class Image {
            +mat: cv::Mat
            +Image()
            +Image(cv::Mat m)
            +width() int
            +height() int
            +channels() int
            +empty() bool
        }
        
        class Generator {
            <<template T>>
            +get_return_object()
            +initial_suspend()
            +final_suspend()
            +unhandled_exception()
        }
        
        class ThreadPool {
            -workers_: vector
            -tasks_: queue
            -mutex_: mutex
            +ThreadPool(num_threads)
            +enqueue(f)
            +submit(f) future
        }
        
        class ThreadSafeQueue {
            -queue_: queue
            -mutex_: mutex
            -condition_: condition_variable_any
            -semaphore_: binary_semaphore
            -shutdown_: atomic
            +ThreadSafeQueue()
            +push(U value)
            +pop(stop_token) optional
            +try_pop() optional
            +shutdown()
            +empty() bool
            +size() size_t
            +is_shutdown() bool
        }
        
        class PerformanceMetrics {
            -frame_count_: atomic
            -total_latency_ms_: atomic
            -min_latency_ms_: atomic
            -max_latency_ms_: atomic
            -start_time_: time_point
            -recent_latencies_mutex_: mutex
            -recent_latencies_: vector
            +record_frame_latency(latency_ms)
            +get_snapshot() MetricsSnapshot
            +get_fps() double
        }
        
        MockDetector --> Detection
        Image --> Generator
        Result --> Detection
        ThreadPool --> ThreadSafeQueue
        ThreadSafeQueue --> Result
        PerformanceMetrics --> Result
        
        click PerceptionError href "../modules/perception.result.cppm" _blank
        click Result href "../modules/perception.result.cppm" _blank
        click Detection href "../modules/perception.detector.simple.cppm" _blank
        click MockDetector href "../modules/perception.detector.simple.cppm" _blank
        click Image href "../modules/perception.image_loader.simple.cppm" _blank
        click Generator href "../modules/perception.image_loader.simple.cppm" _blank
        click ThreadPool href "../modules/perception.async.cppm" _blank
        click ThreadSafeQueue href "../modules/perception.queue.cppm" _blank
        click PerformanceMetrics href "../modules/perception.metrics.cppm" _blank`,

    // Поток данных
    dataflow: `graph TB
        A[<a href="../src/main.cpp#L92" target="_blank" style="color:#e65100;text-decoration:none;">Камера</a>] --> B[<a href="../modules/perception.detector.simple.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">Обработка</a>]
        B --> C[<a href="../modules/perception.queue.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">Очередь</a>]
        C --> D[<a href="../modules/perception.detector.simple.cppm" target="_blank" style="color:#4a90e2;text-decoration:none;">Детекция</a>]
        D --> E[<a href="../src/main.cpp#L98" target="_blank" style="color:#880e4f;text-decoration:none;">Результат</a>]
        
        classDef input fill:#fff3e0,stroke:#e65100,stroke-width:2px
        classDef processing fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
        classDef output fill:#fce4ec,stroke:#880e4f,stroke-width:2px
        
        class A input
        class B,C,D processing
        class E output`,

    // Потоки
    threads: `graph TB
        Main[<a href="../src/main.cpp" target="_blank" style="color:#ff6f00;text-decoration:none;">Главный поток</a>] --> Worker1[<a href="../modules/perception.async.cppm" target="_blank" style="color:#01579b;text-decoration:none;">Рабочий поток 1</a>]
        Main --> Worker2[<a href="../modules/perception.async.cppm" target="_blank" style="color:#01579b;text-decoration:none;">Рабочий поток 2</a>]
        Main --> Worker3[<a href="../modules/perception.async.cppm" target="_blank" style="color:#01579b;text-decoration:none;">Рабочий поток 3</a>]
        
        Worker1 --> Queue1[<a href="../modules/perception.queue.cppm" target="_blank" style="color:#1b5e20;text-decoration:none;">Очередь 1</a>]
        Worker2 --> Queue2[<a href="../modules/perception.queue.cppm" target="_blank" style="color:#1b5e20;text-decoration:none;">Очередь 2</a>]
        Worker3 --> Queue3[<a href="../modules/perception.queue.cppm" target="_blank" style="color:#1b5e20;text-decoration:none;">Очередь 3</a>]
        
        Queue1 --> Result[<a href="../modules/perception.result.cppm" target="_blank" style="color:#880e4f;text-decoration:none;">Результат</a>]
        Queue2 --> Result
        Queue3 --> Result
        
        classDef main fill:#ffecb3,stroke:#ff6f00,stroke-width:2px
        classDef worker fill:#e1f5fe,stroke:#01579b,stroke-width:2px
        classDef queue fill:#e8f5e8,stroke:#1b5e20,stroke-width:2px
        
        class Main main
        class Worker1,Worker2,Worker3 worker
        class Queue1,Queue2,Queue3 queue
        class Result output`,

    // Производительность
    performance: `graph LR
        Input[<a href="../src/main.cpp#L85" target="_blank" style="color:#e65100;text-decoration:none;">Входные данные</a>] --> Process[<a href="../modules/perception.detector.simple.cppm" target="_blank" style="color:#1565c0;text-decoration:none;">Обработка</a>]
        Process --> Metrics[<a href="../modules/perception.metrics.cppm" target="_blank" style="color:#33691e;text-decoration:none;">Сбор метрик</a>]
        Metrics --> Output[<a href="../src/main.cpp#L120" target="_blank" style="color:#880e4f;text-decoration:none;">Вывод</a>]
        
        classDef input fill:#fff3e0,stroke:#e65100,stroke-width:2px
        classDef process fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
        classDef metrics fill:#e8f5e8,stroke:#33691e,stroke-width:2px
        classDef output fill:#fce4ec,stroke:#880e4f,stroke-width:2px
        
        class Input input
        class Process process
        class Metrics metrics
        class Output output`
};
