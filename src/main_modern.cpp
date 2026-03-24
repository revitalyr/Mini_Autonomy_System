module;

#include <iostream>
#include <opencv2/opencv.hpp>
#include <memory>

// Import modern C++23 modules
import perception.concepts;
import perception.queue;
import perception.pipeline;
import perception.result;
import perception.metrics;
import perception.detector;
import perception.async;
import perception.image_loader;

int main() {
    std::cout << "🚀 Mini Autonomy System - Modern C++23 Demo\n";
    std::cout << "==========================================\n\n";

    try {
        // Modern queue with C++23 features
        perception::ThreadSafeQueue<cv::Mat> frame_queue;
        
        // Modern metrics with atomic operations
        perception::PerformanceMetrics metrics;
        
        // Modern detector with result types
        auto detector_result = perception::detectors::create_yolo_detector("model.onnx", 0.5f);
        
        if (!detector_result) {
            std::cout << "❌ Failed to create detector: " << detector_result.error().message() << "\n";
            return 1;
        }
        
        auto& detector = *detector_result;
        
        std::cout << "✅ Modern perception system initialized\n";
        std::cout << "📊 Using C++23 features:\n";
        std::cout << "   - C++20 modules for better compilation\n";
        std::cout << "   - std::expected for error handling\n";
        std::cout << "   - Concepts for type safety\n";
        std::cout << "   - Ranges for data processing\n";
        std::cout << "   - std::stop_token for cancellation\n";
        std::cout << "   - Atomic operations for performance\n";
        std::cout << "   - RAII timers for latency measurement\n\n";
        
        // Demo with a simple frame
        cv::Mat demo_frame(480, 640, CV_8UC3, cv::Scalar(50, 100, 150));
        
        // Add some text to make it interesting
        cv::putText(demo_frame, "Modern C++23 Demo", cv::Point(50, 50), 
                   cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
        
        // Push to queue
        frame_queue.push(demo_frame);
        
        // Process with modern timer
        {
            perception::LatencyTimer timer(metrics);
            
            // Detect objects
            auto detection_result = detector.detect(demo_frame);
            
            if (detection_result) {
                const auto& detections = *detection_result;
                std::cout << "🎯 Detected " << detections.size() << " objects\n";
                
                // C++20: Structured bindings with custom type
                for (const auto& detection : detections) {
                    const auto& [bbox, confidence, class_id, class_name] = detection;
                    std::cout << "   Object " << class_id << ": " 
                              << class_name << " (conf: " << confidence << ")\n";
                    
                    // Draw on frame
                    cv::rectangle(demo_frame, bbox, cv::Scalar(0, 255, 0), 2);
                    cv::putText(demo_frame, class_name, bbox.tl(), 
                               cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
                }
            } else {
                std::cout << "❌ Detection failed: " << detection_result.error().message() << "\n";
            }
        } // Timer automatically records latency
        
        // Show performance metrics
        std::cout << "\n";
        metrics.print_metrics();
        
        // Modern functional pipeline demo
        std::cout << "\n🔧 Functional Pipeline Demo:\n";
        
        perception::FunctionalPipeline<cv::Mat, std::string> text_pipeline;
        
        // Add stages with lambda functions
        text_pipeline.add_stage([](const cv::Mat& img) -> cv::Mat {
            cv::Mat processed;
            cv::cvtColor(img, processed, cv::COLOR_BGR2GRAY);
            return processed;
        });
        
        text_pipeline.add_stage([](const cv::Mat& img) -> std::string {
            return "Processed image: " + std::to_string(img.rows) + "x" + std::to_string(img.cols);
        });
        
        auto result = text_pipeline.process(demo_frame);
        std::cout << "   Pipeline result: " << result << "\n";
        
        // Async operations demo
        std::cout << "\n⚡ Async Operations Demo:\n";
        
        // Example 1: Using Detector's Coroutine based async
        // We use spawn() to bridge the coroutine Task to a std::future for the main thread
        auto future_detections = perception::spawn(detector.detect_async(demo_frame.clone()));
        std::cout << "   Detection started asynchronously...\n";
        
        // Do other work while detection runs in the background thread pool
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        auto async_result = future_detections.get();
        if (async_result) {
            std::cout << "   Async detection completed: " << async_result->size() << " objects\n";
        }

        // Example 2: Using Coroutine-based Task<T> (Modern Style)
        std::cout << "   Loading image via coroutine Task...\n";
        auto task = perception::AsyncImageLoader::load_async("test_image.jpg");
        // In a real app, you would co_await this inside another task.
        // Here we use sync_wait to bridge to main().
        // Since file doesn't exist, we expect an error, which proves the flow works.
        auto img_result = perception::sync_wait(std::move(task));
        
        if (img_result) {
            std::cout << "   Image loaded via coroutine!\n";
        } else {
            std::cout << "   Coroutine load result: " << img_result.error().message() << " (Expected if file missing)\n";
        }
        
        std::cout << "\n✨ Modern C++23 features demonstrated successfully!\n";
        
        // Show final frame
        cv::imshow("Modern C++23 Perception System", demo_frame);
        std::cout << "📺 Press any key to exit...\n";
        cv::waitKey(0);
        cv::destroyAllWindows();
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
