module;

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <memory>
#include <atomic>

export module perception.types;

/**
 * @brief Common data types for perception system
 *
 * Defines standard data structures used throughout the perception system
 * for images, detections, and configuration.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

export namespace perception {

    // Type aliases for convenience
    using String = std::string;
    using StringView = std::string_view;

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using Optional = std::optional<T>;

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T>
    using Atomic = std::atomic<T>;

    using AtomicBool = std::atomic<bool>;
    
    using Mutex = std::mutex;
    using LockGuard = std::lock_guard<std::mutex>;
    using UniqueLock = std::unique_lock<std::mutex>;
    
    using Milliseconds = std::chrono::milliseconds;
    using Seconds = std::chrono::seconds;
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Clock = std::chrono::high_resolution_clock;

    using Count = size_t;
    
    // Utility functions
    template<typename T, typename... Args>
    auto make_unique(Args&&... args) -> UniquePtr<T> {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    auto to_milliseconds(std::chrono::nanoseconds duration) -> Milliseconds {
        return std::chrono::duration_cast<Milliseconds>(duration);
    }

    /**
     * @brief Configuration parameters
     */
    struct Config {
        size_t thread_pool_size = 4;
        int max_detections = 100;
        float confidence_threshold = 0.5f;
        double detector_sensitivity = 16.0; // MOG2 varThreshold
        float tracker_iou_threshold = 0.3f;
        int tracker_max_age = 10;
        bool enable_gpu = false;
    };

} // namespace perception
