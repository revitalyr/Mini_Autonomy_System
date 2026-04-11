module;

#include <coroutine>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <memory>

export module perception.image_loader;

import perception.types;
import perception.async;
import perception.result;

export namespace perception {

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
                    (void)from;
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
        struct Impl; // PIMPL - hides OpenCV implementation
        std::unique_ptr<Impl> impl_;

    public:
        explicit AsyncImageLoader(size_t num_threads = 2);
        ~AsyncImageLoader();

        // Load images from directory asynchronously
        Generator<PerceptionResult<ImageData>> load_images(const std::filesystem::path& directory);

        // Load single image asynchronously
        std::future<PerceptionResult<ImageData>> load_image(const std::filesystem::path& path);
    };
}
