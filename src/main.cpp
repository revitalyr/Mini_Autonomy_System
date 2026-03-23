#include "core/pipeline.hpp"
#include "core/metrics.hpp"
#include "io/gstreamer_capture.hpp"
#include "io/imu_simulator.hpp"
#include "vision/detector.hpp"
#include "vision/tracker.hpp"
#include "vision/fusion.hpp"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <map>
#include <chrono>
#include <string>

class CaptureStage : public PipelineStage {
private:
    VideoCaptureGStreamer& capture_;
    ThreadSafeQueue<cv::Mat>& frame_queue_;
    std::atomic<bool>& running_;
    
public:
    CaptureStage(VideoCaptureGStreamer& capture, 
                ThreadSafeQueue<cv::Mat>& frame_queue,
                std::atomic<bool>& running)
        : capture_(capture), frame_queue_(frame_queue), running_(running) {}
    
    void run() override {
        cv::Mat frame;
        while (running_) {
            if (capture_.read(frame)) {
                frame_queue_.push(frame.clone());
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        frame_queue_.shutdown();
    }
    
    void stop() override {
        running_ = false;
    }
};

class DetectionStage : public PipelineStage {
private:
    Detector& detector_;
    ThreadSafeQueue<cv::Mat>& frame_queue_;
    ThreadSafeQueue<std::vector<Detection>>& detection_queue_;
    Metrics& metrics_;
    std::atomic<bool>& running_;
    
public:
    DetectionStage(Detector& detector,
                  ThreadSafeQueue<cv::Mat>& frame_queue,
                  ThreadSafeQueue<std::vector<Detection>>& detection_queue,
                  Metrics& metrics,
                  std::atomic<bool>& running)
        : detector_(detector), frame_queue_(frame_queue), detection_queue_(detection_queue),
          metrics_(metrics), running_(running) {}
    
    void run() override {
        cv::Mat frame;
        while (running_) {
            if (frame_queue_.pop(frame, std::chrono::milliseconds(100))) {
                metrics_.frame_start();
                
                auto detections = detector_.detect(frame);
                detection_queue_.push(detections);
                
                // Draw bounding boxes
                for (const auto& detection : detections) {
                    cv::rectangle(frame, detection.bbox, cv::Scalar(0, 255, 0), 2);
                    cv::putText(frame, 
                               detection.class_name + " " + std::to_string(static_cast<int>(detection.confidence * 100)) + "%",
                               detection.bbox.tl(), 
                               cv::FONT_HERSHEY_SIMPLEX, 
                               0.5, 
                               cv::Scalar(0, 255, 0), 
                               1);
                }
                
                // FPS overlay
                float fps = metrics_.get_fps();
                cv::putText(frame, "FPS: " + std::to_string(static_cast<int>(fps)),
                           cv::Point(10, 30), 
                           cv::FONT_HERSHEY_SIMPLEX, 
                           1.0, 
                           cv::Scalar(0, 0, 255), 
                           2);
                
                // Latency overlay
                float latency = metrics_.get_last_latency();
                cv::putText(frame, "Latency: " + std::to_string(static_cast<int>(latency)) + "ms",
                           cv::Point(10, 60), 
                           cv::FONT_HERSHEY_SIMPLEX, 
                           1.0, 
                           cv::Scalar(0, 0, 255), 
                           2);
                
                cv::imshow("Detection", frame);
                if (cv::waitKey(1) == 27) { // ESC key
                    running_ = false;
                    break;
                }
                
                metrics_.frame_end();
            }
        }
        detection_queue_.shutdown();
    }
    
    void stop() override {
        running_ = false;
    }
};

class TrackingStage : public PipelineStage {
private:
    Tracker& tracker_;
    ThreadSafeQueue<std::vector<Detection>>& detection_queue_;
    ThreadSafeQueue<std::vector<Track>>& track_queue_;
    std::atomic<bool>& running_;
    
public:
    TrackingStage(Tracker& tracker,
                 ThreadSafeQueue<std::vector<Detection>>& detection_queue,
                 ThreadSafeQueue<std::vector<Track>>& track_queue,
                 std::atomic<bool>& running)
        : tracker_(tracker), detection_queue_(detection_queue), track_queue_(track_queue),
          running_(running) {}
    
    void run() override {
        std::vector<Detection> detections;
        while (running_) {
            if (detection_queue_.pop(detections, std::chrono::milliseconds(100))) {
                auto tracks = tracker_.update(detections);
                track_queue_.push(tracks);
            }
        }
        track_queue_.shutdown();
    }
    
    void stop() override {
        running_ = false;
    }
};

class FusionStage : public PipelineStage {
private:
    Fusion& fusion_;
    IMUSimulator& imu_;
    ThreadSafeQueue<std::vector<Track>>& track_queue_;
    std::atomic<bool>& running_;
    std::map<int, std::vector<cv::Point>>& trajectories_;
    
public:
    FusionStage(Fusion& fusion,
               IMUSimulator& imu,
               ThreadSafeQueue<std::vector<Track>>& track_queue,
               std::atomic<bool>& running,
               std::map<int, std::vector<cv::Point>>& trajectories)
        : fusion_(fusion), imu_(imu), track_queue_(track_queue), running_(running),
          trajectories_(trajectories) {}
    
    void run() override {
        std::vector<Track> tracks;
        while (running_) {
            if (track_queue_.pop(tracks, std::chrono::milliseconds(100))) {
                auto pose = fusion_.update(tracks, imu_.get_data());
                
                // Update trajectories
                for (const auto& track : tracks) {
                    cv::Point center(track.bbox.x + track.bbox.width / 2,
                                    track.bbox.y + track.bbox.height / 2);
                    trajectories_[track.id].push_back(center);
                    
                    // Limit trajectory length
                    if (trajectories_[track.id].size() > 100) {
                        trajectories_[track.id].erase(trajectories_[track.id].begin());
                    }
                }
                
                // Draw trajectories
                cv::Mat trajectory_frame = cv::Mat::zeros(480, 640, CV_8UC3);
                for (const auto& [id, points] : trajectories_) {
                    if (points.size() > 1) {
                        for (size_t i = 1; i < points.size(); ++i) {
                            cv::line(trajectory_frame, points[i-1], points[i], 
                                   cv::Scalar(255, 255, 0), 2);
                        }
                        cv::circle(trajectory_frame, points.back(), 3, 
                                  cv::Scalar(0, 0, 255), -1);
                        cv::putText(trajectory_frame, std::to_string(id), points.back(),
                                  cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                                  cv::Scalar(255, 255, 255), 1);
                    }
                }
                
                // Pose information overlay
                cv::putText(trajectory_frame, 
                           "Pose: X=" + std::to_string(static_cast<int>(pose.x * 100)) + 
                           "cm Y=" + std::to_string(static_cast<int>(pose.y * 100)) + "cm",
                           cv::Point(10, 30), 
                           cv::FONT_HERSHEY_SIMPLEX, 
                           0.7, 
                           cv::Scalar(255, 255, 255), 
                           2);
                
                cv::putText(trajectory_frame, 
                           "Active Tracks: " + std::to_string(tracks.size()),
                           cv::Point(10, 60), 
                           cv::FONT_HERSHEY_SIMPLEX, 
                           0.7, 
                           cv::Scalar(255, 255, 255), 
                           2);
                
                cv::imshow("Trajectories", trajectory_frame);
                if (cv::waitKey(1) == 27) { // ESC key
                    running_ = false;
                    break;
                }
                
                // Print pose to console
                std::cout << "\rPose: X=" << std::fixed << std::setprecision(2) << pose.x 
                         << " Y=" << pose.y << " Z=" << pose.z 
                         << " | Tracks: " << tracks.size() << std::flush;
            }
        }
    }
    
    void stop() override {
        running_ = false;
    }
};

int main(int argc, char* argv[]) {
    std::string video_source = "demo_video.mp4";
    if (argc > 1) {
        video_source = argv[1];
    }
    
    std::cout << "=== Mini Autonomy System Demo ===" << std::endl;
    std::cout << "Video source: " << video_source << std::endl;
    std::cout << "Press ESC to exit" << std::endl;
    std::cout << "=================================" << std::endl;
    
    try {
        // Initialize components
        VideoCaptureGStreamer capture(video_source, 640, 480, 30);
        if (!capture.open()) {
            std::cerr << "Error: Cannot open video source: " << video_source << std::endl;
            return -1;
        }
        
        IMUSimulator imu(0.01f, 0.5f, 0.1f);
        Detector detector("", 0.5f, 0.4f, 640, 640);
        Tracker tracker(30, 10, 0.3f);
        Fusion fusion(0.01f, 0.1f, 0.01f, 0.1f, 0.5f, 0.1f);
        Metrics metrics;
        
        // Create threaded pipeline
        ThreadedPipeline pipeline;
        std::atomic<bool> running{true};
        
        // Create queues
        ThreadSafeQueue<cv::Mat> frame_queue;
        ThreadSafeQueue<std::vector<Detection>> detection_queue;
        ThreadSafeQueue<std::vector<Track>> track_queue;
        
        // Track trajectories
        std::map<int, std::vector<cv::Point>> trajectories;
        
        // Add pipeline stages
        pipeline.add_stage(std::make_unique<CaptureStage>(capture, frame_queue, running));
        pipeline.add_stage(std::make_unique<DetectionStage>(detector, frame_queue, detection_queue, metrics, running));
        pipeline.add_stage(std::make_unique<TrackingStage>(tracker, detection_queue, track_queue, running));
        pipeline.add_stage(std::make_unique<FusionStage>(fusion, imu, track_queue, running, trajectories));
        
        // Start pipeline
        pipeline.start();
        
        std::cout << "Pipeline started. Running demo..." << std::endl;
        
        // Wait for pipeline to finish or user to exit
        while (pipeline.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Check if all windows are closed
            if (cv::waitKey(1) == 27) {
                running = false;
                break;
            }
        }
        
        // Stop pipeline
        pipeline.stop();
        
        std::cout << std::endl << "Pipeline stopped." << std::endl;
        metrics.print_stats();
        
        // Clean up
        cv::destroyAllWindows();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
