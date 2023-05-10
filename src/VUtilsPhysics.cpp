#include <assert.h>

#include "VUtils.h"
#include "VUtilsPhysics.h"
#include "VUtilsMath.h"

namespace VUtils::Physics {

    static constexpr float EPS = .1f;

    // The main function that returns true if line segment 'p1q1'
    // and 'p2q2' intersect.
    bool LinesIntersect(const Vector2f& p1, const Vector2f& q1, const Vector2f& p2, const Vector2f& q2) {
        static auto&& onSegment = [](const Vector2f& p, const Vector2f& q, const Vector2f& r) -> bool {
            return q.x <= std::max(p.x, r.x) + EPS && q.x >= std::min(p.x, r.x) - EPS &&
                q.y <= std::max(p.y, r.y) + EPS && q.y >= std::min(p.y, r.y) - EPS;
        };

        // To find orientation of ordered triplet (p, q, r).
        // The function returns following values
        // 0 --> p, q and r are collinear
        // 1 --> Clockwise
        // 2 --> Counterclockwise
        static auto&& orientation = [](const Vector2f& p, const Vector2f& q, const Vector2f& r) -> int {
            // See https://www.geeksforgeeks.org/orientation-3-ordered-points/
            // for details of below formula.
            float val = (q.y - p.y) * (r.x - q.x) -
                (q.x - p.x) * (r.y - q.y);

            if (abs(val) < EPS) return 0;  // collinear

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

    /*
    // https://stackoverflow.com/a/9997374
    bool LinesIntersect(Vector2f a1, Vector2f a2, Vector2f b1, Vector2f b2) {

        // Return the area of triangle formed by points
        static auto&& ccw = [](Vector2f a, Vector2f b, Vector2f c) {
            return (c.y - a.y) * (b.x - a.x)
            > (b.y - a.y) * (c.x - a.x);
        };

        float a1 = ccw(a, c, d);
        float b1 = ccw(b, c, d);
        float c1 = ccw(a, b, c);
        float d1 = ccw(a, b, d);

        // Orientation check
        if (a1 != b1 && 
            c1 != d1)
            return true;

        // Return whether p is in a rectangular region formed by a and b
        static auto&& inRegion = [](Vector2f p, Vector2f a, Vector2f b)
        {
            if (p.x <= std::max(a.x, b.x) && p.x >= std::min(a.x, b.x) &&
                p.y <= std::max(a.y, b.y) && p.y >= std::min(a.y, b.y))
                return true;

            return false;
        };

        return std::abs(a1) < 0.01f && onSegment()
            || std::abs(b1) < 0.01f
            || std::abs(c1) < 0.01f
            || std::abs(d1) < 0.01f;
    }*/

    // Return whether point lies inside rect at origin
    bool PointInsideRect(Vector3f size, Vector3f pos) {
        size *= .5f;

        return pos.x >= -size.x && pos.x <= size.x &&
            pos.y >= -size.y && pos.y <= size.y &&
            pos.z >= -size.z && pos.z <= size.z;
    }

    bool PointInsideRect(Vector3f size1, Vector3f pos1, Vector3f pos2) {
        return PointInsideRect(size1, pos2 - pos1);
    }

    bool PointInsideRect(Vector3f size1, Vector3f pos1, Quaternion rot1, Vector3f pos2) {
        // Normalize the point relative to the rectangle
        //  Get the difference in positions, then apply inverse rotation
        Vector3f pos = Quaternion::Inverse(rot1) * (pos2 - pos1);

        return PointInsideRect(size1, pos);
    }

    bool RectInsideRect(Vector3f size1, Vector3f pos1, Quaternion rot1,
        Vector3f size2, Vector3f pos2, Quaternion rot2) 
    {
        // Get the difference between the 2 angles
        // https://wirewhiz.com/quaternion-tips/
        Quaternion rot = Quaternion::Inverse(rot1) * rot2;

        size2 *= .5f;

        // Now bound detection
        return PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3f(size2.x, size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3f(-size2.x, size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3f(size2.x, size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3f(-size2.x, size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3f(size2.x, -size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3f(-size2.x, -size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3f(size2.x, -size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3f(-size2.x, -size2.y, size2.z));
    }



    bool RectOverlapRect(Vector3f size1, Vector3f pos1, Quaternion rot1,
        Vector3f size2, Vector3f pos2, Quaternion rot2, std::string& desmos) {
        // Determine whether rectangles overlap in any way

        // If a point is in a rect from either other, or a line intersects
        // they overlap

        assert(abs(rot1.x) < .01f && abs(rot1.z) < .01f);
        assert(abs(rot2.x) < .01f && abs(rot2.z) < .01f);

        size1 *= .5f;
        size2 *= .5f;

        bool overlaps = false;

        {
            float a1 = pos1.y - size1.y;
            float a2 = pos1.y + size1.y;
            float b1 = pos2.y - size2.y;
            float b2 = pos2.y + size2.y;

            if (!(VUtils::Math::Between(a1, b1, b2) ||
                VUtils::Math::Between(a2, b1, b2) ||
                VUtils::Math::Between(b1, a1, a2) ||
                VUtils::Math::Between(b2, a1, a2)))
                return false;

            // Check that the rectangles vertically overlap
            //if (!((a1 >= b1 && a1 <= b2)
            //    || (a2 >= b1 && a2 <= b2)
            //    || (b1 >= a1 && b1 <= a2)
            //    || (b2 >= a1 && b2 <= a2)))
            //    return false;
        }

        Vector3f v3_a_br = pos1 + rot1 * Vector3f(size1.x, pos1.y, -size1.z);
        Vector3f v3_a_bl = pos1 + rot1 * Vector3f(-size1.x, pos1.y, -size1.z);
        Vector3f v3_a_ur = pos1 + rot1 * Vector3f(size1.x, pos1.y, size1.z);
        Vector3f v3_a_ul = pos1 + rot1 * Vector3f(-size1.x, pos1.y, size1.z);

        Vector3f v3_b_br = pos2 + rot2 * Vector3f(size2.x, pos2.y, -size2.z);
        Vector3f v3_b_bl = pos2 + rot2 * Vector3f(-size2.x, pos2.y, -size2.z);
        Vector3f v3_b_ur = pos2 + rot2 * Vector3f(size2.x, pos2.y, size2.z);
        Vector3f v3_b_ul = pos2 + rot2 * Vector3f(-size2.x, pos2.y, size2.z);

        Vector2f a_br(v3_a_br.x, v3_a_br.z);
        Vector2f a_bl(v3_a_bl.x, v3_a_bl.z);
        Vector2f a_ur(v3_a_ur.x, v3_a_ur.z);
        Vector2f a_ul(v3_a_ul.x, v3_a_ul.z);

        Vector2f b_br(v3_b_br.x, v3_b_br.z);
        Vector2f b_bl(v3_b_bl.x, v3_b_bl.z);
        Vector2f b_ur(v3_b_ur.x, v3_b_ur.z);
        Vector2f b_ul(v3_b_ul.x, v3_b_ul.z);

        // Only testing x / z intersections (y is redundant because of the initial test)
        if (LinesIntersect(a_br, a_ur, b_br, b_ur))
            overlaps = true;
        else if (LinesIntersect(a_br, a_ur, b_ur, b_ul))
            overlaps = true;
        else if (LinesIntersect(a_br, a_ur, b_ul, b_bl))
            overlaps = true;
        else if (LinesIntersect(a_br, a_ur, b_bl, b_br))
            overlaps = true;

        else if (LinesIntersect(a_ur, a_ul, b_br, b_ur))
            overlaps = true;
        else if (LinesIntersect(a_ur, a_ul, b_ur, b_ul))
            overlaps = true;
        else if (LinesIntersect(a_ur, a_ul, b_ul, b_bl))
            overlaps = true;
        else if (LinesIntersect(a_ur, a_ul, b_bl, b_br))
            overlaps = true;

        else if (LinesIntersect(a_ul, a_bl, b_br, b_ur))
            overlaps = true;
        else if (LinesIntersect(a_ul, a_bl, b_ur, b_ul))
            overlaps = true;
        else if (LinesIntersect(a_ul, a_bl, b_ul, b_bl))
            overlaps = true;
        else if (LinesIntersect(a_ul, a_bl, b_bl, b_br))
            overlaps = true;

        else if (LinesIntersect(a_bl, a_br, b_br, b_ur))
            overlaps = true;
        else if (LinesIntersect(a_bl, a_br, b_ur, b_ul))
            overlaps = true;
        else if (LinesIntersect(a_bl, a_br, b_ul, b_bl))
            overlaps = true;
        else if (LinesIntersect(a_bl, a_br, b_bl, b_br))
            overlaps = true;

        // Now test whether rectangle is inside rectangle
        // This case is less likely (I think) due to this only being used in dungeon generator
        // room placement (which is funky and jank, unlikely to be inner-overlaps)
        //if (PointInsideRect(size1, pos1, rot1, a1)

        // Some unity-specific Valheim dungeon room placement tests are required
        //  determine whether rooms can be inside each other or not

        // Set sizes back to normal
        size1 *= 2.f;
        size2 *= 2.f;

        if (PointInsideRect(size1, pos1, rot1, v3_b_br))
            overlaps = true;
        else if (PointInsideRect(size1, pos1, rot1, v3_b_bl))
            overlaps = true;
        else if (PointInsideRect(size1, pos1, rot1, v3_b_ur))
            overlaps = true;
        else if (PointInsideRect(size1, pos1, rot1, v3_b_ul))
            overlaps = true;
        else if (PointInsideRect(size2, pos2, rot2, v3_a_br))
            overlaps = true;
        else if (PointInsideRect(size2, pos2, rot2, v3_a_bl))
            overlaps = true;
        else if (PointInsideRect(size2, pos2, rot2, v3_a_ur))
            overlaps = true;
        else if (PointInsideRect(size2, pos2, rot2, v3_a_ul))
            overlaps = true;

        if (!overlaps) {
            // easy desmos copy/paste
            ////LOG(INFO) << "polygon((" << v3_a_br.x << "," << v3_a_br.z << "),("
            //    << v3_a_ur.x << "," << v3_a_ur.z << "),("
            //    << v3_a_ul.x << "," << v3_a_ul.z << "),("
            //    << v3_a_bl.x << "," << v3_a_bl.z << "))";
        
            //LOG(INFO) << " - polygon((" << v3_b_br.x << "," << v3_b_br.z << "),("
                //<< v3_b_ur.x << "," << v3_b_ur.z << "),("
                //<< v3_b_ul.x << "," << v3_b_ul.z << "),("
                //<< v3_b_bl.x << "," << v3_b_bl.z << "))";

            ////LOG(INFO) << "";
        }

        std::stringstream ss;

        ss << "polygon((" << v3_a_br.x << "," << v3_a_br.z << "),("
            << v3_a_ur.x << "," << v3_a_ur.z << "),("
            << v3_a_ul.x << "," << v3_a_ul.z << "),("
            << v3_a_bl.x << "," << v3_a_bl.z << "))";

        desmos = ss.str();

        return overlaps;
    }

    bool RectOverlapRect(Vector3f size1, Vector3f pos1, Quaternion rot1,
        Vector3f size2, Vector3f pos2, Quaternion rot2) {
        assert(false);
        throw std::runtime_error("not implemented");
    }

    std::pair<Vector3f, Quaternion> LocalToGlobal(const Vector3f &childLocalPos, const Quaternion &childLocalRot,
        const Vector3f &parentPos, const Quaternion &parentRot) {

        auto childPos = parentPos + parentRot * (childLocalPos);
        auto childRot = childLocalRot * parentRot;

        return { childPos, childRot };
    }

    std::pair<Vector3f, Quaternion> GlobalToLocal(const Vector3f& globalPos, const Quaternion& globalRot,
        const Vector3f& parentPos, const Quaternion& parentRot) {

        // solve for localPos
        // globalPos = parentPos + parentRot * (localPos);
        // globalPos - parentPos = parentRot * localPos
        // localPos = Quaternion::Inverse(globalRot) * (globalPos - parentPos)

        auto localPos = Quaternion::Inverse(globalRot) * (globalPos - parentPos);
        auto localRot = Quaternion::Inverse(globalRot) * parentRot;

        return { localPos, localRot };



        //Quaternion childWorldRot = parentRot * childLocalRot;
        //Vector3f pointOnRot = (childWorldRot * Vector3f::FORWARD).Normalized() * childLocalPos.Magnitude();
        //
        //Vector3f childWorldPos = pointOnRot + parentPos;
        //
        //return { childWorldPos, childWorldRot };
    }
}
