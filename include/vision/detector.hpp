#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct Detection {
    cv::Rect bbox;        // Bounding box (x, y, width, height)
    float confidence;     // Detection confidence [0, 1]
    int class_id;         // Class ID
    std::string class_name; // Optional class name
    
    Detection() : bbox(0, 0, 0, 0), confidence(0.0f), class_id(-1) {}
    
    Detection(const cv::Rect& box, float conf, int cls_id, const std::string& cls_name = "")
        : bbox(box), confidence(conf), class_id(cls_id), class_name(cls_name) {}
};

class Detector {
private:
    std::string model_path_;
    float confidence_threshold_;
    float nms_threshold_;
    int input_width_;
    int input_height_;
    
#ifdef HAVE_TENSORRT
    void* tensorrt_engine_;
    void* tensorrt_context_;
#endif
    
public:
    Detector(const std::string& model_path = "", 
             float confidence_threshold = 0.5f,
             float nms_threshold = 0.4f,
             int input_width = 640,
             int input_height = 640);
    
    ~Detector();
    
    bool load_model(const std::string& model_path);
    std::vector<Detection> detect(const cv::Mat& frame);
    
    void set_confidence_threshold(float threshold) { confidence_threshold_ = threshold; }
    void set_nms_threshold(float threshold) { nms_threshold_ = threshold; }
    
    // For demo purposes - generate fake detections
    std::vector<Detection> generate_fake_detections(const cv::Mat& frame);
    
private:
    cv::Mat preprocess_frame(const cv::Mat& frame);
    std::vector<Detection> postprocess_outputs(const std::vector<float>& outputs, 
                                              const cv::Size& original_size);
    
#ifdef HAVE_TENSORRT
    bool load_tensorrt_model(const std::string& engine_path);
    std::vector<float> infer_tensorrt(const cv::Mat& input);
#endif
};
