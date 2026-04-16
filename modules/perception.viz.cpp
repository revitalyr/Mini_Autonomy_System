/**
 * @file perception.viz.cpp
 * @brief Implementation of visualization functions for tracking results
 */

module;

#include <opencv2/core/version.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <format>
#include <perception/constants.h>

module perception.viz;

import perception.types;
import perception.geom;
import perception.tracking;

/**
 * @def CV_VERSION_MAJOR
 * @brief Ensures OpenCV version compatibility
 * @details Prevents mixing OpenCV 5 headers with OpenCV 4 libraries
 */
#if CV_VERSION_MAJOR != 4
#error "OpenCV version mismatch: Visualization module requires OpenCV 4 headers."
#endif

namespace perception::viz {

using namespace perception::constants;

/**
 * @brief Draw tracking results on an image
 * @param image Input image to draw on
 * @param tracks Vector of tracking results to visualize
 * @details Draws colored bounding boxes with track IDs, class names, and confidence scores
 */
auto drawTracks(const ImageData& image, const Vector<tracking::Track>& tracks) -> void {
    if (image.m_impl->mat.empty()) return;

    cv::Mat bgr = image.m_impl->mat;

    for (const auto& track : tracks) {
        cv::Rect rect(track.bbox.x, track.bbox.y, track.bbox.width, track.bbox.height);
        
        cv::Scalar color;
        switch (track.id % 6) {
            case 0: color = {0, 0, 255}; break;
            case 1: color = {0, 255, 0}; break;
            case 2: color = {255, 0, 0}; break;
            case 3: color = {0, 255, 255}; break;
            case 4: color = {255, 255, 0}; break;
            default: color = {255, 0, 255}; break;
        }

        cv::rectangle(bgr, rect, color, 2);

        String text = std::format("ID:{} {} ({:.2f})", track.id, track.class_name, track.confidence);
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        
        cv::rectangle(bgr, 
            cv::Point(rect.x, rect.y - textSize.height - 5),
            cv::Point(rect.x + textSize.width, rect.y),
            color, cv::FILLED);
            
        cv::putText(bgr, text, cv::Point(rect.x, rect.y - 5),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, {255, 255, 255}, 1);
    }

    cv::imshow("Perception Tracking", bgr);
    cv::waitKey(1);
}

/**
 * @brief Close all OpenCV visualization windows
 */
auto terminateWindows() -> void {
    cv::destroyAllWindows();
}

}