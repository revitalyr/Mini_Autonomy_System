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

    /**
     * Detection result containing bounding box, confidence, and class information
     */
    struct Detection {
        Rect bbox;              // Bounding box coordinates
        float confidence;      // Detection confidence (0.0 to 1.0)
        int class_id;          // Class identifier
        std::string class_name; // Human-readable class name

        Detection(Rect b, float conf, int id, std::string name)
            : bbox(std::move(b)), confidence(conf), class_id(id), class_name(std::move(name)) {}
    };

    /**
     * Filter detections by minimum confidence threshold
     * @param threshold Minimum confidence required (0.0 to 1.0)
     * @return Predicate function for filtering
     */
    export auto filter_by_confidence(float threshold) {
        return [threshold](const Detection& det) {
            return det.confidence >= threshold;
        };
    }

    /**
     * Motion-based object detector using background subtraction
     * Detects moving objects in image sequences and classifies them
     * into person, car, bicycle, or generic object categories
     */
    class Detector {
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

    public:
        explicit Detector();
        ~Detector();

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

} // namespace perception
