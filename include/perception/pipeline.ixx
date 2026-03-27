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

export module perception.pipeline;

import perception.concepts;
import perception.queue;
import perception.async;

export namespace perception {

    // Modern pipeline stage with concepts
    template<PipelineStage Stage>
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
    class AsyncPipeline {
    private:
        std::vector<std::unique_ptr<PipelineStageWrapperBase>> stages_;
        std::vector<std::jthread> workers_;
        std::atomic<bool> running_{false};
        
    public:
        template<PipelineStage Stage, typename... Args>
        void add_stage(Args&&... args) {
            stages_.emplace_back(std::make_unique<PipelineStageWrapper<Stage>>(std::forward<Args>(args)...));
        }
        
        void start() {
            running_ = true;
            
            // Create worker threads
            for (auto& stage : stages_) {
                workers_.emplace_back([stage = stage.get()]() mutable {
                    stage.run();
                });
            }
        }
        
        void stop() {
            running_ = false;
            
            // Stop all stages
            for (auto& stage : stages_) {
                stage.stop();
            }
            
            // Wait for all threads
            for (auto& worker : workers_) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }
        
        bool is_running() const noexcept {
            return running_.load();
        }
        
        size_t stage_count() const noexcept {
            return stages_.size();
        }
    };
}
