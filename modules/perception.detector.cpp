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
        std::vector<std::string> class_names{"person", "car", "bicycle"};
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

            // Generate some fake detections for demo
            if (cv_frame.rows > 100 && cv_frame.cols > 100) {
                detections.emplace_back(
                    Rect(50, 50, 100, 200), 0.8f, 0, "person"
                );
                detections.emplace_back(
                    Rect(200, 100, 150, 80), 0.6f, 1, "car"
                );
            }
        }

        return detections;
    }

    const std::vector<std::string>& MockDetector::get_class_names() const {
        return impl_->class_names;
    }

} // namespace perception
