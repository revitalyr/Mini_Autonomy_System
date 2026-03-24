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
            double current_min = min_latency_ms_.load(std::memory_order_relaxed);
            while (latency_ms < current_min && 
                   !min_latency_ms_.compare_exchange_weak(current_min, latency_ms, 
                                                          std::memory_order_relaxed)) {
                // Retry if compare_exchange failed
            }
            
            double current_max = max_latency_ms_.load(std::memory_order_relaxed);
            while (latency_ms > current_max && 
                   !max_latency_ms_.compare_exchange_weak(current_max, latency_ms, 
                                                          std::memory_order_relaxed)) {
                // Retry if compare_exchange failed
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

        [[nodiscard]] MetricsSnapshot get_snapshot() const {
            const uint64_t frames = frame_count_.load(std::memory_order_relaxed);
            const auto now = std::chrono::steady_clock::now();
            const auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
            
            double fps = 0.0;
            if (uptime.count() > 0) {
                fps = (static_cast<double>(frames) * 1000.0) / static_cast<double>(uptime.count());
            }
            
            const double total_latency = total_latency_ms_.load(std::memory_order_relaxed);
            const double avg_latency = frames > 0 ? total_latency / frames : 0.0;
            const double min_lat = min_latency_ms_.load(std::memory_order_relaxed);
            const double max_lat = max_latency_ms_.load(std::memory_order_relaxed);
            
            // Calculate percentiles from sliding window
            double p95 = 0.0, p99 = 0.0;
            {
                std::lock_guard<std::mutex> lock(recent_latencies_mutex_);
                if (!recent_latencies_.empty()) {
                    auto sorted_latencies = recent_latencies_;
                    std::ranges::sort(sorted_latencies);
                    
                    const size_t size = sorted_latencies.size();
                    p95 = sorted_latencies[static_cast<size_t>(size * 0.95)];
                    p99 = sorted_latencies[static_cast<size_t>(size * 0.99)];
                }
            }
            
            return {
                .fps = fps,
                .avg_latency_ms = avg_latency,
                .min_latency_ms = min_lat == std::numeric_limits<double>::max() ? 0.0 : min_lat,
                .max_latency_ms = max_lat,
                .p95_latency_ms = p95,
                .p99_latency_ms = p99,
                .total_frames = frames,
                .uptime = uptime
            };
        }

        void reset() noexcept {
            frame_count_.store(0, std::memory_order_relaxed);
            total_latency_ms_.store(0.0, std::memory_order_relaxed);
            min_latency_ms_.store(std::numeric_limits<double>::max(), std::memory_order_relaxed);
            max_latency_ms_.store(0.0, std::memory_order_relaxed);
            start_time_ = std::chrono::steady_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(recent_latencies_mutex_);
                recent_latencies_.clear();
            }
        }

        // C++23 std::print support
        void print_metrics() const {
            const auto snapshot = get_snapshot();
            std::println("📊 Performance Metrics:");
            std::println("   FPS: {:.2f}", snapshot.fps);
            std::println("   Avg Latency: {:.2f}ms", snapshot.avg_latency_ms);
            std::println("   Min/Max Latency: {:.2f}ms / {:.2f}ms", 
                        snapshot.min_latency_ms, snapshot.max_latency_ms);
            std::println("   P95/P99 Latency: {:.2f}ms / {:.2f}ms", 
                        snapshot.p95_latency_ms, snapshot.p99_latency_ms);
            std::println("   Total Frames: {}", snapshot.total_frames);
            std::println("   Uptime: {}ms", snapshot.uptime.count());
        }
    };

    // RAII timer for automatic latency recording
    class LatencyTimer {
    private:
        PerformanceMetrics& metrics_;
        std::chrono::steady_clock::time_point start_time_;

    public:
        explicit LatencyTimer(PerformanceMetrics& metrics) 
            : metrics_(metrics), start_time_(std::chrono::steady_clock::now()) {}

        ~LatencyTimer() noexcept {
            const auto end_time = std::chrono::steady_clock::now();
            const auto latency = std::chrono::duration<double, std::milli>(end_time - start_time_);
            metrics_.record_frame_latency(latency.count());
        }

        // Disable copy and move
        LatencyTimer(const LatencyTimer&) = delete;
        LatencyTimer& operator=(const LatencyTimer&) = delete;
        LatencyTimer(LatencyTimer&&) = delete;
        LatencyTimer& operator=(LatencyTimer&&) = delete;
    };

}
