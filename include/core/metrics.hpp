#pragma once

#include <chrono>
#include <iostream>
#include <string>

class Metrics {
private:
    std::chrono::steady_clock::time_point frame_start_time_;
    float last_latency_ms_ = 0.0f;
    int total_frames_ = 0;
    std::chrono::steady_clock::time_point start_time_;
    
public:
    Metrics() : start_time_(std::chrono::steady_clock::now()) {}
    
    void frame_start() {
        frame_start_time_ = std::chrono::steady_clock::now();
    }
    
    void frame_end() {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - frame_start_time_);
        last_latency_ms_ = duration.count() / 1000.0f;
        total_frames_++;
    }
    
    float get_last_latency() const { return last_latency_ms_; }
    
    float get_fps() const {
        if (last_latency_ms_ > 0.0f) {
            return 1000.0f / last_latency_ms_;
        }
        return 0.0f;
    }
    
    int get_total_frames() const { return total_frames_; }
    
    float get_average_fps() const {
        auto now = std::chrono::steady_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        if (total_time.count() > 0) {
            return static_cast<float>(total_frames_) / total_time.count();
        }
        return 0.0f;
    }
    
    void print_stats() const {
        std::cout << "=== Performance Metrics ===" << std::endl;
        std::cout << "Current FPS: " << get_fps() << std::endl;
        std::cout << "Average FPS: " << get_average_fps() << std::endl;
        std::cout << "Frame Latency: " << last_latency_ms_ << " ms" << std::endl;
        std::cout << "Total Frames: " << total_frames_ << std::endl;
        std::cout << "===========================" << std::endl;
    }
};
