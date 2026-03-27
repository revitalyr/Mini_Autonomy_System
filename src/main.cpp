import perception.concepts;
import perception.queue;
import perception.pipeline;
import perception.metrics;
import perception.detector;
import perception.async;
import perception.result;

import <iostream>;
import <memory>;
import <thread>;
import <chrono>;

int main() {
    std::cout << "=== Mini Autonomy System Demo (C++20 Modules) ===" << std::endl;
    
    // Create components using modules
    auto queue = std::make_unique<perception::ThreadSafeQueue<int>>();
    auto metrics = std::make_unique<perception::PerformanceMetrics>();
    auto detector = std::make_unique<perception::MockDetector>();
    auto thread_pool = std::make_unique<perception::ThreadPool>();
    
    // Initialize components
    std::cout << "Initializing components..." << std::endl;
    
    // Test queue functionality
    std::cout << "Testing thread-safe queue..." << std::endl;
    queue->push(42);
    queue->push(100);
    
    auto result = queue->try_pop();
    if (result) {
        std::cout << "Popped value: " << *result << std::endl;
    }
    
    // Test metrics
    std::cout << "Testing performance metrics..." << std::endl;
    metrics->record_frame_latency(10.5);
    metrics->record_frame_latency(12.3);
    metrics->record_frame_latency(9.8);
    
    // Test detector
    std::cout << "Testing detector..." << std::endl;
    auto detections = detector->detect();
    std::cout << "Found " << detections.size() << " detections" << std::endl;
    
    for (const auto& det : detections) {
        std::cout << "  Detection: class=" << det.class_name 
                  << " confidence=" << det.confidence 
                  << " bbox=(" << det.x << "," << det.y << "," << det.width << "," << det.height << ")" 
                  << std::endl;
    }
    
    // Test thread pool
    std::cout << "Testing thread pool..." << std::endl;
    auto future = thread_pool->submit([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 42;
    });
    
    std::cout << "Thread pool result: " << future.get() << std::endl;
    
    // Print metrics
    std::cout << "\n=== Performance Metrics ===" << std::endl;
    auto snapshot = metrics->get_snapshot();
    std::cout << "FPS: " << snapshot.fps << std::endl;
    std::cout << "Average Latency: " << snapshot.avg_latency_ms << "ms" << std::endl;
    std::cout << "Total Frames: " << snapshot.total_frames << std::endl;
    std::cout << "Uptime: " << snapshot.uptime.count() << "ms" << std::endl;
    
    std::cout << "\nDemo completed successfully!" << std::endl;
    return 0;
}
