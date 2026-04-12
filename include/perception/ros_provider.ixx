module;
#include <coroutine>
#include <filesystem>
#include <string>
#include <vector>

export module perception.ros_provider;
import perception.types;
import perception.result;
import perception.async;
import perception.image_loader;

namespace perception {

    /**
     * ROSBAG data provider for reading ROS bag files
     * Supports synchronized image and IMU data streaming
     */
    export class RosBagProvider {
    public:
        using FrameType = VioFrame;
        using GeneratorType = Generator<Expected<VioFrame, PerceptionError>>;

        /**
         * @brief Constructor
         */
        RosBagProvider();

        /**
         * @brief Destructor
         */
        ~RosBagProvider();

        /**
         * Open a ROSBAG file
         * @param path Path to the .bag file
         */
        auto open(const std::string& path) -> Expected<void, PerceptionError>;

        /**
         * Close the ROSBAG file
         */
        auto close() -> void;

        /**
         * Check if a bag file is open
         */
        auto is_open() const -> bool;

        /**
         * Stream synchronized data from the bag file
         * @param img_topic Image topic name
         * @param imu_topic IMU topic name
         * @return Generator yielding synchronized frames
         */
        auto stream_data(const std::string& img_topic, const std::string& imu_topic)
            -> GeneratorType;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace perception
