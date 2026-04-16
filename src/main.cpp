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
#include <utility>
#include <memory>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <thread>
#include <opencv2/opencv.hpp>
#include "perception/constants.h"

import perception.pipeline;
import perception.image_loader;
import perception.viz;
import perception.async;
import perception.result;
import perception.calibration;
import perception.geom;
import perception.types;
import perception.detector;
import perception.tracking;

namespace perception {

/**
 * Helper for filtering detections by confidence
 */
auto filter_by_confidence(float threshold) {
    return [threshold](const perception::geom::Detection& d) {
        return d.confidence >= threshold;
    };
}

// ============================================================================
// ASYNC PROCESSING PIPELINE
// ============================================================================

// ============================================================================
// DEMO FUNCTIONS
// ============================================================================

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
            | std::views::filter(filter_by_confidence(
                constants::detector::DEFAULT_CONFIDENCE_THRESHOLD))
            | std::views::take(3);

        std::cout << constants::demo::HEADER_DETECTION_FILTER << "\n";
        std::cout << "Total detections: " << test_detections.size() << "\n";
        std::cout << "High confidence detections (>= 0.5):\n";

        for (const perception::geom::Detection& detection : high_confidence_detections) {
            float aspect_ratio = static_cast<float>(detection.bbox.width) / static_cast<float>(detection.bbox.height);
            float area = static_cast<float>(detection.bbox.width * detection.bbox.height);

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
 * Find demo data directory from various possible locations
 */
auto find_demo_data_directory() -> std::filesystem::path {
    std::vector<std::filesystem::path> possible_paths = {
        std::string(constants::paths::DEMO_DATA_DIR),                    // From project root
        std::string(constants::paths::DEMO_DATA_DIR_PARENT),             // From build directory
        std::string(constants::paths::DEMO_DATA_DIR_GRANDPARENT),        // From nested build directory
    };
    
    for (const auto& path : possible_paths) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            return path;
        }
    }
    
    return {};  // Empty path if not found
}

/**
 * Demo: Load and process actual JPG images from demo/data
 */
auto demo_coroutines() noexcept -> Result<void> {
    try {
        std::cout << constants::demo::HEADER_IMAGE_LOAD << "\n";
        std::cout << constants::demo::LABEL_LOADING_FROM << "\n";

        // For demonstration, let's use HOG for generated frames and DNN for loaded images.
        // If DNN model paths are not provided, it will default to MotionBased.
        vision::Detector detector_hog(vision::DetectorType::HOG);
        vision::Detector detector_dnn(vision::DetectorType::DNN, 16.0, "demo/data/yolo11n.onnx", "demo/data/coco.names"); 
        auto image_path = io::findDemoDataDirectory();
        
        if (image_path.empty()) {
            std::cout << constants::demo::MSG_FALLBACK_GENERATED << "\n";
            auto frame_generator = io::generateTestFrames(5);
            size_t frame_count = 0;
            for (auto&& frame : frame_generator) {
                ++frame_count;
                std::cout << "Generated frame " << frame_count << " ("
                          << frame.m_width << "x" << frame.m_height << ")\n";

                // Выполняем детекцию на сгенерированном кадре
                auto detections = detector_hog.detect(frame); // Use HOG for generated frames
                Vector<tracking::Track> tracks;
                for (const auto& det : detections) {
                    tracking::Track track;
                    track.id = static_cast<int>(tracks.size());
                    track.bbox = det.bbox;
                    track.confidence = det.confidence;
                    track.class_name = det.class_name;
                    track.age = 1;
                    tracks.push_back(std::move(track));
                }

                viz::drawTracks(frame, tracks);
                cv::waitKey(5000); // Ждем 5 секунд для каждого кадра
            }
            viz::terminateWindows();
            return {};
        }

        auto image_generator = io::loadImagesFromDirectory(image_path);

        size_t frame_count = 0;
        for (auto&& frame : image_generator) {
            ++frame_count;
            std::cout << "Loaded image " << frame_count << " ("
                      << frame.m_width << "x" << frame.m_height << ", "
                      << frame.m_channels << " channels)\n";

            // Выполняем детекцию на реальном изображении
            auto detections = detector_dnn.detect(frame); // Use DNN for loaded images
            
            // Преобразуем детекции в треки для визуализации
            Vector<tracking::Track> tracks;
            for (const auto& det : detections) {
                tracking::Track track;
                track.id = static_cast<int>(tracks.size());
                track.bbox = det.bbox;
                track.confidence = det.confidence;
                track.class_name = det.class_name;
                track.age = 1;
                tracks.push_back(std::move(track));
            }

            // Отрисовка и показ (используем 5 секунд по запросу)
            viz::drawTracks(frame, tracks);
            cv::waitKey(5000); 
        }
        
        // Закрываем окна после завершения цикла
        viz::terminateWindows();
        
        std::cout << "Total images loaded: " << frame_count << "\n";

        return {};
    } catch (const std::exception& e) {
        std::cerr << "Error in demo_coroutines: " << e.what() << "\n";
        return Result<void>(std::error_code(static_cast<int>(PerceptionError::ProcessingError), std::generic_category()));
    }
}

