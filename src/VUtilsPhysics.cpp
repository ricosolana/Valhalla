#include "VUtilsPhysics.h"
#include "VUtilsMath.h"

namespace VUtils::Physics {

    // The main function that returns true if line segment 'p1q1'
    // and 'p2q2' intersect.
    bool LinesIntersect(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2)
    {
        static auto&& onSegment = [](Vector2 p, Vector2 q, Vector2 r)
        {
            if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
                q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y))
                return true;

            return false;
        };

        static auto&& orientation = [](Vector2 p, Vector2 q, Vector2 r)
        {
            // See https://www.geeksforgeeks.org/orientation-3-ordered-points/
            // for details of below formula.
            int val = (q.y - p.y) * (r.x - q.x) -
                (q.x - p.x) * (r.y - q.y);

            if (val == 0) return 0;  // collinear

            return (val > 0) ? 1 : 2; // clock or counterclock wise
        };

        // Find the four orientations needed for general and
        // special cases
        int o1 = orientation(p1, q1, p2);
        int o2 = orientation(p1, q1, q2);
        int o3 = orientation(p2, q2, p1);
        int o4 = orientation(p2, q2, q1);

        // General case
        if (o1 != o2 && o3 != o4)
            return true;

        // Special Cases
        // p1, q1 and p2 are collinear and p2 lies on segment p1q1
        if (o1 == 0 && onSegment(p1, p2, q1)) return true;

        // p1, q1 and q2 are collinear and q2 lies on segment p1q1
        if (o2 == 0 && onSegment(p1, q2, q1)) return true;

        // p2, q2 and p1 are collinear and p1 lies on segment p2q2
        if (o3 == 0 && onSegment(p2, p1, q2)) return true;

        // p2, q2 and q1 are collinear and q1 lies on segment p2q2
        if (o4 == 0 && onSegment(p2, q1, q2)) return true;

        return false; // Doesn't fall in any of the above cases
    }

    // Return whether point lies inside rect at origin
    bool PointInsideRect(Vector3 size, Vector3 pos) {
        size *= .5f;

        return pos.x > -size.x && pos.x < size.x &&
            pos.y > -size.y && pos.y < size.y &&
            pos.z > -size.z && pos.z < size.z;
    }

    bool PointInsideRect(Vector3 size1, Vector3 pos1, Vector3 pos2) {
        return PointInsideRect(size1, pos2 - pos1);
    }

    bool PointInsideRect(Vector3 size1, Vector3 pos1, Quaternion rot1, Vector3 pos2) {
        Vector3 pos = rot1 * (pos2 - pos1);

        return PointInsideRect(size1, pos1);
    }

    bool RectInsideRect(Vector3 size1, Vector3 pos1, Quaternion rot1,
        Vector3 size2, Vector3 pos2, Quaternion rot2) 
    {
        // Get the difference between the 2 angles
        // https://wirewhiz.com/quaternion-tips/
        Quaternion rot = Quaternion::Inverse(rot1) * rot2;

        size2 *= .5f;

        // Now bound detection
        return PointInsideRect(size1, pos1, rot, pos2 + rot * Vector3(size2.x, size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot, pos2 + rot * Vector3(-size2.x, size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot, pos2 + rot * Vector3(size2.x, size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot, pos2 + rot * Vector3(-size2.x, size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot, pos2 + rot * Vector3(size2.x, -size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot, pos2 + rot * Vector3(-size2.x, -size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot, pos2 + rot * Vector3(size2.x, -size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot, pos2 + rot * Vector3(-size2.x, -size2.y, size2.z));
    }







    bool RectOverlapRect(Vector3 size1, Vector3 pos1, Quaternion rot1,
        Vector3 size2, Vector3 pos2, Quaternion rot2) {
        // Determine whether rectangles overlap in any way

        // If a point is in a rect from either other, or a line intersects
        // they overlap

        {
            float a1 = pos1.y - size1.y * 0.5f;
            float a2 = pos1.y + size1.y * 0.5f;
            float b1 = pos2.y - size2.y * 0.5f;
            float b2 = pos2.y + size2.y * 0.5f;

            // Check that the rectangles vertically overlap
            if (!((a1 >= b1 && a1 <= b2)
                || (a2 >= b1 && a2 <= b2)
                || (b1 >= a1 && b1 <= a2)
                || (b2 >= a1 && b2 <= a2)))
                return false;
        }

        Vector3 a1 = pos1 + rot1 * Vector3(size1.x, size1.y, -size1.z);
        Vector3 a2 = pos1 + rot1 * Vector3(-size1.x, size1.y, -size1.z);
        Vector3 a3 = pos1 + rot1 * Vector3(size1.x, size1.y, size1.z);
        Vector3 a4 = pos1 + rot1 * Vector3(-size1.x, size1.y, size1.z);

        Vector3 b1 = pos2 + rot2 * Vector3(size2.x, size2.y, -size2.z);
        Vector3 b2 = pos2 + rot2 * Vector3(-size2.x, size2.y, -size2.z);
        Vector3 b3 = pos2 + rot2 * Vector3(size2.x, size2.y, size2.z);
        Vector3 b4 = pos2 + rot2 * Vector3(-size2.x, size2.y, size2.z);

        Vector2 j1(a1.x, a1.z);
        Vector2 j2(a2.x, a2.z);
        Vector2 j3(a3.x, a3.z);
        Vector2 j4(a4.x, a4.z);

        Vector2 k1(b1.x, b1.z);
        Vector2 k2(b2.x, b2.z);
        Vector2 k3(b3.x, b3.z);
        Vector2 k4(b4.x, b4.z);

        // Only testing x / z intersections (y is redundant because of the initial test)
        if (LinesIntersect(j1, j3, k1, k3)
            || LinesIntersect(j1, j3, k3, k4)
            || LinesIntersect(j1, j3, k4, k2)
            || LinesIntersect(j1, j3, k2, k1)

            || LinesIntersect(j3, k4, k1, k3)
            || LinesIntersect(j3, k4, k3, k4)
            || LinesIntersect(j3, k4, k4, k2)
            || LinesIntersect(j3, k4, k2, k1)

            || LinesIntersect(j4, k2, k1, k3)
            || LinesIntersect(j4, k2, k3, k4)
            || LinesIntersect(j4, k2, k4, k2)
            || LinesIntersect(j4, k2, k2, k1)

            || LinesIntersect(j2, k1, k1, k3)
            || LinesIntersect(j2, k1, k3, k4)
            || LinesIntersect(j2, k1, k4, k2)
            || LinesIntersect(j2, k1, k2, k1)) 
        {
            return true;
        }

        // Now test whether rectangle is inside rectangle
        // This case is less likely (I think) due to this only being used in dungeon generator
        // room placement (which is funky and jank, unlikely to be inner-overlaps)
        //if (PointInsideRect(size1, pos1, rot1, a1)

        // Some unity-specific Valheim dungeon room placement tests are required
        //  determine whether rooms can be inside each other or not

        return false;
    }



    std::pair<Vector3, Quaternion> LocalToGlobal(const Vector3 &childLocalPos, const Quaternion &childLocalRot,
        const Vector3 &parentPos, const Quaternion &parentRot) {

        Quaternion childWorldRot = parentRot * childLocalRot;
        Vector3 pointOnRot = (childWorldRot * Vector3::FORWARD).Normalized() * childLocalPos.Magnitude();

        Vector3 childWorldPos = pointOnRot + parentPos;

        return { childWorldPos, childWorldRot };
    }
}
