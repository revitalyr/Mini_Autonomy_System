/**
 * @file perception.ros_provider.cpp
 * @brief Implementation of ROSBAG data provider using ROS 2
 *
 * Provides synchronized image and IMU data from ROSBAG files
 * using the RosBagProvider module with ROS 2 APIs.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

#include "perception/ros_provider.hpp"
#include "perception/result.hpp"
#include "perception/image_loader.hpp"

#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <rclcpp/serialization.hpp>
#include <opencv2/opencv.hpp>

#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include <coroutine>
#include <unordered_map>
#include <mutex>

namespace perception {

struct RosBagProvider::Impl {
    std::unique_ptr<rosbag2_cpp::Reader> reader;
    std::string current_path;
    bool is_open = false;
    std::mutex mutex_;
};

RosBagProvider::RosBagProvider() : impl_(std::make_unique<Impl>()) {}

RosBagProvider::~RosBagProvider() = default;

Expected<void, PerceptionError> RosBagProvider::open(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            return error<void>(PerceptionError::InvalidInput);
        }

        impl_->reader = std::make_unique<rosbag2_cpp::Reader>();
        
        rosbag2_storage::StorageOptions storage_options;
        storage_options.uri = path;
        storage_options.storage_id = "sqlite3";
        
        rosbag2_cpp::ConverterOptions converter_options;
        converter_options.input_serialization_format = "cdr";
        converter_options.output_serialization_format = "cdr";

        impl_->reader->open(storage_options, converter_options);
        impl_->current_path = path;
        impl_->is_open = true;

        return {};
    } catch (const std::exception& e) {
        return error<void>(PerceptionError::InvalidInput);
    }
}

void RosBagProvider::close() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (impl_->is_open && impl_->reader) {
        impl_->reader->close();
        impl_->is_open = false;
    }
}

bool RosBagProvider::is_open() const {
    return impl_->is_open;
}

RosBagProvider::GeneratorType RosBagProvider::stream_data(const std::string& img_topic, const std::string& imu_topic) {
    (void)img_topic;
    (void)imu_topic;
    
    if (!impl_->is_open || !impl_->reader) {
        co_yield error<VioFrame>(PerceptionError::InvalidInput);
        co_return;
    }

    std::unordered_map<std::string, std::vector<std::shared_ptr<rosbag2_storage::SerializedBagMessage>>> image_messages;
    std::unordered_map<std::string, std::vector<std::shared_ptr<rosbag2_storage::SerializedBagMessage>>> imu_messages;
    
    bool read_success = true;
    
    try {
        while (impl_->reader->has_next()) {
            auto msg = impl_->reader->read_next();
            
            if (msg->topic_name.find("/cam") != std::string::npos || 
                msg->topic_name.find("/image") != std::string::npos) {
                image_messages[msg->topic_name].push_back(msg);
            }
            
            if (msg->topic_name.find("/imu") != std::string::npos) {
                imu_messages[msg->topic_name].push_back(msg);
            }
        }
    } catch (...) {
        read_success = false;
    }
    
    if (!read_success) {
        co_yield error<VioFrame>(PerceptionError::InvalidInput);
        co_return;
    }
    
    std::string actual_img_topic;
    for (const auto& [topic, msgs] : image_messages) {
        if (!msgs.empty()) {
            actual_img_topic = topic;
            break;
        }
    }
    
    std::string actual_imu_topic;
    for (const auto& [topic, msgs] : imu_messages) {
        if (!msgs.empty()) {
            actual_imu_topic = topic;
            break;
        }
    }
    
    if (actual_img_topic.empty()) {
        co_yield error<VioFrame>(PerceptionError::InvalidInput);
        co_return;
    }
    
    rclcpp::Serialization<sensor_msgs::msg::Image> image_serializer;
    rclcpp::Serialization<sensor_msgs::msg::Imu> imu_serializer;
    
    for (const auto& serialized_msg : image_messages[actual_img_topic]) {
        VioFrame frame;
        sensor_msgs::msg::Image img_msg;
        
        rclcpp::SerializedMessage serialized_msg_wrapper;
        serialized_msg_wrapper.reserve(serialized_msg->serialized_data->buffer_length);
        std::memcpy(
            serialized_msg_wrapper.get_rcl_serialized_message().buffer,
            serialized_msg->serialized_data->buffer,
            serialized_msg->serialized_data->buffer_length
        );
        serialized_msg_wrapper.get_rcl_serialized_message().buffer_length = serialized_msg->serialized_data->buffer_length;
        
        bool deserialize_success = true;
        
        try {
            image_serializer.deserialize_message(&serialized_msg_wrapper, &img_msg);
            
            int width = img_msg.width;
            int height = img_msg.height;
            
            cv::Mat cv_img(height, width, CV_8UC3);
            
            if (img_msg.encoding == "bgr8" || img_msg.encoding == "8UC3") {
                std::copy(img_msg.data.begin(), img_msg.data.end(), cv_img.data);
            } else if (img_msg.encoding == "rgb8") {
                cv::Mat rgb_img(height, width, CV_8UC3, const_cast<uint8_t*>(img_msg.data.data()));
                cv::cvtColor(rgb_img, cv_img, cv::COLOR_RGB2BGR);
            } else if (img_msg.encoding == "mono8" || img_msg.encoding == "8UC1") {
                cv::Mat gray_img(height, width, CV_8UC1, const_cast<uint8_t*>(img_msg.data.data()));
                cv::cvtColor(gray_img, cv_img, cv::COLOR_GRAY2BGR);
            } else if (img_msg.encoding == "mono16") {
                // mono16 is 16-bit grayscale, need to convert to 8-bit then to BGR
                cv::Mat mono16_img(height, width, CV_16UC1, const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(img_msg.data.data())));
                cv::Mat mono8_img;
                mono16_img.convertTo(mono8_img, CV_8UC1, 1.0 / 256.0);
                cv::cvtColor(mono8_img, cv_img, cv::COLOR_GRAY2BGR);
            } else {
                deserialize_success = false;
            }
            
            if (deserialize_success) {
                ImageData img_data(cv_img.cols, cv_img.rows, cv_img.channels());
                const auto* data = cv_img.data;
                std::copy(data, data + cv_img.total() * cv_img.elemSize(), img_data.data.begin());
                
                frame.image = std::move(img_data);
                frame.timestamp = img_msg.header.stamp.sec + img_msg.header.stamp.nanosec * 1e-9;
                
                if (!actual_imu_topic.empty()) {
                    double img_time = frame.timestamp;
                    
                    for (const auto& imu_serialized : imu_messages[actual_imu_topic]) {
                        sensor_msgs::msg::Imu imu_msg;
                        
                        rclcpp::SerializedMessage imu_serialized_wrapper;
                        imu_serialized_wrapper.reserve(imu_serialized->serialized_data->buffer_length);
                        std::memcpy(
                            imu_serialized_wrapper.get_rcl_serialized_message().buffer,
                            imu_serialized->serialized_data->buffer,
                            imu_serialized->serialized_data->buffer_length
                        );
                        imu_serialized_wrapper.get_rcl_serialized_message().buffer_length = imu_serialized->serialized_data->buffer_length;
                        
                        try {
                            imu_serializer.deserialize_message(&imu_serialized_wrapper, &imu_msg);
                            
                            double imu_time = imu_msg.header.stamp.sec + imu_msg.header.stamp.nanosec * 1e-9;
                            if (std::abs(imu_time - img_time) < 0.1) {
                                IMUData imu_data;
                                imu_data.timestamp = imu_time;
                                imu_data.accelerometer_x = imu_msg.linear_acceleration.x;
                                imu_data.accelerometer_y = imu_msg.linear_acceleration.y;
                                imu_data.accelerometer_z = imu_msg.linear_acceleration.z;
                                imu_data.gyroscope_x = imu_msg.angular_velocity.x;
                                imu_data.gyroscope_y = imu_msg.angular_velocity.y;
                                imu_data.gyroscope_z = imu_msg.angular_velocity.z;
                                frame.imu_samples.push_back(imu_data);
                            }
                        } catch (...) {
                            // Skip IMU messages that fail to deserialize
                        }
                    }
                }
                
                co_yield success<VioFrame>(std::move(frame));
            }
        } catch (...) {
            deserialize_success = false;
        }
        
        if (!deserialize_success) {
            continue;
        }
    }
}

} // namespace perception
