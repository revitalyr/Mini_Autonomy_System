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

export module perception.async;

export namespace perception {

    // --- ThreadPool ---
    class ThreadPool {
    public:
        explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
            for (size_t i = 0; i < num_threads; ++i) {
                workers_.emplace_back([this](std::stop_token st) {
                    while (!st.stop_requested()) {
                        std::function<void()> task;
                        {
                            std::unique_lock lock(mutex_);
                            condition_.wait(lock, st, [this] { return !tasks_.empty(); });
                            if (st.stop_requested() && tasks_.empty()) return;
                            
                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }
                        task();
                    }
                });
            }
        }

        void enqueue(std::function<void()> f) {
            {
                std::unique_lock lock(mutex_);
                tasks_.push(std::move(f));
            }
            condition_.notify_one();
        }

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

    private:
        std::vector<std::jthread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable_any condition_;
    };

    // Глобальный пул потоков
    export ThreadPool& get_global_thread_pool() {
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
    export auto schedule_on(ThreadPool& pool) {
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

    export std::future<void> run_async(Task<void> task) {
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
    export Task<void> when_all(std::vector<Task<void>> tasks) {
        for (auto& t : tasks) {
            co_await t;
        }
        co_return;
    }

    export Task<void> sleep_for(std::chrono::milliseconds ms) {
        std::this_thread::sleep_for(ms);
        co_return;
    }
}