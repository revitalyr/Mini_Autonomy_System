module;

#include <opencv2/core.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <memory>
#include <atomic>
#include "constants.h"

export module perception.types;

/**
 * @brief Common data types for perception system
 *
 * Defines standard data structures used throughout the perception system
 * for images, detections, and configuration.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

export namespace perception {

    // Type aliases for convenience
    using String = std::string;
    using StringView = std::string_view;

    using ByteBuffer = uint8_t*;
    using ConstByteBuffer = const uint8_t*;

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using Optional = std::optional<T>;

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T>
    using Atomic = std::atomic<T>;

    using AtomicBool = std::atomic<bool>;
    
    using Mutex = std::mutex;
    using LockGuard = std::lock_guard<std::mutex>;
    using UniqueLock = std::unique_lock<std::mutex>;
    
    using Milliseconds = std::chrono::milliseconds;
    using Seconds = std::chrono::seconds;
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Clock = std::chrono::high_resolution_clock;

    using Count = size_t;
    
    // Utility functions
    template<typename T, typename... Args>
    auto make_unique(Args&&... args) -> UniquePtr<T> {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    auto to_milliseconds(std::chrono::nanoseconds duration) -> Milliseconds {
        return std::chrono::duration_cast<Milliseconds>(duration);
    }

    /**
     * @brief Supported pixel formats for image ingestion
     */
    export enum class PixelFormat {
        Bgr,      ///< 8-bit BGR (OpenCV default)
        Rgb,      ///< 8-bit RGB
        Gray,     ///< 8-bit grayscale
        Gray16,   ///< 16-bit grayscale (unsigned short)
        Bgr16,    ///< 16-bit BGR (unsigned short)
        Bgr32F,   ///< 32-bit BGR (float)
        Bgra,     ///< 8-bit BGRA
        Rgba,     ///< 8-bit RGBA
        YuvI420,  ///< YUV 4:20 Planar (Y-plane followed by U and V)
        YuvNv12   ///< YUV 4:20 Semi-Planar (Y-plane followed by interleaved UV)
    };

    /**
     * @brief Opaque implementation for PIMPL pattern
     */
    export struct ImageImpl {
        cv::Mat mat;
    };

    /**
     * @brief Raw image data with pixel buffer (PIMPL wrapper)
     */
    export struct ImageData {
        ImageData();
        ~ImageData();

        // Move operations (required for unique_ptr)
        ImageData(ImageData&&) noexcept;
        ImageData& operator=(ImageData&&) noexcept;

        // Copy operations - use clone() for deep copy
        ImageData(const ImageData& other);
        ImageData& operator=(const ImageData& other);

        /**
         * @brief Creates a deep copy of the image data using OpenCV's clone()
         */
        [[nodiscard]] auto clone() const -> ImageData;

        /**
         * @brief Ensures the ImageData owns its internal buffer.
         * If it's currently a non-owning view, a deep copy is performed.
         */
        auto ensure_owned() -> void;

        /**
         * @brief Creates an ImageData object from a raw buffer
         * @param data Pointer to the raw pixel data
         * @param width Image width in pixels
         * @param height Image height in pixels
         * @param format Source pixel format
         * @param copy_data If false, creates a zero-copy view (buffer must outlive ImageData)
         */
        static auto fromRaw(ConstByteBuffer data, int width, int height, PixelFormat format, bool copy_data = true) -> ImageData;

        /**
         * @brief Provides access to the raw data buffer
         */
        [[nodiscard]] auto data() -> ByteBuffer;
        [[nodiscard]] auto data() const -> ConstByteBuffer;

        /**
         * @brief Returns true if the image has no data
         */
        [[nodiscard]] auto empty() const -> bool;

        int m_width = 0;
        int m_height = 0;
        int m_channels = 0;
        int m_bit_depth = 8;
        double m_timestamp = 0.0;

        UniquePtr<ImageImpl> m_impl; // Public for cross-module perception implementations
    };

    /**
     * @brief Configuration parameters
     */
    namespace stereo {
        /**
         * @brief Configuration for the stereo matching algorithm.
         */
        struct StereoMatchingConfig {
            int min_disparity = 0;
            int num_disparities = 16 * 5; // Must be divisible by 16
            int block_size = 11;          // Odd number, 3-11 for BM, 3-15 for SGBM
            bool use_sgbm = true;         // Use StereoSGBM (more accurate) or StereoBM (faster)
            int p1 = 0;                   // SGBM parameter
            int p2 = 0;                   // SGBM parameter
            int disp_12_max_diff = -1;    // SGBM parameter
            int pre_filter_cap = 31;      // SGBM parameter
            int uniqueness_ratio = 10;    // SGBM parameter
            int speckle_window_size = 100; // SGBM parameter
            int speckle_range = 32;       // SGBM parameter
        };
    }

    struct Config {
        size_t thread_pool_size = constants::pipeline::DEFAULT_THREAD_POOL_SIZE;
        int max_detections = constants::pipeline::DEFAULT_MAX_DETECTIONS;
        float confidence_threshold = constants::detector::DEFAULT_CONFIDENCE_THRESHOLD;
        bool enable_gpu = constants::pipeline::DEFAULT_ENABLE_GPU;
        // Detector parameters
        int detector_sensitivity = constants::detector::BG_SUBTRACTOR_VAR_THRESHOLD;
        // DNN parameters
        String dnn_model_path;
        String dnn_names_path;
        float dnn_confidence_threshold = 0.5f;
        float dnn_nms_threshold = 0.4f;
        // Motion detection threshold
        double motion_threshold = constants::detector::BG_SUBTRACTOR_VAR_THRESHOLD;
        // Tracking parameters
        float tracker_iou_threshold = 0.3f;
        int tracker_max_age = 5;
        // Stereo matching parameters
        stereo::StereoMatchingConfig stereo_matching_config;
        // Calibration parameters
        double mono_calib_alpha = 0.0; // Alpha for mono undistortion
    };

    /**
     * @brief IMU data structure
     */
    struct IMUData {
        double timestamp;
        double accelerometer_x;
        double accelerometer_y;
        double accelerometer_z;
        double gyroscope_x;
        double gyroscope_y;
        double gyroscope_z;

        IMUData() : timestamp(constants::defaults::IMU_TIMESTAMP_DEFAULT),
                    accelerometer_x(constants::defaults::IMU_ACCEL_DEFAULT),
                    accelerometer_y(constants::defaults::IMU_ACCEL_DEFAULT),
                    accelerometer_z(constants::defaults::IMU_ACCEL_DEFAULT),
                    gyroscope_x(constants::defaults::IMU_GYRO_DEFAULT),
                    gyroscope_y(constants::defaults::IMU_GYRO_DEFAULT),
                    gyroscope_z(constants::defaults::IMU_GYRO_DEFAULT) {}
    };

    /**
     * @brief VIO frame with image and IMU data
     */
    struct VioFrame {
        double timestamp;
        ImageData image;
        std::vector<IMUData> imu_samples;
    };

} // namespace perception
