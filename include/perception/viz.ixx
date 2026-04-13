export module perception.viz;
import perception.types;
import perception.geom;
import perception.tracking;

export namespace perception::viz {

    /**
     * @brief Draw tracking results on an image frame
     */
    auto drawTracks(const geom::ImageData& image, const Vector<tracking::Track>& tracks) -> void;

    /**
     * @brief Close all visualization windows
     */
    auto terminateWindows() -> void;
}