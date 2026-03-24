#pragma once

#include "detector.hpp"
#include <map>
#include <vector>

struct Track {
    int id;              // Unique track ID
    cv::Rect bbox;       // Current bounding box
    float vx, vy;        // Velocity in x and y directions (pixels/frame)
    int age;             // Number of frames since track creation
    int consecutive_misses; // Frames without detection
    float confidence;    // Track confidence score
    
    Track() : id(-1), bbox(0, 0, 0, 0), vx(0), vy(0), age(0), consecutive_misses(0), confidence(0.0f) {}
    
    Track(int track_id, const cv::Rect& box, float vel_x = 0.0f, float vel_y = 0.0f)
        : id(track_id), bbox(box), vx(vel_x), vy(vel_y), age(1), consecutive_misses(0), confidence(1.0f) {}
};

class Tracker {
private:
    std::map<int, Track> tracks_;
    int next_track_id_;
    int max_age_;                    // Maximum track age before deletion
    int max_consecutive_misses_;     // Maximum missed frames before deletion
    float min_confidence_;           // Minimum track confidence
    
    // Kalman filter parameters (simplified)
    float process_noise_;            // Process noise covariance
    float measurement_noise_;        // Measurement noise covariance
    float error_covariance_;         // Error covariance
    
public:
    Tracker(int max_age = 30, int max_consecutive_misses = 10, float min_confidence = 0.3f);
    
    std::vector<Track> update(const std::vector<Detection>& detections);
    
    // Simple SORT-like tracking
    std::vector<Track> simple_sort(const std::vector<Detection>& detections);
    
    // Kalman filter prediction
    cv::Rect predict_bbox(const Track& track);
    
    // Hungarian algorithm for assignment (simplified greedy approach)
    std::vector<std::pair<int, int>> assign_detections_to_tracks(
        const std::vector<Detection>& detections,
        const std::vector<Track>& tracks);
    
    // Calculate IoU (Intersection over Union)
    float calculate_iou(const cv::Rect& box1, const cv::Rect& box2);
    
    // Get track statistics
    size_t get_active_track_count() const { return tracks_.size(); }
    std::vector<Track> get_all_tracks() const;
    
    void clear_all_tracks();
    
private:
    void update_track_with_detection(Track& track, const Detection& detection);
    void predict_track(Track& track);
    void remove_old_tracks();
};
