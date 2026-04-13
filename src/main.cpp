/**
 * @file main.cpp
 * @brief Demo application for the Mini Autonomy System
 *
 * Demonstrates object detection, image processing, and async pipeline
 * functionality using motion-based detection and batch image loading.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

#include <iostream>
#include <ranges>
#include <format>

import perception.pipeline;
import perception.image_loader;
import perception.viz;
import perception.async;
import perception.result;
import perception.geom;
import perception.types;

namespace perception {

/**
 * @brief Helper to create a confidence filter for ranges
 */
auto filter_by_confidence(float threshold) {
    return [threshold](const geom::Detection& d) {
        return d.confidence >= threshold;
    };
}

 /**
  * @brief Demo functions for the Mini Autonomy System
  */

/**
 * Demo: Filter detections by confidence threshold
 */
auto demo_ranges_and_concepts() noexcept -> Result<void> {
    try {
        perception::Vector<perception::geom::Detection> test_detections{
            perception::geom::Detection{perception::geom::Rect{10, 10, 50, 50}, 0.9f, 1, "person"},
            perception::geom::Detection{perception::geom::Rect{100, 100, 80, 80}, 0.7f, 2, "car"},
            perception::geom::Detection{perception::geom::Rect{200, 200, 30, 30}, 0.3f, 3, "bicycle"},
            perception::geom::Detection{perception::geom::Rect{300, 300, 60, 60}, 0.8f, 1, "person"}
        };

        auto high_confidence_detections = test_detections
            | std::views::filter(filter_by_confidence(0.5))
            | std::views::take(3);

        std::cout << "=== Detection Filtering Demo ===\n";
        std::cout << "Total detections: " << test_detections.size() << "\n";
        std::cout << "High confidence detections (>= 0.5):\n";

        for (const perception::geom::Detection& detection : high_confidence_detections) {
            float aspect_ratio = static_cast<float>(detection.bbox.width) / detection.bbox.height;
            float area = detection.bbox.width * detection.bbox.height;

            std::cout << "  - " << detection.class_name << ": confidence=" 
                      << std::format("{:.2f}", detection.confidence) << "\n";
            std::cout << "    Bounding box: (" << detection.bbox.x << ", " << detection.bbox.y 
                      << ", " << detection.bbox.width << "x" << detection.bbox.height << ")\n";
            std::cout << "    Aspect ratio: " << std::format("{:.2f}", aspect_ratio) << "\n";
            std::cout << "    Area: " << std::format("{:.0f}", area) << " pixels\n";
            
            // Explain classification reasoning
            std::cout << "    Classification reasoning: ";
            if (detection.class_name == "person") {
                std::cout << "Tall narrow object (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                          << ", area " << std::format("{:.0f}", area) << ")\n";
            } else if (detection.class_name == "car") {
                std::cout << "Wide object with large area (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                          << ", area " << std::format("{:.0f}", area) << ")\n";
            } else if (detection.class_name == "bicycle") {
                std::cout << "Medium-sized object (aspect ratio " << std::format("{:.2f}", aspect_ratio) 
                          << ", area " << std::format("{:.0f}", area) << ")\n";
            } else {
                std::cout << "Generic object (doesn't match specific categories)\n";
            }
        }

        return {};
    } catch (const std::exception&) {
        return Result<void>(std::error_code(static_cast<int>(PerceptionError::InvalidInput), std::generic_category()));
    }
}

/**
 * Demo: Load and process actual JPG images from demo/data
 */
auto demo_coroutines() noexcept -> Result<void> {
    try {
        std::cout << "=== Image Loading Demo ===\n";
        std::cout << "Loading images from demo/data/...\n";

        auto image_path = io::findDemoDataDirectory();
        
        if (image_path.empty()) {
            std::cout << "Demo data directory not found, falling back to generated frames\n";
            auto frame_generator = io::generateTestFrames(5);
            size_t frame_count = 0;
            for (const auto& frame : frame_generator) {
                ++frame_count;
                std::cout << "Generated frame " << frame_count << " (" 
                          << frame.width << "x" << frame.height << ")\n";
            }
            return {};
        }

        auto image_generator = io::loadImagesFromDirectory(image_path);
        
        size_t frame_count = 0;
        for (const auto& frame : image_generator) {
            ++frame_count;
            std::cout << "Loaded image " << frame_count << " (" 
                      << frame.width << "x" << frame.height << ", "
                      << frame.channels << " channels)\n";
        }
        
        std::cout << "Total images loaded: " << frame_count << "\n";

        return {};
    } catch (const std::exception&) {
        return Result<void>(std::error_code(static_cast<int>(PerceptionError::InvalidInput), std::generic_category()));
    }
}

/**
 * Demo: Async object detection pipeline with real images
 */
