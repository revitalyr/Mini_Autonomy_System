module;
#include <concepts>
#include <type_traits>
#include <ranges>
#include <format>
#include <functional>

export module perception.concepts;
import perception.types;

/**
 * @brief Modern C++26 concepts for perception system
 *
 * This module defines all concepts used throughout the perception system
 * with modern C++26 features including requires clauses, concepts composition,
 * and advanced type checking.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

namespace perception {

    // ============================================================================
    // BASIC TYPE CONCEPTS
    // ============================================================================

    /**
     * @brief Concept for numeric types
     */
    export template<typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    /**
     * @brief Concept for container types
     */
    export template<typename T>
    concept Container = requires(T t) {
        typename T::value_type;
        { t.size() } -> std::convertible_to<std::size_t>;
        { t.begin() } -> std::input_or_output_iterator<typename T::iterator>;
        { t.end() } -> std::input_or_output_iterator<typename T::iterator>;
    };

    // ============================================================================
    // PIPELINE STAGE CONCEPTS
    // ============================================================================

    /**
     * @brief Concept for pipeline stage
     */
    export template<typename T>
    concept PipelineStage = requires(T stage) {
        { stage.run() } -> std::same_as<void>;
        { stage.stop() } -> std::same_as<void>;
    };

    /**
     * @brief Concept for detector
     */
    export template<typename T>
    concept DetectorConcept = requires(T detector, const ImageData& image) {
        { detector.detect(image) } -> std::convertible_to<Vector<Detection>>;
        { detector.get_class_names() } -> std::convertible_to<const Vector<std::string>&>;
    };

    // ============================================================================
    // QUEUE CONCEPTS
    // ============================================================================

    /**
     * @brief Concept for thread-safe queue
     */
    export template<typename T>
    concept Queue = requires(T q) {
        typename T::value_type;
        { q.empty() } -> std::convertible_to<bool>;
        { q.size() } -> std::convertible_to<std::size_t>;
        { q.shutdown() } -> std::same_as<void>;
    };

    // ============================================================================
    // METRICS CONCEPTS
    // ============================================================================

    /**
     * @brief Concept for metrics provider
     */
    export template<typename T>
    concept MetricsProvider = requires(T m) {
        { m.get_snapshot() } -> std::convertible_to<typename T::Snapshot>;
    };

    // ============================================================================
    // ASYNC CONCEPTS
    // ============================================================================

    /**
     * @brief Concept for async operation
     */
    export template<typename T>
    concept AsyncOperation = requires(T op) {
        { op.get() } -> std::same_as<typename T::result_type>;
        { op.is_ready() } -> std::convertible_to<bool>;
    };

    /**
     * @brief Concept for thread pool
     */
    export template<typename T>
    concept ThreadPoolConcept = requires(T pool, std::function<void()> task) {
        { pool.submit(task) } -> std::same_as<void>;
        { pool.get_thread_count() } -> std::convertible_to<std::size_t>;
    };

    // ============================================================================
    // DATA PROVIDER CONCEPTS
    // ============================================================================

    /**
     * @brief Concept for image loader
     */
    export template<typename T>
    concept ImageLoader = requires(T loader, const std::filesystem::path& path) {
        { loader.load_image(path) } -> std::same_as<std::future<PerceptionResult<ImageData>>>;
    };

    /**
     * @brief Concept for data provider
     */
    export template<typename T>
    concept DataProvider = requires(T provider, const std::string& path) {
        { provider.open(path) } -> std::same_as<Expected<void, PerceptionError>>;
        { provider.close() } -> std::same_as<void>;
        { provider.is_open() } -> std::convertible_to<bool>;
    };

    // ============================================================================
    // RESULT TYPE CONCEPTS
    // ============================================================================

    /**
     * @brief Concept for result type
     */
    export template<typename T>
    concept ResultType = requires(T result) {
        { result.has_value() } -> std::convertible_to<bool>;
        { result.error() } -> std::convertible_to<std::error_code>;
    };

} // namespace perception
