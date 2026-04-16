/**
 * @file perception.detector.cpp
 * @brief Implementation of object detection algorithms
 */

module;

#include <opencv2/core/version.hpp>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <fstream>
#include <perception/constants.h>

module perception.detector;

import perception.types;
import perception.geom;

/**
 * @def CV_VERSION_MAJOR
 * @brief Ensures OpenCV version compatibility
 * @details Prevents mixing OpenCV 5 headers with OpenCV 4 libraries
 */
#if CV_VERSION_MAJOR != 4
#error "OpenCV version mismatch: Headers are version 5+ (AlgorithmHint present), but the project is linking against OpenCV 4 libraries."
#endif

namespace perception::vision {

using namespace perception::constants;

/**
 * @struct Detector::Impl
 * @brief Private implementation details for Detector class
 * @details Uses PIMPL pattern to hide OpenCV dependencies from module interface
 */
struct Detector::Impl {
    DetectorType m_detector_type;                   ///< Type of detector (MotionBased, HOG, DNN)
    cv::Ptr<cv::BackgroundSubtractorMOG2> m_back_sub; ///< Background subtractor for motion detection
    cv::HOGDescriptor m_hog;                          ///< HOG descriptor for person detection
    cv::dnn::Net m_dnn_net;                          ///< Neural network for DNN-based detection
    Vector<String> m_dnn_class_names;                 ///< Class names for DNN model
    float m_dnn_confidence_threshold;                 ///< Confidence threshold for DNN detections
    float m_dnn_nms_threshold;                        ///< Non-maximum suppression threshold for DNN

