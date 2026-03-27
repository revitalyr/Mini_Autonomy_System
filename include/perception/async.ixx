module;

#include <coroutine>
#include <exception>
#include <variant>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>
#include <future>
#include <tuple>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <stop_token>

export module perception.async;

export namespace perception {

    class ThreadPool {
    public:
        explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
            for (size_t i = 0; i < num_threads; ++i) {
                workers_.emplace_back([this](std::stop_token st) {
                    while (!st.stop_requested()) {
                        std::function<void()> task;
                        {
                            std::unique_lock lock(mutex_);
                            // Wait for task or stop request
                            if (!condition_.wait(lock, st, [this] { return !tasks_.empty(); })) {
                                return; // Stop requested
                            }
                            
                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }
                        task();
                    }
                });
            }
        }

        ~ThreadPool() = default; // jthreads automatically join and request stop

        // Disable copy/move to prevent logic errors with pool
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        // Modern async task submission with C++23 features
        template<typename Func, typename... Args>
        requires std::invocable<Func, Args...>
        auto submit(Func&& func, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>> {
            using ReturnType = std::invoke_result_t<Func, Args...>;
            
            auto task = [f = std::forward<Func>(func), ... a = std::forward<Args>(args)]() mutable -> ReturnType {
                return std::invoke(f, a...);
            };
            
            auto promise = std::make_shared<std::promise<ReturnType>>();
            auto future = promise->get_future();
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                tasks_.emplace([promise, task = std::move(task)]() mutable {
                    try {
                        if constexpr (std::is_void_v<ReturnType>) {
                            task();
                            promise->set_value();
                        } else {
                            promise->set_value(task());
                        }
                    } catch (...) {
                        promise->set_exception(std::current_exception());
                    }
                });
                condition_.notify_one();
            }
            
            return future;
        }

        // Wait for all tasks to complete
        void wait_for_all() {
            // Implementation would wait for all pending tasks
            // This is a simplified version
        }

        size_t get_pending_count() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return tasks_.size();
        }

    private:
        std::vector<std::jthread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable_any condition_;
    };

    // Modern async utilities with C++23
    template<typename T>
    class AsyncValue {
    private:
        std::future<T> future_;
        
    public:
        template<typename... Args>
        explicit AsyncValue(Args&&... args) 
            : future_(std::async(std::launch::async, 
                std::forward<Args>(args)...)) {}
        
        // C++23: awaitable interface
        bool await_ready() const noexcept {
            return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }
        
        void await_suspend(std::coroutine_handle<> handle) const {
            // Simple implementation - in real code would be more complex
            while (!await_ready()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            handle.resume();
        }
        
        T await_resume() {
            return future_.get();
        }
    };
}
