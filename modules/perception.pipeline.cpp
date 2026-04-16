/**
 * @file perception.pipeline.cpp
 * @brief Implementation of asynchronous processing pipeline
 */

module;

#include <thread>
#include <chrono>
#include <perception/constants.h>

module perception.pipeline;

import perception.types;
import perception.geom;
import perception.result;
import perception.queue;
import perception.metrics;
import perception.detector;
import perception.tracking;
import perception.async;

namespace perception {

using namespace perception::constants;

/**
 * @class LatencyTimer
 * @brief RAII helper to automatically record processing latency
 * @details Records frame processing time on destruction
 */
class LatencyTimer {
public:
    PerformanceMetrics* metrics; ///< Performance metrics to record to
    Clock::time_point start_time; ///< Start time for latency measurement

    /**
     * @brief Constructor
     * @param m Performance metrics pointer
     */
    explicit LatencyTimer(PerformanceMetrics* m) 
        : metrics(m), start_time(Clock::now()) {}

    /**
     * @brief Destructor records latency
     */
    ~LatencyTimer() {
        if (metrics) {
            auto end_time = Clock::now();
            metrics->record_frame_latency(static_cast<double>(to_milliseconds(end_time - start_time).count()));
        }
    }
};

/**
 * @struct AsyncProcessingPipeline::Impl
 * @brief Private implementation details for AsyncProcessingPipeline
 */
struct AsyncProcessingPipeline::Impl {
    UniquePtr<ThreadSafeQueue<UniquePtr<ImageData>>> image_queue; ///< Queue for input images
    UniquePtr<ThreadSafeQueue<OutputFrame>> result_queue; ///< Queue for processed results
    UniquePtr<ThreadPool> thread_pool; ///< Thread pool for parallel processing
    UniquePtr<PerformanceMetrics> metrics; ///< Performance tracking metrics
    UniquePtr<vision::Detector> detector; ///< Object detector instance
    UniquePtr<tracking::Tracker> tracker; ///< Object tracker instance
    AtomicBool is_running{false}; ///< Pipeline running state

    /**
     * @brief Constructor
     * @param config Pipeline configuration
     */
    Impl(const Config& config)
        : image_queue(perception::make_unique<ThreadSafeQueue<UniquePtr<ImageData>>>())
        , result_queue(perception::make_unique<ThreadSafeQueue<OutputFrame>>())
        , thread_pool(perception::make_unique<ThreadPool>(config.thread_pool_size))
        , metrics(perception::make_unique<PerformanceMetrics>())
        , detector(perception::make_unique<vision::Detector>(
            config.detector_sensitivity,
            config.dnn_model_path,
            config.dnn_names_path,
            ::perception::constants::pipeline::DEFAULT_DETECTION_CONFIDENCE_THRESHOLD,
            ::perception::constants::pipeline::DEFAULT_DETECTION_NMS_THRESHOLD
        ))
        , tracker(perception::make_unique<tracking::Tracker>(
            ::perception::constants::pipeline::DEFAULT_IOU_THRESHOLD,
            ::perception::constants::pipeline::DEFAULT_MAX_TRACK_AGE
        )){}
};

/**
 * @brief Constructor
 * @param config Pipeline configuration
 */
AsyncProcessingPipeline::AsyncProcessingPipeline(const Config& config)
    : m_pimpl(perception::make_unique<Impl>(config)) {}

/**
 * @brief Destructor
 * @details Stops the pipeline if running
 */
AsyncProcessingPipeline::~AsyncProcessingPipeline() {
    stop();
}

/**
 * @brief Start the asynchronous processing pipeline
 * @return Result indicating success or failure
 * @details Submits processing tasks to thread pool
 */
auto AsyncProcessingPipeline::start() noexcept -> Result<void> {
    if (m_pimpl->is_running.exchange(true)) return {};

    for (Count i = 0; i < m_pimpl->thread_pool->get_thread_count(); ++i) {
        m_pimpl->thread_pool->submit([this] { processingLoop(); });
    }
    return {};
}

/**
 * @brief Stop the asynchronous processing pipeline
 * @details Shuts down queues and stops worker threads
 */
auto AsyncProcessingPipeline::stop() noexcept -> void {
    if (!m_pimpl->is_running.exchange(false)) return;
    m_pimpl->image_queue->shutdown();
    m_pimpl->result_queue->shutdown();
}

/**
 * @brief Submit an image for processing
 * @param image Image data to process
 * @return Result indicating success or failure
 */
auto AsyncProcessingPipeline::processImage(UniquePtr<ImageData> image) noexcept -> Result<void> {
    if (!m_pimpl->is_running.load()) return Result<void>(std::error_code(static_cast<int>(PerceptionError::QueueShutdown), std::generic_category()));
    m_pimpl->image_queue->push(std::move(image));
    return {};
}

/**
 * @brief Retrieve processed results
 * @param timeout Maximum time to wait for results
 * @return Result containing OutputFrame or error
 */
auto AsyncProcessingPipeline::getResults(Milliseconds timeout) noexcept -> Result<OutputFrame> {
    if (auto res = m_pimpl->result_queue->try_pop()) return std::move(*res);
    if (auto res = m_pimpl->result_queue->pop_timeout(timeout)) return std::move(*res);
    return Result<OutputFrame>(std::error_code(static_cast<int>(PerceptionError::QueueEmpty), std::generic_category()));
}

/**
 * @brief Get current performance metrics snapshot
 * @return Snapshot of performance metrics
 */
auto AsyncProcessingPipeline::getMetrics() const noexcept -> PerformanceMetrics::Snapshot {
    return m_pimpl->metrics->get_snapshot();
}

/**
 * @brief Main processing loop for worker threads
 * @details Pops images from queue, runs detection and tracking, pushes results
 */
auto AsyncProcessingPipeline::processingLoop() noexcept -> void {
    while (m_pimpl->is_running.load()) {
        auto img_opt = m_pimpl->image_queue->try_pop();
        if (!img_opt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        Vector<tracking::Track> tracks;
        {
            LatencyTimer timer(m_pimpl->metrics.get());
            auto detections = m_pimpl->detector->detect(*(*img_opt));
            tracks = m_pimpl->tracker->update(detections);
        }

        OutputFrame result{std::move(*img_opt), std::move(tracks)};
        m_pimpl->result_queue->push(std::move(result));
    }
}

} // namespace perception