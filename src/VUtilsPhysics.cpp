#include <assert.h>

#include "VUtils.h"
#include "VUtilsPhysics.h"
#include "VUtilsMath.h"

namespace VUtils::Physics {

    // The main function that returns true if line segment 'p1q1'
    // and 'p2q2' intersect.
    /*
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

    bool LinesIntersect2(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2) {
        // calculates the triangle's size (formed by the "anchor" segment and additional point)
        static auto&& Area2 = [](Vector2 a, Vector2 b, Vector2 c) {
            return (b.x - a.x) * (c.y - a.y) -
                (c.x - a.x) * (b.y - a.y);
        };
        
        static auto&& IsOnLeft = [](Vector2 a, Vector2 b, Vector2 c) {
            return Area2(a, b, c) > 0;
        };

        static auto&& IsOnRight = [](Vector2 a, Vector2 b, Vector2 c) {
            return Area2(a, b, c) < 0;
        };

        static auto&& IsCollinear = [](Vector2 a, Vector2 b, Vector2 c) {
            return std::abs(Area2(a, b, c)) < 0.001f;
        };

        //bool onLeft = IsOnLeft()

        //if ()

        return false;
    }*/

    // https://stackoverflow.com/a/9997374
    bool LinesIntersect(Vector2 a, Vector2 b, Vector2 c, Vector2 d) {

        static auto&& ccw = [](Vector2 a, Vector2 b, Vector2 c) {
            return (c.y - a.y) * (b.x - a.x)
            > (b.y - a.y) * (c.x - a.x);
        };

        return ccw(a, c, d) != ccw(b, c, d)
            && ccw(a, b, c) != ccw(a, b, d);
    }

    // Return whether point lies inside rect at origin
    bool PointInsideRect(Vector3 size, Vector3 pos) {
        size *= .5f;

        return pos.x >= -size.x && pos.x <= size.x &&
            pos.y >= -size.y && pos.y <= size.y &&
            pos.z >= -size.z && pos.z <= size.z;
    }

    bool PointInsideRect(Vector3 size1, Vector3 pos1, Vector3 pos2) {
        return PointInsideRect(size1, pos2 - pos1);
    }

    bool PointInsideRect(Vector3 size1, Vector3 pos1, Quaternion rot1, Vector3 pos2) {
        Vector3 pos = Quaternion::Inverse(rot1) * (pos2 - pos1);

        return PointInsideRect(size1, pos);
    }

