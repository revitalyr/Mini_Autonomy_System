/**
 * @file async.hpp
 * @brief Modern C++20/23 async operations and coroutines for perception system
 * 
 * This header provides modern asynchronous operations using C++20 coroutines,
 * thread pools, and async utilities for the perception system.
 * 
 * @author Mini Autonomy System
 * @date 2026
 */

#pragma once

#include <coroutine>
#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stop_token>
#include <atomic>
#include <memory>
#include <functional>
#include <type_traits>
#include <ranges>

// Import modern type system
#include "types.hpp"
#include "result.hpp"

namespace perception {

// ============================================================================
// MODERN COROUTINE UTILITIES
// ============================================================================

/**
 * @brief Generator coroutine type for producing sequences
 * 
 * This provides a modern coroutine-based generator that can be used
 * for producing sequences of values lazily.
 */
template<typename T>
class Generator {
public:
    struct promise_type {
        T m_value{};
        
        auto get_return_object() -> Generator {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        static constexpr auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        static constexpr auto final_suspend() noexcept -> std::suspend_always { return {}; }
        
        auto yield_value(T value) noexcept -> std::suspend_always {
            m_value = std::move(value);
            return {};
        }
        
        constexpr auto return_void() noexcept -> void {}
        constexpr auto unhandled_exception() noexcept -> void { std::terminate(); }
    };
    
private:
    std::coroutine_handle<promise_type> m_handle;
    
public:
    explicit Generator(std::coroutine_handle<promise_type> handle) noexcept : m_handle{handle} {}
    
    ~Generator() noexcept {
        if (m_handle) {
            m_handle.destroy();
        }
    }
    
    // Move-only type
    Generator(const Generator&) = delete;
    auto operator=(const Generator&) -> Generator& = delete;
    
    Generator(Generator&& other) noexcept : m_handle{std::exchange(other.m_handle, nullptr)} {}
    auto operator=(Generator&& other) noexcept -> Generator& {
        if (this != &other) {
            if (m_handle) {
                m_handle.destroy();
            }
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }
    
    struct Iterator {
        std::coroutine_handle<promise_type> m_handle;
        
        auto operator==(std::default_sentinel_t) const noexcept -> bool {
            return !m_handle || m_handle.done();
        }
        
        auto operator++() noexcept -> void {
            m_handle.resume();
        }
        
        auto operator*() const noexcept -> const T& {
            return m_handle.promise().m_value;
        }
    };
    
    auto begin() const noexcept -> Iterator {
        if (m_handle && !m_handle.done()) {
            m_handle.resume();
        }
        return Iterator{m_handle};
    }
    
    auto end() const noexcept -> std::default_sentinel_t {
        return {};
    }
};

/**
 * @brief Async task type for coroutine-based operations
 * 
 * This provides a modern coroutine-based task that can be used
 * for asynchronous operations with proper error handling.
 */
template<typename T>
class Task {
public:
    struct promise_type {
        std::promise<T> m_promise;
        
        auto get_return_object() -> Task {
            return Task{m_promise.get_future()};
        }
        
        static constexpr auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        static constexpr auto final_suspend() noexcept -> std::suspend_always { return {}; }
        
        auto return_value(T value) noexcept -> void {
            m_promise.set_value(std::move(value));
        }
        
        auto unhandled_exception() noexcept -> void {
            m_promise.set_exception(std::current_exception());
        }
    };
    
private:
    Future<T> m_future;
    
public:
    explicit Task(Future<T>&& future) noexcept : m_future{std::move(future)} {}
    
    // Move-only type
    Task(const Task&) = delete;
    auto operator=(const Task&) -> Task& = delete;
    
    Task(Task&&) = default;
    auto operator=(Task&&) -> Task& = default;
    
    auto get() -> T {
        return m_future.get();
    }
    
    auto wait() -> void {
        m_future.wait();
    }
    
    auto wait_for(Milliseconds timeout) -> bool {
        return m_future.wait_for(timeout) == std::future_status::ready;
    }
};

/**
 * @brief Specialization for void tasks
 */
template<>
class Task<void> {
public:
    struct promise_type {
        std::promise<void> m_promise;
        
        auto get_return_object() -> Task {
            return Task{m_promise.get_future()};
        }
        
        static constexpr auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        static constexpr auto final_suspend() noexcept -> std::suspend_always { return {}; }
        
        constexpr auto return_void() noexcept -> void {}
        
        auto unhandled_exception() noexcept -> void {
            m_promise.set_exception(std::current_exception());
        }
    };
    
private:
    Future<void> m_future;
    
public:
    explicit Task(Future<void>&& future) noexcept : m_future{std::move(future)} {}
    
