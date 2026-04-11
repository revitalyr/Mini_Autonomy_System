#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <chrono>
#include <mutex>
#include <cstdint>

export module perception.types;

/**
 * @brief Type aliases and common types for perception system
 *
 * This module defines all common type aliases used throughout the perception system
 * using standard C++20/23 types.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

export namespace perception {

// ============================================================================
// STANDARD TYPE ALIASES
// ============================================================================

// String types
using String = std::string;
using StringView = std::string_view;

// Container types
template<typename T>
using Vector = std::vector<T>;

template<typename T>
using Optional = std::optional<T>;

// C++20-compatible Expected type using std::variant
template<typename T, typename E>
class Expected {
public:
    Expected(T value) : data(std::move(value)) {}
    Expected(E error) : data(std::move(error)) {}

    bool has_value() const { return std::holds_alternative<T>(data); }
    bool has_error() const { return std::holds_alternative<E>(data); }

    const T& value() const { return std::get<T>(data); }
    T& value() { return std::get<T>(data); }

    const E& error() const { return std::get<E>(data); }
    E& error() { return std::get<E>(data); }

    explicit operator bool() const { return has_value(); }

private:
    std::variant<T, E> data;
};

// Time types
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;

// Thread types
using Mutex = std::mutex;

// ============================================================================
// PERCEPTION-SPECIFIC TYPES
// ============================================================================

// Error type for perception operations
struct PerceptionError {
    String message;
    int code;

    PerceptionError() : code(0) {}
    PerceptionError(String msg, int c = 0) : message(std::move(msg)), code(c) {}
};

// Confidence type (0.0 to 1.0)
using Confidence = double;

// Rectangle type for bounding boxes
struct Rect {
    double x, y, width, height;

    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(double x_, double y_, double w, double h)
        : x(x_), y(y_), width(w), height(h) {}

    double area() const { return width * height; }
};

// Image data structure using only std types
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

// Callable type alias
template<typename T>
using Callable = T;

} // namespace perception
