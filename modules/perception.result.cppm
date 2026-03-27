module;

#include <string>
#include <system_error>
#include <variant>
#include <future>
#include <optional>
#include <chrono>
#include <stdexcept>

export module perception.result;

export namespace perception {

    // Error types for the perception system
    enum class PerceptionError {
        Success = 0,
        InvalidInput,
        ModelLoadFailed,
        InferenceFailed,
        QueueEmpty,
        QueueShutdown,
        ThreadError,
        CameraError,
        IMUError,
        TrackerError,
        FusionError
    };

    // Error category for system_error integration
    class PerceptionErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override {
            return "perception_error";
        }

        std::string message(int ev) const override {
            switch (static_cast<PerceptionError>(ev)) {
                case PerceptionError::Success:
                    return "Success";
                case PerceptionError::InvalidInput:
                    return "Invalid input provided";
                case PerceptionError::ModelLoadFailed:
                    return "Failed to load model";
                case PerceptionError::InferenceFailed:
                    return "Inference failed";
                case PerceptionError::QueueEmpty:
                    return "Queue is empty";
                case PerceptionError::QueueShutdown:
                    return "Queue has been shutdown";
                case PerceptionError::ThreadError:
                    return "Thread operation failed";
                case PerceptionError::CameraError:
                    return "Camera operation failed";
                case PerceptionError::IMUError:
                    return "IMU operation failed";
                case PerceptionError::TrackerError:
                    return "Tracker operation failed";
                case PerceptionError::FusionError:
                    return "Fusion operation failed";
                default:
                    return "Unknown error";
            }
        }
    };

    inline const PerceptionErrorCategory& get_perception_error_category() {
        static PerceptionErrorCategory instance;
        return instance;
    }

    // Make PerceptionError work with std::error_code
    inline std::error_code make_error_code(PerceptionError e) {
        return {static_cast<int>(e), get_perception_error_category()};
    }

} // namespace perception

// Enable std::error_code support
template<>
struct std::is_error_code_enum<perception::PerceptionError> : std::true_type {};

export namespace perception {

    // Simple result type for C++20 (instead of std::expected)
    template<typename T>
    class Result {
    private:
        std::variant<T, std::error_code> m_value;
        
    public:
        Result() = default;
        Result(T&& value) : m_value(std::move(value)) {}
        Result(const T& value) : m_value(value) {}
        Result(std::error_code error) : m_value(error) {}
        
        bool has_value() const noexcept {
            return m_value.index() == 0;
        }
        
        T& value() {
            if (!has_value()) {
                throw std::runtime_error("Result does not contain a value");
            }
            return std::get<T>(m_value);
        }
        
        const T& value() const {
            if (!has_value()) {
                throw std::runtime_error("Result does not contain a value");
            }
            return std::get<T>(m_value);
        }
        
        std::error_code error() const {
            if (has_value()) {
                return {};
            }
            return std::get<std::error_code>(m_value);
        }
        
        explicit operator bool() const noexcept {
            return has_value();
        }
    };

    // Specialization for void
    template<>
    class Result<void> {
    private:
        std::optional<std::error_code> m_error;
        
    public:
        Result() : m_error(std::nullopt) {}
        Result(std::error_code error) : m_error(error) {}
        
        bool has_value() const noexcept {
            return !m_error.has_value();
        }
        
        std::error_code error() const {
            return m_error.value_or(std::error_code{});
        }
        
        explicit operator bool() const noexcept {
            return has_value();
        }
    };

    // Type aliases
    template<typename T>
    using PerceptionResult = Result<T>;

    // Helper functions for creating results
    template<typename T>
    constexpr PerceptionResult<T> success(T&& value) noexcept {
        return PerceptionResult<T>(std::forward<T>(value));
    }

    inline PerceptionResult<void> success() noexcept {
        return PerceptionResult<void>();
    }

    template<typename T>
    constexpr PerceptionResult<T> error(PerceptionError err) noexcept {
        return PerceptionResult<T>(make_error_code(err));
    }

    // Async result type
    template<typename T>
    class AsyncResult {
    private:
        std::future<PerceptionResult<T>> m_future;
        
    public:
        explicit AsyncResult(std::future<PerceptionResult<T>>&& fut) 
            : m_future(std::move(fut)) {}

        // Wait for result
        PerceptionResult<T> get() {
            return m_future.get();
        }

        // Check if ready
        bool is_ready() const {
            return m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        // Wait with timeout
        std::optional<PerceptionResult<T>> wait_for(std::chrono::milliseconds timeout) {
            if (m_future.wait_for(timeout) == std::future_status::ready) {
                return m_future.get();
            }
            return std::nullopt;
        }
    };

}
