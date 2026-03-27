/**
 * @file main.cpp
 * @brief Main demo application for the Mini Autonomy System
 * 
 * This file demonstrates the usage of the perception system modules including:
 * - Thread-safe queue operations
 * - Performance metrics tracking
 * - Object detection with OpenCV
 * - Thread pool for async operations
 * - C++20 modules integration
 * 
 * @author Mini Autonomy System
 * @date 2026
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>

// Import perception system modules
import perception.concepts;
import perception.queue;
import perception.pipeline;
import perception.metrics;
import perception.detector;
import perception.async;
import perception.result;

/**
 * @brief Main function demonstrating the perception system
 * 
 * This function creates and tests all major components of the perception system:
 * 1. Thread-safe queue for data flow
 * 2. Performance metrics for monitoring
 * 3. Mock detector for object detection
 * 4. Thread pool for async operations
 * 
 * @return int Exit code (0 for success)
 */
int main() {
    std::cout << "=== Mini Autonomy System Demo (C++20 Modules) ===" << std::endl;
    std::cout << "Program started successfully!" << std::endl;
    
    try {
        // Create components using modules
        std::cout << "Creating queue..." << std::endl;
        /// Thread-safe queue for integer data
        auto queue = std::make_unique<perception::ThreadSafeQueue<int>>();
        std::cout << "Queue created!" << std::endl;
        
        std::cout << "Creating metrics..." << std::endl;
        /// Performance metrics tracker
        auto metrics = std::make_unique<perception::PerformanceMetrics>();
        std::cout << "Metrics created!" << std::endl;
        
        std::cout << "Creating detector..." << std::endl;
        /// Mock object detector using OpenCV
        auto detector = std::make_unique<perception::MockDetector>();
        std::cout << "Detector created!" << std::endl;
        
        std::cout << "Creating thread pool..." << std::endl;
        /// Thread pool for async operations
        auto thread_pool = std::make_unique<perception::ThreadPool>();
        std::cout << "Thread pool created!" << std::endl;
        
        // Initialize components
        std::cout << "Initializing components..." << std::endl;
        
        // Test queue functionality
        std::cout << "Testing thread-safe queue..." << std::endl;
        /// Push test values to queue
        queue->push(42);
        queue->push(100);
        
        /// Try to pop values from queue
        auto result = queue->try_pop();
        if (result) {
            std::cout << "Popped value: " << *result << std::endl;
        }
        
        // Test metrics
        std::cout << "Testing performance metrics..." << std::endl;
        /// Record some frame latencies for testing
        metrics->record_frame_latency(10.5);
        metrics->record_frame_latency(12.3);
        metrics->record_frame_latency(9.8);
        
        // Test detector
        std::cout << "Testing detector..." << std::endl;
        /// Create a test frame (black image)
        cv::Mat test_frame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
        /// Run detection on test frame
        auto detections = detector->detect(test_frame);
        std::cout << "Found " << detections.size() << " detections" << std::endl;
        
        /// Print detection results
        for (const auto& det : detections) {
            std::cout << "  Detection: class=" << det.class_name 
                      << " confidence=" << det.confidence 
                      << " bbox=(" << det.bbox.x << "," << det.bbox.y << "," << det.bbox.width << "," << det.bbox.height << ")" 
                      << std::endl;
        }
        
        // Test thread pool
        std::cout << "Testing thread pool..." << std::endl;
        /// Submit a simple async task
        auto future = thread_pool->submit([]() -> int {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 42;
        });
        
        /// Get the result from async task
        std::cout << "Thread pool result: " << future.get() << std::endl;
        
        // Print metrics
        std::cout << "\n=== Performance Metrics ===" << std::endl;
        /// Get performance snapshot
        auto snapshot = metrics->get_snapshot();
        std::cout << "FPS: " << snapshot.fps << std::endl;
        std::cout << "Average Latency: " << snapshot.avg_latency_ms << "ms" << std::endl;
        std::cout << "Total Frames: " << snapshot.total_frames << std::endl;
        std::cout << "Uptime: " << snapshot.duration_seconds << "s" << std::endl;
        
        std::cout << "\nDemo completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        /// Handle any exceptions that occur during demo
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        /// Handle any unknown exceptions
        std::cout << "Unknown error occurred!" << std::endl;
        return 1;
    }
}
