module;
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <ranges>
#include <algorithm>
#include <stop_token>
#include <future>
#include <coroutine>

export module perception.pipeline;
export import perception.types;
import perception.concepts;
import perception.queue;
import perception.async;

namespace perception {

    // Modern pipeline stage with concepts
    export template<PipelineStage Stage>
    class PipelineStageWrapper {
    private:
        Stage stage_;
        std::stop_source stop_source_;
        
    public:
        template<typename... Args>
        explicit PipelineStageWrapper(Args&&... args) 
            : stage_(std::forward<Args>(args)...) {}

        void run() {
            std::stop_token token = stop_source_.get_token();
            while (!token.stop_requested()) {
                stage_.run();
            }
            stage_.stop();
        }

        void stop() {
            stop_source_.request_stop();
        }

        Stage& get_stage() noexcept { return stage_; }
        const Stage& get_stage() const noexcept { return stage_; }
    };

    // Modern async pipeline with coroutines support
    export class AsyncPipeline {
    private:
        struct StageContainer {
            std::unique_ptr<void, std::function<void(void*)>> instance;
            std::function<void()> run_fn;
            std::function<void()> stop_fn;
        };

        std::vector<StageContainer> stages_;
        std::future<void> pipeline_task_;
        std::atomic<bool> running_{false};

    public:
        AsyncPipeline() = default;
        
        ~AsyncPipeline() {
            stop();
        }

        // Add stage with type erasure for any PipelineStage
        template<PipelineStage Stage, typename... Args>
        void add_stage(Args&&... args) {
            auto stage_wrapper = std::make_unique<PipelineStageWrapper<Stage>>(
                std::forward<Args>(args)...
            );
            
            StageContainer container;
            
            // Create run/stop type-erased delegates
            container.run_fn = [raw = stage_wrapper.get()]() { raw->run(); };
            container.stop_fn = [raw = stage_wrapper.get()]() { raw->stop(); };
            
            // Store ownership
            container.instance = std::unique_ptr<void, std::function<void(void*)>>(
                stage_wrapper.release(),
                [](void* ptr) { delete static_cast<PipelineStageWrapper<Stage>*>(ptr); }
            );
            
            stages_.push_back(std::move(container));
        }

        void start() {
            if (running_) return;

            running_ = true;

            // Spawn the pipeline supervisor
            pipeline_task_ = std::async(std::launch::async, [this]() {
                for (auto& stage : stages_) {
                    std::thread([&stage]() {
                        stage.run_fn();
                    }).detach();
                }
            });
        }

        void stop() noexcept {
            if (!running_) return;
            
            running_ = false;
            
            // Stop all stages
            for (auto& stage : stages_) {
                stage.stop_fn();
            }
            
            // Wait for pipeline supervisor (and all stages) to finish
            if (pipeline_task_.valid()) {
                pipeline_task_.wait();
            }
        }

        [[nodiscard]] bool is_running() const noexcept {
            return running_;
        }

        [[nodiscard]] size_t stage_count() const noexcept {
            return stages_.size();
        }
    };

    // Modern functional pipeline with ranges
    export template<typename Input, typename Output>
    class FunctionalPipeline {
    private:
        std::vector<std::function<Output(const Input&)>> stages_;
        
    public:
        template<typename Func>
        requires std::invocable<Func, Input>
        void add_stage(Func&& func) {
            stages_.emplace_back(std::forward<Func>(func));
        }

        Output process(const Input& input) const {
            auto result = input;
            for (const auto& stage : stages_) {
                // For simplicity, assuming all stages return the same type
                // In practice, you'd use type composition
                result = stage(result);
            }
            return result;
        }

        // Process a range of inputs
        auto process_range(std::ranges::range auto&& inputs) const {
            return inputs | std::views::transform([this](const auto& input) {
                return process(input);
            });
        }
    };

}
