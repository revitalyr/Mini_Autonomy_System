module;

#include <opencv2/video/tracking.hpp>
#include <algorithm>
#include <set>

module perception.tracking;

namespace perception::tracking {

/**
 * @brief Internal state for a single tracked object using cv::KalmanFilter
 */
struct TrackInternal {
    int id;
    cv::KalmanFilter kf;
    perception::String class_name;
    float confidence;
    int age{0};
    int missed_frames{0};

    TrackInternal(int track_id, const geom::Rect& initial_rect, const perception::String& label, float conf)
        : id(track_id), kf(8, 4, 0), class_name(label), confidence(conf) {
        
        // Constant Velocity Model: State is [x, y, w, h, vx, vy, vw, vh]
        kf.transitionMatrix = (cv::Mat_<float>(8, 8) << 
            1,0,0,0,1,0,0,0,
            0,1,0,0,0,1,0,0,
            0,0,1,0,0,0,1,0,
            0,0,0,1,0,0,0,1,
            0,0,0,0,1,0,0,0,
            0,0,0,0,0,1,0,0,
            0,0,0,0,0,0,1,0,
            0,0,0,0,0,0,0,1);

        cv::setIdentity(kf.measurementMatrix);
        cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(1e-4));
        cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-1));
        cv::setIdentity(kf.errorCovPost, cv::Scalar::all(0.1));

        kf.statePost.at<float>(0) = static_cast<float>(initial_rect.x);
        kf.statePost.at<float>(1) = static_cast<float>(initial_rect.y);
        kf.statePost.at<float>(2) = static_cast<float>(initial_rect.width);
        kf.statePost.at<float>(3) = static_cast<float>(initial_rect.height);
    }

    auto predict() -> geom::Rect {
        cv::Mat prediction = kf.predict();
        return geom::Rect{
            static_cast<int>(prediction.at<float>(0)),
            static_cast<int>(prediction.at<float>(1)),
            static_cast<int>(prediction.at<float>(2)),
            static_cast<int>(prediction.at<float>(3))
        };
    }

    void update(const geom::Rect& measurement) {
        cv::Mat measure_mat = (cv::Mat_<float>(4, 1) << 
            static_cast<float>(measurement.x),
            static_cast<float>(measurement.y),
            static_cast<float>(measurement.width),
            static_cast<float>(measurement.height));
        kf.correct(measure_mat);
        missed_frames = 0;
    }
};

struct Tracker::Impl {
    Vector<TrackInternal> tracks;
};

Tracker::Tracker(float iou_threshold, int max_age) 
    : m_pimpl(make_unique<Impl>()), m_iou_threshold(iou_threshold), m_max_age(max_age) {}

Tracker::~Tracker() = default;

auto Tracker::calculate_iou(const geom::Rect& a, const geom::Rect& b) -> float {
    int x1 = std::max(a.x, b.x);
    int y1 = std::max(a.y, b.y);
    int x2 = std::min(a.x + a.width, b.x + b.width);
    int y2 = std::min(a.y + a.height, b.y + b.height);

    if (x2 <= x1 || y2 <= y1) return 0.0f;

    float intersection = static_cast<float>((x2 - x1) * (y2 - y1));
    float area_a = static_cast<float>(a.width * a.height);
    float area_b = static_cast<float>(b.width * b.height);
    return intersection / (area_a + area_b - intersection);
}

auto Tracker::update(const Vector<geom::Detection>& detections) -> Vector<Track> {
    // 1. Prediction step
    for (auto& track : m_pimpl->tracks) {
        track.predict();
        track.age++;
        track.missed_frames++;
    }

    // 2. Data Association (Greedy IoU Matching)
    // This is a "greedy assignment with global view" which is more sophisticated than simple greedy
    // but not a full Munkres/Hungarian algorithm. It aims to find the best overall matches.
    // For a true Hungarian algorithm, a dedicated library or a more complex implementation would be needed.
    std::set<size_t> matched_detections_indices;
    std::set<Count> matched_tracks_indices;

    for (Count i = 0; i < m_pimpl->tracks.size(); ++i) {
        float max_iou = -1.0f;
        int best_det_idx = -1;

        // Get predicted bbox from Kalman filter
        cv::Mat state = m_pimpl->tracks[i].kf.statePre;
        geom::Rect predicted_rect{
            static_cast<int>(state.at<float>(0)), static_cast<int>(state.at<float>(1)),
            static_cast<int>(state.at<float>(2)), static_cast<int>(state.at<float>(3))
        };

        for (Count j = 0; j < detections.size(); ++j) {
            if (matched_detections_indices.contains(j)) continue;
            
            float iou = calculate_iou(predicted_rect, detections[j].bbox);
            if (iou > m_iou_threshold && iou > max_iou) {
                max_iou = iou;
                best_det_idx = static_cast<int>(j);
            }
        }
        
        if (best_det_idx != -1) {
            m_pimpl->tracks[i].update(detections[best_det_idx].bbox);
            m_pimpl->tracks[i].confidence = detections[best_det_idx].confidence;
            m_pimpl->tracks[i].class_name = detections[best_det_idx].class_name; // Update class name if it changes
            
            matched_tracks_indices.insert(i);
            matched_detections_indices.insert(best_det_idx);
        }
    }

    // 3. Initialize new tracks for unmatched detections
    for (Count i = 0; i < detections.size(); ++i) {
        if (!matched_detections_indices.contains(i)) {
            m_pimpl->tracks.emplace_back(m_next_id++, detections[i].bbox, detections[i].class_name, detections[i].confidence);
        }
    }

    // 4. Cleanup old tracks
    std::erase_if(m_pimpl->tracks, [this](const auto& t) {
        return t.missed_frames > m_max_age;
    });

    // 5. Convert to public Track structures
    Vector<Track> results;
    for (const auto& t : m_pimpl->tracks) {
        // Only return tracks that were either matched in this frame or are still active (not too many missed frames)
        if (t.missed_frames <= m_max_age) { 
            cv::Mat state = t.kf.statePost;
            results.push_back({
                t.id,
                geom::Rect{
                    static_cast<int>(state.at<float>(0)),
                    static_cast<int>(state.at<float>(1)),
                    static_cast<int>(state.at<float>(2)),
                    static_cast<int>(state.at<float>(3))
                },
                t.confidence,
                t.class_name,
                t.age,
                t.missed_frames
            });
        }
    }

    return results;
}

} // namespace perception::tracking