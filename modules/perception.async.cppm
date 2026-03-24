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

        // Disable copy/move to prevent logic errors with the pool
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        template<typename F>
        void enqueue(F&& f) {
            {
                std::lock_guard lock(mutex_);
                tasks_.emplace(std::forward<F>(f));
            }
            condition_.notify_one();
        }

    private:
        std::vector<std::jthread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable_any condition_;
    };

    // Global accessor for the shared thread pool
    inline ThreadPool& get_global_thread_pool() {
        static ThreadPool pool;
        return pool;
    }

    // Awaiter to switch execution to the thread pool
    struct ThreadPoolAwaiter {
        ThreadPool& pool_;
        constexpr bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) const {
            pool_.enqueue([h]() mutable { h.resume(); });
        }
        constexpr void await_resume() const noexcept {}
    };

    inline auto schedule_on(ThreadPool& pool) {
        return ThreadPoolAwaiter{pool};
    }

    template<typename T>
    class Task {
    public:
        struct promise_type;
        using value_type = T;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type {
            T value_;
            std::exception_ptr exception_;
            std::coroutine_handle<> continuation_;

            Task get_return_object() {
                return Task(handle_type::from_promise(*this));
            }

            std::suspend_always initial_suspend() noexcept { return {}; }
            
            struct FinalAwaiter {
                bool await_ready() const noexcept { return false; }
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    auto& promise = h.promise();
                    return promise.continuation_ ? promise.continuation_ : std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            
            FinalAwaiter final_suspend() noexcept { return {}; }

            void unhandled_exception() { exception_ = std::current_exception(); }

            template<std::convertible_to<T> From>
            void return_value(From&& from) {
                value_ = std::forward<From>(from);
            }
        };

        explicit Task(handle_type h) : h_(h) {}
        
        ~Task() { 
            if (h_) h_.destroy(); 
        }

        // Non-copyable, movable
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;
        
        Task(Task&& other) noexcept : h_(other.h_) { 
            other.h_ = nullptr; 
        }
        
        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (h_) h_.destroy();
                h_ = other.h_;
                other.h_ = nullptr;
            }
            return *this;
        }

        // Awaiter support
        bool await_ready() const noexcept { 
            return !h_ || h_.done(); 
        }

        void await_suspend(std::coroutine_handle<> awaiting_h) {
            h_.promise().continuation_ = awaiting_h;
            h_.resume();
        }

        T await_resume() {
            if (h_.promise().exception_) {
                std::rethrow_exception(h_.promise().exception_);
            }
            return std::move(h_.promise().value_);
        }

        // Result access for synchronous waiting
        T get() {
            if (!h_.done()) {
                h_.resume();
            }
            // Note: simple get() works for sync wait but might deadlock if task suspends for thread pool
            if (h_.promise().exception_) {
                std::rethrow_exception(h_.promise().exception_);
            }
            return std::move(h_.promise().value_);
        }

        // Manual control for integration
        void resume() {
            if (h_ && !h_.done()) h_.resume();
        }

        bool is_done() const noexcept {
            return !h_ || h_.done();
        }

    private:
        handle_type h_;
    };

    // Specialization for void
    template<>
    struct Task<void>::promise_type {
        std::exception_ptr exception_;

        Task get_return_object() {
            return Task(handle_type::from_promise(*this));
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception_ = std::current_exception(); }
        void return_void() {}
    };

    // Helper to synchronously wait for a task (bridge to sync code)
    template<typename T>
    T sync_wait(Task<T> task) {
        std::promise<T> p;
        auto fut = p.get_future();
        
        // Wrapper coroutine to drive the task
        auto driver = [](Task<T> t, std::promise<T> p) -> Task<void> {
            try {
                if constexpr (std::is_void_v<T>) {
                    co_await t;
                    p.set_value();
                } else {
                    p.set_value(co_await t);
                }
            } catch (...) {
                p.set_exception(std::current_exception());
            }
        }(std::move(task), std::move(p));
        
        // Kick off the driver
        driver.resume();
        
        return fut.get();
    }

    // Spawn a task asynchronously and get a standard future (Bridge to Legacy/Main)
    template<typename T>
    std::future<T> spawn(Task<T> task) {
        return std::async(std::launch::async, [t = std::move(task)]() mutable {
            return sync_wait(std::move(t));
        });
    }

    // Run multiple tasks in parallel and wait for all to complete
    template <typename T>
    Task<std::vector<T>> when_all(std::vector<Task<T>> tasks) {
        if (tasks.empty()) co_return {};
        
        struct State {
            std::atomic<size_t> count;
            std::vector<T> results;
            std::coroutine_handle<> continuation;
            
            State(size_t n) : count(n), results(n) {}
        };
        
        auto state = std::make_shared<State>(tasks.size());
        
        // Helper to run a task and store result
        auto waiter = [](Task<T> t, std::shared_ptr<State> s, size_t idx) -> Task<void> {
            try {
                s->results[idx] = co_await t;
            } catch (...) {
                // In production, store exception to propagate
            }
            // Decrement count, if 0 resume continuation
            if (s->count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                if (s->continuation) s->continuation.resume();
            }
        };
        
        // Start all tasks
        for (size_t i = 0; i < tasks.size(); ++i) {
            auto w = waiter(std::move(tasks[i]), state, i);
            w.resume(); // Start execution (will suspend at first await if async)
        }
        
        // Suspend caller until all done
        struct Awaiter {
            std::shared_ptr<State> s;
            bool await_ready() { return s->count.load(std::memory_order_acquire) == 0; }
            void await_suspend(std::coroutine_handle<> h) {
                s->continuation = h;
                // Handle race condition: check if finished while suspending
                if (s->count.load(std::memory_order_acquire) == 0) h.resume();
            }
            std::vector<T> await_resume() { return std::move(s->results); }
        };
        
        co_return co_await Awaiter{state};
    }

    // Run multiple tasks in parallel and return the first one that completes
    template <typename T>
    Task<T> when_any(std::vector<Task<T>> tasks) {
        if (tasks.empty()) throw std::runtime_error("when_any: no tasks");

        struct State {
            std::atomic<bool> done{false};
            std::optional<T> result;
            std::exception_ptr exception;
            std::coroutine_handle<> continuation;
        };
        
        auto state = std::make_shared<State>();
        
        // Helper coroutine to await a task and report completion
        auto waiter = [](Task<T> t, std::shared_ptr<State> s) -> Task<void> {
            try {
                T val = co_await t;
                // Try to set done flag (only first one succeeds)
                if (!s->done.exchange(true, std::memory_order_acq_rel)) {
                    s->result = std::move(val);
                    if (s->continuation) s->continuation.resume();
                }
            } catch (...) {
                if (!s->done.exchange(true, std::memory_order_acq_rel)) {
                    s->exception = std::current_exception();
                    if (s->continuation) s->continuation.resume();
                }
            }
        };
        
        // Keep tasks alive in this frame
        std::vector<Task<void>> in_flight;
        in_flight.reserve(tasks.size());
        
        for (auto& t : tasks) {
            auto w = waiter(std::move(t), state);
            w.resume();
            in_flight.push_back(std::move(w));
        }
        
        struct Awaiter {
            std::shared_ptr<State> s;
            bool await_ready() { return s->done.load(std::memory_order_acquire); }
            void await_suspend(std::coroutine_handle<> h) {
                s->continuation = h;
                // Handle race condition: check if finished while suspending
                if (s->done.load(std::memory_order_acquire)) h.resume();
            }
            T await_resume() {
                if (s->exception) std::rethrow_exception(s->exception);
                return std::move(*s->result);
            }
        };
        
        co_return co_await Awaiter{state};
    }

}