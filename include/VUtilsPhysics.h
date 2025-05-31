#pragma once

#include "Vector.h"
#include "Quaternion.h"

namespace VUtils::Physics {

    // Check whether 2 lines intersect
    //bool LinesIntersect(const Vector2f& p1, const Vector2f& q1, const Vector2f& p2, const Vector2f& q2);

    // Check whether 2 lines intersect
    //  does not check for collinear intersections
    //bool LinesIntersect(Vector2f a, Vector2f b, Vector2f c, Vector2f d);



    bool PointInsideRect(avledet::util::CSU::Vector3f size1, avledet::util::CSU::Vector3f pos1, avledet::util::CSU::Quaternion rot1, avledet::util::CSU::Vector3f pos2);

    // Checks whether a rectangular region lies completely inside inside another rectangular region
    bool RectInsideRect(avledet::util::CSU::Vector3f size1, avledet::util::CSU::Vector3f pos1, avledet::util::CSU::Quaternion rot1,
        avledet::util::CSU::Vector3f size2, avledet::util::CSU::Vector3f pos2, avledet::util::CSU::Quaternion rot2);

    // Check whether 2 rectangles intersect
    // Quaternion x,z should not be assigned due to non-implementation
    bool RectOverlapRect(avledet::util::CSU::Vector3f size1, avledet::util::CSU::Vector3f pos1, avledet::util::CSU::Quaternion rot1,
        avledet::util::CSU::Vector3f size2, avledet::util::CSU::Vector3f pos2, avledet::util::CSU::Quaternion rot2, std::string& desmos);

    bool RectOverlapRect(avledet::util::CSU::Vector3f size1, avledet::util::CSU::Vector3f pos1, avledet::util::CSU::Quaternion rot1,
        avledet::util::CSU::Vector3f size2, avledet::util::CSU::Vector3f pos2, avledet::util::CSU::Quaternion rot2);

    std::pair<avledet::util::CSU::Vector3f, avledet::util::CSU::Quaternion> LocalToGlobal(const avledet::util::CSU::Vector3f& childLocalPos, const avledet::util::CSU::Quaternion& childLocalRot,
        const avledet::util::CSU::Vector3f& parentPos, const avledet::util::CSU::Quaternion& parentRot);

    // TODO requires testing
    std::pair<avledet::util::CSU::Vector3f, avledet::util::CSU::Quaternion> GlobalToLocal(const avledet::util::CSU::Vector3f& globalPos, const avledet::util::CSU::Quaternion& globalRot,
        const avledet::util::CSU::Vector3f& parentPos, const avledet::util::CSU::Quaternion& parentRot);

}
