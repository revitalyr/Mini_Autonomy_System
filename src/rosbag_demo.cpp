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
#include <format>
#include <filesystem>

#include "perception/ros_provider.hpp"
#include "perception/detector.hpp"
#include "perception/types.hpp"

namespace perception {

auto demo_rosbag_provider(const std::string& bag_path) -> Expected<void, PerceptionError> {
    try {
        std::cout << "=== ROSBAG Provider Demo ===\n";
        std::cout << "Bag file: " << bag_path << "\n";

        // Create ROSBAG provider
        RosBagProvider provider;

        // Open the bag file
        if (auto result = provider.open(bag_path); !result) {
            std::cout << "Failed to open bag file: " << result.error().message() << "\n";
            return result;
        }

        std::cout << "Bag file opened successfully\n";

        // Stream data from the bag
        // Note: Topic names depend on the specific bag file
        std::string img_topic = "/cam0/image_raw";  // Adjust based on your bag
        std::string imu_topic = "/imu0";             // Adjust based on your bag

        std::cout << "Streaming from topics:\n";
        std::cout << "  Image: " << img_topic << "\n";
        std::cout << "  IMU: " << imu_topic << "\n";

        size_t frame_count = 0;
        size_t max_frames = 10;  // Limit for demo

        auto data_stream = provider.stream_data(img_topic, imu_topic);

        // Iterate through the generator
        for (auto&& result : data_stream) {
            if (!result) {
                std::cout << "Error reading frame: " << result.error().message() << "\n";
                continue;
            }

            const VioFrame& frame = result.value();
            frame_count++;

            std::cout << "Frame " << frame_count << ":\n";
            std::cout << "  Image size: " << frame.image.width << "x" << frame.image.height << "\n";
            std::cout << "  IMU samples: " << frame.imu_samples.size() << "\n";
            std::cout << "  Timestamp: " << std::format("{:.6f}", frame.timestamp) << "\n";

            if (frame.imu_samples.size() > 0) {
                const IMUData& imu = frame.imu_samples.back();
                std::cout << "  Latest IMU - Accel: (" 
                          << std::format("{:.3f}", imu.accelerometer_x) << ", "
                          << std::format("{:.3f}", imu.accelerometer_y) << ", "
                          << std::format("{:.3f}", imu.accelerometer_z) << ")\n";
                std::cout << "  Latest IMU - Gyro: ("
                          << std::format("{:.3f}", imu.gyroscope_x) << ", "
                          << std::format("{:.3f}", imu.gyroscope_y) << ", "
                          << std::format("{:.3f}", imu.gyroscope_z) << ")\n";
            }

            if (frame_count >= max_frames) {
                std::cout << "Reached max frames limit (" << max_frames << ")\n";
                break;
            }
        }

        // Close the bag file
        provider.close();
        std::cout << "Bag file closed\n";

        std::cout << "\nTotal frames processed: " << frame_count << "\n";

        return {};
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
        return make_unexpected(PerceptionError::InvalidInput);
    }
}

auto demo_rosbag_with_detection(const std::string& bag_path) -> Expected<void, PerceptionError> {
    try {
        std::cout << "=== ROSBAG + Detection Demo ===\n";
        std::cout << "Bag file: " << bag_path << "\n";

        // Create ROSBAG provider
        RosBagProvider provider;

        // Open the bag file
        if (auto result = provider.open(bag_path); !result) {
            std::cout << "Failed to open bag file: " << result.error().message() << "\n";
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

            const VioFrame& frame = result.value();
            frame_count++;

            // Run detection on the image
            auto detections = detector.detect(frame.image);

            std::cout << "Frame " << frame_count << ": " << detections.size() << " detections\n";
            for (const auto& det : detections) {
                std::cout << "  - " << det.class_name << ": confidence="
                          << std::format("{:.2f}", det.confidence)
                          << ", bbox=(" << det.bbox.x << ", " << det.bbox.y << ", "
                          << det.bbox.width << "x" << det.bbox.height << ")\n";
            }

            if (frame_count >= max_frames) {
                break;
            }
        }

        provider.close();
        std::cout << "\nTotal frames processed: " << frame_count << "\n";

        return {};
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
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
            // Use default ROS 2 test file
            bag_path = "demo/data/dataset_ros2";
        }

        // Check if bag file exists
        if (!std::filesystem::exists(bag_path)) {
            std::cout << "Error: Bag file not found: " << bag_path << "\n";
            std::cout << "Usage: rosbag_demo [path_to_bag_file]\n";
            return 1;
        }

        std::cout << "Bag file path: " << bag_path << "\n\n";

        // Run basic demo
        if (auto result = perception::demo_rosbag_provider(bag_path); !result) {
            std::cout << "ROSBAG provider demo failed: " << result.error().message() << "\n";
            return 1;
        }

        std::cout << "\n---\n";

        // Run demo with detection
        if (auto result = perception::demo_rosbag_with_detection(bag_path); !result) {
            std::cout << "ROSBAG + detection demo failed: " << result.error().message() << "\n";
            return 1;
        }

        std::cout << "\nAll ROSBAG demos completed successfully!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cout << "Unknown error occurred!\n";
        return 1;
    }
}
