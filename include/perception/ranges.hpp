/**
 * @file ranges.hpp
 * @brief Modern C++20/23 ranges utilities for perception system
 * 
 * This header provides modern ranges-based utilities and algorithms
 * specifically designed for perception system operations.
 * 
 * @author Mini Autonomy System
 * @date 2026
 */

#pragma once

#include <ranges>
#include <algorithm>
#include <vector>
#include <functional>
#include <numeric>

// Import modern type system
#include "types.hpp"
#include "concepts.hpp"

namespace perception {

// ============================================================================
// MODERN RANGE VIEWS
// ============================================================================

/**
 * @brief Filter detections by confidence threshold
 * @param min_confidence Minimum confidence value
 * @return Range view filter
 */
constexpr auto filter_by_confidence(Confidence min_confidence) noexcept {
    return [min_confidence](const auto& detection) constexpr {
        return detection.confidence >= min_confidence;
    };
}

/**
 * @brief Filter detections by class name
 * @param class_name Class name to filter by
 * @return Range view filter
 */
constexpr auto filter_by_class(StringView class_name) noexcept {
    return [class_name](const auto& detection) constexpr {
        return detection.class_name == class_name;
    };
}

/**
 * @brief Filter detections by bounding box area
 * @param min_area Minimum area threshold
 * @return Range view filter
 */
constexpr auto filter_by_area(double min_area) noexcept {
    return [min_area](const auto& detection) constexpr {
        return detection.bbox.area() >= min_area;
    };
}

/**
 * @brief Filter detections by aspect ratio
 * @param min_ratio Minimum aspect ratio
 * @param max_ratio Maximum aspect ratio
 * @return Range view filter
 */
constexpr auto filter_by_aspect_ratio(double min_ratio, double max_ratio) noexcept {
    return [min_ratio, max_ratio](const auto& detection) constexpr {
        const auto ratio = static_cast<double>(detection.bbox.width) / detection.bbox.height;
        return ratio >= min_ratio && ratio <= max_ratio;
    };
}

/**
 * @brief Transform detections to class names
 * @return Range view transformer
 */
constexpr auto to_class_names() noexcept {
    return [](const auto& detection) constexpr {
        return detection.class_name;
    };
}

/**
 * @brief Transform detections to confidence values
 * @return Range view transformer
 */
constexpr auto to_confidences() noexcept {
    return [](const auto& detection) constexpr {
        return detection.confidence;
    };
}

/**
 * @brief Transform detections to bounding boxes
 * @return Range view transformer
 */
constexpr auto to_bboxes() noexcept {
    return [](const auto& detection) constexpr {
        return detection.bbox;
    };
}

/**
 * @brief Transform detections to center points
 * @return Range view transformer
 */
constexpr auto to_centers() noexcept {
    return [](const auto& detection) constexpr {
        return Point{
            detection.bbox.x + detection.bbox.width / 2,
            detection.bbox.y + detection.bbox.height / 2
        };
    };
}

/**
 * @brief Transform detections to areas
 * @return Range view transformer
 */
constexpr auto to_areas() noexcept {
    return [](const auto& detection) constexpr {
        return detection.bbox.area();
    };
}

/**
 * @brief Take first N elements from range
 * @param count Number of elements to take
 * @return Range view transformer
 */
constexpr auto take_first(size_t count) noexcept {
    return [count](auto&& range) constexpr {
        return range | std::views::take(count);
    };
}

/**
 * @brief Take last N elements from range
 * @param count Number of elements to take
 * @return Range view transformer
 */
constexpr auto take_last(size_t count) noexcept {
    return [count](auto&& range) constexpr {
        return range | std::views::drop(range.size() - count) | std::views::take(count);
    };
}

/**
 * @brief Drop first N elements from range
 * @param count Number of elements to drop
 * @return Range view transformer
 */
constexpr auto drop_first(size_t count) noexcept {
    return [count](auto&& range) constexpr {
        return range | std::views::drop(count);
    };
}

/**
 * @brief Sort detections by confidence (descending)
 * @return Range view transformer
 */
constexpr auto sort_by_confidence_desc() noexcept {
    return [](auto range) constexpr {
        Vector<std::ranges::range_value_t<decltype(range)>> result{range.begin(), range.end()};
        std::sort(result.begin(), result.end(), 
                 [](const auto& a, const auto& b) { return a.confidence > b.confidence; });
        return result;
    };
}

/**
 * @brief Sort detections by area (descending)
 * @return Range view transformer
 */
constexpr auto sort_by_area_desc() noexcept {
    return [](auto range) constexpr {
        Vector<std::ranges::range_value_t<decltype(range)>> result{range.begin(), range.end()};
        std::sort(result.begin(), result.end(), 
                 [](const auto& a, const auto& b) { return a.bbox.area() > b.bbox.area(); });
        return result;
    };
}

/**
 * @brief Sort detections by class name then confidence
 * @return Range view transformer
 */
constexpr auto sort_by_class_then_confidence() noexcept {
    return [](auto range) constexpr {
        Vector<std::ranges::range_value_t<decltype(range)>> result{range.begin(), range.end()};
        std::sort(result.begin(), result.end(), 
                 [](const auto& a, const auto& b) {
                     if (a.class_name != b.class_name) {
                         return a.class_name < b.class_name;
                     }
                     return a.confidence > b.confidence;
                 });
        return result;
    };
}

/**
 * @brief Group detections by class name
 * @return Range view transformer
 */
constexpr auto group_by_class() noexcept {
    return [](auto range) constexpr {
        std::unordered_map<String, Vector<std::ranges::range_value_t<decltype(range)>>> grouped;
        
        for (const auto& detection : range) {
            grouped[detection.class_name].emplace_back(detection);
        }
        
        return grouped;
    };
}

/**
 * @brief Remove duplicate detections based on IoU threshold
 * @param iou_threshold IoU threshold for duplicate detection
 * @return Range view transformer
 */
constexpr auto non_max_suppression(double iou_threshold = 0.5) noexcept {
    return [iou_threshold](auto range) constexpr {
        Vector<std::ranges::range_value_t<decltype(range)>> result;
        Vector<std::ranges::range_value_t<decltype(range)>> detections{range.begin(), range.end()};
        
        // Sort by confidence descending
        std::sort(detections.begin(), detections.end(), 
                 [](const auto& a, const auto& b) { return a.confidence > b.confidence; });
        
        for (const auto& detection : detections) {
            bool is_duplicate = false;
            
            for (const auto& kept : result) {
                const auto iou = calculate_iou(detection.bbox, kept.bbox);
                if (iou > iou_threshold && detection.class_name == kept.class_name) {
                    is_duplicate = true;
                    break;
                }
            }
            
            if (!is_duplicate) {
                result.emplace_back(detection);
            }
        }
        
        return result;
    };
}

// ============================================================================
// MODERN RANGE ALGORITHMS
// ============================================================================

/**
 * @brief Calculate Intersection over Union (IoU) for two bounding boxes
 * @param box1 First bounding box
 * @param box2 Second bounding box
 * @return IoU value between 0 and 1
 */
constexpr auto calculate_iou(const Rect& box1, const Rect& box2) noexcept -> double {
    const auto intersection = box1 & box2;
    const auto union_area = box1.area() + box2.area() - intersection.area();
    
    return union_area > 0 ? static_cast<double>(intersection.area()) / union_area : 0.0;
}

/**
 * @brief Calculate average confidence for a range of detections
 * @param range Range of detections
 * @return Average confidence value
 */
constexpr auto average_confidence(const Range auto& range) noexcept -> Confidence {
    if (range.empty()) return 0.0;
    
    auto sum = std::accumulate(range.begin(), range.end(), 0.0, 
                               [](double acc, const auto& detection) {
                                   return acc + detection.confidence;
                               });
    
    return sum / static_cast<double>(range.size());
}

/**
 * @brief Find detection with maximum confidence
 * @param range Range of detections
 * @return Iterator to max confidence detection
 */
constexpr auto max_confidence(const Range auto& range) noexcept {
    return std::ranges::max_element(range, {}, [](const auto& detection) {
        return detection.confidence;
    });
}

/**
 * @brief Find detection with minimum confidence
 * @param range Range of detections
 * @return Iterator to min confidence detection
 */
constexpr auto min_confidence(const Range auto& range) noexcept {
    return std::ranges::min_element(range, {}, [](const auto& detection) {
        return detection.confidence;
    });
}

/**
 * @brief Count detections by class name
 * @param range Range of detections
 * @param class_name Class name to count
 * @return Number of detections with given class
 */
constexpr auto count_by_class(const Range auto& range, StringView class_name) noexcept -> size_t {
    return std::ranges::count_if(range, [class_name](const auto& detection) {
        return detection.class_name == class_name;
    });
}

/**
 * @brief Get unique class names from range
 * @param range Range of detections
 * @return Vector of unique class names
 */
constexpr auto unique_classes(const Range auto& range) -> Vector<String> {
    std::unordered_set<String> classes;
    
    for (const auto& detection : range) {
        classes.insert(detection.class_name);
    }
    
    return Vector<String>{classes.begin(), classes.end()};
}

/**
 * @brief Calculate statistics for confidence values
 * @param range Range of detections
 * @return Tuple of (min, max, avg, std_dev)
 */
constexpr auto confidence_statistics(const Range auto& range) noexcept 
    -> std::tuple<Confidence, Confidence, Confidence, Confidence> {
    
    if (range.empty()) {
        return {0.0, 0.0, 0.0, 0.0};
    }
    
    Vector<Confidence> confidences;
    confidences.reserve(range.size());
    
    for (const auto& detection : range) {
        confidences.emplace_back(detection.confidence);
    }
    
    const auto min_conf = *std::ranges::min_element(confidences);
    const auto max_conf = *std::ranges::max_element(confidences);
    const auto avg_conf = std::accumulate(confidences.begin(), confidences.end(), 0.0) / confidences.size();
    
    // Calculate standard deviation
    const auto variance = std::accumulate(confidences.begin(), confidences.end(), 0.0,
                                         [avg_conf](double acc, Confidence conf) {
                                             return acc + (conf - avg_conf) * (conf - avg_conf);
                                         }) / confidences.size();
    
    const auto std_dev = std::sqrt(variance);
    
    return {min_conf, max_conf, avg_conf, std_dev};
}

/**
 * @brief Filter and transform in one operation (compile-time optimized)
 * @tparam Filter Filter type
 * @tparam Transform Transform type
 * @param range Input range
 * @param filter Filter predicate
 * @param transform Transform function
 * @return Vector of transformed and filtered results
 */
template<typename Filter, typename Transform>
constexpr auto filter_transform(const Range auto& range, Filter&& filter, Transform&& transform) {
    using InputType = std::ranges::range_value_t<decltype(range)>;
    using OutputType = std::invoke_result_t<Transform, InputType>;
    
    Vector<OutputType> result;
    
    for (const auto& item : range) {
        if (std::invoke(filter, item)) {
            result.emplace_back(std::invoke(transform, item));
        }
    }
    
    return result;
}

/**
 * @brief Parallel processing of ranges using thread pool
 * @tparam Func Processing function type
 * @param pool Thread pool to use
 * @param range Input range
 * @param func Processing function
 * @param chunk_size Size of chunks for parallel processing
 * @return Vector of futures for processed chunks
 */
template<typename Func>
requires Callable<Func, std::ranges::range_value_t<decltype(std::declval<const Vector<int>&>())>>
auto parallel_process_range(ThreadPool& pool, const Range auto& range, Func&& func, 
                            size_t chunk_size = 1000) -> Vector<decltype(pool.submit(func, *range.begin()))> {
    
    using RangeType = decltype(range);
    using ValueType = std::ranges::range_value_t<RangeType>;
    using ResultType = std::invoke_result_t<Func, ValueType>;
    
    Vector<Future<ResultType>> futures;
    
    auto it = range.begin();
    const auto end = range.end();
    
    while (it != end) {
        Vector<ValueType> chunk;
        
        // Collect chunk
        for (size_t i = 0; i < chunk_size && it != end; ++i, ++it) {
            chunk.emplace_back(*it);
        }
        
        // Submit chunk for processing
        futures.emplace_back(pool.submit([func = std::forward<Func>(func), chunk = std::move(chunk)]() mutable {
            Vector<ResultType> results;
            results.reserve(chunk.size());
            
            for (auto& item : chunk) {
                results.emplace_back(func(std::move(item)));
            }
            
            return results;
        }));
    }
    
    return futures;
}

// ============================================================================
// MODERN RANGE PIPELINE OPERATORS
// ============================================================================

/**
 * @brief Pipeline operator for range transformations
 * @tparam T Input type
 * @tparam Func Function type
 * @param value Input value
 * @param func Function to apply
 * @return Transformed value
 */
template<typename T, typename Func>
constexpr auto operator|(T&& value, Func&& func) 
    noexcept(std::is_nothrow_invocable_v<Func, T>) 
    -> std::invoke_result_t<Func, T> {
    return std::invoke(std::forward<Func>(func), std::forward<T>(value));
}

/**
 * @brief Pipeline builder for range operations
 */
class RangePipeline {
private:
    Vector<std::function<void()>> m_operations;
    
public:
    /**
     * @brief Add filter operation to pipeline
     * @tparam Filter Filter type
     * @param filter Filter predicate
     * @return Reference to pipeline for chaining
     */
    template<typename Filter>
    requires Callable<Filter, std::ranges::range_value_t<Vector<int>>>
    auto filter(Filter&& filter) -> RangePipeline& {
        m_operations.emplace_back([filter = std::forward<Filter>(filter)]() mutable {
            // Implementation would depend on current state
        });
        return *this;
    }
    
    /**
     * @brief Add transform operation to pipeline
     * @tparam Transform Transform type
     * @param transform Transform function
     * @return Reference to pipeline for chaining
     */
    template<typename Transform>
    requires Callable<Transform, std::ranges::range_value_t<Vector<int>>>
    auto transform(Transform&& transform) -> RangePipeline& {
        m_operations.emplace_back([transform = std::forward<Transform>(transform)]() mutable {
            // Implementation would depend on current state
        });
        return *this;
    }
    
    /**
     * @brief Execute pipeline on range
     * @tparam Range Range type
     * @param range Input range
     * @return Processed range
     */
    template<typename Range>
    requires Range<Range>
    auto execute(const Range& range) -> Vector<std::ranges::range_value_t<Range>> {
        // Apply operations in sequence
        Vector<std::ranges::range_value_t<Range>> result{range.begin(), range.end()};
        
        // Process operations (simplified for demonstration)
        for (const auto& operation : m_operations) {
            operation();
        }
        
        return result;
    }
};

/**
 * @brief Create a new range pipeline
 * @return Range pipeline instance
 */
constexpr auto make_pipeline() noexcept -> RangePipeline {
    return RangePipeline{};
}

} // namespace perception
