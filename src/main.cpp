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

#include <iostream>
#include <ranges>
#include <coroutine>
#include <chrono>
#include <thread>
#include <format>

#include "perception/concepts.hpp"
#include "perception/queue.hpp"
#include "perception/pipeline.hpp"
#include "perception/metrics.hpp"
#include "perception/detector.hpp"
#include "perception/async.hpp"
#include "perception/result.hpp"
#include "perception/types.hpp"

namespace perception {

// ============================================================================
// IMAGE FRAME GENERATOR
// ============================================================================

/**
 * Generator for streaming image frames
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
    auto start() noexcept -> Expected<void, PerceptionError> {
        try {
            m_is_running.store(true, std::memory_order_release);

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
    auto process_image(ImageData image) noexcept -> Expected<void, PerceptionError> {
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
     * Get detection results
     * @param timeout Maximum time to wait for results
     * @return Detected objects or error
     */
    auto get_results(Milliseconds timeout = Milliseconds{1000}) noexcept
        -> Expected<Vector<Detection>, PerceptionError> {

        if (auto result = m_result_queue->try_pop()) {
            return std::move(*result);
        }

        if (auto result = m_result_queue->pop_timeout(timeout)) {
            return std::move(*result);
        }

        return make_unexpected_typed<Vector<Detection>>(PerceptionError::QueueEmpty);
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
auto demo_ranges_and_concepts() noexcept -> Expected<void, PerceptionError> {
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
            std::cout << "  - " << detection.class_name << ": confidence=" 
                      << std::format("{:.2f}", detection.confidence) << "\n";
        }

        return {};
    } catch (const std::exception&) {
        return make_unexpected(PerceptionError::InvalidInput);
    }
}

/**
 * Demo: Generate test image frames
 */
auto demo_coroutines() noexcept -> Expected<void, PerceptionError> {
    try {
        std::cout << "=== Frame Generation Demo ===\n";

        auto frame_generator = generate_test_frames(5);
        size_t frame_count = 0;

        for (const auto& frame : frame_generator) {
            ++frame_count;
            std::cout << "Generated frame " << frame_count << " (" 
                      << frame.width << "x" << frame.height << ")\n";
        }

        return {};
    } catch (const std::exception&) {
        return make_unexpected(PerceptionError::InvalidInput);
    }
}

/**
 * Demo: Async object detection pipeline
 */
auto demo_async_processing() noexcept -> Expected<void, PerceptionError> {
    try {
        std::cout << "=== Async Detection Pipeline Demo ===\n";

        auto pipeline = AsyncProcessingPipeline{};

        if (auto result = pipeline.start(); !result) {
            return result;
        }

        for (const auto& frame : generate_test_frames(3)) {
            ImageData frame_copy = frame;
            if (auto result = pipeline.process_image(std::move(frame_copy)); !result) {
                pipeline.stop();
                return result;
            }
        }

        for (int i = 0; i < 3; ++i) {
            if (auto results = pipeline.get_results(); results) {
                std::cout << "Got " << results.value().size() << " detections\n";
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
        return make_unexpected(PerceptionError::ThreadError);
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
