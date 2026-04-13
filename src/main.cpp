/**
 * @file main.cpp
 * @brief Demo application for the Mini Autonomy System
 *
 * Demonstrates object detection, image processing, and async pipeline
 * functionality using motion-based detection and batch image loading.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <ranges>
#include <coroutine>
#include <chrono>
#include <thread>
#include <format>
#include <utility>
#include <memory>
#include <filesystem>
#include <vector>
#include <algorithm>

import perception.concepts;
import perception.queue;
import perception.pipeline;
import perception.metrics;
import perception.detector;
import perception.async;
import perception.result;
import perception.types;

namespace perception {

// ============================================================================
// IMAGE LOADING AND GENERATION
// ============================================================================

/**
 * Coroutine generator for lazy sequence streaming
 */
template<typename T>
struct Generator {
    struct promise_type {
        T m_value{};
        auto get_return_object() -> Generator {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        static auto initial_suspend() -> std::suspend_always { return {}; }
        static auto final_suspend() noexcept -> std::suspend_always { return {}; }
        auto yield_value(T value) -> std::suspend_always {
            m_value = std::move(value);
            return {};
        }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> m_handle;

    explicit Generator(std::coroutine_handle<promise_type> handle) : m_handle{handle} {}
    ~Generator() { if (m_handle) m_handle.destroy(); }
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& other) noexcept : m_handle{std::exchange(other.m_handle, nullptr)} {}
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (m_handle) m_handle.destroy();
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }

    struct Iterator {
        std::coroutine_handle<promise_type> m_handle;
        bool operator==(std::default_sentinel_t) const { return !m_handle || m_handle.done(); }
        auto operator++() -> void { if (m_handle) m_handle.resume(); }
        auto operator*() const -> const T& { return m_handle.promise().m_value; }
    };

    auto begin() -> Iterator {
        if (m_handle && !m_handle.done()) m_handle.resume();
        return Iterator{m_handle};
    }
    auto end() -> std::default_sentinel_t { return {}; }
};

/**
 * Simple image loader using OpenCV
 * @param path Path to image file
 * @return ImageData or nullopt if loading failed
 */
inline auto load_image_file(const std::filesystem::path& path) -> std::optional<ImageData> {
    cv::Mat img = cv::imread(path.string(), cv::IMREAD_COLOR);
    if (img.empty()) {
        return std::nullopt;
    }
    
    // Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(img, rgb, cv::COLOR_BGR2RGB);
    
    ImageData data;
    data.width = rgb.cols;
    data.height = rgb.rows;
    data.channels = rgb.channels();
    data.data.assign(rgb.data, rgb.data + (rgb.total() * rgb.channels()));
    
    return data;
}

/**
 * Generator for streaming image frames from directory
 * @param directory Path to directory containing images
 * @return Generator yielding loaded image data
 */
auto load_images_from_directory(const std::filesystem::path& directory) -> Generator<ImageData> {
    if (!std::filesystem::exists(directory)) {
        co_return;
    }
    
    std::vector<std::filesystem::path> image_files;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
                image_files.push_back(entry.path());
            }
        }
    }
    
    std::sort(image_files.begin(), image_files.end());
    
    for (const auto& path : image_files) {
        if (auto img = load_image_file(path)) {
            co_yield std::move(*img);
        }
    }
}

/**
 * Generate test image frames with gradient pattern
 * @param count Number of frames to generate
 * @return Generator yielding image data frames
 */
auto generate_test_frames(size_t count = 10) -> Generator<ImageData> {
    for (size_t i = 0; i < count; ++i) {
        ImageData frame{480, 640, 3};

        // Fill with test pattern (gradient)
        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                int idx = (y * frame.width + x) * frame.channels;
                uint8_t value = static_cast<uint8_t>((x + y + i * 10) % 256);
                frame.data[idx] = value;         // R
                frame.data[idx + 1] = value;     // G
                frame.data[idx + 2] = value;     // B
            }
        }

        co_yield frame;
    }
}

// ============================================================================
// ASYNC PROCESSING PIPELINE
// ============================================================================

/**
 * Pipeline for asynchronous image processing and object detection
 */
