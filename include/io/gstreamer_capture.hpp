#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>

class VideoCaptureGStreamer {
private:
    cv::VideoCapture cap_;
    std::string source_;
    int width_;
    int height_;
    int fps_;
    
public:
    VideoCaptureGStreamer(const std::string& source, int width = 640, int height = 480, int fps = 30);
    ~VideoCaptureGStreamer() = default;
    
    bool open();
    bool read(cv::Mat& frame);
    void release();
    bool is_opened() const;
    
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    int get_fps() const { return fps_; }
    
    // Get backend-specific properties
    double get_property(int prop_id) const;
    bool set_property(int prop_id, double value);
    
private:
    std::string build_gstreamer_pipeline() const;
};
