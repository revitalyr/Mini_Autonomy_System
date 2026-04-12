module;
#include <vector>
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>

export module perception.detector;
import perception.types;
import perception.concepts;
import perception.result;
import perception.metrics;

namespace perception {

    /**
     * Filter detections by minimum confidence threshold
     * @param threshold Minimum confidence required (0.0 to 1.0)
     * @return Predicate function for filtering
     */
    export inline auto filter_by_confidence(float threshold) {
        return [threshold](const Detection& det) {
            return det.confidence >= threshold;
        };
    }

    /**
     * Motion-based object detector using background subtraction
     * Detects moving objects in image sequences and classifies them
     * into person, car, bicycle, or generic object categories
     */
    export class Detector {
    private:
        cv::Ptr<cv::BackgroundSubtractorMOG2> bg_subtractor_;
        cv::Mat background_frame_;
        int frame_count_;

    public:
        Detector();
        ~Detector() = default;

        /**
         * Detect objects in an image frame
         * @param frame Image data to analyze
         * @return List of detected objects with bounding boxes and classifications
         */
        std::vector<Detection> detect(const ImageData& frame);

        /**
         * Get supported class names
         * @return List of class names the detector can identify
         */
        const std::vector<std::string>& get_class_names() const;

    private:
        int get_class_id(const std::string& class_name);
        std::string classify_object(const cv::Rect& bbox, double area);
        float calculate_confidence(double area, const cv::Rect& bbox);
    };

    // Implementations
    Detector::Detector()
        : bg_subtractor_(cv::createBackgroundSubtractorMOG2(500, 16, true))
        , frame_count_(0) {}

    std::vector<Detection> Detector::detect(const ImageData& frame) {
        std::vector<Detection> detections;

        if (frame.data.empty() || frame.width == 0 || frame.height == 0) {
            return detections;
        }

        // Convert ImageData to cv::Mat
        cv::Mat img(frame.height, frame.width, CV_8UC3, const_cast<unsigned char*>(frame.data.data()));

        // Apply background subtraction
        cv::Mat fg_mask;
        bg_subtractor_->apply(img, fg_mask);

        // Morphological operations to clean up the mask
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_CLOSE, kernel);

        // Find contours
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(fg_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // Process each contour
        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < 500) { // Filter small noise
                continue;
            }

            cv::Rect bbox = cv::boundingRect(contour);

            // Classify object based on size and aspect ratio
            std::string class_name = classify_object(bbox, area);
            float confidence = calculate_confidence(area, bbox);

            detections.emplace_back(
                Rect(bbox.x, bbox.y, bbox.width, bbox.height),
                confidence,
                get_class_id(class_name),
                class_name
            );
        }

        frame_count_++;
        return detections;
    }

    int Detector::get_class_id(const std::string& class_name) {
        const auto& names = get_class_names();
        auto it = std::find(names.begin(), names.end(), class_name);
        if (it != names.end()) {
            return static_cast<int>(std::distance(names.begin(), it));
        }
        return 3; // generic
    }

    std::string Detector::classify_object(const cv::Rect& bbox, double area) {
        float aspect_ratio = static_cast<float>(bbox.width) / bbox.height;

        // Simple heuristic classification
        if (aspect_ratio > 0.2f && aspect_ratio < 0.5f && area > 5000) {
            return "person";
        } else if (aspect_ratio > 0.8f && aspect_ratio < 2.5f && area > 8000) {
            return "car";
        } else if (aspect_ratio > 0.6f && aspect_ratio < 1.5f && area > 2000 && area < 5000) {
            return "bicycle";
        }
        return "generic";
    }

    float Detector::calculate_confidence(double area, const cv::Rect& bbox) {
        (void)bbox;
        // Confidence based on area and bounding box quality
        float base_confidence = 0.7f;
        if (area > 10000) base_confidence += 0.1f;
        if (area < 1000) base_confidence -= 0.2f;
        return std::clamp(base_confidence, 0.0f, 1.0f);
    }

    const std::vector<std::string>& Detector::get_class_names() const {
        static const std::vector<std::string> class_names = {
            "person", "car", "bicycle", "generic"
        };
        return class_names;
    }

} // namespace perception
