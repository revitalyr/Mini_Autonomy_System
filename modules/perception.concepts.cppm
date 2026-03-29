export module perception.concepts;

// Import modern type system
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

export namespace perception {

// ============================================================================
// CORE CONCEPTS
// ============================================================================

/**
 * @brief Concept for numeric types with modern C++26 features
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
 * @brief Concept for container types with modern ranges
 */
template<typename T>
concept Container = requires(T t) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    { t.begin() } -> std::convertible_to<typename T::iterator>;
    { t.end() } -> std::convertible_to<typename T::iterator>;
    { t.cbegin() } -> std::convertible_to<typename T::const_iterator>;
    { t.cend() } -> std::convertible_to<typename T::const_iterator>;
    { t.size() } -> std::convertible_to<typename T::size_type>;
    { t.empty() } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for range types (C++20 ranges)
 */
template<typename T>
concept Range = std::ranges::range<T>;

/**
 * @brief Concept for callable types with perfect forwarding
 */
template<typename F, typename... Args>
concept Callable = requires(F f, Args&&... args) {
    std::invoke(f, std::forward<Args>(args)...);
};

/**
 * @brief Concept for move-only types
 */
template<typename T>
concept MoveOnly = std::movable<T> && !std::copyable<T>;

/**
 * @brief Concept for hashable types
 */
template<typename T>
concept Hashable = requires(T t) {
    { std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

// ============================================================================
// PERCEPTION-SPECIFIC CONCEPTS
// ============================================================================

/**
 * @brief Modern concept for pipeline stages with error handling
 */
template<typename T>
concept PipelineStage = requires(T stage) {
    { stage.run() } -> std::same_as<Expected<void, PerceptionError>>;
    { stage.stop() } -> std::same_as<Expected<void, PerceptionError>>;
    { stage.is_running() } -> std::convertible_to<bool>;
    typename T::ConfigType;
};

/**
 * @brief Enhanced concept for detector types with modern features
 */
template<typename T>
concept Detector = requires(T detector, const Image& frame) {
    { detector.detect(frame) } -> std::same_as<Vector<typename T::DetectionType>>;
    typename T::DetectionType;
    requires Container<Vector<typename T::DetectionType>>;
    
    // Additional requirements for modern detectors
    { detector.get_confidence_threshold() } -> std::convertible_to<Confidence>;
    { detector.set_confidence_threshold(Confidence{}) } -> std::same_as<void>;
    { detector.get_supported_classes() } -> Range;
};

/**
 * @brief Modern concept for tracker types with ranges support
 */
template<typename T>
concept Tracker = requires(T tracker, const Vector<typename T::DetectionType>& detections) {
    typename T::DetectionType;
    { tracker.update(detections) } -> std::same_as<Vector<typename T::DetectionType>>;
    { tracker.get_tracks() } -> Range;
    { tracker.reset() } -> std::same_as<void>;
};

/**
 * @brief Enhanced concept for queue types with modern features
 */
template<typename T>
concept Queue = requires(T queue, typename T::value_type value) {
    typename T::value_type;
    
    // Basic operations
    { queue.push(std::move(value)) } -> std::same_as<Expected<void, PerceptionError>>;
    { queue.try_pop() } -> std::same_as<Optional<typename T::value_type>>;
    { queue.empty() } -> std::convertible_to<bool>;
    { queue.size() } -> std::convertible_to<std::size_t>;
    
    // Modern features
    { queue.shutdown() } -> std::same_as<void>;
    { queue.is_shutdown() } -> std::convertible_to<bool>;
    
    // Timeout support
    requires requires(T q, typename T::value_type v, Milliseconds timeout) {
        { q.pop_for(timeout) } -> std::same_as<Optional<typename T::value_type>>;
    };
};

/**
 * @brief Modern concept for metrics providers with enhanced tracking
 */
template<typename T>
concept MetricsProvider = requires(T metrics, Milliseconds latency) {
    typename T::Snapshot;
    
    // Basic metrics
    { metrics.record_frame_latency(latency) } -> std::same_as<void>;
    { metrics.get_snapshot() } -> std::same_as<typename T::Snapshot>;
    { metrics.get_fps() } -> std::convertible_to<double>;
    
    // Enhanced metrics
    { metrics.reset() } -> std::same_as<void>;
    { metrics.get_uptime() } -> std::convertible_to<Seconds>;
    
    // Snapshot requirements
    requires requires(typename T::Snapshot snapshot) {
        { snapshot.fps } -> std::convertible_to<double>;
        { snapshot.avg_latency_ms } -> std::convertible_to<double>;
        { snapshot.total_frames } -> std::convertible_to<std::size_t>;
        { snapshot.duration_seconds } -> std::convertible_to<double>;
    };
};

/**
 * @brief Modern concept for async operations with C++26 features
 */
template<typename T>
concept AsyncOperation = requires(T operation) {
    typename T::value_type;
    
    // Basic async operations
    { operation.get() } -> std::same_as<Expected<typename T::value_type, PerceptionError>>;
    { operation.wait() } -> std::same_as<void>;
    { operation.wait_for(Milliseconds{}) } -> std::same_as<bool>;
    
    // Status checking
    { operation.is_ready() } -> std::convertible_to<bool>;
    { operation.has_value() } -> std::convertible_to<bool>;
    { operation.has_error() } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for thread pool types
 */
template<typename T>
concept ThreadPool = requires(T pool) {
    { pool.get_thread_count() } -> std::convertible_to<std::size_t>;
    { pool.get_active_threads() } -> std::convertible_to<std::size_t>;
    { pool.shutdown() } -> std::same_as<void>;
    { pool.is_shutdown() } -> std::convertible_to<bool>;
    
    // Task submission
    requires requires(T p, Callable<void> task) {
        { p.submit(task) } -> std::same_as<AsyncOperation<decltype(p.submit(task))>>;
    };
    
    requires requires(T p, Callable<int> task) {
        { p.submit(task) } -> std::same_as<AsyncOperation<decltype(p.submit(task))>>;
    };
};

/**
 * @brief Concept for image loader types
 */
template<typename T>
concept ImageLoader = requires(T loader, StringView source) {
    { loader.load(source) } -> std::same_as<Expected<Image, PerceptionError>>;
    { loader.is_available() } -> std::convertible_to<bool>;
    { loader.get_supported_formats() } -> Range;
    typename T::ImageType;
    requires std::same_as<typename T::ImageType, Image>;
};

/**
 * @brief Concept for result types with modern error handling
 */
template<typename T>
concept ResultType = requires(T result) {
    typename T::value_type;
    typename T::error_type;
    
    { result.has_value() } -> std::convertible_to<bool>;
    { result.has_error() } -> std::convertible_to<bool>;
    { result.value() } -> std::same_as<typename T::value_type>;
    { result.error() } -> std::same_as<typename T::error_type>;
    
    // Operator overloads
    requires std::convertible_to<bool, decltype(result)>;
    requires requires(T r) {
        { r.operator bool() } -> std::convertible_to<bool>;
    };
};

// ============================================================================
// COMPOSED CONCEPTS
// ============================================================================

/**
 * @brief Concept for complete perception pipeline components
 */
template<typename T>
concept PerceptionComponent = 
    PipelineStage<T> && 
    requires {
        typename T::ConfigType;
        requires Container<typename T::ConfigType>;
    };

/**
 * @brief Concept for thread-safe components
 */
template<typename T>
concept ThreadSafe = requires(T component) {
    typename T::mutex_type;
    { component.get_mutex() } -> std::convertible_to<typename T::mutex_type&>;
    requires std::same_as<typename T::mutex_type, Mutex>;
};

/**
 * @brief Concept for configurable components
 */
template<typename T>
concept Configurable = requires(T component, typename T::ConfigType config) {
    typename T::ConfigType;
    { component.configure(config) } -> std::same_as<void>;
    { component.get_config() } -> std::same_as<const typename T::ConfigType&>;
};

/**
 * @brief Concept for observable components (callback support)
 */
template<typename T>
concept Observable = requires(T component, Callable<void> callback) {
    { component.add_observer(callback) } -> std::same_as<void>;
    { component.remove_observer(callback) } -> std::same_as<void>;
    { component.notify_observers() } -> std::same_as<void>;
};

// ============================================================================
// CONSTRAINTS AND VALIDATION CONCEPTS
// ============================================================================

/**
 * @brief Concept for valid detection results
 */
template<typename T>
concept ValidDetection = requires(T detection) {
    { detection.confidence } -> std::convertible_to<Confidence>;
    { detection.class_name } -> std::convertible_to<StringView>;
    { detection.bbox } -> std::convertible_to<Rect>;
    
    // Validation constraints
    requires requires(T d) {
        { d.confidence >= 0.0 && d.confidence <= 1.0 } -> std::convertible_to<bool>;
        { !d.class_name.empty() } -> std::convertible_to<bool>;
        { d.bbox.area() > 0 } -> std::convertible_to<bool>;
    };
};

/**
 * @brief Concept for valid performance metrics
 */
template<typename T>
concept ValidMetrics = requires(T metrics) {
    { metrics.fps } -> std::convertible_to<double>;
    { metrics.avg_latency_ms } -> std::convertible_to<double>;
    { metrics.total_frames } -> std::convertible_to<std::size_t>;
    
    // Validation constraints
    requires requires(T m) {
        { m.fps >= 0.0 } -> std::convertible_to<bool>;
        { m.avg_latency_ms >= 0.0 } -> std::convertible_to<bool>;
        { m.total_frames >= 0 } -> std::convertible_to<std::size_t>;
    };
};

// ============================================================================
// UTILITY CONCEPTS
// ============================================================================

/**
 * @brief Concept for types that can be formatted
 */
template<typename T>
concept Formattable = requires(T t) {
    { std::format("{}", t) } -> std::same_as<String>;
};

/**
 * @brief Concept for serializable types
 */
template<typename T>
concept Serializable = requires(T t, String format) {
    { t.serialize(format) } -> std::same_as<String>;
    { T::deserialize(String{}, format) } -> std::same_as<T>;
};

/**
 * @brief Concept for comparable types
 */
template<typename T>
concept Comparable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
    { a <=> b } -> std::convertible_to<std::partial_ordering>;
};

} // namespace perception
