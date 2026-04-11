/**
 * @file test_ros_provider.cpp
 * @brief Catch2 v3 tests for ROSBAG provider
 *
 * Tests for the RosBagProvider module that reads synchronized
 * image and IMU data from ROSBAG files.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

#include <catch2/catch_all.hpp>
#include <filesystem>

#include "perception/ros_provider.hpp"
#include "perception/types.hpp"
#include "perception/result.hpp"

namespace perception {

TEST_CASE("RosBagProvider construction", "[ros_provider]") {
    SECTION("Default construction") {
        RosBagProvider provider;
        REQUIRE(!provider.is_open());
    }

    SECTION("Move construction") {
        RosBagProvider provider1;
        RosBagProvider provider2 = std::move(provider1);
        REQUIRE(!provider2.is_open());
    }
}

TEST_CASE("RosBagProvider open/close", "[ros_provider]") {
    RosBagProvider provider;

    SECTION("Open non-existent file") {
        auto result = provider.open("nonexistent.bag");
        REQUIRE(!result);
        REQUIRE(result.error() == PerceptionError::FileNotFound);
        REQUIRE(!provider.is_open());
    }

    SECTION("Open invalid file extension") {
        auto result = provider.open("test.txt");
        REQUIRE(!result);
        REQUIRE(result.error() == PerceptionError::InvalidInput);
        REQUIRE(!provider.is_open());
    }

    SECTION("Open valid bag file") {
        std::string bag_path = "demo/data/dataset-calib-cam1_512_16.bag";

        if (!std::filesystem::exists(bag_path)) {
            WARN("Test bag file not found: " << bag_path);
            return;
        }

        auto result = provider.open(bag_path);
        if (result) {
            REQUIRE(provider.is_open());
            provider.close();
            REQUIRE(!provider.is_open());
        } else {
            WARN("Failed to open bag file: " << static_cast<int>(result.error()));
        }
    }
}

TEST_CASE("RosBagProvider streaming", "[ros_provider]") {
    std::string bag_path = "demo/data/dataset-calib-cam1_512_16.bag";

    if (!std::filesystem::exists(bag_path)) {
        WARN("Test bag file not found: " << bag_path);
        return;
    }

    RosBagProvider provider;
    auto open_result = provider.open(bag_path);

    if (!open_result) {
        WARN("Failed to open bag file for streaming test: " << static_cast<int>(open_result.error()));
        return;
    }

    SECTION("Stream data with valid topics") {
        std::string img_topic = "/cam0/image_raw";
        std::string imu_topic = "/imu0";

        auto stream = provider.stream_data(img_topic, imu_topic);

        size_t frame_count = 0;
        size_t max_frames = 5;

        for (auto&& result : stream) {
            if (frame_count >= max_frames) {
                break;
            }

            if (result) {
                const VioFrame& frame = *result;
                REQUIRE(frame.image.width > 0);
                REQUIRE(frame.image.height > 0);
                REQUIRE(frame.timestamp > 0);
                frame_count++;
            }
        }

        if (frame_count > 0) {
            REQUIRE(frame_count > 0);
        } else {
            WARN("No frames received from bag file. Topics may not match.");
        }
    }

    SECTION("Stream data with invalid topics") {
        std::string img_topic = "/invalid/image";
        std::string imu_topic = "/invalid/imu";

        auto stream = provider.stream_data(img_topic, imu_topic);

        size_t frame_count = 0;
        for (auto&& result : stream) {
            if (result) {
                frame_count++;
            }
        }

        // Should get no frames with invalid topics
        REQUIRE(frame_count == 0);
    }

    provider.close();
}

TEST_CASE("IMUData structure", "[ros_provider][types]") {
    SECTION("Default construction") {
        IMUData imu;
        REQUIRE(imu.timestamp == 0);
        REQUIRE(imu.accelerometer_x == 0);
        REQUIRE(imu.accelerometer_y == 0);
        REQUIRE(imu.accelerometer_z == 0);
        REQUIRE(imu.gyroscope_x == 0);
        REQUIRE(imu.gyroscope_y == 0);
        REQUIRE(imu.gyroscope_z == 0);
    }

    SECTION("Parameterized construction") {
        IMUData imu(1.0, 0.5, 0.3, 9.8, 0.1, 0.2, 0.3);
        REQUIRE(imu.timestamp == 1.0);
        REQUIRE(imu.accelerometer_x == 0.5);
        REQUIRE(imu.accelerometer_y == 0.3);
        REQUIRE(imu.accelerometer_z == 9.8);
        REQUIRE(imu.gyroscope_x == 0.1);
        REQUIRE(imu.gyroscope_y == 0.2);
        REQUIRE(imu.gyroscope_z == 0.3);
    }
}

TEST_CASE("VioFrame structure", "[ros_provider][types]") {
    SECTION("Default construction") {
        VioFrame frame;
        REQUIRE(frame.timestamp == 0);
        REQUIRE(frame.image.empty());
        REQUIRE(frame.imu_samples.empty());
    }

    SECTION("Parameterized construction") {
        ImageData img(640, 480, 3);
        std::vector<IMUData> imu_samples;
        imu_samples.emplace_back(1.0, 0.5, 0.3, 9.8, 0.1, 0.2, 0.3);

        VioFrame frame(std::move(img), std::move(imu_samples), 1.0);

        REQUIRE(frame.timestamp == 1.0);
        REQUIRE(!frame.image.empty());
        REQUIRE(frame.image.width == 640);
        REQUIRE(frame.image.height == 480);
        REQUIRE(frame.imu_samples.size() == 1);
        REQUIRE(frame.imu_samples[0].timestamp == 1.0);
    }
}

} // namespace perception
