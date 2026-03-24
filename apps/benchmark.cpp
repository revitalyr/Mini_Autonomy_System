#include "core/benchmark.hpp"
#include "core/thread_safe_queue.hpp"
#include "vision/detector.hpp"
#include "vision/tracker.hpp"
#include "vision/fusion.hpp"
#include "io/imu_simulator.hpp"
#include "core/metrics.hpp"
#include <opencv2/opencv.hpp>
#include <random>

class PerformanceBenchmark {
private:
    Benchmark benchmark_;
    cv::Mat test_frame_;
    std::vector<Detection> test_detections_;
    std::vector<Track> test_tracks_;
    
public:
    PerformanceBenchmark() {
        // Create test frame
        test_frame_ = cv::Mat::zeros(480, 640, CV_8UC3);
        cv::rectangle(test_frame_, cv::Rect(100, 100, 80, 80), cv::Scalar(255, 255, 255), -1);
        
        // Create test detections
        test_detections_.emplace_back(cv::Rect(100, 100, 50, 50), 0.9f, 0);
        test_detections_.emplace_back(cv::Rect(200, 200, 60, 60), 0.8f, 1);
        
        // Create test tracks
        test_tracks_.emplace_back(1, cv::Rect(100, 100, 50, 50), 1.0f, 0.5f);
        test_tracks_.emplace_back(2, cv::Rect(200, 200, 60, 60), -0.5f, 0.3f);
    }
    
    void run_all_benchmarks() {
        std::cout << "=== Running Performance Benchmarks ===" << std::endl;
        
        benchmark_thread_safe_queue();
        benchmark_detector();
        benchmark_tracker();
        benchmark_fusion();
        benchmark_imu();
        benchmark_metrics();
        
        benchmark_.print_results();
        benchmark_.save_to_csv("benchmark_results.csv");
        
        std::cout << "\nResults saved to benchmark_results.csv" << std::endl;
    }
    
private:
    void benchmark_thread_safe_queue() {
        ThreadSafeQueue<int> queue;
        
        // Benchmark push operations
        auto push_result = benchmark_.benchmark("ThreadSafeQueue_Push", [&queue]() {
            queue.push(42);
        }, 10000);
        
        // Benchmark pop operations
        auto pop_result = benchmark_.benchmark("ThreadSafeQueue_Pop", [&queue]() {
            int value;
            queue.pop(value, std::chrono::milliseconds(10));
        }, 10000);
        
        // Benchmark concurrent operations
        ThreadSafeQueue<int> concurrent_queue;
        auto concurrent_result = benchmark_.benchmark("ThreadSafeQueue_Concurrent", [&concurrent_queue]() {
            // Simulate producer-consumer pattern
            concurrent_queue.push(42);
            int value;
            concurrent_queue.pop(value, std::chrono::milliseconds(1));
        }, 5000);
    }
    
    void benchmark_detector() {
        Detector detector("", 0.5f, 0.4f, 640, 640);
        
        auto result = benchmark_.benchmark("Detector_Detect", [&detector, this]() {
            auto detections = detector.detect(test_frame_);
            // Prevent compiler optimization
            volatile size_t count = detections.size();
            (void)count;
        }, 1000);
    }
    
    void benchmark_tracker() {
        Tracker tracker(30, 10, 0.3f);
        
        auto result = benchmark_.benchmark("Tracker_Update", [&tracker, this]() {
            auto tracks = tracker.update(test_detections_);
            // Prevent compiler optimization
            volatile size_t count = tracks.size();
            (void)count;
        }, 1000);
    }
    
    void benchmark_fusion() {
        Fusion fusion;
        IMUSimulator imu;
        
        auto result = benchmark_.benchmark("Fusion_Update", [&fusion, &imu, this]() {
            auto pose = fusion.update(test_tracks_, imu.get_data());
            // Prevent compiler optimization
            volatile float x = pose.x;
            (void)x;
        }, 1000);
    }
    
    void benchmark_imu() {
        IMUSimulator imu;
        
        auto result = benchmark_.benchmark("IMU_GetData", [&imu]() {
            auto data = imu.get_data();
            // Prevent compiler optimization
            volatile float ax = data.ax;
            (void)ax;
        }, 10000);
    }
    
    void benchmark_metrics() {
        Metrics metrics;
        
        auto result = benchmark_.benchmark("Metrics_Frame", [&metrics]() {
            metrics.frame_start();
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            metrics.frame_end();
        }, 10000);
    }
    
    void benchmark_memory_operations() {
        std::vector<int> large_vector(10000);
        
        auto result = benchmark_.benchmark("Memory_Copy", [&large_vector]() {
            std::vector<int> copy = large_vector;
            // Prevent compiler optimization
            volatile size_t size = copy.size();
            (void)size;
        }, 1000);
    }
    
    void benchmark_math_operations() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        std::vector<float> data(1000);
        for (auto& val : data) {
            val = dist(gen);
        }
        
        auto result = benchmark_.benchmark("Math_Operations", [&data]() {
            float sum = 0.0f;
            for (float val : data) {
                sum += std::sin(val) * std::cos(val) + std::sqrt(std::abs(val));
            }
            // Prevent compiler optimization
            volatile float result = sum;
            (void)result;
        }, 1000);
    }
};

int main() {
    PerformanceBenchmark perf_benchmark;
    perf_benchmark.run_all_benchmarks();
    return 0;
}
