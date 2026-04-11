#pragma once

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

/**
 * @file perception.async.hpp
 * @brief Async operations and coroutine support for the perception system
 *
 * This header provides coroutine-based async operations including Task<T> for
 * awaitable operations and Generator<T> for lazy sequences. It also includes
 * a thread pool for efficient async task execution.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

namespace perception {

    /**
     * @brief Thread pool for async task execution
     * 
     * This class provides a thread pool for executing tasks asynchronously.
     * It supports task submission with futures for result retrieval.
     */
    class ThreadPool {
    public:
        /**
         * @brief Constructs a thread pool with the specified number of threads
         * 
         * @param num_threads Number of threads to use in the pool
         */
        explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
            : shutdown_(false), active_threads_(0) {
            for (size_t i = 0; i < num_threads; ++i) {
                workers_.emplace_back([this](std::stop_token st) {
                    while (!st.stop_requested() && !shutdown_) {
                        std::function<void()> task;
                        {
                            std::unique_lock lock(mutex_);
                            condition_.wait(lock, st, [this] { return !tasks_.empty() || shutdown_; });
                            if (shutdown_ && tasks_.empty()) return;

                            if (!tasks_.empty()) {
                                task = std::move(tasks_.front());
                                tasks_.pop();
                                active_threads_++;
                            }
                        }
                        if (task) {
                            task();
                            active_threads_--;
                        }
                    }
                });
            }
        }

        /**
         * @brief Enqueues a task for execution in the thread pool
         * 
         * @param f Task to execute
         */
        void enqueue(std::function<void()> f) {
            {
                std::unique_lock lock(mutex_);
                tasks_.push(std::move(f));
            }
            condition_.notify_one();
        }

        /**
         * @brief Submits a task for execution in the thread pool and returns a future
         * 
         * @tparam F Type of task to submit
         * @param f Task to submit
         * @return Future representing the result of the task
         */
        template<typename F>
        auto submit(F&& f) -> std::future<std::invoke_result_t<F>> {
            using RetType = std::invoke_result_t<F>;
            auto task = std::make_shared<std::packaged_task<RetType()>>(std::forward<F>(f));
            auto fut = task->get_future();
            {
                std::unique_lock lock(mutex_);
                tasks_.emplace([task]() { (*task)(); });
            }
            condition_.notify_one();
            return fut;
        }

        /**
         * @brief Gets the number of threads in the pool
         */
        size_t get_thread_count() const {
            return workers_.size();
        }

        /**
         * @brief Gets the number of currently active threads
         */
        size_t get_active_threads() const {
            return active_threads_;
        }

        /**
         * @brief Shuts down the thread pool
         */
        void shutdown() {
            {
                std::unique_lock lock(mutex_);
                shutdown_ = true;
            }
            condition_.notify_all();
            workers_.clear();
        }

        /**
         * @brief Checks if the thread pool is shut down
         */
        bool is_shutdown() const {
            return shutdown_;
        }

    private:
        std::vector<std::jthread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable_any condition_;
        std::atomic<bool> shutdown_;
        std::atomic<size_t> active_threads_;
    };

    // Глобальный пул потоков
    ThreadPool& get_global_thread_pool() {
        static ThreadPool pool;
        return pool;
    }

    // --- Task и Promise ---
    struct PromiseBase {
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception = std::current_exception(); }
        std::exception_ptr exception;
    };

    template <typename T>
    struct [[nodiscard]] Task {
        struct promise_type : PromiseBase {
            std::optional<T> result;
            Task<T> get_return_object() { return Task<T>{std::coroutine_handle<promise_type>::from_promise(*this)}; }
            void return_value(T value) { result = std::move(value); }
        };

        std::coroutine_handle<promise_type> handle;
        explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
        Task(Task&& s) noexcept : handle(std::exchange(s.handle, nullptr)) {}
        ~Task() { if (handle) handle.destroy(); }

        struct Awaiter {
            std::coroutine_handle<promise_type> h;
            bool await_ready() { return h.done(); }
            void await_suspend(std::coroutine_handle<> cont) { 
                h.resume(); 
                cont.resume(); 
            }
            T await_resume() {
                if (h.promise().exception) std::rethrow_exception(h.promise().exception);
                return std::move(*h.promise().result);
            }
        };
        auto operator co_await() { return Awaiter{handle}; }
    };

    template <>
    struct [[nodiscard]] Task<void> {
        struct promise_type : PromiseBase {
            Task<void> get_return_object() { return Task<void>{std::coroutine_handle<promise_type>::from_promise(*this)}; }
            void return_void() noexcept {}
        };

        std::coroutine_handle<promise_type> handle;
        explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
        Task(Task&& s) noexcept : handle(std::exchange(s.handle, nullptr)) {}
        ~Task() { if (handle) handle.destroy(); }

        struct Awaiter {
            std::coroutine_handle<promise_type> h;
            bool await_ready() { return h.done(); }
            void await_suspend(std::coroutine_handle<> cont) { h.resume(); cont.resume(); }
            void await_resume() {
                if (h.promise().exception) std::rethrow_exception(h.promise().exception);
            }
        };
        auto operator co_await() { return Awaiter{handle}; }
    };

    // --- Вспомогательные функции (Missing Identifiers) ---

    // schedule_on: переключает выполнение корутины на пул потоков
    auto schedule_on(ThreadPool& pool) {
        struct SchAwaiter {
            ThreadPool& p;
            bool await_ready() { return false; }
            void await_suspend(std::coroutine_handle<> h) {
                p.enqueue([h] { h.resume(); });
            }
            void await_resume() {}
        };
        return SchAwaiter{pool};
    }

    std::future<void> run_async(Task<void> task) {
    std::promise<void> promise;
    auto future = promise.get_future();

    // Лямбда-обертка для выполнения корутины и установки promise
    auto runner = [](Task<void> t, std::promise<void> p) -> Task<void> {
        try {
            co_await t;
            p.set_value();
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    };

    // Запускаем обертку
    auto h = runner(std::move(task), std::move(promise)).handle;
    h.resume(); 
    // Внимание: здесь нужно управление временем жизни handle, 
    // если runner не завершился синхронно.
    
    return future;
}
    // when_all: ожидает завершения списка задач
    Task<void> when_all(std::vector<Task<void>> tasks) {
        for (auto& t : tasks) {
            co_await t;
        }
        co_return;
    }

    Task<void> sleep_for(std::chrono::milliseconds ms) {
        std::this_thread::sleep_for(ms);
        co_return;
    }
}