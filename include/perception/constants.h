/**
 * @file constants.h
 * @brief Centralized constants and configuration values for the perception system
 *
 * This header contains all constants, string literals, and magic values used
 * throughout the perception system to avoid hardcoding and enable easy configuration.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

#pragma once

#include <string_view>
#include <array>
#include <cstddef>
#include <cstdint>

namespace perception {
namespace constants {

// ============================================================================
// DETECTOR CONSTANTS
// ============================================================================

namespace detector {
    // Background subtractor parameters
    constexpr int BG_SUBTRACTOR_HISTORY = 500;
    constexpr int BG_SUBTRACTOR_VAR_THRESHOLD = 16;
    constexpr bool BG_SUBTRACTOR_DETECT_SHADOWS = true;

    // Morphological operation parameters
    constexpr int MORPH_KERNEL_WIDTH = 5;
    constexpr int MORPH_KERNEL_HEIGHT = 5;

    // Contour filtering
    constexpr double MIN_CONTOUR_AREA = 500.0;
    constexpr double MIN_PERSON_AREA = 5000.0;
    constexpr double MIN_CAR_AREA = 8000.0;
    constexpr double MIN_BICYCLE_AREA = 2000.0;
    constexpr double MAX_BICYCLE_AREA = 5000.0;

    // Aspect ratio thresholds for classification
    constexpr float PERSON_ASPECT_MIN = 0.2f;
    constexpr float PERSON_ASPECT_MAX = 0.5f;
    constexpr float CAR_ASPECT_MIN = 0.8f;
    constexpr float CAR_ASPECT_MAX = 2.5f;
    constexpr float BICYCLE_ASPECT_MIN = 0.6f;
    constexpr float BICYCLE_ASPECT_MAX = 1.5f;

    // Confidence calculation
    constexpr float BASE_CONFIDENCE = 0.7f;
    constexpr float LARGE_OBJECT_CONFIDENCE_BONUS = 0.1f;
    constexpr double LARGE_OBJECT_AREA_THRESHOLD = 10000.0;
    constexpr float SMALL_OBJECT_CONFIDENCE_PENALTY = 0.2f;
    constexpr double SMALL_OBJECT_AREA_THRESHOLD = 1000.0;
    constexpr float MIN_CONFIDENCE = 0.0f;
    constexpr float MAX_CONFIDENCE = 1.0f;

    // Default confidence threshold for filtering
    constexpr float DEFAULT_CONFIDENCE_THRESHOLD = 0.5f;

    // Class names
    constexpr std::string_view CLASS_NAME_PERSON = "person";
    constexpr std::string_view CLASS_NAME_CAR = "car";
    constexpr std::string_view CLASS_NAME_BICYCLE = "bicycle";
    constexpr std::string_view CLASS_NAME_GENERIC = "generic";
    constexpr std::array<std::string_view, 4> CLASS_NAMES = {
        CLASS_NAME_PERSON, CLASS_NAME_CAR, CLASS_NAME_BICYCLE, CLASS_NAME_GENERIC
    };

    // Generic class ID (fallback)
    constexpr int GENERIC_CLASS_ID = 3;
}

// ============================================================================
// IMAGE PROCESSING CONSTANTS
// ============================================================================

namespace image {
    // Supported image extensions
    constexpr std::string_view EXT_JPG = ".jpg";
    constexpr std::string_view EXT_JPEG = ".jpeg";
    constexpr std::string_view EXT_PNG = ".png";
    constexpr std::string_view EXT_BMP = ".bmp";
    constexpr std::array<std::string_view, 4> SUPPORTED_EXTENSIONS = {
        EXT_JPG, EXT_JPEG, EXT_PNG, EXT_BMP
    };

    // Default test frame dimensions
    constexpr int DEFAULT_TEST_FRAME_WIDTH = 640;
    constexpr int DEFAULT_TEST_FRAME_HEIGHT = 480;
    constexpr int DEFAULT_TEST_FRAME_CHANNELS = 3;

    // Default number of test frames to generate
    constexpr size_t DEFAULT_TEST_FRAME_COUNT = 10;

    // OpenCV color conversion
    constexpr int COLOR_CONVERSION_CODE = 4; // cv::COLOR_BGR2RGB = 4
}

// ============================================================================
// ROSBAG CONSTANTS
// ============================================================================

namespace rosbag {
    // Default topic names
    constexpr std::string_view DEFAULT_IMAGE_TOPIC = "/cam0/image_raw";
    constexpr std::string_view DEFAULT_IMU_TOPIC = "/imu0";

    // Demo limits
    constexpr size_t DEMO_MAX_FRAMES = 10;
}

// ============================================================================
// PIPELINE CONSTANTS
// ============================================================================

namespace pipeline {
    // Default configuration values
    constexpr size_t DEFAULT_THREAD_POOL_SIZE = 4;
    constexpr int DEFAULT_MAX_DETECTIONS = 100;
    constexpr bool DEFAULT_ENABLE_GPU = false;

    // Timing
    constexpr size_t DEFAULT_QUEUE_TIMEOUT_MS = 1000;
    constexpr size_t THREAD_SLEEP_MS = 1;

    // Processing limits
    constexpr size_t MAX_REAL_IMAGES_TO_PROCESS = 6;
    constexpr size_t MAX_GENERATED_FRAMES_TO_PROCESS = 3;
    constexpr size_t DEFAULT_DETECTIONS_TO_DISPLAY = 3;

    // Detection constants (moved from detector namespace for pipeline use)
    constexpr float DEFAULT_DETECTION_CONFIDENCE_THRESHOLD = 0.5f;
    constexpr float DEFAULT_DETECTION_NMS_THRESHOLD = 0.4f;

    // Tracking constants (moved from tracking namespace for pipeline use)
    constexpr float DEFAULT_IOU_THRESHOLD = 0.3f;
    constexpr int DEFAULT_MAX_TRACK_AGE = 5;
}

// ============================================================================
// METRICS CONSTANTS
// ============================================================================

namespace metrics {
    // Maximum number of latency samples to keep
    constexpr size_t MAX_LATENCY_SAMPLES = 10000;

    // Percentile indices
    constexpr double P95_PERCENTILE = 0.95;
    constexpr double P99_PERCENTILE = 0.99;

    // Output formatting
    constexpr int FPS_PRECISION = 2;
    constexpr int LATENCY_PRECISION = 2;
}

// ============================================================================
// FILE PATH CONSTANTS
// ============================================================================

namespace paths {
    // Demo data directory paths (in order of search priority)
    constexpr std::string_view DEMO_DATA_DIR = "demo/data";
    constexpr std::string_view DEMO_DATA_DIR_PARENT = "../demo/data";
    constexpr std::string_view DEMO_DATA_DIR_GRANDPARENT = "../../demo/data";

    // Default bag file
    constexpr std::string_view DEFAULT_BAG_FILE = "demo/data/dataset-calib-cam1_512_16.bag";
}

// ============================================================================
// ERROR MESSAGE CONSTANTS
// ============================================================================

namespace errors {
    // Error category name
    constexpr std::string_view ERROR_CATEGORY_NAME = "perception_error";

    // Error messages
    constexpr std::string_view MSG_SUCCESS = "Success";
    constexpr std::string_view MSG_INVALID_INPUT = "Invalid input provided";
    constexpr std::string_view MSG_PROCESSING_ERROR = "Processing error occurred";
    constexpr std::string_view MSG_THREAD_ERROR = "Thread operation error";
    constexpr std::string_view MSG_QUEUE_EMPTY = "Queue is empty";
    constexpr std::string_view MSG_QUEUE_SHUTDOWN = "Queue has been shut down";
    constexpr std::string_view MSG_RESOURCE_UNAVAILABLE = "Resource unavailable";
    constexpr std::string_view MSG_TIMEOUT = "Operation timed out";
    constexpr std::string_view MSG_UNKNOWN_ERROR = "Unknown error";
    constexpr std::string_view MSG_FILE_NOT_FOUND = "File not found";
    constexpr std::string_view MSG_INVALID_CALIBRATION_DATA = "Invalid calibration data";
    constexpr std::string_view MSG_IO_ERROR = "I/O error";
}

// ============================================================================
// DEMO OUTPUT CONSTANTS
// ============================================================================

namespace demo {
    // Section headers
    constexpr std::string_view HEADER_MAIN = "=== Mini Autonomy System Demo ===";
    constexpr std::string_view HEADER_DETECTION_FILTER = "=== Detection Filtering Demo ===";
    constexpr std::string_view HEADER_IMAGE_LOAD = "=== Image Loading Demo ===";
    constexpr std::string_view HEADER_VISUALIZATION = "=== Bounding Box Visualization Demo ===";
    constexpr std::string_view HEADER_ASYNC_PIPELINE = "=== Async Detection Pipeline Demo (Real Images) ===";
    constexpr std::string_view HEADER_ROSBAG = "=== ROSBAG Provider Demo ===";
    constexpr std::string_view HEADER_ROSBAG_DETECTION = "=== ROSBAG + Detection Demo ===";

    // Status messages
    constexpr std::string_view MSG_PROGRAM_STARTED = "Program started successfully!";
    constexpr std::string_view MSG_ALL_DEMOS_COMPLETED = "All demos completed successfully!";
    constexpr std::string_view MSG_FALLBACK_GENERATED = "Demo data directory not found, falling back to generated frames";
    constexpr std::string_view MSG_NO_REAL_IMAGES = "No real images found, using generated test frames";

    // Labels
    constexpr std::string_view LABEL_TOTAL_DETECTIONS = "Total detections: ";
    constexpr std::string_view LABEL_HIGH_CONFIDENCE = "High confidence detections (>= 0.5):";
    constexpr std::string_view LABEL_LOADING_FROM = "Loading images from demo/data/...";
    constexpr std::string_view LABEL_PROCESSING = "Processing ";
    constexpr std::string_view LABEL_FPS = "FPS: ";
    constexpr std::string_view LABEL_AVG_LATENCY = "Average latency: ";
    constexpr std::string_view LABEL_BAG_FILE = "Bag file: ";
    constexpr std::string_view LABEL_STREAMING_TOPICS = "Streaming from topics:";
    constexpr std::string_view LABEL_IMAGE_TOPIC = "  Image: ";
    constexpr std::string_view LABEL_IMU_TOPIC = "  IMU: ";
    constexpr std::string_view LABEL_FRAME = "Frame ";
    constexpr std::string_view LABEL_DETECTIONS = " detections";

    // Classification explanations
    constexpr std::string_view EXPLAIN_PERSON = "Tall narrow object (aspect ratio ";
    constexpr std::string_view EXPLAIN_CAR = "Wide object with large area (aspect ratio ";
    constexpr std::string_view EXPLAIN_BICYCLE = "Medium-sized object (aspect ratio ";
    constexpr std::string_view EXPLAIN_GENERIC = "Generic object (doesn't match specific categories)";
    constexpr std::string_view EXPLAIN_MOTION = "Motion detected (background subtraction)";

    // Format specifiers
    constexpr std::string_view FORMAT_ASPECT_RATIO = "{:.2f}";
    constexpr std::string_view FORMAT_CONFIDENCE = "{:.2f}";
    constexpr std::string_view FORMAT_AREA = "{:.0f}";
    constexpr std::string_view FORMAT_TIMESTAMP = "{:.6f}";
}

// ============================================================================
// DEFAULT CONFIGURATION
// ============================================================================

namespace defaults {
    // IMU data initialization
    constexpr double IMU_TIMESTAMP_DEFAULT = 0.0;
    constexpr double IMU_ACCEL_DEFAULT = 0.0;
    constexpr double IMU_GYRO_DEFAULT = 0.0;

    // VioFrame initialization
    constexpr double VIO_FRAME_TIMESTAMP_DEFAULT = 0.0;
}

} // namespace constants
} // namespace perception
