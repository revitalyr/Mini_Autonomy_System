module;
#include <string>
#include <system_error>
#include <variant>
#include <future>
#include <optional>
#include <chrono>
#include <stdexcept>
#include "constants.h"

export module perception.result;

/**
 * @file perception.result.ixx
 * @brief Result types and error handling for the perception system
 *
 * This module provides custom Result<T> type for error handling as a replacement
 * for std::expected (C++23) to maintain C++20 compatibility. It also includes
 * async result types for coroutine-based operations.
 *
 * @author Mini Autonomy System
 * @date 2026
 */

namespace perception {

    /**
     * @brief Error codes for perception operations
     */
    export enum class PerceptionError {
        Success = 0,
        InvalidInput,
        ProcessingError,
        ThreadError,
        QueueEmpty,
        QueueShutdown,
        ResourceUnavailable,
        Timeout,
        UnknownError,
        FileNotFound,
        InvalidCalibrationData,
        IOError
    };

    /**
     * @brief Error category for PerceptionError
     */
    export class PerceptionErrorCategory : public std::error_category {
    public:
        const char* name() const noexcept override {
            return std::string(constants::errors::ERROR_CATEGORY_NAME).c_str();
        }

        std::string message(int ev) const override {
            switch (static_cast<PerceptionError>(ev)) {
                case PerceptionError::Success:
                    return std::string(constants::errors::MSG_SUCCESS);
                case PerceptionError::InvalidInput:
                    return std::string(constants::errors::MSG_INVALID_INPUT);
                case PerceptionError::ProcessingError:
                    return std::string(constants::errors::MSG_PROCESSING_ERROR);
                case PerceptionError::ThreadError:
                    return std::string(constants::errors::MSG_THREAD_ERROR);
                case PerceptionError::QueueEmpty:
                    return std::string(constants::errors::MSG_QUEUE_EMPTY);
                case PerceptionError::QueueShutdown:
                    return std::string(constants::errors::MSG_QUEUE_SHUTDOWN);
                case PerceptionError::ResourceUnavailable:
                    return std::string(constants::errors::MSG_RESOURCE_UNAVAILABLE);
                case PerceptionError::Timeout:
                    return std::string(constants::errors::MSG_TIMEOUT);
                case PerceptionError::FileNotFound:
                    return std::string(constants::errors::MSG_FILE_NOT_FOUND);
                case PerceptionError::InvalidCalibrationData:
                    return std::string(constants::errors::MSG_INVALID_CALIBRATION_DATA);
                case PerceptionError::IOError:
                    return std::string(constants::errors::MSG_IO_ERROR);
                case PerceptionError::UnknownError:
                default:
                    return std::string(constants::errors::MSG_UNKNOWN_ERROR);
            }
        }

        static const PerceptionErrorCategory& instance() {
            static PerceptionErrorCategory category;
            return category;
        }
    };

    /**
     * @brief Create std::error_code from PerceptionError
     */
    inline std::error_code make_error_code(PerceptionError e) {
        return std::error_code(static_cast<int>(e), PerceptionErrorCategory::instance());
    }

} // namespace perception

namespace std {
    template<>
    struct is_error_code_enum<perception::PerceptionError> : true_type {};
}

namespace perception {

    /**
     * @brief Result type for error handling (similar to std::expected)
     * @tparam T Type of the success value
     */
    export template<typename T>
    class Result {
    private:
        std::variant<T, std::error_code> data_;

    public:
        Result() : data_(std::error_code()) {}
        Result(T value) : data_(std::move(value)) {}
        Result(std::error_code ec) : data_(ec) {}

        explicit operator bool() const noexcept {
            return std::holds_alternative<T>(data_);
        }

        bool has_value() const noexcept {
            return std::holds_alternative<T>(data_);
        }

        T& value() & {
            return std::get<T>(data_);
        }

        const T& value() const& {
            return std::get<T>(data_);
        }

        T&& value() && {
            return std::get<T>(std::move(data_));
        }

        std::error_code error() const noexcept {
            return std::holds_alternative<std::error_code>(data_) 
                   ? std::get<std::error_code>(data_) 
                   : std::error_code{};
        }

        std::string message() const {
            if (has_value()) {
                return "Success";
            }
            return error().message();
        }
    };

    /**
     * @brief Specialization for void result
     */
    export template<>
    class Result<void> {
    private:
        std::error_code ec_;

    public:
        Result() : ec_{} {}
        Result(std::error_code ec) : ec_(ec) {}

        explicit operator bool() const noexcept {
            return !ec_;
        }

        bool has_value() const noexcept {
            return !ec_;
        }

        std::error_code error() const noexcept {
            return ec_;
        }

        std::string message() const {
            return ec_.message();
        }
    };

    /**
     * @brief Alias for Result<T> for backward compatibility
     */
    export template<typename T>
    using PerceptionResult = Result<T>;

    /**
     * @brief Alias for Expected<T, PerceptionError> (using custom Result for C++23 compatibility)
     */
    export template<typename T>
    using Expected = Result<T>;

    export template<typename E>
    auto make_unexpected(E err) {
        return Result<void>(make_error_code(err));
    }
    
    export template<typename T = void>
    auto make_unexpected(PerceptionError err) -> Result<T> {
        return Result<T>(make_error_code(err));
    }    

    /**
     * @brief Async result type for coroutine operations
     */
    export template<typename T>
    struct AsyncResult {
        std::future<Result<T>> future;

        auto get() -> Result<T> {
            return future.get();
        }

        auto wait() -> void {
            future.wait();
        }

        auto is_ready() const -> bool {
            return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }
    };

} // namespace perception
