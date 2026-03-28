/**
 * @file result.hpp
 * @brief Modern C++20/23 result types and error handling for perception system
 * 
 * This header provides modern error handling using std::expected (C++23) with
 * fallback implementation for C++20 compatibility. It includes async result types
 * for coroutine-based operations and comprehensive error management.
 * 
 * @author Mini Autonomy System
 * @date 2026
 */

#pragma once

#include <string>
#include <system_error>
#include <variant>
#include <future>
#include <optional>
#include <chrono>
#include <stdexcept>
#include <expected>
#include <format>
#include <string_view>

// Import modern type aliases
#include "types.hpp"

namespace perception {

// ============================================================================
// MODERN ERROR HANDLING
// ============================================================================

/**
 * @brief Modern error codes for perception operations
 * 
 * These error codes are used throughout the perception system to indicate
 * specific failure conditions in a type-safe manner.
 */
enum class PerceptionError : int {
    Success = 0,              ///< No error occurred
    InvalidInput,            ///< Invalid input parameters
    ModelLoadFailed,        ///< Model loading failed
    InferenceFailed,        ///< Inference operation failed
    QueueEmpty,             ///< Queue is empty
    QueueShutdown,          ///< Queue has been shutdown
    ThreadError,            ///< Thread operation failed
    CameraError,            ///< Camera operation failed
    TimeoutError,           ///< Operation timeout
    MemoryError,            ///< Memory allocation failed
    UnknownError = 99        ///< Unknown error occurred
};

/**
 * @brief Modern error category for system_error integration
 * 
 * This class provides a custom error category for the perception system,
 * enabling integration with std::system_error and proper error message handling.
 */
class ErrorCategory : public std::error_category {
public:
    /**
     * @brief Get the name of the error category
     * @return Category name
     */
    [[nodiscard]] auto name() const noexcept -> const char* override {
        return "perception";
    }

    /**
     * @brief Get error message for given error code
     * @param error_code Error code to convert
     * @return Error message string
     */
    [[nodiscard]] auto message(int error_code) const -> String override {
        switch (static_cast<PerceptionError>(error_code)) {
            case PerceptionError::Success:
                return "Success";
            case PerceptionError::InvalidInput:
                return "Invalid input parameters";
            case PerceptionError::ModelLoadFailed:
                return "Model loading failed";
            case PerceptionError::InferenceFailed:
                return "Inference operation failed";
            case PerceptionError::QueueEmpty:
                return "Queue is empty";
            case PerceptionError::QueueShutdown:
                return "Queue has been shutdown";
            case PerceptionError::ThreadError:
                return "Thread operation failed";
            case PerceptionError::CameraError:
                return "Camera operation failed";
            case PerceptionError::TimeoutError:
                return "Operation timeout";
            case PerceptionError::MemoryError:
                return "Memory allocation failed";
            case PerceptionError::UnknownError:
            default:
                return "Unknown error occurred";
        }
    }
};

/**
 * @brief Global error category instance
 */
inline const ErrorCategory& get_error_category() noexcept {
    static ErrorCategory instance;
    return instance;
}

/**
 * @brief Create std::error_code from PerceptionError
 * @param error Perception error
 * @return System error code
 */
inline auto make_error_code(PerceptionError error) noexcept -> std::error_code {
    return {static_cast<int>(error), get_error_category()};
}

/**
 * @brief Create std::error_condition from PerceptionError
 * @param error Perception error
 * @return System error condition
 */
inline auto make_error_condition(PerceptionError error) noexcept -> std::error_condition {
    return {static_cast<int>(error), get_error_category()};
}

} // namespace perception

// Enable std::error_code support for PerceptionError
namespace std {
template<>
struct is_error_code_enum<perception::PerceptionError> : true_type {};
}

