#pragma once

#include "Vector.h"
#include "Quaternion.h"

namespace VUtils::Physics {

    // Check whether 2 lines intersect
    bool LinesIntersect(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2);

    bool PointInsideRect(Vector3 size1, Vector3 pos1, Quaternion rot1, Vector3 pos2);

    // Checks whether a rectangular region lies completely inside inside another rectangular region
    bool RectInsideRect(Vector3 size1, Vector3 pos1, Quaternion rot1,
        Vector3 size2, Vector3 pos2, Quaternion rot2);

    // Check whether 2 rectangles intersect
    // Quaternion x,z should not be assigned due to non-implementation
    bool RectOverlapRect(Vector3 size1, Vector3 pos1, Quaternion rot1,
        Vector3 size2, Vector3 pos2, Quaternion rot2);

    std::pair<Vector3, Quaternion> LocalToGlobal(const Vector3& childLocalPos, const Quaternion& childLocalRot,
        const Vector3& parentPos, const Quaternion& parentRot);

}
