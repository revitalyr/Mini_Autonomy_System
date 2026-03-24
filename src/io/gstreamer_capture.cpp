#include "gstreamer_capture.hpp"

VideoCaptureGStreamer::VideoCaptureGStreamer(const std::string& source, int width, int height, int fps)
    : source_(source), width_(width), height_(height), fps_(fps) {
}

bool VideoCaptureGStreamer::open() {
#ifdef HAVE_GSTREAMER
    if (source_.find("rtsp://") == 0 || source_.find("rtmp://") == 0) {
        // Use GStreamer for network streams
        std::string pipeline = build_gstreamer_pipeline();
        return cap_.open(pipeline, cv::CAP_GSTREAMER);
    } else
#endif
    {
        // Use OpenCV for local files and cameras
        return cap_.open(source_);
    }
}

bool VideoCaptureGStreamer::read(cv::Mat& frame) {
    return cap_.read(frame);
}

void VideoCaptureGStreamer::release() {
    cap_.release();
}

bool VideoCaptureGStreamer::is_opened() const {
    return cap_.isOpened();
}

double VideoCaptureGStreamer::get_property(int prop_id) const {
    return cap_.get(prop_id);
}

bool VideoCaptureGStreamer::set_property(int prop_id, double value) {
    return cap_.set(prop_id, value);
}

std::string VideoCaptureGStreamer::build_gstreamer_pipeline() const {
    std::string pipeline;
    
    if (source_.find("rtsp://") == 0) {
        // RTSP source
        pipeline = "rtspsrc location=" + source_ + " ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! ";
    } else if (source_.find("rtmp://") == 0) {
        // RTMP source
        pipeline = "rtmpsrc location=" + source_ + " ! flvdemux ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! ";
    } else {
        // File source
        pipeline = "filesrc location=" + source_ + " ! decodebin ! videoconvert ! videoscale ! ";
    }
    
    // Add scaling and format conversion
    pipeline += "video/x-raw,width=" + std::to_string(width_) + 
               ",height=" + std::to_string(height_) + 
               ",format=BGR ! appsink sync=false";
    
    return pipeline;
}
