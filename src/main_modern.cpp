/**
 * @file main_modern.cpp
 * @brief Modern C++26 main demo application for the Mini Autonomy System
 * 
 * This file demonstrates modern C++26 usage including:
 * - Concepts and ranges
 * - std::expected for error handling
 * - Coroutines for async operations
 * - Modern type aliases and semantic naming
 * - RAII and smart pointers
 * - Modern initialization patterns
 * 
 * @author Mini Autonomy System
 * @date 2026
 */

#include <iostream>
#include <ranges>
#include <coroutine>
#include <print>
#include <chrono>
#include <opencv2/opencv.hpp>

// Import modern perception system types
import perception.concepts;
import perception.queue;
import perception.pipeline;
import perception.metrics;
import perception.detector;
import perception.async;
import perception.result;

// Import modern type aliases
#include <perception/types.hpp>

namespace perception {

// ============================================================================
// MODERN C++26 COROUTINE GENERATOR FOR IMAGE PROCESSING
// ============================================================================

/**
 * @brief Generator coroutine for image frames
 */
template<typename T>
struct Generator {
    struct promise_type {
        T m_value{};
        
        auto get_return_object() noexcept -> Generator {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        static constexpr auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        static constexpr auto final_suspend() noexcept -> std::suspend_always { return {}; }
        
        auto yield_value(T value) noexcept -> std::suspend_always {
            m_value = std::move(value);
            return {};
        }
        
        constexpr auto return_void() noexcept -> void {}
        constexpr auto unhandled_exception() noexcept -> void { std::terminate(); }
    };
    
    std::coroutine_handle<promise_type> m_handle;
    
    explicit Generator(std::coroutine_handle<promise_type> handle) noexcept : m_handle{handle} {}
    
    ~Generator() noexcept {
        if (m_handle) {
            m_handle.destroy();
        }
    }
    
    // Move-only type
    Generator(const Generator&) = delete;
    auto operator=(const Generator&) -> Generator& = delete;
    
    Generator(Generator&& other) noexcept : m_handle{std::exchange(other.m_handle, nullptr)} {}
    auto operator=(Generator&& other) noexcept -> Generator& {
        if (this != &other) {
            if (m_handle) {
                m_handle.destroy();
            }
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }
    
    struct Iterator {
        std::coroutine_handle<promise_type> m_handle;
        
        auto operator==(std::default_sentinel_t) const noexcept -> bool {
            return !m_handle || m_handle.done();
        }
        
        auto operator++() noexcept -> void {
            m_handle.resume();
        }
        
        auto operator*() const noexcept -> const T& {
            return m_handle.promise().m_value;
        }
    };
    
    auto begin() const noexcept -> Iterator {
        if (m_handle && !m_handle.done()) {
            m_handle.resume();
        }
        return Iterator{m_handle};
    }
    
    auto end() const noexcept -> std::default_sentinel_t {
        return {};
    }
};

/**
 * @brief Modern image frame generator
 */
auto generate_test_frames(size_t count = 10) -> Generator<Image> {
    for (size_t i = 0; i < count; ++i) {
        // Create test frame with modern initialization
        Image frame{480, 640, CV_8UC3, cv::Scalar{0, 0, 0}};
        
        // Add some visual content for testing
        cv::rectangle(frame, Rect{50 + i * 10, 50, 100, 100}, 
                     cv::Scalar{255, 255, 255}, 2);
        
        co_yield frame;
    }
}

// ============================================================================
// MODERN C++26 ASYNC PROCESSING PIPELINE
// ============================================================================

/**
 * @brief Modern async processing pipeline
 */
class AsyncProcessingPipeline {
private:
    UniquePtr<ThreadSafeQueue<Image>> m_image_queue;
    UniquePtr<ThreadSafeQueue<Vector<Detection>>> m_result_queue;
    UniquePtr<ThreadPool> m_thread_pool;
    UniquePtr<PerformanceMetrics> m_metrics;
    UniquePtr<MockDetector> m_detector;
    Atomic<bool> m_is_running{false};
    
public:
    explicit AsyncProcessingPipeline(const Config& config = Config{}) 
        : m_image_queue{make_unique<ThreadSafeQueue<Image>>()}
        , m_result_queue{make_unique<ThreadSafeQueue<Vector<Detection>>>()}
        , m_thread_pool{make_unique<ThreadPool>(config.thread_pool_size)}
        , m_metrics{make_unique<PerformanceMetrics>()}
        , m_detector{make_unique<MockDetector>()} {}
    
