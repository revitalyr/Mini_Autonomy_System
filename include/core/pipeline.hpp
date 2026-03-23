#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include "thread_safe_queue.hpp"

class PipelineStage {
public:
    virtual ~PipelineStage() = default;
    virtual void run() = 0;
    virtual void stop() {}
};

class ThreadedPipeline {
private:
    std::vector<std::unique_ptr<PipelineStage>> stages_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
    
public:
    ~ThreadedPipeline() {
        stop();
    }
    
    template<typename Stage, typename... Args>
    void add_stage(Args&&... args) {
        stages_.emplace_back(std::make_unique<Stage>(std::forward<Args>(args)...));
    }
    
    void add_stage(std::unique_ptr<PipelineStage> stage) {
        stages_.push_back(std::move(stage));
    }
    
    void start() {
        if (running_) return;
        
        running_ = true;
        
        for (auto& stage : stages_) {
            workers_.emplace_back([stage_ptr = stage.get()]() {
                if (stage_ptr) {
                    stage_ptr->run();
                }
            });
        }
    }
    
    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        // Stop all stages
        for (auto& stage : stages_) {
            if (stage) {
                stage->stop();
            }
        }
        
        // Wait for all threads to finish
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        workers_.clear();
    }
    
    bool is_running() const {
        return running_;
    }
    
    size_t stage_count() const {
        return stages_.size();
    }
};
