module;
#include <coroutine>
#include <utility>
#include <iterator>
#include <exception>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

export module perception.async;
import perception.types;

export namespace perception {

    /**
     * @brief Coroutine generator for lazy sequence streaming
     */
    template<typename T>
    struct Generator {
        struct promise_type {
            T m_value{};
            auto get_return_object() -> Generator {
                return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            static auto initial_suspend() -> std::suspend_always { return {}; }
            static auto final_suspend() noexcept -> std::suspend_always { return {}; }
            auto yield_value(T value) -> std::suspend_always {
                m_value = std::move(value);
                return {};
            }
            void return_void() {}
            void unhandled_exception() { std::terminate(); }
        };

        std::coroutine_handle<promise_type> m_handle;

        explicit Generator(std::coroutine_handle<promise_type> handle) : m_handle{handle} {}
        ~Generator() { if (m_handle) m_handle.destroy(); }
        
        Generator(const Generator&) = delete;
        auto operator=(const Generator&) -> Generator& = delete;
        
        Generator(Generator&& other) noexcept : m_handle{std::exchange(other.m_handle, nullptr)} {}
        auto operator=(Generator&& other) noexcept -> Generator& {
            if (this != &other) {
                if (m_handle) m_handle.destroy();
                m_handle = std::exchange(other.m_handle, nullptr);
            }
            return *this;
        }

        struct Iterator {
            std::coroutine_handle<promise_type> m_handle;
            bool operator==(std::default_sentinel_t) const { return !m_handle || m_handle.done(); }
            auto operator++() -> void { if (m_handle) m_handle.resume(); }
            auto operator*() const -> const T& { return m_handle.promise().m_value; }
        };

        auto begin() -> Iterator {
            if (m_handle && !m_handle.done()) m_handle.resume();
            return Iterator{m_handle};
        }
        auto end() -> std::default_sentinel_t { return {}; }
    };

    /**
     * @brief Simple thread pool for asynchronous task execution
     */
    export class ThreadPool {
    public:
        explicit ThreadPool(Count thread_count) {
            for (Count i = 0; i < thread_count; ++i) {
                m_threads.emplace_back([this] {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock lock(m_mutex);
                            m_cv.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                            if (m_stop && m_tasks.empty()) return;
                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        ~ThreadPool() {
            {
                std::lock_guard lock(m_mutex);
                m_stop = true;
            }
            m_cv.notify_all();
            for (auto& t : m_threads) {
                if (t.joinable()) t.join();
            }
        }

        void submit(std::function<void()> task) {
            {
                std::lock_guard lock(m_mutex);
                m_tasks.push(std::move(task));
            }
            m_cv.notify_one();
        }

        auto get_thread_count() const noexcept -> Count { return m_threads.size(); }

    private:
        std::vector<std::thread> m_threads;
        std::queue<std::function<void()>> m_tasks;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_stop{false};
    };
}