    /**
     * @brief Constructor for Detector implementation
     * @param type Detector type to initialize
     * @param threshold Sensitivity threshold for background subtraction
     * @param dnn_model_path Path to DNN model file (e.g., YOLO ONNX)
     * @param dnn_names_path Path to class names file for DNN
     * @param dnn_conf_thresh Confidence threshold for DNN
     * @param dnn_nms_thresh NMS threshold for DNN
     */
    Impl(DetectorType type, double threshold, const String& dnn_model_path, const String& dnn_names_path, float dnn_conf_thresh, float dnn_nms_thresh)
        : m_detector_type(type), m_dnn_confidence_threshold(dnn_conf_thresh), m_dnn_nms_threshold(dnn_nms_thresh) {

        if (m_detector_type == DetectorType::MotionBased) {
            m_back_sub = cv::createBackgroundSubtractorMOG2(500, threshold, true);
        } else if (m_detector_type == DetectorType::HOG) {
            m_hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
        } else if (m_detector_type == DetectorType::DNN) {
            if (!dnn_model_path.empty()) {
                m_dnn_net = cv::dnn::readNet(dnn_model_path);
                if (m_dnn_net.empty()) {
                    std::cerr << "Error: Could not load DNN model from " << dnn_model_path << std::endl;
                } else {
                    m_dnn_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
                    m_dnn_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
                }

                std::ifstream ifs(dnn_names_path.c_str());
                if (ifs.is_open()) {
                    std::string line;
                    while (std::getline(ifs, line)) m_dnn_class_names.push_back(line);
                } else {
                    std::cerr << "Warning: DNN class names file not found: " << dnn_names_path << std::endl;
                }
            } else {
                std::cerr << "Error: DNN model path not provided for DNN detector type." << std::endl;
            }
        }
    }
};

/**
 * @brief Default constructor for motion-based detector
 * @param threshold Sensitivity threshold for background subtraction
 * @param dnn_model_path Path to DNN model file
 * @param dnn_names_path Path to class names file
 * @param dnn_conf_thresh Confidence threshold for DNN
 * @param dnn_nms_thresh NMS threshold for DNN
 */
Detector::Detector(double threshold, const String& dnn_model_path, const String& dnn_names_path, float dnn_conf_thresh, float dnn_nms_thresh)
    : m_pimpl(perception::make_unique<Impl>(DetectorType::MotionBased, threshold, dnn_model_path, dnn_names_path, dnn_conf_thresh, dnn_nms_thresh)) {}

/**
 * @brief Constructor with explicit detector type
 * @param type Detector type to use
 * @param threshold Sensitivity threshold for background subtraction
 * @param dnn_model_path Path to DNN model file
 * @param dnn_names_path Path to class names file
 * @param dnn_conf_thresh Confidence threshold for DNN
 * @param dnn_nms_thresh NMS threshold for DNN
 */
Detector::Detector(DetectorType type, double threshold, const String& dnn_model_path, const String& dnn_names_path, float dnn_conf_thresh, float dnn_nms_thresh)
    : m_pimpl(perception::make_unique<Impl>(type, threshold, dnn_model_path, dnn_names_path, dnn_conf_thresh, dnn_nms_thresh)) {}

/**
 * @brief Destructor
 */
Detector::~Detector() = default;

/**
 * @brief Detect objects in an image
 * @param image Input image data
 * @return Vector of detected objects with bounding boxes and confidence scores
 * @details Supports motion-based, HOG, and DNN detection methods
 */
auto Detector::detect(const ImageData& image) -> Vector<geom::Detection> {
    if (image.empty()) {
        return {};
    }

    cv::Mat processed_frame;
    cv::Mat current_mat = image.m_impl->mat;

    if (current_mat.empty()) {
        return {};
    }

    if (current_mat.depth() == CV_16U) { // Handle 16-bit images (e.g., Gray16)
        cv::Mat temp_8u;
        // Scale 16-bit (0-65535) to 8-bit (0-255)
        current_mat.convertTo(temp_8u, CV_8U, 1.0 / 256.0); 
        if (temp_8u.channels() == 1) {
            cv::cvtColor(temp_8u, processed_frame, cv::COLOR_GRAY2BGR);
        } else if (temp_8u.channels() == 3) { // Should not happen for Gray16, but for other 16-bit types
            processed_frame = temp_8u;
            } else {
            cv::cvtColor(temp_8u, processed_frame, cv::COLOR_GRAY2BGR); // Fallback to BGR
        }
    } else if (current_mat.depth() == CV_32F) { // Handle 32-bit float images (e.g., Bgr32F)
        cv::Mat temp_8u;
        // Scale float (0.0-1.0 or custom range) to 8-bit (0-255)
        // Assuming float images are normalized to 0.0-1.0. If not, a proper scaling factor is needed.
        current_mat.convertTo(temp_8u, CV_8U, 255.0);
        if (temp_8u.channels() == 1) {
            cv::cvtColor(temp_8u, processed_frame, cv::COLOR_GRAY2BGR);
        } else {
            processed_frame = temp_8u;
        }
    } else if (current_mat.depth() == CV_8U) { // Handle 8-bit images
        if (current_mat.channels() == 1) { // Gray
            cv::cvtColor(current_mat, processed_frame, cv::COLOR_GRAY2BGR);
        } else if (current_mat.channels() == 4) { // BGRA
            cv::cvtColor(current_mat, processed_frame, cv::COLOR_BGRA2BGR);
        } else { // BGR (or RGB from image_loader, which should be BGR now)
            processed_frame = current_mat;
        }
    } else {
        // Fallback for unsupported formats, or if current_mat is already in a suitable format
        // For now, we'll assume it's BGR 8-bit if no conversion was explicitly handled.
        processed_frame = current_mat;
    }

    Vector<geom::Detection> detections;
    
    if (m_pimpl->m_detector_type == DetectorType::MotionBased) {
        cv::Mat fg_mask;
        m_pimpl->m_back_sub->apply(processed_frame, fg_mask);

        // Post-processing to remove noise and bridge gaps
        cv::threshold(fg_mask, fg_mask, 200, 255, cv::THRESH_BINARY);
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_OPEN, kernel);
        cv::dilate(fg_mask, fg_mask, kernel, cv::Point(-1, -1), 2);

        // Find object contours
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(fg_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& contour : contours) {
            const double area = cv::contourArea(contour);
            
            // Filter out small movements/noise
            if (area < 400) {
                continue;
            }

            const cv::Rect rect = cv::boundingRect(contour);
            const float aspectRatio = static_cast<float>(rect.width) / rect.height;
            
            // Calculate advanced geometry metrics
            std::vector<cv::Point> hull;
            cv::convexHull(contour, hull);
            const double hullArea = cv::contourArea(hull);
            const float solidity = (hullArea > 0) ? static_cast<float>(area / hullArea) : 0.0f;
            const float extent = static_cast<float>(area) / (rect.width * rect.height);
            
            String class_name = "object";
            int class_id = 0;

            // Improved heuristic-based classification
            if (aspectRatio > 0.2f && aspectRatio < 0.8f && extent < 0.8f) {
                if (solidity > 0.5f) {
                    class_name = "person";
                    class_id = 1;
                }
            } else if (aspectRatio > 1.1f && area > 2000 && extent > 0.6f) {
                class_name = "car";
                class_id = 2;
            } else if (aspectRatio > 0.5f && aspectRatio < 1.2f && area > 800) {
                if (solidity < 0.6f) { // Bicycles usually have lower solidity due to frame gaps
                    class_name = "bicycle";
                    class_id = 3;
                }
            }

            detections.emplace_back(
                geom::Rect{rect.x, rect.y, rect.width, rect.height},
                0.85f, // Confidence estimate for motion-based detection
                class_id,
                class_name
            );
        }
    } else if (m_pimpl->m_detector_type == DetectorType::HOG) {
        std::vector<cv::Rect> hog_found_locations;
        std::vector<double> hog_weights;
        m_pimpl->m_hog.detectMultiScale(processed_frame, hog_found_locations, hog_weights, 0, cv::Size(8, 8), cv::Size(), 1.05, 2, false);

        for (size_t i = 0; i < hog_found_locations.size(); ++i) {
            const cv::Rect& hog_rect = hog_found_locations[i];
            float confidence = static_cast<float>(hog_weights[i]);

            // HOG is primarily for persons, so we directly add them.
            // No need to check for overlap with other types of detections here,
            // as HOG is a standalone detector in this context.
            detections.emplace_back(
                geom::Rect{hog_rect.x, hog_rect.y, hog_rect.width, hog_rect.height},
                confidence,
                1, // Class ID for person
                "person"
            );
        }
    } else if (m_pimpl->m_detector_type == DetectorType::DNN) {
        if (m_pimpl->m_dnn_net.empty()) {
            std::cerr << "Error: DNN model not loaded. Cannot perform detection." << std::endl;
            return {};
        }

        cv::Mat blob;
        // YOLOv8 typically expects 640x640, 0-1 scaling, no mean subtraction, RGB swap=false (if input is RGB)
        // Assuming `frame` is RGB (as per image_loader.cpp converting to RGB)
        cv::dnn::blobFromImage(processed_frame, blob, 1/255.0, cv::Size(640, 640), cv::Scalar(), false, false);
        m_pimpl->m_dnn_net.setInput(blob);

        std::vector<cv::Mat> outputs;
        m_pimpl->m_dnn_net.forward(outputs, m_pimpl->m_dnn_net.getUnconnectedOutLayersNames());

        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> class_ids;

        // Process YOLOv8 output. Assuming outputs[0] is [1, num_classes + 4, num_detections] (transposed)
        // Standard YOLOv8/v11 output is [1, 84, 8400]. We transpose to [8400, 84] to get detections as rows.
        cv::Mat output_data = outputs[0];
        if (output_data.dims == 3) {
            // Squeeze first dimension [1, 84, 8400] -> [84, 8400]
            cv::Mat squeezed = output_data.reshape(1, output_data.size[1]);
            // Transpose [84, 8400] -> [8400, 84]
            cv::transpose(squeezed, output_data);
        } else if (output_data.dims == 2) { // Already [detections, 5+classes]
            // Do nothing
        } else {
            std::cerr << "Warning: Unexpected YOLOv8 output shape. Detections might be incorrect." << std::endl;
            return {};
        }
        
        // Calculate scaling factors (assuming 640x640 model input)
        float x_factor = static_cast<float>(processed_frame.cols) / 640.0f;
        float y_factor = static_cast<float>(processed_frame.rows) / 640.0f;

        for (int i = 0; i < output_data.rows; ++i) {
            float* data = (float*)output_data.row(i).data;
            cv::Mat scores = output_data.row(i).colRange(4, output_data.cols); // Class scores start from index 4
            cv::Point class_id_point;
            double max_class_score;
            cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id_point);
            
            if (max_class_score > m_pimpl->m_dnn_confidence_threshold) { // Keep only high confidence detections
                // Scale coordinates from model space (640) to image space
                float x_center = data[0] * x_factor;
                float y_center = data[1] * y_factor;
                float bbox_w = data[2] * x_factor;
                float bbox_h = data[3] * y_factor;

                int left = static_cast<int>(x_center - bbox_w / 2.0f);
                int top = static_cast<int>(y_center - bbox_h / 2.0f);
                int width = static_cast<int>(bbox_w);
                int height = static_cast<int>(bbox_h);

                // Clamp box to image boundaries to prevent overflow and OOB errors
                int x1 = std::clamp(left, 0, processed_frame.cols - 1);
                int y1 = std::clamp(top, 0, processed_frame.rows - 1);
                int x2 = std::clamp(left + width, x1 + 1, processed_frame.cols);
                int y2 = std::clamp(top + height, y1 + 1, processed_frame.rows);

                boxes.push_back(cv::Rect(x1, y1, x2 - x1, y2 - y1));
                confidences.push_back(static_cast<float>(max_class_score));
                class_ids.push_back(class_id_point.x);
            }
        }

        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, m_pimpl->m_dnn_confidence_threshold, m_pimpl->m_dnn_nms_threshold, indices);

        for (int idx : indices) {
            cv::Rect box = boxes[idx];
            int class_id = class_ids[idx];
            float confidence = confidences[idx];
            String class_name = (class_id >= 0 && class_id < m_pimpl->m_dnn_class_names.size()) ? m_pimpl->m_dnn_class_names[class_id] : "unknown";

            // Add DNN detection. A more robust system would merge/prioritize with existing detections.
            detections.emplace_back(
                geom::Rect{box.x, box.y, box.width, box.height},
                confidence,
                class_id,
                class_name
            );
        }
    }

    return detections;
}

