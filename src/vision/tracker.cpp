#include "tracker.hpp"
#include <algorithm>
#include <cmath>

Tracker::Tracker(int max_age, int max_consecutive_misses, float min_confidence)
    : next_track_id_(0),
      max_age_(max_age),
      max_consecutive_misses_(max_consecutive_misses),
      min_confidence_(min_confidence),
      process_noise_(0.1f),
      measurement_noise_(0.1f),
      error_covariance_(1.0f) {
}

std::vector<Track> Tracker::update(const std::vector<Detection>& detections) {
    return simple_sort(detections);
}

std::vector<Track> Tracker::simple_sort(const std::vector<Detection>& detections) {
    std::vector<Track> active_tracks;
    
    // Predict existing tracks
    for (auto& pair : tracks_) {
        predict_track(pair.second);
    }
    
    // Assign detections to tracks
    std::vector<Track> track_list;
    for (const auto& pair : tracks_) {
        track_list.push_back(pair.second);
    }
    
    auto assignments = assign_detections_to_tracks(detections, track_list);
    
    // Update assigned tracks
    std::vector<bool> detection_used(detections.size(), false);
    std::vector<bool> track_updated(track_list.size(), false);
    
    for (const auto& assignment : assignments) {
        int detection_idx = assignment.first;
        int track_idx = assignment.second;
        
        if (detection_idx >= 0 && detection_idx < detections.size() &&
            track_idx >= 0 && track_idx < track_list.size()) {
            
            update_track_with_detection(tracks_[track_list[track_idx].id], detections[detection_idx]);
            detection_used[detection_idx] = true;
            track_updated[track_idx] = true;
        }
    }
    
    // Create new tracks for unassigned detections
    for (size_t i = 0; i < detections.size(); ++i) {
        if (!detection_used[i] && detections[i].confidence >= min_confidence_) {
            Track new_track(next_track_id_++, detections[i].bbox);
            tracks_[new_track.id] = new_track;
        }
    }
    
    // Mark unassigned tracks as missed
    for (size_t i = 0; i < track_list.size(); ++i) {
        if (!track_updated[i]) {
            int track_id = track_list[i].id;
            tracks_[track_id].consecutive_misses++;
        }
    }
    
    // Remove old tracks
    remove_old_tracks();
    
    // Return active tracks
    for (const auto& pair : tracks_) {
        if (pair.second.consecutive_misses < max_consecutive_misses_) {
            active_tracks.push_back(pair.second);
        }
    }
    
    return active_tracks;
}

cv::Rect Tracker::predict_bbox(const Track& track) {
    cv::Rect predicted = track.bbox;
    predicted.x += static_cast<int>(track.vx);
    predicted.y += static_cast<int>(track.vy);
    return predicted;
}

std::vector<std::pair<int, int>> Tracker::assign_detections_to_tracks(
    const std::vector<Detection>& detections,
    const std::vector<Track>& tracks) {
    
    std::vector<std::pair<int, int>> assignments;
    std::vector<bool> detection_assigned(detections.size(), false);
    std::vector<bool> track_assigned(tracks.size(), false);
    
    // Greedy assignment based on IoU
    while (true) {
        float best_iou = 0.3f; // Minimum IoU threshold
        int best_detection = -1;
        int best_track = -1;
        
        for (size_t i = 0; i < detections.size(); ++i) {
            if (detection_assigned[i]) continue;
            
            for (size_t j = 0; j < tracks.size(); ++j) {
                if (track_assigned[j]) continue;
                
                float iou = calculate_iou(detections[i].bbox, tracks[j].bbox);
                if (iou > best_iou) {
                    best_iou = iou;
                    best_detection = i;
                    best_track = j;
                }
            }
        }
        
        if (best_detection == -1 || best_track == -1) {
            break; // No more valid assignments
        }
        
        assignments.emplace_back(best_detection, best_track);
        detection_assigned[best_detection] = true;
        track_assigned[best_track] = true;
    }
    
    return assignments;
}

float Tracker::calculate_iou(const cv::Rect& box1, const cv::Rect& box2) {
    int x1 = std::max(box1.x, box2.x);
    int y1 = std::max(box1.y, box2.y);
    int x2 = std::min(box1.x + box1.width, box2.x + box2.width);
    int y2 = std::min(box1.y + box1.height, box2.y + box2.height);
    
    if (x2 <= x1 || y2 <= y1) {
        return 0.0f;
    }
    
    int intersection = (x2 - x1) * (y2 - y1);
    int area1 = box1.width * box1.height;
    int area2 = box2.width * box2.height;
    int union_area = area1 + area2 - intersection;
    
    return static_cast<float>(intersection) / static_cast<float>(union_area);
}

std::vector<Track> Tracker::get_all_tracks() const {
    std::vector<Track> all_tracks;
    for (const auto& pair : tracks_) {
        all_tracks.push_back(pair.second);
    }
    return all_tracks;
}

void Tracker::clear_all_tracks() {
    tracks_.clear();
    next_track_id_ = 0;
}

void Tracker::update_track_with_detection(Track& track, const Detection& detection) {
    // Update velocity (simple exponential smoothing)
    float alpha = 0.3f;
    track.vx = alpha * (detection.bbox.x - track.bbox.x) + (1.0f - alpha) * track.vx;
    track.vy = alpha * (detection.bbox.y - track.bbox.y) + (1.0f - alpha) * track.vy;
    
    // Update bounding box
    track.bbox = detection.bbox;
    
    // Update confidence (exponential smoothing with detection confidence)
    track.confidence = alpha * detection.confidence + (1.0f - alpha) * track.confidence;
    
    // Reset consecutive misses
    track.consecutive_misses = 0;
    
    // Increment age
    track.age++;
}

void Tracker::predict_track(Track& track) {
    // Simple linear prediction
    track.bbox.x += static_cast<int>(track.vx);
    track.bbox.y += static_cast<int>(track.vy);
    
    // Decay confidence slightly
    track.confidence *= 0.99f;
    
    // Increment age
    track.age++;
}

void Tracker::remove_old_tracks() {
    auto it = tracks_.begin();
    while (it != tracks_.end()) {
        const Track& track = it->second;
        
        bool should_remove = false;
        
        // Remove if too old
        if (track.age > max_age_) {
            should_remove = true;
        }
        
        // Remove if too many consecutive misses
        if (track.consecutive_misses > max_consecutive_misses_) {
            should_remove = true;
        }
        
        // Remove if confidence too low
        if (track.confidence < min_confidence_) {
            should_remove = true;
        }
        
        if (should_remove) {
            it = tracks_.erase(it);
        } else {
            ++it;
        }
    }
}