    // Move-only type
    AsyncProcessingPipeline(const AsyncProcessingPipeline&) = delete;
    auto operator=(const AsyncProcessingPipeline&) -> AsyncProcessingPipeline& = delete;
    
    AsyncProcessingPipeline(AsyncProcessingPipeline&&) = default;
    auto operator=(AsyncProcessingPipeline&&) -> AsyncProcessingPipeline& = default;
    
    ~AsyncProcessingPipeline() = default;
    
    /**
     * @brief Start processing with modern error handling
     */
    auto start() noexcept -> Expected<void, PerceptionError> {
        try {
            m_is_running.store(true, std::memory_order_release);
            
            // Submit processing tasks to thread pool
            for (size_t i = 0; i < m_thread_pool->get_thread_count(); ++i) {
                m_thread_pool->submit([this] {
                    processing_loop();
                });
            }
            
            return {};
        } catch (const std::exception&) {
            return make_unexpected(PerceptionError::ThreadError);
        }
    }
    
    /**
     * @brief Stop processing gracefully
     */
    auto stop() noexcept -> void {
        m_is_running.store(false, std::memory_order_release);
        m_image_queue->shutdown();
        m_result_queue->shutdown();
    }
    
    /**
     * @brief Process image with modern error handling
     */
    auto process_image(Image image) noexcept -> Expected<void, PerceptionError> {
        if (!m_is_running.load(std::memory_order_acquire)) {
            return make_unexpected(PerceptionError::QueueShutdown);
        }
        
        try {
            m_image_queue->push(std::move(image));
            return {};
        } catch (const std::exception&) {
            return make_unexpected(PerceptionError::QueueEmpty);
        }
    }
    
    /**
     * @brief Get results with timeout
     */
    auto get_results(Milliseconds timeout = Milliseconds{1000}) noexcept 
        -> Expected<Vector<Detection>, PerceptionError> {
        
        if (auto result = m_result_queue->try_pop()) {
            return std::move(*result);
        }
        
        return make_unexpected(PerceptionError::QueueEmpty);
    }
    
