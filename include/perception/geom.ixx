/**
 * @file perception.geom.ixx
 * @brief Geometric primitives for perception
 */

module;

#include <string>
#include <vector>

export module perception.geom;

import perception.types;

export namespace perception::geom {

    /**
     * @brief Represents a 2D bounding box.
     */
    struct Rect {
        int x, y, width, height;
    };

    /**
     * @brief Represents a 3D point in Cartesian coordinates.
     */
    struct Point3D {
        double x, y, z;
    };

    /**
     * @brief Represents an object detection
     */
    struct Detection {
        Rect bbox;
        float confidence;
        int class_id;
        String class_name;
    };

    /**
     * @brief Represents a 3D object detection.
     */
    struct Detection3D {
        Rect bbox_2d;
        Point3D position_3d;
        float confidence;
        String class_name;
    };

} // namespace perception::geom