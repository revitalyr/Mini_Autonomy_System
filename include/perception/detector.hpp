#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <chrono>

namespace perception {

    // Detection result structure
    struct Detection {
        cv::Rect bbox;
        float confidence;
        int class_id;
        std::string class_name;
        
        Detection() = default;
        Detection(cv::Rect box, float conf, int cls_id, std::string cls_name = "")
            : bbox(std::move(box)), confidence(conf), class_id(cls_id), 
              class_name(std::move(cls_name)) {}
    };

    // Mock detector for testing
    class MockDetector {
    private:
        cv::Mat test_frame_;
        std::vector<Detection> mock_detections_;
        
    public:
        MockDetector() {
            // Create test frame
            test_frame_ = cv::Mat::zeros(480, 640, CV_8UC3);
            cv::rectangle(test_frame_, cv::Rect(100, 100, 80, 80), cv::Scalar(255, 255, 255), -1);
            
            // Create mock detections
            mock_detections_.emplace_back(cv::Rect(100, 100, 50, 50), 0.9f, 0, "person");
            mock_detections_.emplace_back(cv::Rect(200, 200, 60, 60), 0.8f, 1, "car");
        }
        
        std::vector<Detection> detect(const cv::Mat& frame) const {
            // Simulate detection processing
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            if (frame.size() == test_frame_.size()) {
                return mock_detections_;
            }
            
            return {};
        }
    };

}
