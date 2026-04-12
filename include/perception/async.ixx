module;
#include <coroutine>
#include <exception>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <stop_token>
#include <memory>
#include <future>
#include <chrono>

export module perception.async;

/**
 * @file perception.async.ixx
 * @brief Async operations and coroutine support for the perception system
 *
 * This module provides coroutine-based async operations including Task<T> for
 * awaitable operations and Generator<T> for lazy sequences. It also includes
 * a thread pool for efficient async task execution.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

namespace perception {

    // ============================================================================
    // TASK<T> - COROUTINE AWAITABLE
    // ============================================================================

    /**
     * @brief Task type for coroutine-based async operations
     * @tparam T Return type of the coroutine
     */
    export template<typename T>
    class Task {
    public:
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

    private:
        handle_type handle_;

    public:
        explicit Task(handle_type h) : handle_(h) {}
        ~Task() { if (handle_) handle_.destroy(); }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;
        Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (handle_) handle_.destroy();
                handle_ = std::exchange(other.handle_, {});
            }
            return *this;
        }

        T get() {
            if (!handle_) {
                throw std::runtime_error("Task handle is null");
            }
            handle_.resume();
            if (handle_.done()) {
                return handle_.promise().get_result();
            }
            throw std::runtime_error("Task not complete");
        }

        bool is_ready() const {
            return handle_ && handle_.done();
        }
    };

    template<typename T>
    struct Task<T>::promise_type {
        std::optional<T> result_;
        std::exception_ptr exception_;

        Task get_return_object() {
            return Task(handle_type::from_promise(*this));
        }

        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_value(T value) {
            result_ = std::move(value);
        }

        void unhandled_exception() {
            exception_ = std::current_exception();
        }

        T get_result() {
            if (exception_) {
                std::rethrow_exception(exception_);
            }
            return std::move(*result_);
        }
    };

    // Specialization for Task<void>
    export template<>
    class Task<void> {
    public:
        struct promise_type {
            std::exception_ptr exception_;

            Task<void> get_return_object() {
                return Task<void>(handle_type::from_promise(*this));
            }

            std::suspend_never initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            void return_void() {}

            void unhandled_exception() {
                exception_ = std::current_exception();
            }

            void get_result() {
                if (exception_) {
                    std::rethrow_exception(exception_);
                }
            }
        };

        using handle_type = std::coroutine_handle<promise_type>;

    private:
        handle_type handle_;

    public:
        explicit Task(handle_type h) : handle_(h) {}
        ~Task() { if (handle_) handle_.destroy(); }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;
        Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (handle_) handle_.destroy();
                handle_ = std::exchange(other.handle_, {});
            }
            return *this;
        }

        void get() {
            if (!handle_) {
                throw std::runtime_error("Task handle is null");
            }
            handle_.resume();
            if (handle_.done()) {
                return;
            }
            throw std::runtime_error("Task not complete");
        }

        bool is_ready() const {
            return handle_ && handle_.done();
        }
    };

    // ============================================================================
    // THREAD POOL
    // ============================================================================

    /**
     * @brief Simple thread pool for async task execution
     */
    export class ThreadPool {
    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        std::atomic<bool> stop_{false};

    public:
        explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
            for (size_t i = 0; i < num_threads; ++i) {
                workers_.emplace_back([this] {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex_);
                            condition_.wait(lock, [this] {
                                return stop_.load() || !tasks_.empty();
                            });
                            if (stop_.load() && tasks_.empty()) {
                                return;
                            }
                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }
                        task();
                    }
                });
            }
        }

        ~ThreadPool() {
            stop_.store(true);
            condition_.notify_all();
            for (auto& worker : workers_) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        void submit(std::function<void()> task) {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (stop_.load()) {
                    throw std::runtime_error("ThreadPool is stopped");
                }
                tasks_.emplace(std::move(task));
            }
            condition_.notify_one();
        }

        size_t get_thread_count() const {
            return workers_.size();
        }

        static ThreadPool& get_global_thread_pool() {
            static ThreadPool pool;
            return pool;
        }
    };

    // ============================================================================
    // ASYNC HELPERS
    // ============================================================================

    /**
     * @brief Schedule a coroutine on the thread pool
     */
    export struct schedule_on {
        ThreadPool& pool;

        explicit schedule_on(ThreadPool& p) : pool(p) {}

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> h) {
            pool.submit([h]() { h.resume(); });
        }

        void await_resume() const noexcept {}
    };

    /**
     * @brief Run a function as a coroutine task
     */
    export template<typename F>
    auto run_async(F&& func) -> Task<decltype(func())> {
        co_return func();
    }

    /**
     * @brief Sleep for a duration
     */
    export inline auto sleep_for(std::chrono::milliseconds duration) -> Task<void> {
        std::this_thread::sleep_for(duration);
        co_return;
    }

    /**
     * @brief Wait for all tasks to complete
     */
    export inline auto when_all(std::vector<Task<void>> tasks) -> Task<void> {
        for (auto& task : tasks) {
            task.get();
        }
        co_return;
    }

} // namespace perception
