module;

#include <expected>
#include <string>
#include <system_error>
#include <variant>

export module perception.result;

export namespace perception {

    // Error types for perception system
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

    // Global error category instance
    inline const PerceptionErrorCategory& get_perception_error_category() noexcept {
        static PerceptionErrorCategory instance;
        return instance;
    }

    // Create std::error_code from PerceptionError
    inline std::error_code make_error_code(PerceptionError e) noexcept {
        return { static_cast<int>(e), get_perception_error_category() };
    }

    // Modern expected type aliases for convenience
    template<typename T>
    using PerceptionResult = std::expected<T, std::error_code>;

    // Helper functions for creating results
    template<typename T>
    constexpr PerceptionResult<T> success(T&& value) noexcept {
        return std::expected<T, std::error_code>{std::forward<T>(value)};
    }

    constexpr PerceptionResult<void> success() noexcept {
        return std::expected<void, std::error_code>{};
    }

    template<typename T>
    constexpr PerceptionResult<T> error(PerceptionError e) noexcept {
        return std::unexpected(make_error_code(e));
    }

    // Common result creators
    template<typename T>
    constexpr PerceptionResult<T> invalid_input() noexcept {
        return error<T>(PerceptionError::InvalidInput);
    }

    template<typename T>
    constexpr PerceptionResult<T> model_load_failed() noexcept {
        return error<T>(PerceptionError::ModelLoadFailed);
    }

    template<typename T>
    constexpr PerceptionResult<T> inference_failed() noexcept {
        return error<T>(PerceptionError::InferenceFailed);
    }

    template<typename T>
    constexpr PerceptionResult<T> queue_empty() noexcept {
        return error<T>(PerceptionError::QueueEmpty);
    }

    template<typename T>
    constexpr PerceptionResult<T> queue_shutdown() noexcept {
        return error<T>(PerceptionError::QueueShutdown);
    }
}
