#include <cmath>
#include <stdexcept>

#include "Quaternion.h"
#include "Vector.h"
#include "VUtils.h"

namespace avledet::util::CSU {

    static constexpr auto PI = 3.1415926535897932384626f;

    Quaternion const Quaternion::IDENTITY = {0, 0, 0, 1};

    Quaternion::Quaternion() :
        Quaternion(IDENTITY)
    {
    }

    Quaternion::Quaternion(float x, float y, float z, float w) :
        x(x),
        y(y),
        z(z),
        w(w)
    {
    }

    Quaternion::Quaternion(Vector3f v, float w)
    {
        this->x = v.x;
        this->y = v.y;
        this->z = v.z;
        this->w = w;
    }

    float Quaternion::length_squared() const
    {
        return x * x + y * y + z * z + w * w;
    }

    Vector3f Quaternion::xyz() const
    {
        return Vector3f(x, y, z);
    }

    Vector3f Quaternion::euler_angles() const
    {
        return normalize_angles(Internal_ToEulerRad(*this) * 180.f / PI);
    }

    Vector3f Quaternion::Internal_ToEulerRad(Quaternion rotation)
    {
        float sqw  = rotation.w * rotation.w;
        float sqx  = rotation.x * rotation.x;
        float sqy  = rotation.y * rotation.y;
        float sqz  = rotation.z * rotation.z;
        float unit = sqx + sqy + sqz + sqw;// if normalised is one, otherwise is correction factor
        float test = rotation.x * rotation.w - rotation.y * rotation.z;

        Vector3f v;
        if (test > 0.4995f * unit) {// singularity at north pole
            v.y = 2.f * std::atan2(rotation.y, rotation.x);
            v.x = PI * .5f;
            v.z = 0;
        } else if (test < -0.4995f * unit) {// singularity at south pole
            v.y = -2.f * std::atan2(rotation.y, rotation.x);
            v.x = -PI * .5f;
            v.z = 0;
        } else {
            auto q = Quaternion(rotation.w, rotation.z, rotation.x, rotation.y);
            v.y    = (float) std::atan2(2.f * q.x * q.w + 2.f * q.y * q.z,
                                        1 - 2.f * (q.z * q.z + q.w * q.w));// Yaw
            v.x    = (float) std::asin(2.f * (q.x * q.z - q.w * q.y));     // Pitch
            v.z    = (float) std::atan2(2.f * q.x * q.y + 2.f * q.z * q.w,
                                        1 - 2.f * (q.y * q.y + q.z * q.z));// Roll
        }
        return v;
    }

    Vector3f Quaternion::normalize_angles(Vector3f angles)
    {
        angles.x = normalize_angles(angles.x);
        angles.y = normalize_angles(angles.y);
        angles.z = normalize_angles(angles.z);
        return angles;
    }

    float Quaternion::normalize_angles(float angle)
    {
        float modAngle = std::fmod(angle, 360.0f);

        if (modAngle < 0.0f)
            return modAngle + 360.0f;
        else
            return modAngle;
    }

    Vector3f Quaternion::operator*(Vector3f point) const
    {
        float x2 = x * 2.f;
        float y2 = y * 2.f;
        float z2 = z * 2.f;

        float x2s = x * x2;
        float y2s = y * y2;
        float z2s = z * z2;

        float xy2 = x * y2;
        float xz2 = x * z2;
        float yz2 = y * z2;

        float wx2 = w * x2;
        float wy2 = w * y2;
        float wz2 = w * z2;

        return {(1.f - (y2s + z2s)) * point.x + (xy2 - wz2) * point.y + (xz2 + wy2) * point.z,
                (xy2 + wz2) * point.x + (1.f - (x2s + z2s)) * point.y + (yz2 - wx2) * point.z,
                (xz2 - wy2) * point.x + (yz2 + wx2) * point.y + (1.f - (x2s + y2s)) * point.z};
    }

