/**
 * @file perception.tracking.ixx
 * @brief Object tracking algorithms
 */

export module perception.tracking;
import perception.types;
import perception.geom;

/**
 * @brief Temporal object tracking and state estimation
 */
export namespace perception::tracking {

    /**
     * @brief Represents a tracked object across multiple frames
     */
    struct Track {
        int id;                     // Unique tracking ID
        geom::Rect bbox;            // Current estimated bounding box
        float confidence;           // Detection confidence
        perception::String class_name; // Object classification
        int age;                    // Total number of frames since creation
        int missed_frames;          // Consecutive frames without a detection match
    };

    /**
     * @brief Multi-object tracker using Kalman Filters and IoU association
     */
    class Tracker {
    public:
        /**
         * @brief Construct a new Tracker
         * @param iou_threshold Minimum IoU to consider a match (0.0 to 1.0)
         * @param max_age Maximum consecutive missed frames before deleting a track
         */
        explicit Tracker(float iou_threshold = 0.3f, int max_age = 10);
        
        /**
         * @brief Destroy the Tracker object
         */
        ~Tracker();

        /**
         * @brief Update tracks with new detections from the current frame
         * 
         * @param detections New detections from the Detector
         * @return Vector of active, confirmed tracks
         */
        auto update(const Vector<geom::Detection>& detections) -> Vector<Track>;

    private:
        struct Impl;
        UniquePtr<Impl> m_pimpl;
        
        float m_iou_threshold;
        int m_max_age;
        int m_next_id{1};

        static auto calculate_iou(const geom::Rect& a, const geom::Rect& b) -> float;
    };

} // namespace perception::tracking