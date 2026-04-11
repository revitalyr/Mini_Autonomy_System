module;

#include <vector>
#include <string>
#include <memory>

export module perception.detector;

import perception.types;
import perception.concepts;
import perception.result;
import perception.metrics;

export namespace perception {

    // Detection result structure using only std types
    struct Detection {
        Rect bbox;              // Bounding box (from perception.types)
        float confidence;      // Confidence score
        int class_id;          // Class ID
        std::string class_name; // Class name

        Detection(Rect b, float conf, int id, std::string name)
            : bbox(std::move(b)), confidence(conf), class_id(id), class_name(std::move(name)) {}
    };

    // Helper function for filtering detections by confidence
    export auto filter_by_confidence(float threshold) {
        return [threshold](const Detection& det) {
            return det.confidence >= threshold;
        };
    }

    // Mock detector for testing
    class MockDetector {
    private:
        struct Impl; // PIMPL - hides OpenCV implementation
        std::unique_ptr<Impl> impl_;

    public:
        explicit MockDetector();
        ~MockDetector();

        // Use ImageData (from perception.types) instead of cv::Mat
        std::vector<Detection> detect(const ImageData& frame);

        const std::vector<std::string>& get_class_names() const;
    };

} // namespace perception
