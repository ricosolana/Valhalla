#pragma once

#include "Quaternion.h"
#include "Vector.h"

namespace VUtils::Physics {

    // Check whether 2 lines intersect
    //bool LinesIntersect(const Vector2f& p1, const Vector2f& q1, const Vector2f& p2, const Vector2f& q2);

    // Check whether 2 lines intersect
    //  does not check for collinear intersections
    //bool LinesIntersect(Vector2f a, Vector2f b, Vector2f c, Vector2f d);


    bool PointInsideRect(Vector3f size1, Vector3f pos1, Quaternion rot1, Vector3f pos2);

    // Checks whether a rectangular region lies completely inside inside another rectangular region
    bool RectInsideRect(Vector3f size1, Vector3f pos1, Quaternion rot1, Vector3f size2, Vector3f pos2,
                        Quaternion rot2);

    // Check whether 2 rectangles intersect
    // Quaternion x,z should not be assigned due to non-implementation
    [[deprecated("broken")]]
    bool RectOverlapRect(Vector3f size1, Vector3f pos1, Quaternion rot1, Vector3f size2, Vector3f pos2,
                         Quaternion rot2, std::string &desmos);

    [[deprecated("broken")]]
    bool RectOverlapRect(Vector3f size1, Vector3f pos1, Quaternion rot1, Vector3f size2, Vector3f pos2,
                         Quaternion rot2);

    bool BoxBoxOverlap(
        Vector3f pos1, Vector3f size1, Quaternion rot1,
        Vector3f pos2, Vector3f size2, Quaternion rot2,
        std::string* desmos_dbg = nullptr);

    std::pair<Vector3f, Quaternion> LocalToGlobal(Vector3f const &childLocalPos,
                                                  Quaternion const &childLocalRot, Vector3f const &parentPos,
                                                  Quaternion const &parentRot);

    // TODO requires testing
    std::pair<Vector3f, Quaternion> GlobalToLocal(Vector3f const &globalPos, Quaternion const &globalRot,
                                                  Vector3f const &parentPos, Quaternion const &parentRot);

}// namespace VUtils::Physics
