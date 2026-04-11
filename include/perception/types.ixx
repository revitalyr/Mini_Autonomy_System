module;

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <expected>
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

// ============================================================================
// STANDARD TYPE ALIASES
// ============================================================================

using String = std::string;
using StringView = std::string_view;

export template<typename T>
using Vector = std::vector<T>;

export template<typename T>
using Optional = std::optional<T>;

export template<typename T, typename E>
using Expected = std::expected<T, E>;

export template<typename E>
using Unexpected = std::unexpected<E>;

export template<typename E>
auto make_unexpected(E e) { return std::unexpected(e); }

using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;

export auto to_milliseconds(auto duration) -> Milliseconds {
    return std::chrono::duration_cast<Milliseconds>(duration);
}

using Mutex = std::mutex;

export template<typename T>
using UniquePtr = std::unique_ptr<T>;

export using std::make_unique;

export template<typename T>
using Atomic = std::atomic<T>;

// ============================================================================
// PERCEPTION-SPECIFIC TYPES
// ============================================================================

using Confidence = double;

/**
 * Rectangle bounding box for object detection
 */
struct Rect {
    double x, y, width, height;

    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(double x_, double y_, double w, double h)
        : x(x_), y(y_), width(w), height(h) {}

    double area() const { return width * height; }
};

/**
 * Raw image data with pixel buffer
 */
struct ImageData {
    int width;
    int height;
    int channels;
    std::vector<uint8_t> data;

    ImageData() : width(0), height(0), channels(0) {}
    ImageData(int w, int h, int c) : width(w), height(h), channels(c) {
        data.resize(w * h * c);
    }

    bool empty() const { return width == 0 || height == 0; }
};

/**
 * Configuration settings for perception components
 */
struct Config {
    size_t thread_pool_size = 4;
    size_t max_queue_size = 100;
    double confidence_threshold = 0.5;
};

template<typename T>
using Callable = T;

} // namespace perception
