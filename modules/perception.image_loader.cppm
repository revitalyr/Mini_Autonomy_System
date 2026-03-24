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
        
        Generator(Generator&& other) noexcept : h_(other.h_) { other.h_ = nullptr; }
        Generator& operator=(Generator&& other) noexcept {
            if (this != &other) {
                if (h_) h_.destroy();
                h_ = other.h_;
                other.h_ = nullptr;
            }
            return *this;
        }

        struct iterator {
            handle_type h;
            bool done;

            iterator(handle_type handle, bool d) : h(handle), done(d) {}

            iterator& operator++() {
                h.resume();
                if (h.done()) done = true;
                return *this;
            }

            bool operator!=(const iterator& other) const { return done != other.done; }
            T& operator*() { return h.promise().value_; }
            T* operator->() { return &h.promise().value_; }
        };

        iterator begin() {
            if (h_) {
                h_.resume();
                if (h_.done()) return iterator(h_, true);
                return iterator(h_, false);
            }
            return iterator(nullptr, true);
        }

        iterator end() { return iterator(nullptr, true); }
    };

    class AsyncImageLoader {
    public:
        // Load a single image asynchronously
        // Usage: Task<Result<cv::Mat>> task = AsyncImageLoader::load_async("path.jpg");
        static Task<Result<cv::Mat>> load_async(std::string path) {
            co_await schedule_on(get_global_thread_pool());
            
            cv::Mat img = cv::imread(path);
            if (img.empty()) co_return std::unexpected(std::make_error_code(std::errc::io_error));
            co_return img;
        }

        // Stream images from directory lazily
        // Usage: for (auto res : AsyncImageLoader::stream_directory("imgs/")) ...
        static Generator<Result<cv::Mat>> stream_directory(std::string path) {
             namespace fs = std::filesystem;
             if (!fs::exists(path) || !fs::is_directory(path)) {
                 co_yield std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
                 co_return;
             }

             std::vector<fs::path> files;
             for (const auto& entry : fs::directory_iterator(path)) {
                 if (entry.is_regular_file()) {
                     auto ext = entry.path().extension();
                     if (ext == ".jpg" || ext == ".png" || ext == ".jpeg") {
                         files.push_back(entry.path());
                     }
                 }
             }
             
             // Ensure consistent order
             std::sort(files.begin(), files.end());

             for (const auto& file : files) {
                 cv::Mat img = cv::imread(file.string());
                 if (!img.empty()) {
                    co_yield success(std::move(img));
                 } else {
                    co_yield std::unexpected(std::make_error_code(std::errc::io_error));
                 }
             }
        }
    };
}