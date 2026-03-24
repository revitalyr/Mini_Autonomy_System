module;

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <memory>

export module perception.detector;

import perception.concepts;
import perception.result;
import perception.metrics;
import perception.async;

export namespace perception {

    // Modern detection result with structured bindings support
    struct Detection {
        cv::Rect bbox;
        float confidence;
        int class_id;
        std::string class_name;
        
        // C++20: Default comparisons
        auto operator<=>(const Detection& other) const noexcept = default;
        
        // C++20: Structured bindings support
        template<std::size_t N>
        decltype(auto) get() const noexcept {
            if constexpr (N == 0) return bbox;
            else if constexpr (N == 1) return confidence;
            else if constexpr (N == 2) return class_id;
            else if constexpr (N == 3) return class_name;
        }
        
        // C++23: Deduction guides
        Detection() = default;
        Detection(cv::Rect box, float conf, int cls_id, std::string cls_name = "")
            : bbox(std::move(box)), confidence(conf), class_id(cls_id), 
              class_name(std::move(cls_name)) {}
    };

    // Enable structured bindings
}

namespace std {
    template<>
    struct tuple_size<perception::Detection> : std::integral_constant<std::size_t, 4> {};
    
    template<std::size_t N>
    struct tuple_element<N, perception::Detection> {
        using type = decltype(std::declval<perception::Detection>().template get<N>());
    };
}

export namespace perception {

    // Modern detector with concepts and result types
    class ModernDetector {
    private:
        std::string model_path_;
        float confidence_threshold_;
        float nms_threshold_;
        cv::Size input_size_;
        PerformanceMetrics metrics_;
        
        // C++23: Optional for better error handling
        std::optional<cv::dnn::Net> network_;
        
    public:
        // C++23: Deduction guides and explicit object parameters
        explicit ModernDetector(
            std::string model_path = "",
            float confidence_threshold = 0.5f,
            float nms_threshold = 0.4f,
            cv::Size input_size = {640, 640}
        );
        
        ~ModernDetector() = default;
        
        // Modern result-based API
        Result<std::vector<Detection>> detect(const cv::Mat& frame);
        
        // Async detection with coroutines
        Task<Result<std::vector<Detection>>> detect_async(cv::Mat frame) {
            co_await schedule_on(get_global_thread_pool());
            co_return detect(frame);
        }
        
        // Configuration methods
        void set_confidence_threshold(float threshold) noexcept { 
            confidence_threshold_ = threshold; 
        }
        
        void set_nms_threshold(float threshold) noexcept { 
            nms_threshold_ = threshold; 
        }
        
        [[nodiscard]] float get_confidence_threshold() const noexcept { 
            return confidence_threshold_; 
        }
        
        [[nodiscard]] float get_nms_threshold() const noexcept { 
            return nms_threshold_; 
        }
        
        // Performance metrics
        [[nodiscard]] const PerformanceMetrics& get_metrics() const noexcept { 
            return metrics_; 
        }
        
        void reset_metrics() noexcept { 
            metrics_.reset(); 
        }
        
        // Model management
        Result<bool> load_model(const std::string& model_path);
        
        // Batch processing for efficiency
        Task<Result<std::vector<std::vector<Detection>>>> detect_batch(
            std::ranges::range auto&& frames
        ) {
            std::vector<Task<Result<std::vector<Detection>>>> tasks;
            
            for (auto&& frame : frames) {
                // Clone frame to ensure async data safety
                tasks.push_back(detect_async(frame.clone()));
            }
            
            auto results = co_await when_all(std::move(tasks));
            
            std::vector<std::vector<Detection>> batch_results;
            batch_results.reserve(results.size());
            
            for (auto& res : results) {
                if (!res) co_return std::unexpected(res.error());
                batch_results.push_back(std::move(*res));
            }
            
            co_return batch_results;
        }
        
        // Stream processing with ranges
        auto detect_stream(std::ranges::range auto&& frames) {
            return frames | std::views::transform([this](const cv::Mat& frame) {
                return detect(frame);
            });
        }
        
    private:
        cv::Mat preprocess_frame(const cv::Mat& frame);
        std::vector<Detection> postprocess_outputs(
            const std::vector<cv::Mat>& outputs, 
            const cv::Size& original_size
        );
        
        // Helper for demo purposes
        std::vector<Detection> generate_fake_detections(const cv::Mat& frame);
    };

    // Factory functions for common detectors
    namespace detectors {
        Result<ModernDetector> create_yolo_detector(
            const std::string& model_path,
            float confidence_threshold = 0.5f
        );
        
        Result<ModernDetector> create_tensorrt_detector(
            const std::string& engine_path,
            float confidence_threshold = 0.5f
        );
    }

}