class AsyncProcessingPipeline {
private:
    UniquePtr<ThreadSafeQueue<ImageData>> m_image_queue;
    UniquePtr<ThreadSafeQueue<Vector<Detection>>> m_result_queue;
    UniquePtr<ThreadPool> m_thread_pool;
    UniquePtr<PerformanceMetrics> m_metrics;
    UniquePtr<Detector> m_detector;
    Atomic<bool> m_is_running{false};

public:
    explicit AsyncProcessingPipeline(const Config& config = Config{})
        : m_image_queue{make_unique<ThreadSafeQueue<ImageData>>()}
        , m_result_queue{make_unique<ThreadSafeQueue<Vector<Detection>>>()}
        , m_thread_pool{make_unique<ThreadPool>(config.thread_pool_size)}
        , m_metrics{make_unique<PerformanceMetrics>()}
        , m_detector{make_unique<Detector>()} {}

    AsyncProcessingPipeline(const AsyncProcessingPipeline&) = delete;
    auto operator=(const AsyncProcessingPipeline&) -> AsyncProcessingPipeline& = delete;

    AsyncProcessingPipeline(AsyncProcessingPipeline&&) = delete;
    auto operator=(AsyncProcessingPipeline&&) -> AsyncProcessingPipeline& = delete;

    ~AsyncProcessingPipeline() = default;

    /**
     * Start the processing pipeline
     */
    auto start() noexcept -> Result<void> {
        try {
            m_is_running.store(true, std::memory_order_release);

            for (size_t i = 0; i < m_thread_pool->get_thread_count(); ++i) {
                m_thread_pool->submit([this] {
                    processing_loop();
                });
            }

            return {};
        } catch (const std::exception&) {
            return Result<void>(std::error_code(static_cast<int>(PerceptionError::ThreadError), std::generic_category()));
        }
    }

    /**
     * Stop the processing pipeline
     */
    auto stop() noexcept -> void {
        m_is_running.store(false, std::memory_order_release);
        m_image_queue->shutdown();
        m_result_queue->shutdown();
    }

    /**
     * Submit an image for processing
     * @param image Image data to process
     */
    auto process_image(ImageData image) noexcept -> Result<void> {
        if (!m_is_running.load(std::memory_order_acquire)) {
            return Result<void>(std::error_code(static_cast<int>(PerceptionError::QueueShutdown), std::generic_category()));
        }

        try {
            m_image_queue->push(std::move(image));
            return {};
        } catch (const std::exception&) {
            return Result<void>(std::error_code(static_cast<int>(PerceptionError::QueueEmpty), std::generic_category()));
        }
    }

    /**
     * Get detection results
     * @param timeout Maximum time to wait for results
     * @return Detected objects or error
     */
    auto get_results(Milliseconds timeout = Milliseconds{1000}) noexcept
        -> Result<Vector<Detection>> {

        if (auto result = m_result_queue->try_pop()) {
            return std::move(*result);
        }

        if (auto result = m_result_queue->pop_timeout(timeout)) {
            return std::move(*result);
        }

        return Result<Vector<Detection>>(std::error_code(static_cast<int>(PerceptionError::QueueEmpty), std::generic_category()));
    }

    /**
     * Get performance metrics
     */
    auto get_metrics() const noexcept -> PerformanceMetrics::Snapshot {
        return m_metrics->get_snapshot();
    }

private:
    auto processing_loop() noexcept -> void {
        while (m_is_running.load(std::memory_order_acquire)) {
            if (auto image = m_image_queue->try_pop()) {
                auto start_time = std::chrono::high_resolution_clock::now();

                auto detections_result = m_detector->detect(*image);

                auto end_time = std::chrono::high_resolution_clock::now();
                auto latency = to_milliseconds(end_time - start_time);

                m_metrics->record_frame_latency(static_cast<double>(latency.count()));

                m_result_queue->push(std::move(detections_result));
            } else {
                (void)std::this_thread::sleep_for(Milliseconds{1});
            }
        }
    }
};

// ============================================================================
// DEMO FUNCTIONS
// ============================================================================

/**
 * Demo: Filter detections by confidence threshold
 */
