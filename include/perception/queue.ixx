module;
#include <queue>
#include <mutex>
#include <semaphore>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <stop_token>

export module perception.queue;
import perception.concepts;

/**
 * @file perception.queue.ixx
 * @brief Thread-safe queue implementations for the perception system
 *
 * This module provides thread-safe queue implementations with timeout support,
 * shutdown capabilities, and lock-free operations where possible.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

namespace perception {

    /**
     * @brief Thread-safe queue with timeout and shutdown support
     * 
     * This class provides a thread-safe queue that supports:
     * - Push operations with blocking/non-blocking variants
     * - Pop operations with timeout support
     * - Graceful shutdown with stop token support
     * - Size and empty queries
     * 
     * @tparam T Type of elements stored in the queue
     */
    export template<typename T>
    class ThreadSafeQueue {
    private:
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable_any condition_;
        std::binary_semaphore semaphore_{0};
        std::atomic<bool> shutdown_{false};

    public:
        /**
         * @brief Type alias for value type
         */
        using value_type = T;

        /**
         * @brief Default constructor
         */
        constexpr ThreadSafeQueue() = default;

        /**
         * @brief Destructor
         */
        ~ThreadSafeQueue() = default;

        // Modern C++23: Delete copy, enable move by default
        ThreadSafeQueue(const ThreadSafeQueue&) = delete;
        ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
        ThreadSafeQueue(ThreadSafeQueue&&) = default;
        ThreadSafeQueue& operator=(ThreadSafeQueue&&) = default;

        /**
         * @brief Push an element to the queue (blocking)
         * 
         * @param value Element to push
         */
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

        /**
         * @brief Pop an element from the queue (blocking until available)
         * 
         * @param st Stop token for cancellation
         * @return std::optional<T> containing the element if available, empty if cancelled
         */
        std::optional<T> pop(std::stop_token st = {}) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait with stop token support
            if (!condition_.wait(lock, st, [this] { 
                return !queue_.empty() || shutdown_; 
            })) {
                return std::nullopt; // Stopped
            }
            
            if (queue_.empty() || shutdown_) {
                return std::nullopt; // Shutdown
            }
            
            T result = std::move(queue_.front());
            queue_.pop();
            return result;
        }

        /**
         * @brief Pop an element from the queue with timeout
         * 
         * @param timeout Maximum time to wait
         * @return std::optional<T> containing the element if available, empty if timeout
         */
        std::optional<T> pop_timeout(Milliseconds timeout) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (condition_.wait_for(lock, timeout, [this] { 
                return !queue_.empty() || shutdown_; 
            })) {
                if (shutdown_) return std::nullopt;
                
                T result = std::move(queue_.front());
                queue_.pop();
                return result;
            }
            
            return std::nullopt; // Timeout
        }

        /**
         * @brief Try to pop an element from the queue (non-blocking)
         * 
         * @return std::optional<T> containing the element if available, empty otherwise
         */
        std::optional<T> try_pop() {
            if (!semaphore_.try_acquire()) {
                return std::nullopt; // Empty
            }
            
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty() || shutdown_) {
                return std::nullopt; // Empty or shutdown
            }
            T result = std::move(queue_.front());
            queue_.pop();
            return result;
        }

        /**
         * @brief Shut down the queue
         */
        void shutdown() noexcept {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                shutdown_ = true;
            }
            condition_.notify_all();
        }

        /**
         * @brief Check if the queue is empty
         * 
         * @return true if empty, false otherwise
         */
        [[nodiscard]] bool empty() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        /**
         * @brief Get the number of elements in the queue
         * 
         * @return size_t Number of elements
         */
        [[nodiscard]] size_t size() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        /**
         * @brief Check if the queue is shut down
         * 
         * @return true if shut down, false otherwise
         */
        [[nodiscard]] bool is_shutdown() const noexcept {
            return shutdown_.load();
        }
    };

    // Type alias for convenience
    export template<typename T>
    using queue_t = ThreadSafeQueue<T>;

} // namespace perception
