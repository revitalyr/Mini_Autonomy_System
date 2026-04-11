#pragma once

#include <string>
#include <system_error>
#include <variant>
#include <future>
#include <optional>
#include <chrono>
#include <stdexcept>

/**
 * @file perception.result.hpp
 * @brief Result types and error handling for the perception system
 *
 * This header provides custom Result<T> type for error handling as a replacement
 * for std::expected (C++23) to maintain C++20 compatibility. It also includes
 * async result types for coroutine-based operations.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

namespace perception {

    /**
     * @brief Error codes for perception operations
     * 
     * These error codes are used throughout the perception system to indicate
     * specific failure conditions in a type-safe manner.
     */
    enum class PerceptionError {
        Success = 0,              ///< No error occurred
        InvalidInput,            ///< Invalid input parameters
        ModelLoadFailed,        ///< Model loading failed
        InferenceFailed,        ///< Inference operation failed
        QueueEmpty,             ///< Queue is empty
        QueueShutdown,          ///< Queue has been shutdown
        ThreadError,            ///< Thread operation failed
        CameraError,            ///< Camera operation failed
        IMUError,               ///< IMU operation failed
        TrackerError,           ///< Tracker operation failed
        FusionError             ///< Fusion operation failed
    };

    /**
     * @brief Error category for system_error integration
     * 
     * This class provides a custom error category for the perception system,
     * allowing for seamless integration with std::error_code and std::system_error.
     */
    class PerceptionErrorCategory : public std::error_category {
    public:
        /**
         * @brief Get the name of the error category
         * @return The name of the error category
         */
        const char* name() const noexcept override {
            return "perception_error";
        }

        /**
         * @brief Get the error message for a specific error code
         * @param ev The error code
         * @return The error message
         */
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

    /**
     * @brief Get the perception error category instance
     * @return The perception error category instance
     */
    inline const PerceptionErrorCategory& get_perception_error_category() {
        static PerceptionErrorCategory instance;
        return instance;
    }

    /**
     * @brief Make PerceptionError work with std::error_code
     * @param e The PerceptionError code
     * @return The corresponding std::error_code
     */
    inline std::error_code make_error_code(PerceptionError e) {
        return {static_cast<int>(e), get_perception_error_category()};
    }

} // namespace perception

// Enable std::error_code support
template<>
struct std::is_error_code_enum<perception::PerceptionError> : std::true_type {};

namespace perception {

    /**
     * @brief Custom result type for C++20 compatibility
     * 
     * This class provides std::expected-like functionality for C++20,
     * allowing functions to return either a value or an error code.
     * It uses std::variant to store either the successful result or an error.
     * 
     * @tparam T Type of the successful result value
     */
    template<typename T>
    class Result {
    private:
        std::variant<T, std::error_code> m_value;
        
    public:
        /**
         * @brief Default constructor
         */
        Result() = default;
        
        /**
         * @brief Construct a successful result from a value
         * @param value The successful result value
         */
        Result(T&& value) : m_value(std::move(value)) {}
        
        /**
         * @brief Construct a successful result from a value (copy)
         * @param value The successful result value
         */
        Result(const T& value) : m_value(value) {}
        
        /**
         * @brief Construct an error result
         * @param error The error code
         */
        Result(std::error_code error) : m_value(error) {}
        
        /**
         * @brief Check if the result contains a value
         * @return true if successful, false if error
         */
        bool has_value() const noexcept {
            return m_value.index() == 0;
        }
        
        /**
         * @brief Get the result value
         * @return Reference to the stored value
         * @throws std::runtime_error if result contains an error
         */
        T& value() {
            if (!has_value()) {
                throw std::runtime_error("Result does not contain a value");
            }
            return std::get<T>(m_value);
        }
        
        /**
         * @brief Get the result value (const version)
         * @return Const reference to the stored value
         * @throws std::runtime_error if result contains an error
         */
        const T& value() const {
            if (!has_value()) {
                throw std::runtime_error("Result does not contain a value");
            }
            return std::get<T>(m_value);
        }
        
        /**
         * @brief Get the error code
         * @return The stored error code
         * @throws std::runtime_error if result contains a value
         */
        std::error_code error() const {
            if (has_value()) {
                throw std::runtime_error("Result does not contain an error");
            }
            return std::get<std::error_code>(m_value);
        }
        
        /**
         * @brief Conversion to bool for easy checking
         * @return true if successful, false if error
         */
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

    // C++20-compatible Expected type alias
    template<typename T, typename E>
    using Expected = Result<T>;

    template<typename E>
    using Unexpected = E;

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

    template<typename E>
    auto make_unexpected(E e) -> Result<void> {
        return error<void>(static_cast<PerceptionError>(e));
    }

    template<typename T, typename E>
    auto make_unexpected_typed(E e) -> Result<T> {
        return error<T>(static_cast<PerceptionError>(e));
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
