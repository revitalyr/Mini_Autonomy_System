module;

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

export module perception.detector;

import perception.concepts;
import perception.result;
import perception.metrics;

export namespace perception {

    // Detection result structure using OpenCV
    struct Detection {
        cv::Rect bbox;           // Bounding box
        float confidence;       // Confidence score
        int class_id;          // Class ID
        std::string class_name; // Class name
        
        Detection(cv::Rect b, float conf, int id, std::string name)
            : bbox(std::move(b)), confidence(conf), class_id(id), class_name(std::move(name)) {}
    };

    // Mock detector for testing
    class MockDetector {
    private:
        std::vector<std::string> m_class_names{"person", "car", "bicycle"};
        
    public:
        explicit MockDetector() = default;
        
        std::vector<Detection> detect(const cv::Mat& frame) {
            std::vector<Detection> detections;
            
            // Generate some fake detections for demo
            if (frame.rows > 100 && frame.cols > 100) {
                detections.emplace_back(
                    cv::Rect(50, 50, 100, 200), 0.8f, 0, "person"
                );
                detections.emplace_back(
                    cv::Rect(200, 100, 150, 80), 0.6f, 1, "car"
                );
            }
            
            return detections;
        }
        
        const std::vector<std::string>& get_class_names() const { return m_class_names; }
    };

} // namespace perception
