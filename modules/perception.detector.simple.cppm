module;

#include <vector>
#include <string>
#include <memory>

export module perception.detector;

import perception.concepts;
import perception.result;
import perception.metrics;

export namespace perception {

    // Simple detection result without OpenCV
    struct Detection {
        int x, y, width, height;
        float confidence;
        int class_id;
        std::string class_name;
        
        Detection() = default;
        Detection(int x_, int y_, int w, int h, float conf, int cls_id, std::string cls_name = "")
            : x(x_), y(y_), width(w), height(h), confidence(conf), class_id(cls_id), 
              class_name(std::move(cls_name)) {}
    };

    // Mock detector for testing
    class MockDetector {
    private:
        std::vector<Detection> mock_detections_;
        
    public:
        MockDetector() {
            // Create mock detections
            mock_detections_.emplace_back(100, 100, 50, 50, 0.9f, 0, "person");
            mock_detections_.emplace_back(200, 200, 60, 60, 0.8f, 1, "car");
        }
        
        std::vector<Detection> detect() const {
            // Simulate detection processing
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return mock_detections_;
        }
    };
}
