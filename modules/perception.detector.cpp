/**
 * @file perception.detector.cpp
 * @brief Implementation of the motion-based object detector
 */
module;

#include <opencv2/opencv.hpp>
#include <algorithm>

module perception.detector;

namespace perception::vision {

/**
 * @brief Private implementation of the Detector
 */
struct Detector::Impl {
    cv::Ptr<cv::BackgroundSubtractorMOG2> back_sub;
    
    Impl(double threshold) {
        // Initialize MOG2 background subtractor
        back_sub = cv::createBackgroundSubtractorMOG2(500, threshold, true);
    }
};

Detector::Detector(double threshold) : m_pimpl(make_unique<Impl>(threshold)) {}

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
        const float aspect_ratio = static_cast<float>(rect.width) / rect.height;
        
        String class_name = "object";
        int class_id = 0;

        // Simple heuristic-based classification
        if (aspect_ratio < 0.6f) {
            class_name = "person";
            class_id = 1;
        } else if (aspect_ratio > 1.2f && area > 2000) {
            class_name = "car";
            class_id = 2;
        } else if (area > 1200) {
            class_name = "bicycle";
            class_id = 3;
        }

        detections.emplace_back(
            geom::Rect{rect.x, rect.y, rect.width, rect.height},
            0.85f, // Confidence estimate for motion-based detection
            class_id,
            class_name
        );
    }

    return detections;
}

} // namespace perception::vision