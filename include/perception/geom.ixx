module;
#include <vector>
#include <string>
#include <cstdint>

export module perception.geom;
import perception.types;

/**
 * @brief Geometric and sensor data types for the perception system
 */
export namespace perception::geom {

    /**
     * @brief Rectangle for bounding boxes
     */
    struct Rect {
        int x, y;
        int width, height;

        Rect() : x(0), y(0), width(0), height(0) {}
        Rect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}
    };

    /**
     * @brief Detection result containing bounding box, confidence, and class information
     */
    struct Detection {
        Rect bbox;              // Bounding box coordinates
        float confidence;       // Detection confidence (0.0 to 1.0)
        int class_id;           // Class identifier
        perception::String class_name; // Human-readable class name

        Detection(Rect b, float conf, int id, perception::String name)
            : bbox(std::move(b)), confidence(conf), class_id(id), class_name(std::move(name)) {}
    };

    /**
     * @brief Image data structure
     */
    struct ImageData {
        int width;
        int height;
        int channels;
        perception::Vector<uint8_t> data;

        ImageData() : width(0), height(0), channels(0) {}
        ImageData(int w, int h, int c) 
            : width(w), height(h), channels(c), data(w * h * c) {}
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

        IMUData() : timestamp(0), accelerometer_x(0), accelerometer_y(0), accelerometer_z(0),
                    gyroscope_x(0), gyroscope_y(0), gyroscope_z(0) {}
    };

    /**
     * @brief VIO frame with image and IMU data
     */
    struct VioFrame {
        double timestamp;
        ImageData image;
        perception::Vector<IMUData> imu_samples;
    };

} // namespace perception::geom