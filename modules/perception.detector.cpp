module;
// GMF - OpenCV isolated here
#include <opencv2/opencv.hpp>

module perception.detector;

import perception.types;
import perception.concepts;
import perception.result;
import perception.metrics;

#include <vector>
#include <string>
#include <memory>

namespace perception {

    // PIMPL implementation - knows about OpenCV
    struct MockDetector::Impl {
        std::vector<std::string> class_names{"person", "car", "bicycle", "object"};
        cv::Ptr<cv::BackgroundSubtractorMOG2> bg_subtractor;
        cv::Mat prev_frame;

        Impl() : bg_subtractor(cv::createBackgroundSubtractorMOG2(500, 16, true)) {}
    };

    // --- Constructor/Destructor ---

    MockDetector::MockDetector()
        : impl_(std::make_unique<Impl>()) {}

    MockDetector::~MockDetector() = default;

    // --- Detection ---

    std::vector<Detection> MockDetector::detect(const ImageData& frame) {
        std::vector<Detection> detections;

        // Convert ImageData to cv::Mat for processing
        if (!frame.empty()) {
            cv::Mat cv_frame(frame.height, frame.width,
                           frame.channels == 3 ? CV_8UC3 : CV_8UC1,
                           const_cast<uint8_t*>(frame.data.data()));

            // Convert to grayscale for processing
            cv::Mat gray;
            if (frame.channels == 3) {
                cv::cvtColor(cv_frame, gray, cv::COLOR_BGR2GRAY);
            } else {
                gray = cv_frame.clone();
            }

            // Apply Gaussian blur to reduce noise
            cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);

            // Use background subtraction for motion detection
            cv::Mat fg_mask;
            impl_->bg_subtractor->apply(gray, fg_mask);

            // Apply morphological operations to clean up the mask
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
            cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_OPEN, kernel);
            cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_CLOSE, kernel);

            // Find contours in the foreground mask
            std::vector<std::vector<cv::Point>> contours;
            std::vector<cv::Vec4i> hierarchy;
            cv::findContours(fg_mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            // Process contours to generate detections
            for (const auto& contour : contours) {
                double area = cv::contourArea(contour);

                // Filter out small contours (noise)
                if (area < 500) continue;

                // Get bounding rectangle
                cv::Rect bbox = cv::boundingRect(contour);

                // Calculate confidence based on area and shape
                float confidence = std::min(0.95f, static_cast<float>(area) / 10000.0f);

                // Determine class based on aspect ratio and size
                int class_id = 3; // Default: "object"
                std::string class_name = "object";

                double aspect_ratio = static_cast<double>(bbox.width) / bbox.height;

                if (aspect_ratio > 0.3 && aspect_ratio < 0.7 && bbox.height > 100) {
                    class_id = 0; // "person"
                    class_name = "person";
                } else if (aspect_ratio > 1.2 && aspect_ratio < 2.0 && bbox.width > 150) {
                    class_id = 1; // "car"
                    class_name = "car";
                } else if (aspect_ratio > 2.0 && bbox.width > 200) {
                    class_id = 2; // "bicycle"
                    class_name = "bicycle";
                }

                // Add detection
                detections.emplace_back(
                    Rect(bbox.x, bbox.y, bbox.width, bbox.height),
                    confidence,
                    class_id,
                    class_name
                );
            }
        }

        return detections;
    }

    const std::vector<std::string>& MockDetector::get_class_names() const {
        return impl_->class_names;
    }

} // namespace perception
