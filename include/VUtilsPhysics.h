#pragma once

#include "Vector.h"
#include "Quaternion.h"

namespace VUtils::Physics {

    // Check whether 2 lines intersect
    //bool LinesIntersect(const Vector2f& p1, const Vector2f& q1, const Vector2f& p2, const Vector2f& q2);

    // Check whether 2 lines intersect
    //  does not check for collinear intersections
    //bool LinesIntersect(Vector2f a, Vector2f b, Vector2f c, Vector2f d);



    bool PointInsideRect(Vector3f size1, Vector3f pos1, Quaternion rot1, Vector3f pos2);

    // Checks whether a rectangular region lies completely inside inside another rectangular region
    bool RectInsideRect(Vector3f size1, Vector3f pos1, Quaternion rot1,
        Vector3f size2, Vector3f pos2, Quaternion rot2);

    // Check whether 2 rectangles intersect
    // Quaternion x,z should not be assigned due to non-implementation
    bool RectOverlapRect(Vector3f size1, Vector3f pos1, Quaternion rot1,
        Vector3f size2, Vector3f pos2, Quaternion rot2, std::string& desmos);

    bool RectOverlapRect(Vector3f size1, Vector3f pos1, Quaternion rot1,
        Vector3f size2, Vector3f pos2, Quaternion rot2);

    std::pair<Vector3f, Quaternion> LocalToGlobal(const Vector3f& childLocalPos, const Quaternion& childLocalRot,
        const Vector3f& parentPos, const Quaternion& parentRot);

    // TODO requires testing
    std::pair<Vector3f, Quaternion> GlobalToLocal(const Vector3f& globalPos, const Quaternion& globalRot,
        const Vector3f& parentPos, const Quaternion& parentRot);

}
