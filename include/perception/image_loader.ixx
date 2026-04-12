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

namespace perception {

    /**
     * Lazy sequence generator for streaming results
     * @tparam T Type of values in the sequence
     */
    export template<typename T>
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

        // Iterator class for range-based for loops
        struct iterator {
            handle_type h_;
            bool at_end_ = false;

            explicit iterator(handle_type h) : h_(h) {
                if (h_ && !h_.done()) {
                    h_.resume();
                }
            }

            iterator& operator++() {
                if (h_ && !h_.done()) {
                    h_.resume();
                }
                if (h_ && h_.done()) {
                    at_end_ = true;
                }
                return *this;
            }

            T operator*() const {
                if (!h_ || h_.done() || at_end_) {
                    throw std::runtime_error("Generator exhausted");
                }
                return h_.promise().value_;
            }

            bool operator!=(const iterator& other) const {
                if (at_end_) return false;
                if (!h_) return other.h_ != nullptr;
                return h_ != other.h_ && !h_.done();
            }
        };

        iterator begin() {
            return iterator(h_);
        }

        iterator end() {
            return iterator(nullptr);
        }

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

    /**
     * Asynchronous image loader for batch processing
     * Loads images from directories or individual files using a thread pool
     * Supports common image formats: JPEG, PNG, BMP, TIFF, WebP
     */
    export class AsyncImageLoader {
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

    public:
        /**
         * @param num_threads Number of worker threads for parallel loading
         */
        explicit AsyncImageLoader(size_t num_threads = 2);
        ~AsyncImageLoader();

        /**
         * Load all images from a directory
         * @param directory Path to directory containing images
         * @return Generator yielding image data for each image found
         */
        Generator<PerceptionResult<ImageData>> load_images(const std::filesystem::path& directory);

        /**
         * Load a single image file
         * @param path Path to image file
         * @return Future containing image data or error
         */
        std::future<PerceptionResult<ImageData>> load_image(const std::filesystem::path& path);
    };
}
