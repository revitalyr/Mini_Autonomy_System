module;
#include <vector>
#include <string>
#include <memory>

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
    public:
        Detector() = default;
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
    };

    // Implementations
    std::vector<Detection> Detector::detect(const ImageData& frame) {
        (void)frame;
        // TODO: Implement actual detection logic
        return {};
    }

    const std::vector<std::string>& Detector::get_class_names() const {
        static const std::vector<std::string> class_names = {
            "person", "car", "bicycle", "generic"
        };
        return class_names;
    }

} // namespace perception
