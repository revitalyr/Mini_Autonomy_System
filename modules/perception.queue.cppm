module;

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <stop_token>
#include <semaphore>
#include <expected>
#include <concepts>

export module perception.queue;

import perception.concepts;

export namespace perception {

    template<typename T>
    class ThreadSafeQueue {
    private:
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable_any condition_;
        std::binary_semaphore semaphore_{0};
        std::atomic<bool> shutdown_{false};

    public:
        using value_type = T;
        
        constexpr ThreadSafeQueue() = default;
        ~ThreadSafeQueue() = default;

        // Modern C++23: Delete copy, enable move by default
        ThreadSafeQueue(const ThreadSafeQueue&) = delete;
        ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
        ThreadSafeQueue(ThreadSafeQueue&&) = default;
        ThreadSafeQueue& operator=(ThreadSafeQueue&&) = default;

        // Modern push with perfect forwarding
        template<typename U>
        requires std::convertible_to<U, T>
        void push(U&& value) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(std::forward<U>(value));
            }
            condition_.notify_one();
            semaphore_.release();
        }

        // Modern pop with stop_token support (C++20)
        std::expected<T, bool> pop(std::stop_token st = {}) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait with stop token support
            if (!condition_.wait(lock, st, [this] { 
                return !queue_.empty() || shutdown_; 
            })) {
                return std::unexpected(false); // Stopped
            }
            
            if (queue_.empty() || shutdown_) {
                return std::unexpected(true); // Shutdown
            }
            
            T result = std::move(queue_.front());
            queue_.pop();
            return result;
        }

        // Timeout-based pop
        std::expected<T, bool> pop_timeout(std::chrono::milliseconds timeout) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (condition_.wait_for(lock, timeout, [this] { 
                return !queue_.empty() || shutdown_; 
            })) {
                if (shutdown_) return std::unexpected(true);
                
                T result = std::move(queue_.front());
                queue_.pop();
                return result;
            }
            
            return std::unexpected(false); // Timeout
        }

        // Non-blocking try_pop using semaphore
        std::expected<T, bool> try_pop() {
            if (!semaphore_.try_acquire()) {
                return std::unexpected(false); // Empty
            }
            
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty() || shutdown_) {
                return std::unexpected(true);
            }
            
            T result = std::move(queue_.front());
            queue_.pop();
            return result;
        }

        void shutdown() noexcept {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                shutdown_ = true;
            }
            condition_.notify_all();
        }

        [[nodiscard]] bool empty() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        [[nodiscard]] size_t size() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        [[nodiscard]] bool is_shutdown() const noexcept {
            return shutdown_.load();
        }
    };

    // Type alias for convenience
    template<typename T>
    using queue_t = ThreadSafeQueue<T>;

}
