module;

#include <concepts>
#include <ranges>
#include <type_traits>

export module perception.concepts;

export namespace perception {
    // Concept for pipeline stages
    template<typename T>
    concept PipelineStage = requires(T t) {
        { t.run() } -> std::same_as<void>;
        { t.stop() } -> std::same_as<void>;
    };

    // Concept for detectors
    template<typename T>
    concept Detector = requires(T t, const cv::Mat& frame) {
        { t.detect(frame) } -> std::ranges::range;
        typename std::ranges::range_value_t<decltype(t.detect(frame))>;
    };

    // Concept for trackers
    template<typename T, typename Detection>
    concept Tracker = requires(T t, const std::vector<Detection>& detections) {
        { t.update(detections) } -> std::ranges::range;
    };

    // Concept for queues
    template<typename T>
    concept Queue = requires(T q, typename T::value_type value) {
        typename T::value_type;
        { q.push(value) } -> std::same_as<void>;
        { q.pop() } -> std::convertible_to<bool>;
        { q.empty() } -> std::convertible_to<bool>;
    };

    // Concept for performance metrics
    template<typename T>
    concept MetricsProvider = requires(T t) {
        { t.get_fps() } -> std::convertible_to<double>;
        { t.get_latency() } -> std::convertible_to<std::chrono::milliseconds>;
    };

    // Concept for async operations
    template<typename T>
    concept AsyncOperation = requires(T t) {
        typename T::value_type;
        { t.get() } -> std::same_as<typename T::value_type>;
        { t.wait() } -> std::same_as<void>;
    };
}
