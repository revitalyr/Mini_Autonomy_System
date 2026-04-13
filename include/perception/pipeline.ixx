module;
#include <utility>

export module perception.pipeline;
import perception.types;
import perception.geom;
import perception.tracking; // Необходимо для псевдонима OutputFrame
import perception.metrics;
import perception.result;

export namespace perception {

    /**
     * @brief Pipeline for asynchronous image processing and object detection
     */
    class AsyncProcessingPipeline {
    public:
        explicit AsyncProcessingPipeline(const Config& config = Config{});
        ~AsyncProcessingPipeline();

        AsyncProcessingPipeline(const AsyncProcessingPipeline&) = delete;
        auto operator=(const AsyncProcessingPipeline&) -> AsyncProcessingPipeline& = delete;

        auto start() noexcept -> Result<void>;
        auto stop() noexcept -> void;

        auto processImage(geom::ImageData image) noexcept -> Result<void>;
        
        using OutputFrame = std::pair<geom::ImageData, Vector<tracking::Track>>;
        auto getResults(Milliseconds timeout = Milliseconds{1000}) noexcept -> Result<OutputFrame>;

        auto getMetrics() const noexcept -> PerformanceMetrics::Snapshot;

    private:
        void processingLoop() noexcept;
        struct Impl;
        UniquePtr<Impl> m_pimpl;
    };

}