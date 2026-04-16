/**
 * @file perception.detector.ixx
 * @brief Object detection algorithms
 */

module;
#include <vector>
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>
#include "constants.h"

export module perception.detector;
import perception.types;
import perception.geom;
import perception.calibration;

/**
 * @brief Computer vision algorithms and object detection
 */
export namespace perception::vision {

    /**
     * @brief Enum to specify the type of detector to use.
     */
    enum class DetectorType {
        MotionBased, // Original background subtraction based detector
        HOG,         // Histogram of Oriented Gradients for pedestrian detection
        DNN          // Deep Neural Network (e.g., YOLOv8) for general object detection
    };


    /**
     * @brief Motion-based object detector using background subtraction
     * 
     * Uses the PIMPL pattern to keep OpenCV types out of the module interface,
     * ensuring faster compilation and better encapsulation.
     */
    class Detector {
    public:
        /**
         * @brief Construct a new Detector object (defaulting to MotionBased).
         * @param threshold Sensitivity threshold for background subtraction (for MotionBased).
         * @param dnn_model_path Path to DNN model (e.g., YOLOv8 ONNX)
         * @param dnn_names_path Path to class names file
         * @param dnn_conf_thresh Confidence threshold for DNN detections
         * @param dnn_nms_thresh NMS threshold for DNN detections
         */
        explicit Detector(double threshold = 16.0, const String& dnn_model_path = "", const String& dnn_names_path = "", float dnn_conf_thresh = 0.5f, float dnn_nms_thresh = 0.4f);
        
        /**
         * @brief Construct a new Detector object with a specified type.
         * @param type The type of detector to initialize (MotionBased, HOG, or DNN).
         * @param threshold Sensitivity threshold for background subtraction (for MotionBased).
         * @param dnn_model_path Path to DNN model (e.g., YOLOv8 ONNX). Required for DNN type.
         * @param dnn_names_path Path to class names file. Required for DNN type.
         * @param dnn_conf_thresh Confidence threshold for DNN detections.
         * @param dnn_nms_thresh NMS threshold for DNN detections.
         */
        explicit Detector(DetectorType type, double threshold = 16.0, const String& dnn_model_path = "", const String& dnn_names_path = "", float dnn_conf_thresh = 0.5f, float dnn_nms_thresh = 0.4f);

        /**
         * @brief Destroy the Detector object
         */
        ~Detector();

        /**
         * @brief Detect moving objects in the given image
         * @param image Input image data
         * @return Vector of detected objects
         */
        auto detect(const ImageData& image) -> Vector<geom::Detection>;

        /**
         * @brief Detects objects in stereo images and localizes them in 3D space.
         */
        auto detectStereo(const ImageData& left_image, const ImageData& right_image, const ImageData& depth_map, const calibration::CameraMatrix& q_matrix) -> Vector<geom::Detection3D>;

        /**
         * @brief Returns the list of supported class names for the current detector configuration.
         */
        const Vector<String>& get_class_names() const;

    private:
        struct Impl;
        UniquePtr<Impl> m_pimpl;
    };

} // namespace perception::vision
