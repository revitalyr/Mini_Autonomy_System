module;

#include <coroutine>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

export module perception.image_loader;

import perception.result;
import perception.async;

export namespace perception {

    // Simple C++20 Generator for lazy sequences
    template<typename T>
    struct Generator {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type {
            T value_;
            std::exception_ptr exception_;

            Generator get_return_object() {
                return Generator(handle_type::from_promise(*this));
            }
            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void unhandled_exception() { exception_ = std::current_exception(); }
            
            template<std::convertible_to<T> From>
            std::suspend_always yield_value(From&& from) {
                value_ = std::forward<From>(from);
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

    // Modern async image loader with coroutines
    class AsyncImageLoader {
    private:
        ThreadPool thread_pool_;
        
    public:
        explicit AsyncImageLoader(size_t num_threads = 2) 
            : thread_pool_(num_threads) {}

        // Load images from directory asynchronously
        Generator<PerceptionResult<cv::Mat>> load_from_directory(
            const std::filesystem::path& directory,
            const std::vector<std::string>& extensions = {".jpg", ".png", ".bmp"}) {
            
            std::vector<std::filesystem::path> image_paths;
            
            // Collect image paths
            try {
                for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                    if (entry.is_regular_file()) {
                        const auto& path = entry.path();
                        const auto ext = path.extension().string();
                        
                        if (std::ranges::find(extensions, ext) != extensions.end()) {
                            image_paths.push_back(path);
                        }
                    }
                }
            } catch (const std::filesystem::filesystem_error& e) {
                co_yield error<cv::Mat>(PerceptionError::InvalidInput);
                return;
            }
            
            // Load images asynchronously
            for (const auto& path : image_paths) {
                auto future = thread_pool_.submit([path]() -> PerceptionResult<cv::Mat> {
                    try {
                        cv::Mat image = cv::imread(path.string(), cv::IMREAD_COLOR);
                        if (image.empty()) {
                            return error<cv::Mat>(PerceptionError::InvalidInput);
                        }
                        return success<cv::Mat>(std::move(image));
                    } catch (const std::exception& e) {
                        return error<cv::Mat>(PerceptionError::InvalidInput);
                    }
                });
                
                // Wait for result and yield
                co_yield future.get();
            }
        }

        // Load single image asynchronously
        std::future<PerceptionResult<cv::Mat>> load_image(const std::filesystem::path& path) {
            return thread_pool_.submit([path]() -> PerceptionResult<cv::Mat> {
                try {
                    cv::Mat image = cv::imread(path.string(), cv::IMREAD_COLOR);
                    if (image.empty()) {
                        return error<cv::Mat>(PerceptionError::InvalidInput);
                    }
                    return success<cv::Mat>(std::move(image));
                } catch (const std::exception& e) {
                    return error<cv::Mat>(PerceptionError::InvalidInput);
                }
            });
        }
    };
}