    Task(const Task&) = delete;
    auto operator=(const Task&) -> Task& = delete;
    
    Task(Task&&) = default;
    auto operator=(Task&&) -> Task& = default;
    
    auto get() -> void {
        m_future.get();
    }
    
    auto wait() -> void {
        m_future.wait();
    }
    
    auto wait_for(Milliseconds timeout) -> bool {
        return m_future.wait_for(timeout) == std::future_status::ready;
    }
};

// ============================================================================
// MODERN THREAD POOL
// ============================================================================

/**
 * @brief Modern thread pool with C++20 jthread support
 * 
 * This provides a thread-safe, efficient thread pool that supports
 * graceful shutdown with stop tokens and proper RAII management.
 */
class ThreadPool {
private:
    Vector<Thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    Mutex m_mutex;
    std::condition_variable_any m_condition;
    Atomic<bool> m_shutdown{false};
    
public:
    /**
     * @brief Constructor with thread count
     * @param num_threads Number of worker threads
     */
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        m_workers.reserve(num_threads);
        
        for (size_t i = 0; i < num_threads; ++i) {
            m_workers.emplace_back([this](std::stop_token stop_token) {
                worker_loop(stop_token);
            });
        }
    }
    
    /**
     * @brief Destructor - shuts down all threads
     */
    ~ThreadPool() noexcept {
        shutdown();
    }
    
    // Move-only type
    ThreadPool(const ThreadPool&) = delete;
    auto operator=(const ThreadPool&) -> ThreadPool& = delete;
    
    ThreadPool(ThreadPool&&) = delete;
    auto operator=(ThreadPool&&) -> ThreadPool& = delete;
    
    /**
     * @brief Submit a task to the thread pool
     * @tparam Func Function type
     * @tparam Args Argument types
     * @param func Function to execute
     * @param args Arguments to pass to function
     * @return Future for the task result
     */
    template<typename Func, typename... Args>
    requires Callable<Func, Args...>
    auto submit(Func&& func, Args&&... args) -> Future<std::invoke_result_t<Func, Args...>> {
        using ReturnType = std::invoke_result_t<Func, Args...>;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [func = std::forward<Func>(func), args...]() mutable {
                return std::invoke(func, std::forward<Args>(args)...);
            }
        );
        
        auto future = task->get_future();
        
        {
            LockGuard lock{m_mutex};
            if (m_shutdown.load()) {
                throw std::runtime_error{"ThreadPool is shutdown"};
            }
            
            m_tasks.emplace([task]() { (*task)(); });
        }
        
        m_condition.notify_one();
        return future;
    }
    
    /**
     * @brief Submit a task with result wrapper
     * @tparam Func Function type
     * @tparam Args Argument types
     * @param func Function to execute
     * @param args Arguments to pass to function
     * @return AsyncResult for the task
     */
    template<typename Func, typename... Args>
    requires Callable<Func, Args...>
    auto submit_with_result(Func&& func, Args&&... args) -> AsyncResult<std::invoke_result_t<Func, Args...>> {
        using ReturnType = std::invoke_result_t<Func, Args...>;
        
        auto future = submit(
            [func = std::forward<Func>(func), args...]() mutable -> Result<ReturnType> {
                return wrap_result(std::move(func), std::move(args)...);
            }
        );
        
        return make_async_result(std::move(future));
    }
    
    /**
     * @brief Get number of worker threads
     * @return Thread count
     */
    auto get_thread_count() const noexcept -> size_t {
        return m_workers.size();
    }
    
    /**
     * @brief Get number of active threads (approximate)
     * @return Active thread count
     */
    auto get_active_threads() const noexcept -> size_t {
        LockGuard lock{m_mutex};
        return m_tasks.size();
    }
    
    /**
     * @brief Check if thread pool is shutdown
     * @return True if shutdown, false otherwise
     */
    auto is_shutdown() const noexcept -> bool {
        return m_shutdown.load();
    }
    
