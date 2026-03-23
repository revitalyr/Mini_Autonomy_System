#include "detector.hpp"
#include <random>
#include <algorithm>

Detector::Detector(const std::string& model_path, 
                   float confidence_threshold,
                   float nms_threshold,
                   int input_width,
                   int input_height)
    : model_path_(model_path),
      confidence_threshold_(confidence_threshold),
      nms_threshold_(nms_threshold),
      input_width_(input_width),
      input_height_(input_height)
#ifdef HAVE_TENSORRT
      , tensorrt_engine_(nullptr), tensorrt_context_(nullptr)
#endif
{
    if (!model_path_.empty()) {
        load_model(model_path_);
    }
}

Detector::~Detector() {
#ifdef HAVE_TENSORRT
    // Clean up TensorRT resources
    if (tensorrt_context_) {
        // Destroy context
        tensorrt_context_ = nullptr;
    }
    if (tensorrt_engine_) {
        // Destroy engine
        tensorrt_engine_ = nullptr;
    }
#endif
}

bool Detector::load_model(const std::string& model_path) {
    model_path_ = model_path;
    
#ifdef HAVE_TENSORRT
    // Try to load TensorRT engine first
    if (model_path.find(".engine") != std::string::npos) {
        return load_tensorrt_model(model_path);
    }
#endif
    
    // For demo, we'll use fake detections
    return true;
}

std::vector<Detection> Detector::detect(const cv::Mat& frame) {
    if (frame.empty()) {
        return {};
    }
    
#ifdef HAVE_TENSORRT
    if (tensorrt_engine_) {
        cv::Mat preprocessed = preprocess_frame(frame);
        std::vector<float> outputs = infer_tensorrt(preprocessed);
        return postprocess_outputs(outputs, frame.size());
    }
#endif
    
    // Demo mode: generate fake detections
    return generate_fake_detections(frame);
}

std::vector<Detection> Detector::generate_fake_detections(const cv::Mat& frame) {
    std::vector<Detection> detections;
    
    if (frame.empty()) {
        return detections;
    }
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> confidence_dist(0.6f, 0.95f);
    static std::uniform_int_distribution<int> class_dist(0, 2);
    static std::uniform_int_distribution<int> size_dist(50, 150);
    static std::uniform_int_distribution<int> pos_dist_x(50, frame.cols - 200);
    static std::uniform_int_distribution<int> pos_dist_y(50, frame.rows - 200);
    
    // Generate 1-3 fake detections
    int num_detections = std::uniform_int_distribution<int>(1, 3)(gen);
    
    for (int i = 0; i < num_detections; ++i) {
        int x = pos_dist_x(gen);
        int y = pos_dist_y(gen);
        int width = size_dist(gen);
        int height = size_dist(gen);
        
        // Ensure bounding box is within frame
        x = std::max(0, std::min(x, frame.cols - width - 1));
        y = std::max(0, std::min(y, frame.rows - height - 1));
        width = std::min(width, frame.cols - x);
        height = std::min(height, frame.rows - y);
        
        cv::Rect bbox(x, y, width, height);
        float confidence = confidence_dist(gen);
        int class_id = class_dist(gen);
        
        std::vector<std::string> class_names = {"person", "car", "bicycle"};
        std::string class_name = class_id < class_names.size() ? class_names[class_id] : "unknown";
        
        if (confidence >= confidence_threshold_) {
            detections.emplace_back(bbox, confidence, class_id, class_name);
        }
    }
    
    return detections;
}

cv::Mat Detector::preprocess_frame(const cv::Mat& frame) {
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(input_width_, input_height_));
    
    cv::Mat normalized;
    resized.convertTo(normalized, CV_32F, 1.0/255.0);
    
    return normalized;
}

std::vector<Detection> Detector::postprocess_outputs(const std::vector<float>& outputs, 
                                                     const cv::Size& original_size) {
    std::vector<Detection> detections;
    
    // This would contain actual postprocessing logic for real model outputs
    // For demo, return empty
    return detections;
}

#ifdef HAVE_TENSORRT
bool Detector::load_tensorrt_model(const std::string& engine_path) {
    // TensorRT model loading implementation would go here
    // For demo, return false
    return false;
}

std::vector<float> Detector::infer_tensorrt(const cv::Mat& input) {
    // TensorRT inference implementation would go here
    // For demo, return empty
    return {};
}
#endif
