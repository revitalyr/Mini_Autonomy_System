module;
#include <chrono>
#include <atomic>
#include <vector>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <mutex>
#include <limits>
#include <iostream>
#include <iomanip>

export module perception.metrics;
import perception.concepts;

/**
 * @file perception.metrics.ixx
 * @brief Performance metrics for the perception system
 *
 * @author Mini Autonomy System
 * @date 2026
 */

namespace perception {

    /**
     * @brief Performance metrics tracker
     */
    export class PerformanceMetrics {
    private:
        std::atomic<size_t> frame_count_{0};
        std::atomic<double> total_latency_ms_{0.0};
        std::atomic<double> min_latency_ms_{std::numeric_limits<double>::max()};
        std::atomic<double> max_latency_ms_{0.0};
        std::vector<double> latency_samples_;
        mutable std::mutex samples_mutex_;
        TimePoint start_time_;

    public:
        PerformanceMetrics() : start_time_(Clock::now()) {}

        /**
         * @brief Snapshot of current metrics
         */
        export struct Snapshot {
            double fps;
            double avg_latency_ms;
            double min_latency_ms;
            double max_latency_ms;
            double p95_latency_ms;
            double p99_latency_ms;
            size_t total_frames;
        };

        /**
         * @brief Record frame latency
         */
        void record_frame_latency(double latency_ms) {
            frame_count_.fetch_add(1, std::memory_order_relaxed);
            total_latency_ms_.fetch_add(latency_ms, std::memory_order_relaxed);
            
            // Update min/max
            double current_min = min_latency_ms_.load(std::memory_order_relaxed);
            while (latency_ms < current_min) {
                if (min_latency_ms_.compare_exchange_weak(current_min, latency_ms, 
                                                       std::memory_order_relaxed)) {
                    break;
                }
            }
            
            double current_max = max_latency_ms_.load(std::memory_order_relaxed);
            while (latency_ms > current_max) {
                if (max_latency_ms_.compare_exchange_weak(current_max, latency_ms, 
                                                       std::memory_order_relaxed)) {
                    break;
                }
            }
            
            // Store sample for percentiles
            {
                std::lock_guard<std::mutex> lock(samples_mutex_);
                latency_samples_.push_back(latency_ms);
                if (latency_samples_.size() > 10000) {
                    latency_samples_.erase(latency_samples_.begin());
                }
            }
        }

        /**
         * @brief Get current snapshot
         */
        Snapshot get_snapshot() const {
            Snapshot snapshot;
            
            size_t frames = frame_count_.load(std::memory_order_relaxed);
            double total = total_latency_ms_.load(std::memory_order_relaxed);
            
            auto now = Clock::now();
            auto duration = std::chrono::duration_cast<Milliseconds>(now - start_time_);
            double seconds = duration.count() / 1000.0;
            
            snapshot.fps = seconds > 0 ? frames / seconds : 0.0;
            snapshot.avg_latency_ms = frames > 0 ? total / frames : 0.0;
            snapshot.min_latency_ms = min_latency_ms_.load(std::memory_order_relaxed);
            snapshot.max_latency_ms = max_latency_ms_.load(std::memory_order_relaxed);
            snapshot.total_frames = frames;
            
            // Calculate percentiles
            {
                std::lock_guard<std::mutex> lock(samples_mutex_);
                if (!latency_samples_.empty()) {
                    std::vector<double> sorted = latency_samples_;
                    std::sort(sorted.begin(), sorted.end());
                    
                    size_t p95_idx = static_cast<size_t>(sorted.size() * 0.95);
                    size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
                    
                    p95_idx = std::min(p95_idx, sorted.size() - 1);
                    p99_idx = std::min(p99_idx, sorted.size() - 1);
                    
                    snapshot.p95_latency_ms = sorted[p95_idx];
                    snapshot.p99_latency_ms = sorted[p99_idx];
                } else {
                    snapshot.p95_latency_ms = 0.0;
                    snapshot.p99_latency_ms = 0.0;
                }
            }
            
            return snapshot;
        }

        /**
         * @brief Reset metrics
         */
        void reset() {
            frame_count_.store(0, std::memory_order_relaxed);
            total_latency_ms_.store(0.0, std::memory_order_relaxed);
            min_latency_ms_.store(std::numeric_limits<double>::max(), std::memory_order_relaxed);
            max_latency_ms_.store(0.0, std::memory_order_relaxed);
            
            {
                std::lock_guard<std::mutex> lock(samples_mutex_);
                latency_samples_.clear();
            }
            
            start_time_ = Clock::now();
        }

        /**
         * @brief Print metrics to console
         */
        void print() const {
            auto snapshot = get_snapshot();
            
            std::cout << "=== Performance Metrics ===\n";
            std::cout << "FPS: " << std::fixed << std::setprecision(2) << snapshot.fps << "\n";
            std::cout << "Total frames: " << snapshot.total_frames << "\n";
            std::cout << "Average latency: " << snapshot.avg_latency_ms << " ms\n";
            std::cout << "Min latency: " << snapshot.min_latency_ms << " ms\n";
            std::cout << "Max latency: " << snapshot.max_latency_ms << " ms\n";
            std::cout << "P95 latency: " << snapshot.p95_latency_ms << " ms\n";
            std::cout << "P99 latency: " << snapshot.p99_latency_ms << " ms\n";
        }
    };

    /**
     * @brief RAII helper for timing operations
     */
    export class LatencyTimer {
    private:
        PerformanceMetrics& metrics_;
        TimePoint start_;

    public:
        explicit LatencyTimer(PerformanceMetrics& metrics) 
            : metrics_(metrics), start_(Clock::now()) {}

        ~LatencyTimer() {
            auto end = Clock::now();
            auto duration = std::chrono::duration_cast<Milliseconds>(end - start_);
            metrics_.record_frame_latency(static_cast<double>(duration.count()));
        }

        LatencyTimer(const LatencyTimer&) = delete;
        LatencyTimer& operator=(const LatencyTimer&) = delete;
    };

} // namespace perception