    /**
     * @brief Get performance metrics
     */
    auto get_metrics() const noexcept -> PerformanceMetrics::Snapshot {
        return m_metrics->get_snapshot();
    }

private:
    /**
     * @brief Processing loop with modern C++26 features
     */
    auto processing_loop() noexcept -> void {
        while (m_is_running.load(std::memory_order_acquire)) {
            if (auto image = m_image_queue->try_pop()) {
                auto start_time = std::chrono::high_resolution_clock::now();
                
                // Process image with modern error handling
                auto detections_result = m_detector->detect(*image);
                
                auto end_time = std::chrono::high_resolution_clock::now();
                auto latency = to_milliseconds(end_time - start_time);
                
                // Record metrics
                m_metrics->record_frame_latency(latency.count());
                
                // Store results
                m_result_queue->push(std::move(detections_result));
            } else {
                // Brief sleep to prevent busy waiting
                std::this_thread::sleep_for(Milliseconds{1});
            }
        }
    }
};

// ============================================================================
// MODERN C++26 DEMO FUNCTIONS
// ============================================================================

/**
 * @brief Modern demo using ranges and concepts
 */
auto demo_ranges_and_concepts() noexcept -> Expected<void, PerceptionError> {
    try {
        // Create test detections
        Vector<Detection> test_detections{
            Detection{{Rect{10, 10, 50, 50}, 0.9, 1, "person"}},
            Detection{{Rect{100, 100, 80, 80}, 0.7, 2, "car"}},
            Detection{{Rect{200, 200, 30, 30}, 0.3, 3, "bicycle"}},
            Detection{{Rect{300, 300, 60, 60}, 0.8, 1, "person"}}
        };
        
        // Use modern C++26 ranges to filter and transform
        auto high_confidence_detections = test_detections 
            | std::views::filter(filter_by_confidence(0.5))
            | std::views::take_first(3);
        
        // Print results using std::print (C++23)
        std::println("=== Modern Ranges Demo ===");
        std::println("Total detections: {}", test_detections.size());
        std::println("High confidence detections:");
        
        for (const auto& detection : high_confidence_detections) {
            std::println("  - {}: confidence={:.2f}", 
                        detection.class_name, detection.confidence);
        }
        
        return {};
    } catch (const std::exception&) {
        return make_unexpected(PerceptionError::InvalidInput);
    }
}

/**
 * @brief Modern coroutine demo
 */
auto demo_coroutines() noexcept -> Expected<void, PerceptionError> {
    try {
        std::println("=== Modern Coroutines Demo ===");
        
        // Generate frames using coroutine
        auto frame_generator = generate_test_frames(5);
        size_t frame_count = 0;
        
        for (const auto& frame : frame_generator) {
            ++frame_count;
            std::println("Generated frame {} ({}x{})", 
                        frame_count, frame.cols, frame.rows);
        }
        
        return {};
    } catch (const std::exception&) {
        return make_unexpected(PerceptionError::InvalidInput);
    }
}

/**
 * @brief Modern async processing demo
 */
auto demo_async_processing() noexcept -> Expected<void, PerceptionError> {
    try {
        std::println("=== Modern Async Processing Demo ===");
        
        // Create modern pipeline
        auto pipeline = AsyncProcessingPipeline{};
        
        // Start processing
        if (auto result = pipeline.start(); !result) {
            return result;
        }
        
        // Process some test images
        for (const auto& frame : generate_test_frames(3)) {
            if (auto result = pipeline.process_image(frame.clone()); !result) {
                pipeline.stop();
                return result;
            }
        }
        
        // Get results
        for (int i = 0; i < 3; ++i) {
            if (auto results = pipeline.get_results(); results) {
                std::println("Got {} detections", results->size());
            } else {
                break;
            }
        }
        
        // Print metrics
        auto metrics = pipeline.get_metrics();
        std::println("FPS: {:.2f}", metrics.fps);
        std::println("Average latency: {:.2f}ms", metrics.avg_latency_ms);
        
        // Stop pipeline
        pipeline.stop();
        
        return {};
    } catch (const std::exception&) {
        return make_unexpected(PerceptionError::ThreadError);
    }
}

} // namespace perception

// ============================================================================
// MODERN C++26 MAIN FUNCTION
// ============================================================================

auto main() -> int {
    try {
        // Use std::print for modern output (C++23)
        std::println("=== Mini Autonomy System Demo (Modern C++26) ===");
        std::println("Program started successfully!");
        
        // Demo modern C++26 features
        if (auto result = perception::demo_ranges_and_concepts(); !result) {
            std::println("Ranges demo failed: {}", static_cast<int>(result.error()));
            return 1;
        }
        
        if (auto result = perception::demo_coroutines(); !result) {
            std::println("Coroutines demo failed: {}", static_cast<int>(result.error()));
            return 1;
        }
        
        if (auto result = perception::demo_async_processing(); !result) {
            std::println("Async processing demo failed: {}", static_cast<int>(result.error()));
            return 1;
        }
        
        std::println("\nAll demos completed successfully!");
        return 0;
        
    } catch (const std::exception& e) {
        std::println("Error: {}", e.what());
        return 1;
    } catch (...) {
        std::println("Unknown error occurred!");
        return 1;
    }
}
