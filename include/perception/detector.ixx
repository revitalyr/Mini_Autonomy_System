module;

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <memory>

export module perception.detector;

import perception.concepts;
import perception.result;
import perception.metrics;
import perception.async;

export namespace perception {

    // Modern detection result with structured bindings support
    struct Detection {
        cv::Rect bbox;
        float confidence;
        int class_id;
        std::string class_name;
        
        // C++20: Default comparisons
        auto operator<=>(const Detection& other) const noexcept = default;
        
        // C++20: Structured bindings support
        template<std::size_t N>
        decltype(auto) get() const noexcept {
            if constexpr (N == 0) return bbox;
            else if constexpr (N == 1) return confidence;
            else if constexpr (N == 2) return class_id;
            else if constexpr (N == 3) return class_name;
        }
        
        // C++23: Deduction guides
        Detection() = default;
        Detection(cv::Rect box, float conf, int cls_id, std::string cls_name = "")
            : bbox(std::move(box)), confidence(conf), class_id(cls_id), 
              class_name(std::move(cls_name)) {}
    };

    // Enable structured bindings
}

namespace std {
    template<>
    struct tuple_size<perception::Detection> : std::integral_constant<std::size_t, 4> {};
    
    template<std::size_t N>
    struct tuple_element<N, perception::Detection> {
        using type = decltype(std::declval<perception::Detection>().get<N>());
    };
}

export namespace perception {

    // Abstract detector interface with concepts
    template<typename T>
    concept ObjectDetector = Detector<T>;

    // Modern detector factory with concepts
    template<ObjectDetector Detector>
    class DetectorFactory {
    public:
        template<typename... Args>
        static std::unique_ptr<Detector> create(Args&&... args) {
            return std::make_unique<Detector>(std::forward<Args>(args)...);
        }
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