/**
 * Demo: Visualization of detection results with bounding boxes
 */
auto demo_visualization() noexcept -> Result<void> {
    try {
        std::cout << constants::demo::HEADER_VISUALIZATION << "\n";

        // Create sample detections
        Vector<geom::Detection> detections;
        geom::Detection det1;
        det1.bbox = {10, 10, 50, 50};
        det1.confidence = 0.90f;
        det1.class_id = 0;
        det1.class_name = std::string(constants::detector::CLASS_NAME_PERSON);
        detections.push_back(det1);

        geom::Detection det2;
        det2.bbox = {100, 100, 80, 80};
        det2.confidence = 0.70f;
        det2.class_id = 1;
        det2.class_name = std::string(constants::detector::CLASS_NAME_CAR);
        detections.push_back(det2);

        geom::Detection det3;
        det3.bbox = {300, 300, 60, 60};
        det3.confidence = 0.80f;
        det3.class_id = 0;
        det3.class_name = std::string(constants::detector::CLASS_NAME_PERSON);
        detections.push_back(det3);

        // Create a test image
        cv::Mat test_mat(480, 640, CV_8UC3);
        test_mat.setTo(cv::Scalar(128, 128, 128));

        // Create ImageData from the test mat
        ImageData image = ImageData::fromRaw(
            reinterpret_cast<const uint8_t*>(test_mat.data),
            test_mat.cols,
            test_mat.rows,
            PixelFormat::Bgr,
            true
        );

        // Convert detections to tracks for visualization
        Vector<tracking::Track> tracks;
        for (const auto& det : detections) {
            tracking::Track track;
            track.id = static_cast<int>(tracks.size());
            track.bbox = det.bbox;
            track.confidence = det.confidence;
            track.class_name = det.class_name;
            track.age = 1;
            track.missed_frames = 0;
            tracks.push_back(track);
        }

        // Draw tracks on the image
        viz::drawTracks(image, tracks);

        std::cout << "Press any key to close the visualization window...\n";
        cv::waitKey(5000); // Увеличили до 5 секунд

        viz::terminateWindows();

        return {};
    } catch (const std::exception& e) {
        std::cerr << "Exception in demo_visualization: " << e.what() << "\n";
        return Result<void>(std::error_code(static_cast<int>(PerceptionError::ThreadError), std::generic_category()));
    }
}

/**
 * Demo: Async object detection pipeline with real images
 */
