module;

#include <opencv2/opencv.hpp>
#include <coroutine>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <ranges>

export module perception.image_loader;

import perception.async;
import perception.result;

export namespace perception {

    // Image structure using OpenCV Mat
    struct Image {
        cv::Mat mat;  // OpenCV matrix
        
        Image() = default;
        explicit Image(cv::Mat m) : mat(std::move(m)) {}
        
        int width() const { return mat.cols; }
        int height() const { return mat.rows; }
        int channels() const { return mat.channels(); }
        
        bool empty() const { return mat.empty(); }
    };

    // Simple C++20 Generator for lazy sequences
    template<typename T>
    struct Generator {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type {
            std::conditional_t<std::is_void_v<T>, std::monostate, T> value_{};
            std::exception_ptr exception_;

            Generator get_return_object() {
                return Generator(handle_type::from_promise(*this));
            }
            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void unhandled_exception() { exception_ = std::current_exception(); }
            
            template<std::convertible_to<T> From>
            std::suspend_always yield_value(From&& from) {
                if constexpr (std::is_void_v<T>) {
                    // For void type, don't assign anything
                    (void)from; // Suppress unused parameter warning
                } else {
                    value_ = std::forward<From>(from);
                }
                return {};
            }
            
            void return_void() {}
        };

        handle_type h_;

        explicit Generator(handle_type h) : h_(h) {}
        ~Generator() { if (h_) h_.destroy(); }
        
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;
        Generator(Generator&&) = default;
        Generator& operator=(Generator&&) = default;

        T operator++() {
            h_.resume();
            if (h_.done()) {
                throw std::runtime_error("Generator exhausted");
            }
            return std::move(h_.promise().value_);
        }

        bool operator()() {
            return !h_.done();
        }
    };

    // Simple async image loader
    class AsyncImageLoader {
    private:
        ThreadPool thread_pool_;
        
    public:
        explicit AsyncImageLoader(size_t num_threads = 2) 
            : thread_pool_(num_threads) {}

        // Load images from directory asynchronously
        Generator<PerceptionResult<Image>> load_images(const std::filesystem::path& directory) {
            std::vector<std::filesystem::path> image_paths;
            
            // Collect image paths first
            try {
                for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                    if (entry.is_regular_file()) {
                        const auto& path = entry.path();
                        const auto ext = path.extension().string();
                        
                        // Use std::find instead of ranges::find for compatibility
                        static const std::vector<std::string> extensions = {".jpg", ".png", ".bmp"};
                        if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                            image_paths.push_back(path);
                        }
                    }
                }
            } catch (const std::filesystem::filesystem_error&) {
                // Cannot use co_await in catch block, so return error directly
                co_return;
            }
            
            // Load images asynchronously
            for (const auto& path : image_paths) {
                auto future = thread_pool_.submit([path]() -> PerceptionResult<Image> {
                    try {
                        // Load image using OpenCV
                        cv::Mat mat = cv::imread(path.string(), cv::IMREAD_COLOR);
                        if (mat.empty()) {
                            return error<Image>(PerceptionError::InvalidInput);
                        }
                        
                        return success<Image>(Image(std::move(mat)));
                    } catch (const std::exception&) {
                        return error<Image>(PerceptionError::InvalidInput);
                    }
                });
                
                // Wait for result and yield
                co_yield future.get();
            }
        }

        // Load single image asynchronously
        std::future<PerceptionResult<Image>> load_image(const std::filesystem::path& path) {
            return thread_pool_.submit([path]() -> PerceptionResult<Image> {
                try {
                    // Load image using OpenCV
                    cv::Mat mat = cv::imread(path.string(), cv::IMREAD_COLOR);
                    if (mat.empty()) {
                        return error<Image>(PerceptionError::InvalidInput);
                    }
                    
                    return success<Image>(Image(std::move(mat)));
                } catch (const std::exception&) {
                    return error<Image>(PerceptionError::InvalidInput);
                }
            });
        }
    };
}
