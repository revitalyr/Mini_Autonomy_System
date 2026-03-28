/**
 * @file types.hpp
 * @brief Modern C++26 type aliases and concepts for perception system
 * 
 * This header defines all type aliases, concepts, and utilities used
 * throughout the perception system with modern C++26 practices.
 * 
 * @author Mini Autonomy System
 * @date 2026
 */

#pragma once

#include <concepts>
#include <ranges>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <optional>
#include <expected>
#include <atomic>
#include <mutex>
#include <thread>
#include <future>
#include <opencv2/opencv.hpp>

namespace perception {

// ============================================================================
// MODERN C++26 TYPE ALIASES
// ============================================================================

// String types with semantic meaning
using String = std::string;
using StringView = std::string_view;

// Numeric types with semantic meaning
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Duration = std::chrono::duration<double>;

// Container types with semantic meaning
template<typename T>
using Vector = std::vector<T>;

template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Future types
template<typename T>
using Future = std::future<T>;

template<typename T>
using Promise = std::promise<T>;

// Optional and Expected types
template<typename T>
using Optional = std::optional<T>;

template<typename T, typename E>
using Expected = std::expected<T, E>;

// Atomic types
template<typename T>
using Atomic = std::atomic<T>;

// Thread types
using Thread = std::jthread;
using Mutex = std::mutex;
using LockGuard = std::lock_guard<Mutex>;
using UniqueLock = std::unique_lock<Mutex>;

// OpenCV types with semantic meaning
using Image = cv::Mat;
using Rect = cv::Rect;
using Point = cv::Point;
using Size = cv::Size;

// Specific semantic types for the perception system
using Confidence = double;
using ClassId = int;
using FrameId = std::uint64_t;
using Timestamp = std::chrono::system_clock::time_point;

// ============================================================================
// MODERN C++26 CONCEPTS
// ============================================================================

/**
 * @brief Concept for numeric types
 */
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

/**
 * @brief Concept for floating point types
 */
template<typename T>
concept FloatingPoint = std::floating_point<T>;

/**
 * @brief Concept for integral types
 */
template<typename T>
concept Integral = std::integral<T>;

/**
 * @brief Concept for container types
 */
template<typename T>
concept Container = requires(T t) {
    typename T::value_type;
    typename T::iterator;
    { t.begin() } -> std::convertible_to<typename T::iterator>;
    { t.end() } -> std::convertible_to<typename T::iterator>;
    { t.size() } -> std::convertible_to<typename T::size_type>;
};

/**
 * @brief Concept for range types
 */
template<typename T>
concept Range = std::ranges::range<T>;

/**
 * @brief Concept for callable types
 */
template<typename F, typename... Args>
concept Callable = requires(F f, Args&&... args) {
    std::invoke(f, std::forward<Args>(args)...);
};

/**
 * @brief Concept for detector types
 */
template<typename T>
concept Detector = requires(T detector, const Image& frame) {
    { detector.detect(frame) } -> std::convertible_to<Vector<typename T::DetectionType>>;
};

/**
 * @brief Concept for queue types
 */
template<typename T>
concept Queue = requires(T queue, typename T::value_type value) {
    typename T::value_type;
    { queue.push(std::move(value)) };
    { queue.try_pop() } -> std::convertible_to<Optional<typename T::value_type>>;
};

/**
 * @brief Concept for metrics types
 */
template<typename T>
concept MetricsTracker = requires(T metrics, Milliseconds latency) {
    { metrics.record_frame_latency(latency) };
    { metrics.get_snapshot() } -> std::convertible_to<typename T::SnapshotType>;
};

// ============================================================================
// MODERN C++26 CONSTANTS AND ENUMS
// ============================================================================

/**
 * @brief Modern enum class for perception errors
 */
enum class PerceptionError : int {
    Success = 0,
    InvalidInput = 1,
    ModelLoadFailed = 2,
    InferenceFailed = 3,
    QueueEmpty = 4,
    QueueShutdown = 5,
    ThreadError = 6,
    CameraError = 7,
    TimeoutError = 8,
    MemoryError = 9
};

/**
 * @brief Modern enum class for detection classes
 */
enum class DetectionClass : int {
    Unknown = 0,
    Person = 1,
    Car = 2,
    Truck = 3,
    Bicycle = 4,
    Motorcycle = 5,
    TrafficLight = 6,
    StopSign = 7,
    Pedestrian = 8,
    Animal = 9
};

// ============================================================================
// MODERN C++26 CONFIGURATION
// ============================================================================

/**
 * @brief Configuration structure using designated initializers
 */
struct Config {
    String model_path{"model.onnx"};
    String input_source{"camera"};
    Milliseconds inference_timeout{5000};
    Confidence confidence_threshold{0.5};
    size_t max_detections{100};
    bool enable_gpu{false};
    size_t thread_pool_size{std::thread::hardware_concurrency()};
};

/**
 * @brief Performance thresholds
 */
struct PerformanceThresholds {
    Milliseconds max_frame_latency{33};  // 30 FPS
    Confidence min_confidence{0.3};
    size_t max_queue_size{1000};
    Seconds max_uptime{3600};  // 1 hour
};

// ============================================================================
// MODERN C++26 UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Creates a unique pointer with perfect forwarding
 */
template<typename T, typename... Args>
constexpr auto make_unique(Args&&... args) noexcept -> UniquePtr<T> {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/**
 * @brief Creates a shared pointer with perfect forwarding
 */
template<typename T, typename... Args>
constexpr auto make_shared(Args&&... args) noexcept -> SharedPtr<T> {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/**
 * @brief Modern timestamp generator
 */
constexpr auto now() noexcept -> Timestamp {
    return std::chrono::system_clock::now();
}

/**
 * @brief Converts duration to milliseconds
 */
template<typename Rep, typename Period>
constexpr auto to_milliseconds(std::chrono::duration<Rep, Period> duration) noexcept -> Milliseconds {
    return std::chrono::duration_cast<Milliseconds>(duration);
}

/**
 * @brief Checks if a value is within range using concepts
 */
template<Numeric T>
constexpr auto is_within_range(T value, T min, T max) noexcept -> bool {
    return value >= min && value <= max;
}

/**
 * @brief Modern clamp function using concepts
 */
template<Numeric T>
constexpr auto clamp(T value, T min, T max) noexcept -> T {
    return std::clamp(value, min, max);
}

// ============================================================================
// MODERN C++26 ERROR HANDLING UTILITIES
// ============================================================================

/**
 * @brief Creates an expected value
 */
template<typename T>
constexpr auto make_expected(T&& value) noexcept -> Expected<std::decay_t<T>, PerceptionError> {
    return Expected<std::decay_t<T>, PerceptionError>{std::forward<T>(value)};
}

/**
 * @brief Creates an unexpected error
 */
constexpr auto make_unexpected(PerceptionError error) noexcept -> std::unexpected<PerceptionError> {
    return std::unexpected<PerceptionError>{error};
}

/**
 * @brief Converts exception to expected
 */
template<typename T>
constexpr auto catch_to_expected() noexcept -> Expected<T, PerceptionError> {
    try {
        if constexpr (std::is_void_v<T>) {
            return Expected<T, PerceptionError>{};
        } else {
            return Expected<T, PerceptionError>{T{}};
        }
    } catch (const std::exception&) {
        return make_unexpected(PerceptionError::Unknown);
    }
}

// ============================================================================
// MODERN C++26 RANGES UTILITIES
// ============================================================================

/**
 * @brief Filters detections by confidence
 */
constexpr auto filter_by_confidence(Confidence min_conf) noexcept {
    return [min_conf](const auto& detection) constexpr {
        return detection.confidence >= min_conf;
    };
}

/**
 * @brief Transforms detections to class names
 */
constexpr auto to_class_names() noexcept {
    return [](const auto& detection) constexpr {
        return detection.class_name;
    };
}

/**
 * @brief Limits number of results
 */
constexpr auto take_first(size_t count) noexcept {
    return [count](auto&& range) constexpr {
        return range | std::views::take(count);
    };
}

} // namespace perception