auto demo_async_processing() noexcept -> Result<void> {
    try {
        std::cout << constants::demo::HEADER_ASYNC_PIPELINE << "\n";

        // Initialize calibration (optional)
        calibration::CameraCalibration calib;
        bool is_calibrated = false;
        
        auto calib_result = calibration::CameraCalibration::fromYamlFile("calibration.yaml");
        if (calib_result) {
            calib = std::move(calib_result.value());
            calib.setAlpha(0.0); // No black pixels
            is_calibrated = true;
            std::cout << "Successfully loaded camera calibration data.\n";
        } else {
            std::cout << "Note: No calibration.yaml found, proceeding with raw images.\n";
        }

        perception::AsyncProcessingPipeline pipeline{}; // Use the module-exported pipeline

        if (auto result = pipeline.start(); !result) {
            return result;
        }

        // Try to load real images from demo/data
        auto image_path = io::findDemoDataDirectory();
        size_t frames_processed = 0;
        
        if (!image_path.empty()) {
            auto image_generator = io::loadImagesFromDirectory(image_path);
            
            for (auto&& frame_from_generator : image_generator) { // Use auto&& to avoid copying ImageData
                Result<void> pipeline_res;

                if (is_calibrated) {
                    // Move frame_from_generator into undistort, and then move the result into a unique_ptr for the pipeline
                    pipeline_res = pipeline.processImage(perception::make_unique<ImageData>(
                        calib.undistort(std::move(frame_from_generator))));
                } else {
                    // Move frame_from_generator directly into a unique_ptr for the pipeline
                    pipeline_res = pipeline.processImage(perception::make_unique<ImageData>(
                        std::move(frame_from_generator)));
                }

                if (!pipeline_res) {
                    pipeline.stop();
                    return pipeline_res;
                }
                ++frames_processed;
                if (frames_processed >= constants::pipeline::MAX_REAL_IMAGES_TO_PROCESS) break;
            }
        }
        
        // Fall back to generated frames if no real images were loaded
        if (frames_processed == 0) {
            std::cout << constants::demo::MSG_NO_REAL_IMAGES << "\n";
            for (auto&& frame : io::generateTestFrames( // Use auto&& to avoid copying ImageData
                    constants::pipeline::MAX_GENERATED_FRAMES_TO_PROCESS)) {
                // Directly create a unique_ptr with the moved frame
                if (auto result = pipeline.processImage(perception::make_unique<ImageData>(std::move(frame))); !result) {
                    pipeline.stop();
                    return result;
                }
                ++frames_processed;
            }
        }
        
        std::cout << constants::demo::LABEL_PROCESSING << frames_processed << " frames...\n";

        // Wait for pipeline to finish processing all frames
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        int results_count = 0;
        while (results_count < constants::pipeline::DEFAULT_DETECTIONS_TO_DISPLAY) {
            if (auto results_opt = pipeline.getResults(perception::Milliseconds{5000}); results_opt) { // getResults returns OutputFrame
                const auto& output_frame = results_opt.value();
                const auto& tracks = output_frame.second; // OutputFrame is a pair, tracks are in .second
                
                for (const perception::tracking::Track& track : tracks) {
                    float aspect_ratio = static_cast<float>(track.bbox.width) / static_cast<float>(track.bbox.height);
                    float area = static_cast<float>(track.bbox.width * track.bbox.height);
                    
                    std::cout << "  - Track ID " << track.id << " (" << track.class_name << "): confidence="
                              << std::format("{:.2f}", track.confidence) << "\n";
                    std::cout << "    Bounding box: (" << track.bbox.x << ", " << track.bbox.y
                              << ", " << track.bbox.width << "x" << track.bbox.height << ")\n";
                    std::cout << "    Aspect ratio: " << std::format("{:.2f}", aspect_ratio) << "\n";
                    std::cout << "    Area: " << std::format("{:.0f}", area) << " pixels\n";
                    std::cout << "    Age: " << track.age << ", Missed frames: " << track.missed_frames << "\n";
                    
                    // Explain classification reasoning
                    std::cout << "    Classification reasoning: "; // This part needs to be updated to use track.class_name
                    if (track.class_name == std::string(constants::detector::CLASS_NAME_PERSON)) {
                        std::cout << constants::demo::EXPLAIN_PERSON
                                  << std::format(std::string(constants::demo::FORMAT_ASPECT_RATIO), aspect_ratio)
                                  << ", area " << std::format(std::string(constants::demo::FORMAT_AREA), area) << ")\n";
                    } else if (track.class_name == std::string(constants::detector::CLASS_NAME_CAR)) {
                        std::cout << constants::demo::EXPLAIN_CAR
                                  << std::format(std::string(constants::demo::FORMAT_ASPECT_RATIO), aspect_ratio)
                                  << ", area " << std::format(std::string(constants::demo::FORMAT_AREA), area) << ")\n";
                    } else if (track.class_name == std::string(constants::detector::CLASS_NAME_BICYCLE)) {
                        std::cout << constants::demo::EXPLAIN_BICYCLE
                                  << std::format(std::string(constants::demo::FORMAT_ASPECT_RATIO), aspect_ratio)
                                  << ", area " << std::format(std::string(constants::demo::FORMAT_AREA), area) << ")\n";
                    } else if (track.class_name == std::string(constants::detector::CLASS_NAME_GENERIC)) {
                        std::cout << constants::demo::EXPLAIN_GENERIC << "\n";
                    } else {
                        std::cout << constants::demo::EXPLAIN_MOTION << "\n";
                    }
                }
                std::cout << "\n";
                ++results_count;
            } else { // Handle error from getResults
                std::cerr << "No more results from pipeline or error: " << results_opt.error().message() << "\n";
                break; // Exit loop if no more results or an error occurred
            }
        }

        auto metrics = pipeline.getMetrics();
        std::cout << constants::demo::LABEL_FPS
                  << std::format(std::string(constants::demo::FORMAT_CONFIDENCE), metrics.fps) << "\n";
        std::cout << constants::demo::LABEL_AVG_LATENCY
                  << std::format(std::string(constants::demo::FORMAT_CONFIDENCE), metrics.avg_latency_ms) << "ms\n";

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
        // Disable OpenCV plugin loading to avoid GTK/parallel backend attempts
        cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
        cv::setUseOptimized(true);
        
        std::cout << perception::constants::demo::HEADER_MAIN << "\n";
        std::cout << perception::constants::demo::MSG_PROGRAM_STARTED << "\n";

        if (auto result = perception::demo_ranges_and_concepts(); !result) {
            std::cout << "Detection filtering demo failed: " << result.error().message() << "\n";
            return 1;
        }

        if (auto result = perception::demo_coroutines(); !result) {
            std::cout << "Frame generation demo failed: " << result.error().message() << "\n";
            return 1;
        }

        if (auto result = perception::demo_visualization(); !result) {
            std::cout << "Visualization demo failed: " << result.error().message() << "\n";
            // Don't return 1, just continue
        }

        // if (auto result = perception::demo_async_processing(); !result) {
        //     std::cout << "Async processing demo failed: " << result.error().message() << "\n";
        //     // Don't return 1, just continue
        // }

        std::cout << "\n" << perception::constants::demo::MSG_ALL_DEMOS_COMPLETED << "\n";
        return 0;

    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cout << "Unknown error occurred!\n";
        return 1;
    }
}