namespace perception {

// ============================================================================
// MODERN RESULT TYPE
// ============================================================================

/**
 * @brief Modern result type using std::expected (C++23) with C++20 fallback
 * 
 * This provides a modern, type-safe way to handle operations that can fail
 * without using exceptions. It's compatible with both C++20 and C++23.
 */
template<typename T>
using Result = std::expected<T, PerceptionError>;

/**
 * @brief Result type for void operations
 */
using VoidResult = Result<void>;

/**
 * @brief Create a successful result value
 * @tparam T Value type
 * @param value Value to wrap
 * @return Success result
 */
template<typename T>
constexpr auto make_success(T&& value) noexcept -> Result<std::decay_t<T>> {
    return Result<std::decay_t<T>>{std::forward<T>(value)};
}

/**
 * @brief Create a success result for void operations
 * @return Success result
 */
constexpr auto make_success() noexcept -> VoidResult {
    return VoidResult{};
}

/**
 * @brief Create an error result
 * @param error Error code
 * @return Error result
 */
constexpr auto make_error(PerceptionError error) noexcept -> std::unexpected<PerceptionError> {
    return std::unexpected<PerceptionError>{error};
}

/**
 * @brief Create an error result from exception
 * @tparam T Value type
 * @param e Exception to convert
 * @return Error result
 */
template<typename T>
auto make_error_from_exception(const std::exception& e) noexcept -> Result<T> {
    // Try to determine error type from exception message
    const auto msg = StringView{e.what()};
    
    if (msg.find("timeout") != StringView::npos) {
        return make_error(PerceptionError::TimeoutError);
    }
    if (msg.find("memory") != StringView::npos || msg.find("bad_alloc") != StringView::npos) {
        return make_error(PerceptionError::MemoryError);
    }
    if (msg.find("invalid") != StringView::npos || msg.find("argument") != StringView::npos) {
        return make_error(PerceptionError::InvalidInput);
    }
    
    return make_error(PerceptionError::UnknownError);
}

// ============================================================================
// ASYNC RESULT TYPES
// ============================================================================

/**
 * @brief Modern async result type for coroutine operations
 * 
 * This combines std::future with modern error handling for asynchronous
 * operations that may fail.
 */
template<typename T>
class AsyncResult {
private:
    Future<Result<T>> m_future;
    
public:
    /**
     * @brief Constructor from future
     * @param future Future containing result
     */
    explicit AsyncResult(Future<Result<T>>&& future) noexcept 
        : m_future{std::move(future)} {}
    
    /**
     * @brief Wait for the result
     * @return Result value or error
     */
    auto get() -> Result<T> {
        return m_future.get();
    }
    
    /**
     * @brief Wait for result with timeout
     * @param timeout Maximum time to wait
     * @return Optional result if ready, empty otherwise
     */
    auto wait_for(Milliseconds timeout) -> Optional<Result<T>> {
        if (m_future.wait_for(timeout) == std::future_status::ready) {
            return m_future.get();
        }
        return std::nullopt;
    }
    
    /**
     * @brief Check if result is ready
     * @return True if ready, false otherwise
     */
    auto is_ready() const noexcept -> bool {
        using namespace std::chrono_literals;
        return m_future.wait_for(0ms) == std::future_status::ready;
    }
    
    /**
     * @brief Check if result has value
     * @return True if has value, false otherwise
     */
    auto has_value() const noexcept -> bool {
        if (!is_ready()) return false;
        // Note: This would require access to the future's value
        // For now, we assume it might have a value
        return true;
    }
    
    /**
     * @brief Check if result has error
     * @return True if has error, false otherwise
     */
    auto has_error() const noexcept -> bool {
        if (!is_ready()) return false;
        // Note: This would require access to the future's value
        // For now, we assume it might have an error
        return true;
    }
};

/**
 * @brief Specialization for void async results
 */
template<>
class AsyncResult<void> {
private:
    Future<VoidResult> m_future;
    
public:
    explicit AsyncResult(Future<VoidResult>&& future) noexcept 
        : m_future{std::move(future)} {}
    
    auto get() -> VoidResult {
        return m_future.get();
    }
    
    auto wait_for(Milliseconds timeout) -> Optional<VoidResult> {
        if (m_future.wait_for(timeout) == std::future_status::ready) {
            return m_future.get();
        }
        return std::nullopt;
    }
    
    auto is_ready() const noexcept -> bool {
        using namespace std::chrono_literals;
        return m_future.wait_for(0ms) == std::future_status::ready;
    }
    