auto demo_ranges_and_concepts() noexcept -> Result<void> {
    try {
        perception::Vector<Detection> test_detections{
            Detection{Rect{10, 10, 50, 50}, 0.9f, 1, "person"},
            Detection{Rect{100, 100, 80, 80}, 0.7f, 2, "car"},
            Detection{Rect{200, 200, 30, 30}, 0.3f, 3, "bicycle"},
            Detection{Rect{300, 300, 60, 60}, 0.8f, 1, "person"}
        };

        auto high_confidence_detections = test_detections
            | std::views::filter(filter_by_confidence(0.5))
            | std::views::take(3);

        std::cout << "=== Detection Filtering Demo ===\n";
        std::cout << "Total detections: " << test_detections.size() << "\n";
        std::cout << "High confidence detections (>= 0.5):\n";

        for (const auto& detection : high_confidence_detections) {
            float aspect_ratio = static_cast<float>(detection.bbox.width) / detection.bbox.height;
            float area = detection.bbox.width * detection.bbox.height;

            std::cout << "  - " << detection.class_name << ": confidence=" 
                      << std::format("{:.2f}", detection.confidence) << "\n";
            std::cout << "    Bounding box: (" << detection.bbox.x << ", " << detection.bbox.y 
                      << ", " << detection.bbox.width << "x" << detection.bbox.height << ")\n";
            std::cout << "    Aspect ratio: " << std::format("{:.2f}", aspect_ratio) << "\n";
            std::cout << "    Area: " << std::format("{:.0f}", area) << " pixels\n";
            
            // Explain classification reasoning
            std::cout << "    Classification reasoning: ";
            if (detection.class_name == "person") {
                std::cout << "Tall narrow object (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                          << ", area " << std::format("{:.0f}", area) << ")\n";
            } else if (detection.class_name == "car") {
                std::cout << "Wide object with large area (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                          << ", area " << std::format("{:.0f}", area) << ")\n";
            } else if (detection.class_name == "bicycle") {
                std::cout << "Medium-sized object (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                          << ", area " << std::format("{:.0f}", area) << ")\n";
            } else {
                std::cout << "Generic object (doesn't match specific categories)\n";
            }
        }

        return {};
    } catch (const std::exception&) {
        return Result<void>(std::error_code(static_cast<int>(PerceptionError::InvalidInput), std::generic_category()));
    }
}

/**
 * Find demo data directory from various possible locations
 */
auto find_demo_data_directory() -> std::filesystem::path {
    std::vector<std::filesystem::path> possible_paths = {
        "demo/data",                    // From project root
        "../demo/data",                 // From build directory
        "../../demo/data",              // From nested build directory
    };
    
    for (const auto& path : possible_paths) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            return path;
        }
    }
    
    return {};  // Empty path if not found
}

/**
 * Demo: Load and process actual JPG images from demo/data
 */
auto demo_coroutines() noexcept -> Result<void> {
    try {
        std::cout << "=== Image Loading Demo ===\n";
        std::cout << "Loading images from demo/data/...\n";

        auto image_path = find_demo_data_directory();
        
        if (image_path.empty()) {
            std::cout << "Demo data directory not found, falling back to generated frames\n";
            auto frame_generator = generate_test_frames(5);
            size_t frame_count = 0;
            for (const auto& frame : frame_generator) {
                ++frame_count;
                std::cout << "Generated frame " << frame_count << " (" 
                          << frame.width << "x" << frame.height << ")\n";
            }
            return {};
        }

        auto image_generator = load_images_from_directory(image_path);
        
        size_t frame_count = 0;
        for (const auto& frame : image_generator) {
            ++frame_count;
            std::cout << "Loaded image " << frame_count << " (" 
                      << frame.width << "x" << frame.height << ", "
                      << frame.channels << " channels)\n";
        }
        
        std::cout << "Total images loaded: " << frame_count << "\n";

        return {};
    } catch (const std::exception&) {
        return Result<void>(std::error_code(static_cast<int>(PerceptionError::InvalidInput), std::generic_category()));
    }
}

/**
 * Demo: Async object detection pipeline with real images
 */
