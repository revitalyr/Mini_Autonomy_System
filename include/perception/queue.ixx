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

        // Modern pop with timeout and stop_token support
        std::expected<T, bool> pop(std::stop_token stoken = {}, 
                                      std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait with timeout or stop token
            if (!condition_.wait_for(lock, timeout, [&] { 
                return !queue_.empty() || shutdown_ || stoken.stop_requested(); 
            })) {
                return std::unexpected(false); // Timeout
            }
            
            if (shutdown_ || stoken.stop_requested()) {
                return std::unexpected(false); // Shutdown
            }
            
            if (queue_.empty()) {
                return std::unexpected(false); // Empty
            }
            
            T result = std::move(queue_.front());
            queue_.pop();
            return result;
        }

        // Try pop without blocking
        std::expected<T, bool> try_pop() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty() || shutdown_) {
                return std::unexpected(false);
            }
            
            T result = std::move(queue_.front());
            queue_.pop();
            return result;
        }

        // Check if empty
        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        // Get size
        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        // Clear queue
        void clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_ = std::queue<T>{};
        }

        // Shutdown queue
        void shutdown() {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                shutdown_ = true;
            }
            condition_.notify_all();
            semaphore_.release();
        }

        // Check if shutdown
        bool is_shutdown() const {
            return shutdown_.load();
        }
    };
}