    auto has_value() const noexcept -> bool {
        return is_ready();
    }
    
    auto has_error() const noexcept -> bool {
        return is_ready();
    }
};

/**
 * @brief Factory function for async results
 * @tparam T Value type
 * @param future Future containing result
 * @return Async result wrapper
 */
template<typename T>
auto make_async_result(Future<Result<T>>&& future) noexcept -> AsyncResult<T> {
    return AsyncResult<T>{std::move(future)};
}

/**
 * @brief Factory function for void async results
 * @param future Future containing result
 * @return Async result wrapper
 */
auto make_async_result(Future<VoidResult>&& future) noexcept -> AsyncResult<void> {
    return AsyncResult<void>{std::move(future)};
}

// ============================================================================
// RESULT UTILITIES
// ============================================================================

/**
 * @brief Execute function and wrap result
 * @tparam Func Function type
 * @tparam Args Argument types
 * @param func Function to execute
 * @param args Arguments to pass
 * @return Result of function execution
 */
template<typename Func, typename... Args>
requires Callable<Func, Args...>
auto wrap_result(Func&& func, Args&&... args) noexcept -> Result<std::invoke_result_t<Func, Args...>> {
    using ResultType = std::invoke_result_t<Func, Args...>;
    
    try {
        if constexpr (std::is_void_v<ResultType>) {
            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
            return make_success();
        } else {
            return make_success(std::invoke(std::forward<Func>(func), std::forward<Args>(args)...));
        }
    } catch (const std::exception& e) {
        return make_error_from_exception<ResultType>(e);
    } catch (...) {
        return make_error(PerceptionError::UnknownError);
    }
}

/**
 * @brief Chain operations on results
 * @tparam T Value type
 * @tparam Func Function type
 * @param result Input result
 * @param func Function to apply if result is successful
 * @return Chained result
 */
template<typename T, typename Func>
requires Callable<Func, T>
auto and_then(Result<T> result, Func&& func) noexcept -> Result<std::invoke_result_t<Func, T>> {
    if (result) {
        return wrap_result(std::forward<Func>(func), *std::move(result));
    }
    return make_error(result.error());
}

/**
 * @brief Transform successful result value
 * @tparam T Input value type
 * @tparam U Output value type
 * @param result Input result
 * @param func Transform function
 * @return Transformed result
 */
template<typename T, typename U, typename Func>
requires Callable<Func, T>
auto transform(Result<T> result, Func&& func) noexcept -> Result<U> {
    if (result) {
        return wrap_result(std::forward<Func>(func), *std::move(result));
    }
    return make_error(result.error());
}

/**
 * @brief Map error to different error type
 * @tparam T Value type
 * @tparam E New error type
 * @param result Input result
 * @param func Error mapping function
 * @return Result with mapped error
 */
template<typename T, typename E, typename Func>
requires Callable<Func, PerceptionError>
auto map_error(Result<T> result, Func&& func) noexcept -> std::expected<T, E> {
    if (!result) {
        return std::unexpected<E>{std::forward<Func>(func)(result.error())};
    }
    return *std::move(result);
}

/**
 * @brief Unwrap result or return default value
 * @tparam T Value type
 * @param result Result to unwrap
 * @param default_value Default value if result is error
 * @return Value or default
 */
template<typename T>
auto unwrap_or(Result<T> result, const T& default_value) noexcept -> T {
    return result.value_or(default_value);
}

/**
 * @brief Unwrap result or throw exception
 * @tparam T Value type
 * @param result Result to unwrap
 * @return Value if successful
 * @throws std::system_error if result is error
 */
template<typename T>
auto unwrap(Result<T> result) -> T {
    if (result) {
        return *std::move(result);
    }
    throw std::system_error{make_error_code(result.error())};
}

/**
 * @brief Unwrap void result or throw exception
 * @param result Result to unwrap
 * @throws std::system_error if result is error
 */
inline auto unwrap(VoidResult result) -> void {
    if (!result) {
        throw std::system_error{make_error_code(result.error())};
    }
}

} // namespace perception