auto demo_async_processing() noexcept -> Result<void> {
    try {
        std::cout << "=== Async Detection Pipeline Demo (Real Images) ===\n";

        auto pipeline = AsyncProcessingPipeline{};

        if (auto result = pipeline.start(); !result) {
            return result;
        }

        // Try to load real images from demo/data
        auto image_path = find_demo_data_directory();
        size_t frames_processed = 0;
        
        if (!image_path.empty()) {
            auto image_generator = load_images_from_directory(image_path);
            
            for (const auto& frame : image_generator) {
                ImageData frame_copy = frame;
                if (auto process_result = pipeline.process_image(std::move(frame_copy)); !process_result) {
                    pipeline.stop();
                    return process_result;
                }
                ++frames_processed;
                if (frames_processed >= 6) break; // Process up to 6 images
            }
        }
        
        // Fall back to generated frames if no real images were loaded
        if (frames_processed == 0) {
            std::cout << "No real images found, using generated test frames\n";
            for (const auto& frame : generate_test_frames(3)) {
                ImageData frame_copy = frame;
                if (auto result = pipeline.process_image(std::move(frame_copy)); !result) {
                    pipeline.stop();
                    return result;
                }
                ++frames_processed;
            }
        }
        
        std::cout << "Processing " << frames_processed << " frames...\n";

        for (int i = 0; i < 3; ++i) {
            if (auto results = pipeline.get_results(); results) {
                const auto& detections = results.value();
                std::cout << "Frame " << (i + 1) << ": " << detections.size() << " detections\n";
                
                for (const auto& detection : detections) {
                    float aspect_ratio = static_cast<float>(detection.bbox.width) / detection.bbox.height;
                    float area = detection.bbox.width * detection.bbox.height;

                    std::cout << "  - " << detection.class_name << ": confidence=" 
                              << std::format("{:.2f}", detection.confidence) << "\n";
                    std::cout << "    Bounding box: (" << detection.bbox.x << ", " << detection.bbox.y 
                              << ", " << detection.bbox.width << "x" << detection.bbox.height << ")\n";
                    std::cout << "    Aspect ratio: " << std::format("{:.2f}", aspect_ratio) << "\n";
                    std::cout << "    Area: " << std::format("{:.0f}", area) << " pixels\n";
                    
                    // Explain classification reasoning
                    std::cout << "    Classification reasoning: ";
                    if (detection.class_name == "person") {
                        std::cout << "Tall narrow object (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                                  << ", area " << std::format("{:.0f}", area) << ")\n";
                    } else if (detection.class_name == "car") {
                        std::cout << "Wide object with large area (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                                  << ", area " << std::format("{:.0f}", area) << ")\n";
                    } else if (detection.class_name == "bicycle") {
                        std::cout << "Medium-sized object (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                                  << ", area " << std::format("{:.0f}", area) << ")\n";
                    } else if (detection.class_name == "generic") {
                        std::cout << "Generic object (doesn't match specific categories)\n";
                    } else {
                        std::cout << "Motion detected (background subtraction)\n";
                    }
                }
                std::cout << "\n";
            } else {
                break;
            }
        }

        auto metrics = pipeline.get_metrics();
        std::cout << "FPS: " << std::format("{:.2f}", metrics.fps) << "\n";
        std::cout << "Average latency: " << std::format("{:.2f}", metrics.avg_latency_ms) << "ms\n";

        pipeline.stop();

        return {};
    } catch (const std::exception&) {
        return Result<void>(std::error_code(static_cast<int>(PerceptionError::ThreadError), std::generic_category()));
    }
}

} // namespace perception

// ============================================================================
// MAIN
// ============================================================================

auto main() -> int {
    try {
        std::cout << "=== Mini Autonomy System Demo ===\n";
        std::cout << "Program started successfully!\n";

        if (auto result = perception::demo_ranges_and_concepts(); !result) {
            std::cout << "Detection filtering demo failed: " << result.error().message() << "\n";
            return 1;
        }

        if (auto result = perception::demo_coroutines(); !result) {
            std::cout << "Frame generation demo failed: " << result.error().message() << "\n";
            return 1;
        }

        if (auto result = perception::demo_async_processing(); !result) {
            std::cout << "Async pipeline demo failed: " << result.error().message() << "\n";
            return 1;
        }

        std::cout << "\nAll demos completed successfully!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cout << "Unknown error occurred!\n";
        return 1;
    }
}
