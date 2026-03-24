#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <functional>

class Benchmark {
private:
    struct BenchmarkResult {
        std::string name;
        double mean_ms;
        double std_dev_ms;
        double min_ms;
        double max_ms;
        size_t iterations;
        double throughput_ops_per_sec;
    };
    
    std::map<std::string, BenchmarkResult> results_;
    
public:
    // Benchmark a function with multiple iterations
    template<typename Func>
    BenchmarkResult benchmark(const std::string& name, Func&& func, 
                             size_t iterations = 100, size_t warmup = 10) {
        
        // Warmup
        for (size_t i = 0; i < warmup; ++i) {
            func();
        }
        
        std::vector<double> times;
        times.reserve(iterations);
        
        // Benchmark iterations
        for (size_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration<double, std::milli>(end - start);
            times.push_back(duration.count());
        }
        
        // Calculate statistics
        double sum = 0.0;
        double min_time = times[0];
        double max_time = times[0];
        
        for (double time : times) {
            sum += time;
            min_time = std::min(min_time, time);
            max_time = std::max(max_time, time);
        }
        
        double mean = sum / iterations;
        
        // Calculate standard deviation
        double variance = 0.0;
        for (double time : times) {
            variance += (time - mean) * (time - mean);
        }
        variance /= iterations;
        double std_dev = std::sqrt(variance);
        
        // Calculate throughput
        double throughput = 1000.0 / mean; // operations per second
        
        BenchmarkResult result;
        result.name = name;
        result.mean_ms = mean;
        result.std_dev_ms = std_dev;
        result.min_ms = min_time;
        result.max_ms = max_time;
        result.iterations = iterations;
        result.throughput_ops_per_sec = throughput;
        
        results_[name] = result;
        return result;
    }
    
    // Benchmark throughput (operations per second)
    template<typename Func>
    double benchmark_throughput(const std::string& name, Func&& func, 
                               double duration_seconds = 1.0) {
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + std::chrono::duration<double>(duration_seconds);
        
        size_t count = 0;
        while (std::chrono::high_resolution_clock::now() < end_time) {
            func();
            count++;
        }
        
        double throughput = count / duration_seconds;
        return throughput;
    }
    
    // Print all benchmark results
    void print_results() const {
        std::cout << "\n=== Benchmark Results ===" << std::endl;
        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::left << std::setw(25) << "Benchmark" 
                  << std::setw(12) << "Mean (ms)" 
                  << std::setw(12) << "StdDev (ms)"
                  << std::setw(12) << "Min (ms)"
                  << std::setw(12) << "Max (ms)"
                  << std::setw(15) << "Throughput" << std::endl;
        std::cout << std::string(90, '-') << std::endl;
        
        for (const auto& [name, result] : results_) {
            std::cout << std::left << std::setw(25) << result.name
                      << std::setw(12) << result.mean_ms
                      << std::setw(12) << result.std_dev_ms
                      << std::setw(12) << result.min_ms
                      << std::setw(12) << result.max_ms
                      << std::setw(15) << result.throughput_ops_per_sec << " ops/s"
                      << std::endl;
        }
        std::cout << std::string(90, '=') << std::endl;
    }
    
    // Print single benchmark result
    void print_result(const std::string& name) const {
        auto it = results_.find(name);
        if (it != results_.end()) {
            const auto& result = it->second;
            std::cout << "\n=== " << result.name << " ===" << std::endl;
            std::cout << "Mean: " << result.mean_ms << " ms" << std::endl;
            std::cout << "StdDev: " << result.std_dev_ms << " ms" << std::endl;
            std::cout << "Min: " << result.min_ms << " ms" << std::endl;
            std::cout << "Max: " << result.max_ms << " ms" << std::endl;
            std::cout << "Throughput: " << result.throughput_ops_per_sec << " ops/s" << std::endl;
            std::cout << "Iterations: " << result.iterations << std::endl;
        }
    }
    
    // Save results to CSV
    void save_to_csv(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return;
        }
        
        file << "Benchmark,Mean_ms,StdDev_ms,Min_ms,Max_ms,Iterations,Throughput_ops_per_sec\n";
        
        for (const auto& [name, result] : results_) {
            file << result.name << ","
                 << result.mean_ms << ","
                 << result.std_dev_ms << ","
                 << result.min_ms << ","
                 << result.max_ms << ","
                 << result.iterations << ","
                 << result.throughput_ops_per_sec << "\n";
        }
    }
    
    // Clear all results
    void clear() {
        results_.clear();
    }
    
    // Get specific result
    const BenchmarkResult* get_result(const std::string& name) const {
        auto it = results_.find(name);
        return (it != results_.end()) ? &it->second : nullptr;
    }
};