auto demo_async_processing() noexcept -> Result<void> {
    try {
        std::cout << "=== Async Detection Pipeline Demo (Real Images) ===\n";

        auto pipeline = AsyncProcessingPipeline{};

        if (auto result = pipeline.start(); !result) {
            return result;
        }

        // Try to load real images from demo/data
        auto image_path = io::findDemoDataDirectory();
        size_t frames_processed = 0;
        
        if (!image_path.empty()) {
            auto image_generator = io::loadImagesFromDirectory(image_path);
            
            for (const auto& frame : image_generator) {
                geom::ImageData frame_copy = frame;
                if (auto process_result = pipeline.processImage(std::move(frame_copy)); !process_result) {
                    pipeline.stop();
                    return process_result;
                }
                ++frames_processed;
                if (frames_processed >= 6) break; // Process up to 6 images
            }
        }
        
        // Fall back to generated frames if no real images were loaded
        if (frames_processed == 0) {
            std::cout << "No real images found, using generated test frames\n";
            for (const auto& frame : io::generateTestFrames(3)) {
                geom::ImageData frame_copy = frame;
                if (auto result = pipeline.processImage(std::move(frame_copy)); !result) {
                    pipeline.stop();
                    return result;
                }
                ++frames_processed;
            }
        }
        
        std::cout << "Processing " << frames_processed << " frames...\n";

        for (int i = 0; i < frames_processed; ++i) { // Loop for processed frames
            if (auto results_pair = pipeline.getResults(); results_pair) {
                const auto& original_image = results_pair.value().first;
                const auto& tracks = results_pair.value().second;

                std::cout << "Frame " << (i + 1) << ": " << tracks.size() << " tracks\n"; 
                
                // Draw and Show
                viz::drawTracks(original_image, tracks);
                
                for (const perception::tracking::Track& track : tracks) {
                    float aspect_ratio = static_cast<float>(track.bbox.width) / track.bbox.height;
                    float area = track.bbox.width * track.bbox.height;
                    
                    std::cout << "  - Track ID " << track.id << " (" << track.class_name << "): confidence="
                              << std::format("{:.2f}", track.confidence) << "\n";
                    std::cout << "    Bounding box: (" << track.bbox.x << ", " << track.bbox.y
                              << ", " << track.bbox.width << "x" << track.bbox.height << ")\n";
                    std::cout << "    Aspect ratio: " << std::format("{:.2f}", aspect_ratio) << "\n";
                    std::cout << "    Area: " << std::format("{:.0f}", area) << " pixels\n";
                    std::cout << "    Age: " << track.age << ", Missed frames: " << track.missed_frames << "\n";
                    
                    // Explain classification reasoning
                    std::cout << "    Classification reasoning: ";
                    if (track.class_name == "person") {
                        std::cout << "Tall narrow object (aspect ratio " << std::format("{:.2f}", aspect_ratio)
                                  << ", area " << std::format("{:.0f}", area) << ")\n";
                    } else if (track.class_name == "car") {
                        std::cout << "Wide object with large area (aspect ratio " << std::format("{:.2f}", aspect_ratio)
                                  << ", area " << std::format("{:.0f}", area) << ")\n";
                    } else if (track.class_name == "bicycle") {
                        std::cout << "Medium-sized object (aspect ratio " << std::format("{:.2f}", aspect_ratio)
                                  << ", area " << std::format("{:.0f}", area) << ")\n";
                    } else {
                        std::cout << "Motion detected (background subtraction)\n";
                    }
                }
                std::cout << "\n";
            } else {
                std::cerr << "No more results from pipeline or error: " << results_pair.error().message() << "\n";
                break; // Exit loop if no more results or an error occurred
            }
        }

        viz::terminateWindows();

        auto metrics = pipeline.getMetrics();
        std::cout << "FPS: " << std::format("{:.2f}", metrics.fps) << "\n";
        std::cout << "Average latency: " << std::format("{:.2f}", metrics.avg_latency_ms) << "ms\n";

        pipeline.stop();

        return {};
        } catch (const std::exception& e) {
            std::cerr << "Exception in demo_async_processing: " << e.what() << "\n"; // Use cerr for errors
            return Result<void>(std::error_code(static_cast<int>(PerceptionError::ThreadError), std::generic_category())); // Return error
    }
}

} // namespace perception

// ============================================================================
// MAIN
// ============================================================================

auto main() -> int {
    try {
        std::cout << "=== Mini Autonomy System Demo ===\n";
        std::cout << "Program started successfully!\n";

        if (auto result = perception::demo_ranges_and_concepts(); !result) {
            std::cout << "Detection filtering demo failed: " << result.error().message() << "\n";
            return 1;
        }

        if (auto result = perception::demo_coroutines(); !result) {
            std::cout << "Frame generation demo failed: " << result.error().message() << "\n";
            return 1;
        }

        if (auto result = perception::demo_async_processing(); !result) {
            std::cout << "Async pipeline demo failed: " << result.error().message() << "\n";
            return 1;
        }

        std::cout << "\nAll demos completed successfully!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cout << "Unknown error occurred!\n";
        return 1;
    }
}