    /**
     * @brief Shutdown the thread pool gracefully
     */
    auto shutdown() noexcept -> void {
        if (m_shutdown.exchange(true)) {
            return; // Already shutdown
        }
        
        m_condition.notify_all();
        
        // Wait for all threads to finish
        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    /**
     * @brief Worker loop for each thread
     * @param stop_token Stop token for graceful shutdown
     */
    auto worker_loop(std::stop_token stop_token) noexcept -> void {
        while (!stop_token.stop_requested()) {
            std::function<void()> task;
            
            {
                UniqueLock lock{m_mutex};
                
                // Wait for task or shutdown signal
                m_condition.wait(lock, stop_token, [this] {
                    return m_shutdown.load() || !m_tasks.empty();
                });
                
                if (stop_token.stop_requested() || (m_shutdown.load() && m_tasks.empty())) {
                    break;
                }
                
                if (!m_tasks.empty()) {
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                } else {
                    continue;
                }
            }
            
            if (task) {
                try {
                    task();
                } catch (...) {
                    // Log error or handle appropriately
                    // For now, we just continue
                }
            }
        }
    }
};

// ============================================================================
// MODERN ASYNC UTILITIES
// ============================================================================

/**
 * @brief Create a task from a coroutine
 * @tparam T Return type
 * @param func Coroutine function
 * @return Task wrapper
 */
template<typename T>
auto make_task(Callable<Task<T>> auto&& func) -> Task<T> {
    return std::forward<decltype(func)>(func)();
}

/**
 * @brief Create a void task from a coroutine
 * @param func Coroutine function
 * @return Task wrapper
 */
auto make_task(Callable<Task<void>> auto&& func) -> Task<void>;

/**
 * @brief Parallel for loop using thread pool
 * @tparam Index Index type
 * @tparam Func Function type
 * @param pool Thread pool to use
 * @param start Start index
 * @param end End index
 * @param func Function to execute for each index
 * @return Vector of futures for each task
 */
template<typename Index, typename Func>
requires Callable<Func, Index>
auto parallel_for(ThreadPool& pool, Index start, Index end, Func&& func) 
    -> Vector<Future<std::invoke_result_t<Func, Index>>> {
    
    using ReturnType = std::invoke_result_t<Func, Index>;
    Vector<Future<ReturnType>> futures;
    
    const auto count = static_cast<size_t>(end - start);
    futures.reserve(count);
    
    for (Index i = start; i < end; ++i) {
        futures.emplace_back(pool.submit(func, i));
    }
    
    return futures;
}

/**
 * * @brief Parallel map using thread pool
 * @tparam T Input type
 * @tparam U Output type
 * @tparam Func Transform function type
 * @param pool Thread pool to use
 * @param input_range Input range
 * @param func Transform function
 * @return Vector of futures for transformed values
 */
template<typename T, typename U, typename Func>
requires Callable<Func, T>
auto parallel_map(ThreadPool& pool, const Vector<T>& input_range, Func&& func) 
    -> Vector<Future<U>> {
    
    Vector<Future<U>> futures;
    futures.reserve(input_range.size());
    
    for (const auto& item : input_range) {
        futures.emplace_back(pool.submit(func, item));
    }
    
    return futures;
}

/**
 * @brief Wait for all futures to complete
 * @tparam T Result type
 * @param futures Vector of futures
 * @return Vector of results
 */
template<typename T>
auto wait_for_all(Vector<Future<T>>& futures) -> Vector<T> {
    Vector<T> results;
    results.reserve(futures.size());
    
    for (auto& future : futures) {
        results.emplace_back(future.get());
    }
    
    return results;
}

/**
 * @brief Wait for all async results to complete
 * @tparam T Result type
 * @param async_results Vector of async results
 * @return Vector of successful results
 */
template<typename T>
auto wait_for_all_async(Vector<AsyncResult<T>>& async_results) -> Vector<T> {
    Vector<T> results;
    
    for (auto& async_result : async_results) {
        if (auto result = async_result.wait_for(Milliseconds{1000})) {
            if (*result) {
                results.emplace_back(result->value());
            }
        }
    }
    
    return results;
}

/**
 * @brief Async retry mechanism
 * @tparam Func Function type
 * @tparam Args Argument types
 * @param pool Thread pool
 * @param max_retries Maximum number of retries
 * @param delay Delay between retries
 * @param func Function to retry
 * @param args Arguments to pass
 * @return AsyncResult with retry logic
 */
template<typename Func, typename... Args>
requires Callable<Func, Args...>
auto async_retry(ThreadPool& pool, size_t max_retries, Milliseconds delay, 
                Func&& func, Args&&... args) -> AsyncResult<std::invoke_result_t<Func, Args...>> {
    
    using ReturnType = std::invoke_result_t<Func, Args...>;
    
    return make_async_result(pool.submit([max_retries, delay, func = std::forward<Func>(func), 
                                          args...]() mutable -> Result<ReturnType> {
        for (size_t attempt = 0; attempt <= max_retries; ++attempt) {
            auto result = wrap_result(func, args...);
            
            if (result) {
                return result;
            }
            
            if (attempt < max_retries) {
                std::this_thread::sleep_for(delay);
            }
        }
        
        return make_error(PerceptionError::TimeoutError);
    }));
}

} // namespace perception
