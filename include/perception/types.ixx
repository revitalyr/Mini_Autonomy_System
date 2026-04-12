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
import perception.result;

/**
 * @brief Common data types for perception system
 *
 * Defines standard data structures used throughout the perception system
 * for images, detections, and configuration.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

namespace perception {

    // Type aliases for convenience
    export using String = std::string;
    export using StringView = std::string_view;
    
    template<typename T>
    using Vector = std::vector<T>;
    
    template<typename T>
    using Optional = std::optional<T>;
    
    template<typename T>
    using UniquePtr = std::unique_ptr<T>;
    
    template<typename T>
    using SharedPtr = std::shared_ptr<T>;
    
    export using AtomicBool = std::atomic<bool>;
    
    export using Mutex = std::mutex;
    export using LockGuard = std::lock_guard<std::mutex>;
    export using UniqueLock = std::unique_lock<std::mutex>;
    
    export using Milliseconds = std::chrono::milliseconds;
    export using Seconds = std::chrono::seconds;
    export using TimePoint = std::chrono::high_resolution_clock::time_point;
    export using Clock = std::chrono::high_resolution_clock;

    /**
     * @brief Rectangle for bounding boxes
     */
    export struct Rect {
        int x, y;
        int width, height;

        Rect() : x(0), y(0), width(0), height(0) {}
        Rect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}
    };

    /**
     * @brief Image data structure
     */
    export struct ImageData {
        int width;
        int height;
        int channels;
        std::vector<uint8_t> data;

        ImageData() : width(0), height(0), channels(0) {}
        ImageData(int w, int h, int c) 
            : width(w), height(h), channels(c), data(w * h * c) {}
    };

    /**
     * @brief Configuration parameters
     */
    export struct Config {
        size_t thread_pool_size = 4;
        int max_detections = 100;
        float confidence_threshold = 0.5f;
        bool enable_gpu = false;
    };

    /**
     * @brief IMU data structure
     */
    export struct IMUData {
        double timestamp;
        double accelerometer_x;
        double accelerometer_y;
        double accelerometer_z;
        double gyroscope_x;
        double gyroscope_y;
        double gyroscope_z;

        IMUData() : timestamp(0), accelerometer_x(0), accelerometer_y(0), accelerometer_z(0),
                    gyroscope_x(0), gyroscope_y(0), gyroscope_z(0) {}
    };

    /**
     * @brief VIO frame with image and IMU data
     */
    export struct VioFrame {
        double timestamp;
        ImageData image;
        std::vector<IMUData> imu_samples;
    };

} // namespace perception
