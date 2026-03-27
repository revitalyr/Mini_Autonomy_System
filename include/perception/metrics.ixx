module;

#include <chrono>
#include <atomic>
#include <vector>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <format>
#include <print>

export module perception.metrics;

import perception.concepts;

export namespace perception {

    // Modern metrics with atomic operations
    class PerformanceMetrics {
    private:
        std::atomic<uint64_t> frame_count_{0};
        std::atomic<double> total_latency_ms_{0.0};
        std::atomic<double> min_latency_ms_{std::numeric_limits<double>::max()};
        std::atomic<double> max_latency_ms_{0.0};
        std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};
        
        // Sliding window for recent metrics
        mutable std::mutex recent_latencies_mutex_;
        std::vector<double> recent_latencies_;
        static constexpr size_t WINDOW_SIZE = 1000;

    public:
        struct MetricsSnapshot {
            double fps;
            double avg_latency_ms;
            double min_latency_ms;
            double max_latency_ms;
            double p95_latency_ms;
            double p99_latency_ms;
            uint64_t total_frames;
            std::chrono::milliseconds uptime;
        };

        void record_frame_latency(double latency_ms) noexcept {
            frame_count_.fetch_add(1, std::memory_order_relaxed);
            
            // Update atomic statistics
            total_latency_ms_.fetch_add(latency_ms, std::memory_order_relaxed);
            
            // Update min/max with compare_exchange
            double current_min = min_latency_ms_.load();
            while (latency_ms < current_min && 
                   !min_latency_ms_.compare_exchange_weak(current_min, latency_ms)) {
                current_min = min_latency_ms_.load();
            }
            
            double current_max = max_latency_ms_.load();
            while (latency_ms > current_max && 
                   !max_latency_ms_.compare_exchange_weak(current_max, latency_ms)) {
                current_max = max_latency_ms_.load();
            }
            
            // Update sliding window
            {
                std::lock_guard<std::mutex> lock(recent_latencies_mutex_);
                recent_latencies_.push_back(latency_ms);
                if (recent_latencies_.size() > WINDOW_SIZE) {
                    recent_latencies_.erase(recent_latencies_.begin());
                }
            }
        }

        MetricsSnapshot get_snapshot() const noexcept {
            const uint64_t frames = frame_count_.load();
            const double total_latency = total_latency_ms_.load();
            const double min_latency = min_latency_ms_.load();
            const double max_latency = max_latency_ms_.load();
            
            const auto now = std::chrono::steady_clock::now();
            const auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
            
            MetricsSnapshot snapshot;
            snapshot.total_frames = frames;
            snapshot.uptime = uptime;
            snapshot.avg_latency_ms = frames > 0 ? total_latency / frames : 0.0;
            snapshot.min_latency_ms = min_latency;
            snapshot.max_latency_ms = max_latency;
            
            // Calculate percentiles from sliding window
            {
                std::lock_guard<std::mutex> lock(recent_latencies_mutex_);
                if (!recent_latencies_.empty()) {
                    auto sorted_latencies = recent_latencies_;
                    std::ranges::sort(sorted_latencies);
                    
                    const size_t size = sorted_latencies.size();
                    const size_t p95_idx = static_cast<size_t>(0.95 * size);
                    const size_t p99_idx = static_cast<size_t>(0.99 * size);
                    
                    snapshot.p95_latency_ms = sorted_latencies[std::min(p95_idx, size - 1)];
                    snapshot.p99_latency_ms = sorted_latencies[std::min(p99_idx, size - 1)];
                } else {
                    snapshot.p95_latency_ms = 0.0;
                    snapshot.p99_latency_ms = 0.0;
                }
            }
            
            // Calculate FPS
            if (uptime.count() > 0) {
                snapshot.fps = (frames * 1000.0) / uptime.count();
            } else {
                snapshot.fps = 0.0;
            }
            
            return snapshot;
        }

        void reset() noexcept {
            frame_count_.store(0);
            total_latency_ms_.store(0.0);
            min_latency_ms_.store(std::numeric_limits<double>::max());
            max_latency_ms_.store(0.0);
            start_time_ = std::chrono::steady_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(recent_latencies_mutex_);
                recent_latencies_.clear();
            }
        }

        void print_snapshot() const {
            const auto snapshot = get_snapshot();
            
            std::println("=== Performance Metrics ===");
            std::println("FPS: {:.2f}", snapshot.fps);
            std::println("Total Frames: {}", snapshot.total_frames);
            std::println("Uptime: {}ms", snapshot.uptime.count());
            std::println("Average Latency: {:.2f}ms", snapshot.avg_latency_ms);
            std::println("Min Latency: {:.2f}ms", snapshot.min_latency_ms);
            std::println("Max Latency: {:.2f}ms", snapshot.max_latency_ms);
            std::println("95th Percentile: {:.2f}ms", snapshot.p95_latency_ms);
            std::println("99th Percentile: {:.2f}ms", snapshot.p99_latency_ms);
            std::println("========================");
        }
    };
}
