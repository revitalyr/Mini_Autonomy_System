// GMF - ROS and OpenCV isolated here
#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/Imu.h>
#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include <coroutine>

module perception.ros_provider;

import perception.types;
import perception.result;
import perception.async;

namespace perception {

    // PIMPL implementation - knows about ROS and OpenCV
    struct RosBagProvider::Impl {
        std::unique_ptr<rosbag::Bag> bag;
        std::string current_path;
        bool is_open = false;

        Impl() : bag(std::make_unique<rosbag::Bag>()) {}
        ~Impl() {
            if (is_open && bag) {
                bag->close();
            }
        }
    };

    // --- Constructor/Destructor ---

    RosBagProvider::RosBagProvider()
        : impl_(std::make_unique<Impl>()) {}

    RosBagProvider::~RosBagProvider() = default;

    // --- Open/Close ---

    auto RosBagProvider::open(const std::string& path) -> Expected<void, PerceptionError> {
        try {
            // Check if file exists
            if (!std::filesystem::exists(path)) {
                return make_unexpected(PerceptionError::FileNotFound);
            }

            // Check if file is a regular file
            if (!std::filesystem::is_regular_file(path)) {
                return make_unexpected(PerceptionError::InvalidInput);
            }

            // Check file extension
            if (path.size() < 4 || path.substr(path.size() - 4) != ".bag") {
                return make_unexpected(PerceptionError::InvalidInput);
            }

            // Open the bag file
            impl_->bag->open(path, rosbag::bagmode::Read);
            impl_->current_path = path;
            impl_->is_open = true;

            return {};
        } catch (const rosbag::BagException&) {
            return make_unexpected(PerceptionError::InvalidInput);
        } catch (const std::exception&) {
            return make_unexpected(PerceptionError::InvalidInput);
        }
    }

    auto RosBagProvider::close() -> void {
        if (impl_->is_open && impl_->bag) {
            impl_->bag->close();
            impl_->is_open = false;
        }
    }

    auto RosBagProvider::is_open() const -> bool {
        return impl_->is_open;
    }

    // --- Generator for streaming data ---

    // Generator coroutine implementation
    struct RosBagGenerator {
        struct promise_type {
            Expected<VioFrame, PerceptionError> current_value;
            std::exception_ptr exception_;

            RosBagGenerator get_return_object() {
                return RosBagGenerator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void unhandled_exception() {
                exception_ = std::current_exception();
            }

            std::suspend_always yield_value(Expected<VioFrame, PerceptionError> value) {
                current_value = std::move(value);
                return {};
            }

            void return_void() {}
        };

        std::coroutine_handle<promise_type> handle;

        explicit RosBagGenerator(std::coroutine_handle<promise_type> h) : handle(h) {}
        ~RosBagGenerator() {
            if (handle) {
                handle.destroy();
            }
        }

        RosBagGenerator(const RosBagGenerator&) = delete;
        RosBagGenerator& operator=(const RosBagGenerator&) = delete;

        RosBagGenerator(RosBagGenerator&& other) noexcept : handle(std::exchange(other.handle, nullptr)) {}
        RosBagGenerator& operator=(RosBagGenerator&& other) noexcept {
            if (this != &other) {
                if (handle) {
                    handle.destroy();
                }
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        bool next() {
            handle.resume();
            return !handle.done();
        }

        Expected<VioFrame, PerceptionError> get() {
            if (handle.promise().exception_) {
                std::rethrow_exception(handle.promise().exception_);
            }
            return std::move(handle.promise().current_value);
        }
    };

    auto RosBagProvider::stream_data(const std::string& img_topic, const std::string& imu_topic)
        -> GeneratorType {

        if (!impl_->is_open) {
            co_yield make_unexpected(PerceptionError::InvalidInput);
            co_return;
        }

        try {
            rosbag::View view(*impl_->bag, rosbag::TopicQuery({img_topic, imu_topic}));

            VioFrame current_frame;

            for (const auto& message : view) {
                // Handle IMU messages
                if (message.getTopic() == imu_topic) {
                    auto imu_msg = message.instantiate<sensor_msgs::Imu>();
                    if (imu_msg) {
                        IMUData imu_data(
                            imu_msg->header.stamp.toSec(),
                            imu_msg->linear_acceleration.x,
                            imu_msg->linear_acceleration.y,
                            imu_msg->linear_acceleration.z,
                            imu_msg->angular_velocity.x,
                            imu_msg->angular_velocity.y,
                            imu_msg->angular_velocity.z
                        );
                        current_frame.imu_samples.push_back(imu_data);
                    }
                }

                // Handle image messages
                if (message.getTopic() == img_topic) {
                    auto img_msg = message.instantiate<sensor_msgs::Image>();
                    if (img_msg) {
                        try {
                            // Convert ROS Image to cv::Mat using cv_bridge
                            cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(img_msg, sensor_msgs::image_encodings::BGR8);

                            if (cv_ptr && !cv_ptr->image.empty()) {
                                // Convert cv::Mat to ImageData
                                ImageData img_data(cv_ptr->image.cols, cv_ptr->image.rows, cv_ptr->image.channels());
                                const auto* data = cv_ptr->image.data;
                                std::copy(data, data + cv_ptr->image.total() * cv_ptr->image.elemSize(),
                                          img_data.data.begin());

                                current_frame.image = std::move(img_data);
                                current_frame.timestamp = img_msg->header.stamp.toSec();

                                co_yield success(std::move(current_frame));

                                // Reset for next frame
                                current_frame = VioFrame();
                            }
                        } catch (const cv_bridge::Exception&) {
                            co_yield make_unexpected(PerceptionError::DecodeFailure);
                        } catch (const cv::Exception&) {
                            co_yield make_unexpected(PerceptionError::DecodeFailure);
                        }
                    }
                }
            }

        } catch (const rosbag::BagException&) {
            co_yield make_unexpected(PerceptionError::InvalidInput);
        } catch (const std::exception&) {
            co_yield make_unexpected(PerceptionError::InvalidInput);
        }
    }

} // namespace perception
