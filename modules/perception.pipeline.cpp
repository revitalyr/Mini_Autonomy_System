module;

#include <thread>
#include <chrono>

module perception.pipeline;

import perception.queue;
import perception.metrics;
import perception.detector;
import perception.tracking;
import perception.async;

namespace perception {

/**
 * @brief RAII helper to automatically record processing latency
 */
class LatencyTimer {
public:
    PerformanceMetrics* metrics;
    Clock::time_point start_time;

    explicit LatencyTimer(PerformanceMetrics* m) 
        : metrics(m), start_time(Clock::now()) {}

    ~LatencyTimer() {
        if (metrics) {
            auto end_time = Clock::now();
            metrics->record_frame_latency(static_cast<double>(to_milliseconds(end_time - start_time).count()));
        }
    }
};

struct AsyncProcessingPipeline::Impl {
    UniquePtr<ThreadSafeQueue<geom::ImageData>> image_queue;
    UniquePtr<ThreadSafeQueue<OutputFrame>> result_queue;
    UniquePtr<ThreadPool> thread_pool;
    UniquePtr<PerformanceMetrics> metrics;
    UniquePtr<vision::Detector> detector;
    UniquePtr<tracking::Tracker> tracker;
    AtomicBool is_running{false};

    Impl(const Config& config)
        : image_queue(perception::make_unique<ThreadSafeQueue<geom::ImageData>>())
        , result_queue(perception::make_unique<ThreadSafeQueue<OutputFrame>>())
        , thread_pool(perception::make_unique<ThreadPool>(config.thread_pool_size))
        , metrics(perception::make_unique<PerformanceMetrics>())
        , detector(perception::make_unique<vision::Detector>(
            config.detector_sensitivity,
            config.dnn_model_path,
            config.dnn_names_path,
            config.dnn_confidence_threshold,
            config.dnn_nms_threshold
        ))
        , tracker(perception::make_unique<tracking::Tracker>(config.tracker_iou_threshold, config.tracker_max_age))
    {}
};

AsyncProcessingPipeline::AsyncProcessingPipeline(const Config& config)
    : m_pimpl(perception::make_unique<Impl>(config)) {}

AsyncProcessingPipeline::~AsyncProcessingPipeline() {
    stop();
}

auto AsyncProcessingPipeline::start() noexcept -> Result<void> {
    if (m_pimpl->is_running.exchange(true)) return {};

    for (Count i = 0; i < m_pimpl->thread_pool->get_thread_count(); ++i) {
        m_pimpl->thread_pool->submit([this] { processingLoop(); });
    }
    return {};
}

auto AsyncProcessingPipeline::stop() noexcept -> void {
    if (!m_pimpl->is_running.exchange(false)) return;
    m_pimpl->image_queue->shutdown();
    m_pimpl->result_queue->shutdown();
}

auto AsyncProcessingPipeline::processImage(geom::ImageData image) noexcept -> Result<void> {
    if (!m_pimpl->is_running.load()) return Result<void>(std::error_code(static_cast<int>(PerceptionError::QueueShutdown), std::generic_category()));
    m_pimpl->image_queue->push(std::move(image));
    return {};
}

auto AsyncProcessingPipeline::getResults(Milliseconds timeout) noexcept -> Result<OutputFrame> {
    if (auto res = m_pimpl->result_queue->try_pop()) return std::move(*res);
    if (auto res = m_pimpl->result_queue->pop_timeout(timeout)) return std::move(*res);
    return Result<OutputFrame>(std::error_code(static_cast<int>(PerceptionError::QueueEmpty), std::generic_category()));
}

auto AsyncProcessingPipeline::getMetrics() const noexcept -> PerformanceMetrics::Snapshot {
    return m_pimpl->metrics->get_snapshot();
}

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
            auto detections = m_pimpl->detector->detect(*img_opt);
            tracks = m_pimpl->tracker->update(detections);
        }

        OutputFrame result{std::move(*img_opt), std::move(tracks)};
        m_pimpl->result_queue->push(std::move(result));
    }
}

} // namespace perception