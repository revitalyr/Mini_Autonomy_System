/**
 * @file rosbag_demo.cpp
 * @brief Demo application for ROSBAG data provider
 *
 * Demonstrates reading synchronized image and IMU data from ROSBAG files
 * using the RosBagProvider module.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

#include <iostream>
#include <print>
#include <filesystem>

import perception.ros_provider;
import perception.detector;
import perception.types;

namespace perception {

auto demo_rosbag_provider(const std::string& bag_path) -> Expected<void, PerceptionError> {
    try {
        std::println("=== ROSBAG Provider Demo ===");
        std::println("Bag file: {}", bag_path);

        // Create ROSBAG provider
        RosBagProvider provider;

        // Open the bag file
        if (auto result = provider.open(bag_path); !result) {
            std::println("Failed to open bag file: {}", static_cast<int>(result.error()));
            return result;
        }

        std::println("Bag file opened successfully");

        // Stream data from the bag
        // Note: Topic names depend on the specific bag file
        std::string img_topic = "/cam0/image_raw";  // Adjust based on your bag
        std::string imu_topic = "/imu0";             // Adjust based on your bag

        std::println("Streaming from topics:");
        std::println("  Image: {}", img_topic);
        std::println("  IMU: {}", imu_topic);

        size_t frame_count = 0;
        size_t max_frames = 10;  // Limit for demo

        auto data_stream = provider.stream_data(img_topic, imu_topic);

        // Iterate through the generator
        for (auto&& result : data_stream) {
            if (!result) {
                std::println("Error reading frame: {}", static_cast<int>(result.error()));
                continue;
            }

            const VioFrame& frame = *result;
            frame_count++;

            std::println("Frame {}:", frame_count);
            std::println("  Image size: {}x{}", frame.image.width, frame.image.height);
            std::println("  IMU samples: {}", frame.imu_samples.size());
            std::println("  Timestamp: {:.6f}", frame.timestamp);

            if (frame.imu_samples.size() > 0) {
                const IMUData& imu = frame.imu_samples.back();
                std::println("  Latest IMU - Accel: ({:.3f}, {:.3f}, {:.3f})",
                            imu.accelerometer_x, imu.accelerometer_y, imu.accelerometer_z);
                std::println("  Latest IMU - Gyro: ({:.3f}, {:.3f}, {:.3f})",
                            imu.gyroscope_x, imu.gyroscope_y, imu.gyroscope_z);
            }

            if (frame_count >= max_frames) {
                std::println("Reached max frames limit ({})", max_frames);
                break;
            }
        }

        // Close the bag file
        provider.close();
        std::println("Bag file closed");

        std::println("\nTotal frames processed: {}", frame_count);

        return {};
    } catch (const std::exception& e) {
        std::println("Exception: {}", e.what());
        return make_unexpected(PerceptionError::InvalidInput);
    }
}

auto demo_rosbag_with_detection(const std::string& bag_path) -> Expected<void, PerceptionError> {
    try {
        std::println("=== ROSBAG + Detection Demo ===");
        std::println("Bag file: {}", bag_path);

        // Create ROSBAG provider
        RosBagProvider provider;

        // Open the bag file
        if (auto result = provider.open(bag_path); !result) {
            std::println("Failed to open bag file: {}", static_cast<int>(result.error()));
            return result;
        }

        // Create detector
        Detector detector;

        // Stream data from the bag
        std::string img_topic = "/cam0/image_raw";
        std::string imu_topic = "/imu0";

        auto data_stream = provider.stream_data(img_topic, imu_topic);

        size_t frame_count = 0;
        size_t max_frames = 10;

        for (auto&& result : data_stream) {
            if (!result) {
                continue;
            }

            const VioFrame& frame = *result;
            frame_count++;

            // Run detection on the image
            auto detections = detector.detect(frame.image);

            std::println("Frame {}: {} detections", frame_count, detections.size());
            for (const auto& det : detections) {
                std::println("  - {}: confidence={:.2f}, bbox=({:.0f}, {:.0f}, {:.0f}x{:.0f})",
                            det.class_name, det.confidence,
                            det.bbox.x, det.bbox.y, det.bbox.width, det.bbox.height);
            }

            if (frame_count >= max_frames) {
                break;
            }
        }

        provider.close();
        std::println("\nTotal frames processed: {}", frame_count);

        return {};
    } catch (const std::exception& e) {
        std::println("Exception: {}", e.what());
        return make_unexpected(PerceptionError::InvalidInput);
    }
}

} // namespace perception

auto main(int argc, char* argv[]) -> int {
    try {
        std::string bag_path;

        if (argc > 1) {
            bag_path = argv[1];
        } else {
            // Use default test file
            bag_path = "demo/data/dataset-calib-cam1_512_16.bag";
        }

        // Check if bag file exists
        if (!std::filesystem::exists(bag_path)) {
            std::println("Error: Bag file not found: {}", bag_path);
            std::println("Usage: rosbag_demo [path_to_bag_file]");
            return 1;
        }

        // Run basic demo
        if (auto result = perception::demo_rosbag_provider(bag_path); !result) {
            std::println("ROSBAG provider demo failed: {}", static_cast<int>(result.error()));
            return 1;
        }

        std::println("\n---\n");

        // Run demo with detection
        if (auto result = perception::demo_rosbag_with_detection(bag_path); !result) {
            std::println("ROSBAG + detection demo failed: {}", static_cast<int>(result.error()));
            return 1;
        }

        std::println("\nAll ROSBAG demos completed successfully!");
        return 0;

    } catch (const std::exception& e) {
        std::println("Error: {}", e.what());
        return 1;
    } catch (...) {
        std::println("Unknown error occurred!");
        return 1;
    }
}
