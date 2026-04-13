module;

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <format>

module perception.viz;

namespace perception::viz {

auto drawTracks(const geom::ImageData& image, const Vector<tracking::Track>& tracks) -> void {
    if (image.data.empty()) return;

    cv::Mat rgb(image.height, image.width, CV_8UC3, const_cast<uint8_t*>(image.data.data()));
    cv::Mat bgr;
    cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);

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

auto terminateWindows() -> void {
    cv::destroyAllWindows();
}

}