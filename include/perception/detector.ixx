export module perception.detector;
import perception.types;
import perception.geom;

/**
 * @brief Computer vision algorithms and object detection
 */
export namespace perception::vision {

    /**
     * @brief Motion-based object detector using background subtraction
     * 
     * Uses the PIMPL pattern to keep OpenCV types out of the module interface,
     * ensuring faster compilation and better encapsulation.
     */
    class Detector {
    public:
        /**
         * @brief Construct a new Detector object
         * @param threshold Sensitivity threshold for background subtraction
         */
        explicit Detector(double threshold = 16.0);

        /**
         * @brief Destroy the Detector object
         */
        ~Detector();

        /**
         * @brief Detect moving objects in the given image
         * 
         * @param image Input image data from camera or ROSBAG
         * @return Vector of detections containing bounding boxes and classification
         */
        auto detect(const geom::ImageData& image) -> Vector<geom::Detection>;

    private:
        struct Impl;
        UniquePtr<Impl> m_pimpl;
    };

} // namespace perception::vision