module;

export module perception.concepts;

/**
 * @brief Perception namespace
 * 
 * This namespace contains core concepts and interfaces for the perception system.
 */
export namespace perception {
    /**
     * @brief Concept for pipeline stages
     * 
     * A pipeline stage must provide run and stop methods to control its execution.
     * 
     * @tparam T Pipeline stage type to test
     */
    template<typename T>
    concept PipelineStage = requires(T t) {
        { t.run() } -> std::same_as<void>;
        { t.stop() } -> std::same_as<void>;
    };

    /**
     * @brief Concept for detector types
     * 
     * A detector must be able to process OpenCV cv::Mat frames and return
     * a range of detection results. The detections should contain bounding boxes,
     * class names, and confidence scores.
     * 
     * @tparam T Detector type to test
     */
    template<typename T>
    concept Detector = requires(T t, const cv::Mat& frame) {
        { t.detect(frame) } -> std::ranges::range;
        typename std::ranges::range_value_t<decltype(t.detect(frame))>;
    };

    /**
     * @brief Concept for tracker types
     * 
     * A tracker must be able to update its state based on a vector of detections.
     * 
     * @tparam T Tracker type to test
     * @tparam Detection Detection type
     */
    template<typename T, typename Detection>
    concept Tracker = requires(T t, const std::vector<Detection>& detections) {
        { t.update(detections) } -> std::ranges::range;
    };

    /**
     * @brief Concept for queue types
     * 
     * A queue must support push, pop operations with timeout handling,
     * and provide size information. Thread-safe implementations are expected.
     * 
     * @tparam T Queue type to test
     */
    template<typename T>
    concept Queue = requires(T q, typename T::value_type item) {
        { q.push(item) };
        { q.try_pop() } -> std::same_as<std::optional<typename T::value_type>>;
        { q.empty() } -> std::convertible_to<bool>;
        { q.size() } -> std::convertible_to<std::size_t>;
    };

    /**
     * @brief Concept for performance metrics providers
     * 
     * A metrics provider must track frame latency, calculate performance statistics,
     * and provide snapshots of current performance metrics.
     * 
     * @tparam T Metrics provider type to test
     */
    template<typename T>
    concept MetricsProvider = requires(T m, double latency) {
        { m.record_frame_latency(latency) };
        { m.get_snapshot() } -> std::same_as<typename T::Snapshot>;
        { m.get_fps() } -> std::convertible_to<double>;
    };

    /**
     * @brief Concept for async operation types
     * 
     * An async operation must provide a get method to retrieve its result and
     * a wait method to block until the operation is complete.
     * 
     * @tparam T Async operation type to test
     */
    template<typename T>
    concept AsyncOperation = requires(T t) {
        typename T::value_type;
        { t.get() } -> std::same_as<typename T::value_type>;
        { t.wait() } -> std::same_as<void>;
    };
}