/**
 * @brief Get class names supported by the detector
 * @return Reference to vector of class name strings
 */
const Vector<String>& Detector::get_class_names() const {
    if (m_pimpl->m_detector_type == DetectorType::DNN) {
        return m_pimpl->m_dnn_class_names;
    } else if (m_pimpl->m_detector_type == DetectorType::HOG) {
        static const Vector<String> hog_names = {"person"};
        return hog_names;
    } else {
        static const Vector<String> motion_names = {
            String(constants::detector::CLASS_NAME_PERSON),
            String(constants::detector::CLASS_NAME_CAR),
            String(constants::detector::CLASS_NAME_BICYCLE),
            String(constants::detector::CLASS_NAME_GENERIC)
        };
        return motion_names;
    }
}

/**
 * @brief Detect objects in stereo images with 3D localization
 * @param left_image Left camera image
 * @param right_image Right camera image (unused in current implementation)
 * @param depth_map Depth map for 3D localization
 * @param q_matrix_array Camera matrix for stereo rectification (unused in current implementation)
 * @return Vector of 3D detections with spatial coordinates
 * @details Performs 2D detection on left image and projects to 3D using depth map
 */
auto Detector::detectStereo(const ImageData& left_image, const ImageData& right_image, const ImageData& depth_map, const calibration::CameraMatrix& q_matrix_array) -> Vector<geom::Detection3D> {

    Vector<geom::Detection> detections_2d = detect(left_image);

    Vector<geom::Detection3D> detections_3d;
    if (detections_2d.empty()) {
        return detections_3d;
    }

    cv::Mat depth_mat = depth_map.m_impl->mat;
    if (depth_mat.type() != CV_32FC3) {
        std::cerr << "Warning: Depth map is not CV_32FC3. Cannot perform 3D localization.\n";
        return {};
    }

    for (const auto& det_2d : detections_2d) {
        int center_x = det_2d.bbox.x + det_2d.bbox.width / 2;
        int center_y = det_2d.bbox.y + det_2d.bbox.height / 2;

        if (center_x >= 0 && center_x < depth_mat.cols &&
            center_y >= 0 && center_y < depth_mat.rows) {
            
            cv::Vec3f point_3d_cv = depth_mat.at<cv::Vec3f>(center_y, center_x);

            detections_3d.emplace_back(
                geom::Detection3D{det_2d.bbox, geom::Point3D{point_3d_cv[0], point_3d_cv[1], point_3d_cv[2]}, det_2d.confidence, det_2d.class_name}
            );
        }
    }

    return detections_3d;
}

} // namespace perception::vision