    bool RectInsideRect(Vector3 size1, Vector3 pos1, Quaternion rot1,
        Vector3 size2, Vector3 pos2, Quaternion rot2) 
    {
        // Get the difference between the 2 angles
        // https://wirewhiz.com/quaternion-tips/
        Quaternion rot = Quaternion::Inverse(rot1) * rot2;

        size2 *= .5f;

        // Now bound detection
        return PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3(size2.x, size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3(-size2.x, size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3(size2.x, size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3(-size2.x, size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3(size2.x, -size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3(-size2.x, -size2.y, -size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3(size2.x, -size2.y, size2.z))
            && PointInsideRect(size1, pos1, rot1, pos2 + rot * Vector3(-size2.x, -size2.y, size2.z));
    }







    bool RectOverlapRect(Vector3 size1, Vector3 pos1, Quaternion rot1,
        Vector3 size2, Vector3 pos2, Quaternion rot2) {
        // Determine whether rectangles overlap in any way

        // If a point is in a rect from either other, or a line intersects
        // they overlap

        assert(abs(rot1.x) < .01f && abs(rot1.z) < .01f);
        assert(abs(rot2.x) < .01f && abs(rot2.z) < .01f);

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

        size1 *= .5f;
        size2 *= .5f;

        Vector3 v3_a_br = pos1 + rot1 * Vector3(size1.x, size1.y, -size1.z);
        Vector3 v3_a_bl = pos1 + rot1 * Vector3(-size1.x, size1.y, -size1.z);
        Vector3 v3_a_ur = pos1 + rot1 * Vector3(size1.x, size1.y, size1.z);
        Vector3 v3_a_ul = pos1 + rot1 * Vector3(-size1.x, size1.y, size1.z);

        Vector3 v3_b_br = pos2 + rot2 * Vector3(size2.x, size2.y, -size2.z);
        Vector3 v3_b_bl = pos2 + rot2 * Vector3(-size2.x, size2.y, -size2.z);
        Vector3 v3_b_ur = pos2 + rot2 * Vector3(size2.x, size2.y, size2.z);
        Vector3 v3_b_ul = pos2 + rot2 * Vector3(-size2.x, size2.y, size2.z);

        // easy desmos copy/paste
        LOG(INFO) << "polygon((" << v3_a_br.x << "," << v3_a_br.z << "),("
            << v3_a_ur.x << "," << v3_a_ur.z << "),("
            << v3_a_ul.x << "," << v3_a_ul.z << "),("
            << v3_a_bl.x << "," << v3_a_bl.z << "))";

        LOG(INFO) << "polygon((" << v3_b_br.x << "," << v3_b_br.z << "),("
            << v3_b_ur.x << "," << v3_b_ur.z << "),("
            << v3_b_ul.x << "," << v3_b_ul.z << "),("
            << v3_b_bl.x << "," << v3_b_bl.z << "))";

        LOG(INFO) << "";

        Vector2 a_br(v3_a_br.x, v3_a_br.z);
        Vector2 a_bl(v3_a_bl.x, v3_a_bl.z);
        Vector2 a_ur(v3_a_ur.x, v3_a_ur.z);
        Vector2 a_ul(v3_a_ul.x, v3_a_ul.z);

        Vector2 b_br(v3_b_br.x, v3_b_br.z);
        Vector2 b_bl(v3_b_bl.x, v3_b_bl.z);
        Vector2 b_ur(v3_b_ur.x, v3_b_ur.z);
        Vector2 b_ul(v3_b_ul.x, v3_b_ul.z);

        // Only testing x / z intersections (y is redundant because of the initial test)
        if (LinesIntersect(a_br, a_ur, b_br, b_ur))
            return true;
        if (LinesIntersect(a_br, a_ur, b_ur, b_ul))
            return true;
        if (LinesIntersect(a_br, a_ur, b_ul, b_bl))
            return true;
        if (LinesIntersect(a_br, a_ur, b_bl, b_br))
            return true;

        if (LinesIntersect(a_ur, a_ul, b_br, b_ur))
            return true;
        if (LinesIntersect(a_ur, a_ul, b_ur, b_ul))
            return true;
        if (LinesIntersect(a_ur, a_ul, b_ul, b_bl))
            return true;
        if (LinesIntersect(a_ur, a_ul, b_bl, b_br))
            return true;

        if (LinesIntersect(a_ul, a_bl, b_br, b_ur))
            return true;
        if (LinesIntersect(a_ul, a_bl, b_ur, b_ul))
            return true;
        if (LinesIntersect(a_ul, a_bl, b_ul, b_bl))
            return true;
        if (LinesIntersect(a_ul, a_bl, b_bl, b_br))
            return true;

        if (LinesIntersect(a_bl, a_br, b_br, b_ur))
            return true;
        if (LinesIntersect(a_bl, a_br, b_ur, b_ul))
            return true;
        if (LinesIntersect(a_bl, a_br, b_ul, b_bl))
            return true;
        if (LinesIntersect(a_bl, a_br, b_bl, b_br))
            return true;

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
            return true;
        if (PointInsideRect(size1, pos1, rot1, v3_b_bl))
            return true;
        if (PointInsideRect(size1, pos1, rot1, v3_b_ur))
            return true;
        if (PointInsideRect(size1, pos1, rot1, v3_b_ul))
            return true;
        if (PointInsideRect(size2, pos2, rot2, v3_a_br))
            return true;
        if (PointInsideRect(size2, pos2, rot2, v3_a_bl))
            return true;
        if (PointInsideRect(size2, pos2, rot2, v3_a_ur))
            return true;
        if (PointInsideRect(size2, pos2, rot2, v3_a_ul))
            return true;

        return false;
    }



    std::pair<Vector3, Quaternion> LocalToGlobal(const Vector3 &childLocalPos, const Quaternion &childLocalRot,
        const Vector3 &parentPos, const Quaternion &parentRot) {

        auto childPos = parentPos + Quaternion::Inverse(parentRot) * childLocalPos;
        auto childRot = parentRot * childLocalRot;

        return { childPos, childRot };



        //Quaternion childWorldRot = parentRot * childLocalRot;
        //Vector3 pointOnRot = (childWorldRot * Vector3::FORWARD).Normalized() * childLocalPos.Magnitude();
        //
        //Vector3 childWorldPos = pointOnRot + parentPos;
        //
        //return { childWorldPos, childWorldRot };
    }
}
