/**
 * @file perception.detector.cpp
 * @brief Implementation of the motion-based object detector
 */
module;

#include <opencv2/core/version.hpp>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <fstream>

module perception.detector;

// Prevent mixing OpenCV 5 headers with OpenCV 4 libraries
#if CV_VERSION_MAJOR != 4
#error "OpenCV version mismatch: Headers are version 5+ (AlgorithmHint present), but the project is linking against OpenCV 4 libraries."
#endif

namespace perception::vision {

/**
 * @brief Private implementation of the Detector
 */
struct Detector::Impl {
    cv::Ptr<cv::BackgroundSubtractorMOG2> back_sub;
    cv::HOGDescriptor hog;
    cv::dnn::Net dnn_net;
    Vector<String> dnn_class_names;
    float dnn_confidence_threshold;
    float dnn_nms_threshold;
    
    Impl(double threshold, const String& dnn_model_path, const String& dnn_names_path, float dnn_conf_thresh, float dnn_nms_thresh)
        : dnn_confidence_threshold(dnn_conf_thresh), dnn_nms_threshold(dnn_nms_thresh) {
        // Initialize MOG2 background subtractor
        back_sub = cv::createBackgroundSubtractorMOG2(500, threshold, true);
        // Initialize HOG descriptor with the default people detector
        hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());

        // Load DNN model if paths are provided
        if (!dnn_model_path.empty()) {
            dnn_net = cv::dnn::readNet(dnn_model_path); // YOLOv8 ONNX often doesn't need a separate config file
            dnn_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA); // Try CUDA if available
            dnn_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);   // Fallback to DNN_TARGET_CPU if CUDA fails

            // Load class names
            std::ifstream ifs(dnn_names_path.c_str());
            if (ifs.is_open()) {
                std::string line;
                while (std::getline(ifs, line)) dnn_class_names.push_back(line);
            } else {
                std::cerr << "Warning: DNN class names file not found: " << dnn_names_path << std::endl;
            }
        }
    }
};

Detector::Detector(double threshold, const String& dnn_model_path, const String& dnn_names_path, float dnn_conf_thresh, float dnn_nms_thresh)
    : m_pimpl(make_unique<Impl>(threshold, dnn_model_path, dnn_names_path, dnn_conf_thresh, dnn_nms_thresh)) {}

Detector::~Detector() = default;

auto Detector::detect(const geom::ImageData& image) -> Vector<geom::Detection> {
    if (image.data.empty()) {
        return {};
    }

    // Map the raw data to cv::Mat without copying (using const_cast for OpenCV compatibility)
    cv::Mat frame(image.height, image.width, CV_8UC3, const_cast<uint8_t*>(image.data.data()));
    
    cv::Mat fg_mask;
    m_pimpl->back_sub->apply(frame, fg_mask);

    // Post-processing to remove noise and bridge gaps
    cv::threshold(fg_mask, fg_mask, 200, 255, cv::THRESH_BINARY);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_OPEN, kernel);
    cv::dilate(fg_mask, fg_mask, kernel, cv::Point(-1, -1), 2);

    // Find object contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fg_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    Vector<geom::Detection> detections;
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

    // HOG for person detection (can be run on ROIs or full frame)
    // For now, let's run it on the full frame to get more robust person detections
    // and then combine with motion-based ROIs.
    std::vector<cv::Rect> hog_found_locations;
    std::vector<double> hog_weights;
    m_pimpl->hog.detectMultiScale(frame, hog_found_locations, hog_weights, 0, cv::Size(8, 8), cv::Size(), 1.05, 2, false);

    for (size_t i = 0; i < hog_found_locations.size(); ++i) {
        const cv::Rect& hog_rect = hog_found_locations[i];
        float confidence = static_cast<float>(hog_weights[i]);

        // Check for overlap with existing detections to avoid duplicates
        bool overlapped = false;
        for (const auto& det : detections) {
            geom::Rect existing_rect = det.bbox;
            cv::Rect cv_existing_rect(existing_rect.x, existing_rect.y, existing_rect.width, existing_rect.height);
            if ((hog_rect & cv_existing_rect).area() > 0.5 * hog_rect.area()) { // Significant overlap
                overlapped = true;
                break;
            }
        }

        if (!overlapped) {
            detections.emplace_back(
                geom::Rect{hog_rect.x, hog_rect.y, hog_rect.width, hog_rect.height},
                confidence,
                1, // Class ID for person
                "person"
            );
        }
    }

    // DNN Inference (YOLOv8)
    if (!m_pimpl->dnn_net.empty()) {
        cv::Mat blob;
        // YOLOv8 typically expects 640x640, 0-1 scaling, no mean subtraction, RGB swap=false (if input is RGB)
        // Assuming `frame` is RGB (as per image_loader.cpp converting to RGB)
        cv::dnn::blobFromImage(frame, blob, 1/255.0, cv::Size(640, 640), cv::Scalar(), false, false);
        m_pimpl->dnn_net.setInput(blob);

        std::vector<cv::Mat> outputs;
        m_pimpl->dnn_net.forward(outputs, m_pimpl->dnn_net.getUnconnectedOutLayersNames());

        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> class_ids;

        // Process YOLOv8 output. Assuming outputs[0] is [1, num_classes + 4, num_detections] (transposed)
        // Reshape to [num_detections, num_attributes]
        cv::Mat output_data = outputs[0].reshape(1, outputs[0].size[2]);
        
        for (int i = 0; i < output_data.rows; ++i) {
            float* data = (float*)output_data.row(i).data;
            cv::Mat scores = output_data.row(i).colRange(4, output_data.cols); // Class scores start from index 4
            cv::Point class_id_point;
            double max_class_score;
            cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id_point);
            
            if (max_class_score > m_pimpl->dnn_confidence_threshold) {
                float x_center = data[0], y_center = data[1], width = data[2], height = data[3];
                int left = static_cast<int>((x_center - width / 2) * frame.cols);
                int top = static_cast<int>((y_center - height / 2) * frame.rows);
                boxes.push_back(cv::Rect(left, top, static_cast<int>(width * frame.cols), static_cast<int>(height * frame.rows)));
                confidences.push_back(static_cast<float>(max_class_score));
                class_ids.push_back(class_id_point.x);
            }
        }

        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, m_pimpl->dnn_confidence_threshold, m_pimpl->dnn_nms_threshold, indices);

        for (int idx : indices) {
            cv::Rect box = boxes[idx];
            int class_id = class_ids[idx];
            float confidence = confidences[idx];
            String class_name = (class_id >= 0 && class_id < m_pimpl->dnn_class_names.size()) ? m_pimpl->dnn_class_names[class_id] : "unknown";

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

} // namespace perception::vision