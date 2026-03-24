module;

#include <expected>
#include <string>
#include <system_error>
#include <variant>

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

    // Modern result type using std::expected
    template<typename T>
    using Result = std::expected<T, std::error_code>;

    // Helper functions for creating results
    template<typename T>
    constexpr Result<T> success(T&& value) {
        return Result<T>{std::forward<T>(value)};
    }

    template<typename T>
    constexpr Result<T> error(PerceptionError err) {
        return std::unexpected(make_error_code(err));
    }

    // Monadic operations for Result
    template<typename T, typename F>
    auto map(Result<T> result, F&& func) 
        -> Result<decltype(func(*std::move(result)))> {
        
        if (result) {
            return success(func(*std::move(result)));
        }
        return std::unexpected(result.error());
    }

    template<typename T, typename F>
    auto and_then(Result<T> result, F&& func) 
        -> decltype(func(*std::move(result))) {
        
        if (result) {
            return func(*std::move(result));
        }
        return std::unexpected(result.error());
    }

    // Async result type
    template<typename T>
    class AsyncResult {
    private:
        std::future<Result<T>> future_;
        
    public:
        explicit AsyncResult(std::future<Result<T>>&& fut) 
            : future_(std::move(fut)) {}

        // Wait for result
        Result<T> get() {
            return future_.get();
        }

        // Check if ready
        bool is_ready() const {
            return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        // Wait with timeout
        std::optional<Result<T>> wait_for(std::chrono::milliseconds timeout) {
            if (future_.wait_for(timeout) == std::future_status::ready) {
                return future_.get();
            }
            return std::nullopt;
        }
    };

}
