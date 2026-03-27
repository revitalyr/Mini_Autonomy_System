module;

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <concepts>
#include <ranges>
#include <algorithm>
#include <memory>

export module perception_thread_safe_queue;

namespace perception {

// Semantic type aliases
using QueueSize = size_t;
using TimeoutMs = std::chrono::milliseconds;
using LockGuard = std::lock_guard<std::mutex>;
using UniqueLock = std::unique_lock<std::mutex>;

/**
 * @brief Concept for thread-safe queue element types
 */
template<typename T>
concept QueueElement = requires {
    typename T;
    std::is_copy_constructible_v<T>;
    std::is_move_constructible_v<T>;
    std::is_copy_assignable_v<T>;
    std::is_move_assignable_v<T>;
};

/**
 * @brief Modern thread-safe queue implementation with proper naming conventions
 * 
 * This class provides thread-safe queue operations with modern C++20/26 features.
 * Supports timeout, clear operations, RAII resource management, and range-based operations.
 */
template<QueueElement T>
class ThreadSafeQueue {
private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_is_shutdown{false};
    
public:
    /**
     * @brief Default constructor
     */
    constexpr ThreadSafeQueue() = default;
    
    /**
     * @brief Deleted copy constructor
     */
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    
    /**
     * @brief Deleted copy assignment
     */
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    
    /**
     * @brief Modern C++20 move constructor
     */
    ThreadSafeQueue(ThreadSafeQueue&& other) noexcept
        : m_queue(std::move(other.m_queue)), 
          m_mutex(std::move(other.m_mutex)), 
          m_condition(std::move(other.m_condition)), 
          m_is_shutdown(other.m_is_shutdown.load()) {}
    
    /**
     * @brief Modern C++20 move assignment
     */
    ThreadSafeQueue& operator=(ThreadSafeQueue&& other) noexcept {
        if (this != &other) {
            std::lock(m_mutex, other.m_mutex);
            m_queue = std::move(other.m_queue);
            m_mutex = std::move(other.m_mutex);
            m_condition = std::move(other.m_condition);
            m_is_shutdown.store(other.m_is_shutdown.load());
        }
        return *this;
    }
    
    /**
     * @brief Modern destructor with noexcept
     */
    ~ThreadSafeQueue() noexcept {
        this->shutdown();
    }
    
    /**
     * @brief Push value to queue (copy)
     */
    void push(const T& value) {
        {
            LockGuard lock(m_mutex);
            m_queue.emplace(value);
        }
        m_condition.notify_one();
    }
    
    /**
     * @brief Push value to queue (move)
     */
    void push(T&& value) {
        {
            LockGuard lock(m_mutex);
            m_queue.emplace(std::move(value));
        }
        m_condition.notify_one();
    }
    
    /**
     * @brief Push multiple values (C++20 ranges)
     */
    template<std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, T>
    void push_range(Range&& values) {
        {
            LockGuard lock(m_mutex);
            for (auto&& value : values) {
                m_queue.emplace(std::forward<decltype(value)>(value));
            }
        }
        m_condition.notify_all();
    }
    
    /**
     * @brief Wait and pop value with timeout (C++20 improvements)
     */
    [[nodiscard]] std::optional<T> wait_and_pop(
        TimeoutMs timeout = TimeoutMs{100}
    ) {
        UniqueLock lock(m_mutex);
        
        if (m_is_shutdown.load(std::memory_order_acquire)) [[unlikely]] {
            return std::nullopt;
        }
        
        // Modern C++20 wait with predicate
        if (!m_condition.wait_for(lock, timeout, [this] { 
            return !m_queue.empty() || m_is_shutdown.load(std::memory_order_acquire); 
        })) {
            return std::nullopt;
        }
        
        if (m_is_shutdown.load(std::memory_order_acquire) || m_queue.empty()) [[unlikely]] {
            return std::nullopt;
        }
        
        T result = std::move(m_queue.front());
        m_queue.pop();
        m_condition.notify_one();
        return result;
    }
    
    /**
     * @brief Try to pop value without blocking
     */
    [[nodiscard]] std::optional<T> try_pop() {
        LockGuard lock(m_mutex);
        if (m_queue.empty() || m_is_shutdown.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        
        T result = std::move(m_queue.front());
        m_queue.pop();
        return result;
    }
    
    /**
     * @brief Try to pop multiple values (C++20 ranges)
     */
    template<std::output_iterator<T> OutputIt>
    [[nodiscard]] QueueSize try_pop_range(OutputIt out, QueueSize max_count = SIZE_MAX) {
        LockGuard lock(m_mutex);
        if (m_queue.empty() || m_is_shutdown.load(std::memory_order_acquire)) {
            return 0;
        }
        
        QueueSize popped = 0;
        while (!m_queue.empty() && popped < max_count) {
            *out++ = std::move(m_queue.front());
            m_queue.pop();
            ++popped;
        }
        
        return popped;
    }
    
    /**
     * @brief Check if queue is empty
     */
    [[nodiscard]] bool empty() const {
        LockGuard lock(m_mutex);
        return m_queue.empty();
    }
    
    /**
     * @brief Get queue size
     */
    [[nodiscard]] QueueSize size() const {
        LockGuard lock(m_mutex);
        return m_queue.size();
    }
    
    /**
     * @brief Clear queue
     */
    void clear() {
        LockGuard lock(m_mutex);
        std::queue<T> empty;
        m_queue.swap(empty);
    }
    
    /**
     * @brief Shutdown queue
     */
    void shutdown() {
        {
            LockGuard lock(m_mutex);
            m_is_shutdown.store(true, std::memory_order_release);
        }
        m_condition.notify_all();
    }
    
    /**
     * @brief Check if queue is shutdown
     */
    [[nodiscard]] bool is_shutdown() const noexcept {
        return m_is_shutdown.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Reset queue to operational state
     */
    void reset() noexcept {
        {
            LockGuard lock(m_mutex);
            clear();
            m_is_shutdown.store(false, std::memory_order_release);
        }
    }
    
    /**
     * @brief Get capacity hint (for future C++26 implementation)
     */
    [[nodiscard]] constexpr QueueSize capacity() const noexcept {
        return SIZE_MAX; // No capacity limit for std::queue
    }
    
    /**
     * @brief Reserve capacity (no-op for std::queue, but kept for API compatibility)
     */
    void reserve(QueueSize) const noexcept {
        // std::queue doesn't have reserve, but we keep this for API compatibility
    }
};

/**
 * @brief Factory function for creating thread-safe queues
 */
template<QueueElement T>
[[nodiscard]] auto make_thread_safe_queue() {
    return std::make_unique<ThreadSafeQueue<T>>();
}

/**
 * @brief Factory function with initial capacity hint
 */
template<QueueElement T>
[[nodiscard]] auto make_thread_safe_queue(QueueSize /*capacity*/) {
    return std::make_unique<ThreadSafeQueue<T>>();
}

} // namespace perception