    Quaternion Quaternion::operator*(Quaternion rhs) const
    {
        return Quaternion(
                w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y, w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z,
                w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x, w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
    }

    void Quaternion::operator*=(Quaternion rhs)
    {
        *this = *this * rhs;
    }

    float Quaternion::dot(Quaternion b) const
    {
        return x * b.x + y * b.y + z * b.z + w * b.w;
    }

    bool Quaternion::operator==(Quaternion other) const
    {
        return Quaternion::dot(other) > 0.999999f;
    }

    bool Quaternion::operator!=(Quaternion other) const
    {
        return !(*this == other);
    }

    // Originally based on:
    //  https://gist.github.com/HelloKitty/91b7af87aac6796c3da9#file-quaternion-cs-L644
    // Further research finds that the above link is not an equivalent implementation.
    Quaternion Quaternion::euler(float x, float y, float z)
    {
        float yaw         = x * 0.0174532924f;
        float sinYawOver2 = std::sin(yaw * 0.5f);
        float cosYawOver2 = std::cos(yaw * 0.5f);

        float pitch         = y * 0.0174532924f;
        float sinPitchOver2 = std::sin(pitch * 0.5f);
        float cosPitchOver2 = std::cos(pitch * 0.5f);

        float roll         = z * 0.0174532924f;
        float sinRollOver2 = std::sin(roll * 0.5f);
        float cosRollOver2 = std::cos(roll * 0.5f);

        // Important for precision (manual inlining might affect precision)
        float sinYawCosPitch = (sinYawOver2 * cosPitchOver2);
        float cosYawSinPitch = (cosYawOver2 * sinPitchOver2);
        float cosYawCosPitch = (cosYawOver2 * cosPitchOver2);
        sinYawOver2          = -sinYawOver2 * sinPitchOver2;

        Quaternion result;
        result.x = cosRollOver2 * sinYawCosPitch + sinRollOver2 * cosYawSinPitch;
        result.y = cosRollOver2 * cosYawSinPitch - sinRollOver2 * sinYawCosPitch;
        result.z = cosRollOver2 * sinYawOver2 + sinRollOver2 * cosYawCosPitch;
        result.w = cosRollOver2 * cosYawCosPitch - sinRollOver2 * sinYawOver2;

        return result;
    }

    // https://web.archive.org/web/20221126145919/https://gist.github.com/aeroson/043001ca12fe29ee911e
    Quaternion Quaternion::look_rotation(Vector3f forward, Vector3f up)
    {
        // TODO lowercase migrate
        forward        = forward.normal();
        Vector3f right = up.cross(forward).normal();
        up             = forward.cross(right);

        auto m00 = right.x;
        auto m01 = right.y;
        auto m02 = right.z;
        auto m10 = up.x;
        auto m11 = up.y;
        auto m12 = up.z;
        auto m20 = forward.x;
        auto m21 = forward.y;
        auto m22 = forward.z;

        auto num8 = (m00 + m11) + m22;
        Quaternion quaternion;
        if (num8 > 0) {
            auto num     = std::sqrt(num8 + 1.f);
            quaternion.w = num * 0.5f;
            num          = 0.5f / num;
            quaternion.x = (m12 - m21) * num;
            quaternion.y = (m20 - m02) * num;
            quaternion.z = (m01 - m10) * num;
            return quaternion;
        }

        if ((m00 >= m11) && (m00 >= m22)) {
            auto num7    = std::sqrt(((1.f + m00) - m11) - m22);
            auto num4    = 0.5f / num7;
            quaternion.x = 0.5f * num7;
            quaternion.y = (m01 + m10) * num4;
            quaternion.z = (m02 + m20) * num4;
            quaternion.w = (m12 - m21) * num4;
            return quaternion;
        }

        if (m11 > m22) {
            auto num6    = std::sqrt(((1.f + m11) - m00) - m22);
            auto num3    = 0.5f / num6;
            quaternion.x = (m10 + m01) * num3;
            quaternion.y = 0.5f * num6;
            quaternion.z = (m21 + m12) * num3;
            quaternion.w = (m20 - m02) * num3;
            return quaternion;
        }

        auto num5 = std::sqrt(((1.f + m22) - m00) - m11);
        auto num2 = 0.5f / num5;

        quaternion.x = (m20 + m02) * num2;
        quaternion.y = (m21 + m12) * num2;
        quaternion.z = 0.5f * num5;
        quaternion.w = (m01 - m10) * num2;

        return quaternion;
    }

    Quaternion Quaternion::inverse(Quaternion rotation)
    {
        float lengthSq = rotation.length_squared();
        if (lengthSq != 0.0) {
            float i = 1.0f / lengthSq;
            return Quaternion(rotation.xyz() * -i, rotation.w * i);
        }
        return rotation;
    }

}// namespace avledet::util::CSU

std::ostream &operator<<(std::ostream &st, avledet::util::CSU::Quaternion const &quat)
{
    return st << "(" << quat.x << ", " << quat.y << ", " << quat.z << ", " << quat.w << ")";
}