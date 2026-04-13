/**
 * @file rosbag_viz_demo_headless.cpp
 * @brief ROSBAG visualization demo (headless - saves frames to disk)
 *
 * This demo visualizes ROSBAG data by saving frames to disk
 * instead of displaying them in OpenCV windows.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <format>
import perception.ros_provider;
import perception.result;

namespace perception {

/**
 * @brief Visualize ROSBAG data and save frames to disk
 */
Result<void> demo_rosbag_visualization_headless(const std::string& bag_path, const std::string& output_dir) {
    std::cout << "=== ROSBAG Visualization Demo (Headless) ===\n";
    std::cout << "Bag file: " << bag_path << "\n";
    std::cout << "Output directory: " << output_dir << "\n\n";
    
    // Create output directory
    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
    }
    
    RosBagProvider provider;
    
    // Open bag file
    if (auto result = provider.open(bag_path); !result) {
        std::cout << "Failed to open bag file: " << result.error().message() << "\n";
        return Result<void>(std::error_code(static_cast<int>(PerceptionError::InvalidInput), std::generic_category()));
    }
    
    std::cout << "Bag file opened successfully\n\n";
    
    // Stream data with visualization
    auto generator = provider.stream_data("/cam0/image_raw", "/imu0");
    
    int frame_count = 0;
    int max_frames = 100; // Limit frames for demo
    
    for (auto frame_result : generator) {
        if (!frame_result) {
            std::cout << "Error reading frame: " << frame_result.error().message() << "\n";
            continue;
        }
        
        auto& frame = frame_result.value();
        frame_count++;
        
        if (frame_count > max_frames) {
            std::cout << "Reached max frames limit (" << max_frames << ")\n";
            break;
        }
        
        // Convert ImageData to cv::Mat
        cv::Mat cv_img(frame.image.height, frame.image.width, CV_8UC3);
        std::copy(frame.image.data.begin(), frame.image.data.end(), cv_img.data);
        
        // Add IMU data overlay
        if (!frame.imu_samples.empty()) {
            const auto& imu = frame.imu_samples.back(); // Use latest IMU sample
            
            // Create overlay text
            std::string info_text = std::format(
                "Frame: {} | Time: {:.3f} s\n"
                "Accel: ({:.3f}, {:.3f}, {:.3f}) m/s²\n"
                "Gyro: ({:.3f}, {:.3f}, {:.3f}) rad/s\n"
                "IMU samples: {}",
                frame_count,
                frame.timestamp,
                imu.accelerometer_x, imu.accelerometer_y, imu.accelerometer_z,
                imu.gyroscope_x, imu.gyroscope_y, imu.gyroscope_z,
                frame.imu_samples.size()
            );
            
            // Draw text overlay
            int font_face = cv::FONT_HERSHEY_SIMPLEX;
            double font_scale = 0.6;
            int thickness = 2;
            int baseline = 0;
            
            cv::Size text_size = cv::getTextSize(info_text, font_face, font_scale, thickness, &baseline);
            
            // Draw semi-transparent background for text
            cv::Mat overlay = cv_img.clone();
            cv::rectangle(overlay, cv::Point(10, 10), cv::Point(10 + text_size.width + 20, 10 + text_size.height + 20), 
                         cv::Scalar(0, 0, 0), -1);
            cv::addWeighted(overlay, 0.7, cv_img, 0.3, 0, cv_img);
            
            // Draw text
            std::vector<std::string> lines;
            std::string line;
            for (char c : info_text) {
                if (c == '\n') {
                    lines.push_back(line);
                    line.clear();
                } else {
                    line += c;
                }
            }
            if (!line.empty()) {
                lines.push_back(line);
            }
            
            int y_offset = 30;
            for (const auto& text_line : lines) {
                cv::putText(cv_img, text_line, cv::Point(20, y_offset), 
                           font_face, font_scale, cv::Scalar(0, 255, 0), thickness);
                y_offset += 25;
            }
        }
        
        // Save frame to disk
        std::string frame_filename = std::format("{}/frame_{:04d}.png", output_dir, frame_count);
        cv::imwrite(frame_filename, cv_img);
        
        std::cout << "Frame " << frame_count << ": " 
                  << frame.image.width << "x" << frame.image.height
                  << ", IMU samples: " << frame.imu_samples.size()
                  << ", Time: " << frame.timestamp << " s\n";
    }
    
    // Cleanup
    provider.close();
    
    std::cout << "\nTotal frames saved: " << frame_count << "\n";
    std::cout << "Frames saved to: " << output_dir << "\n";
    
    return {};
}

} // namespace perception

auto main(int argc, char* argv[]) -> int {
    try {
        std::string bag_path;
        std::string output_dir = "viz_frames";
        
        if (argc > 1) {
            bag_path = argv[1];
        } else {
            // Use default ROS 2 test file
            bag_path = "demo/data/rosbag-001.db3";
        }
        
        if (argc > 2) {
            output_dir = argv[2];
        }
        
        // Check if bag file exists
        if (!std::filesystem::exists(bag_path)) {
            std::cout << "Error: Bag file not found: " << bag_path << "\n";
            std::cout << "Usage: rosbag_viz_demo_headless [path_to_bag_file] [output_dir]\n";
            return 1;
        }
        
        std::cout << "Bag file path: " << bag_path << "\n\n";
        
        // Run visualization demo
        if (auto result = perception::demo_rosbag_visualization_headless(bag_path, output_dir); !result) {
            std::cout << "ROSBAG visualization demo failed: " << result.error().message() << "\n";
            return 1;
        }
        
        std::cout << "\nROSBAG visualization demo completed successfully!\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
        return 1;
    }
}
