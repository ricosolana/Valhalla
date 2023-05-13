#include <cmath>
#include <stdexcept>

#include "Quaternion.h"
#include "Vector.h"
#include "VUtils.h"

const Quaternion Quaternion::IDENTITY = { 0, 0, 0, 1 };

Quaternion::Quaternion() 
    : Quaternion(IDENTITY) {}

Quaternion::Quaternion(float x, float y, float z, float w) 
    : x(x), y(y), z(z), w(w) {}

Quaternion::Quaternion(Vector3f v, float w)
{
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    this->w = w;
}



float Quaternion::LengthSquared() const {
    return x * x + y * y + z * z + w * w;
}

Vector3f Quaternion::xyz() const {
    return Vector3f(x, y, z);
}



Vector3f Quaternion::operator*(Vector3f point) const {
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

    return {    (1.f - (y2s + z2s)) * point.x   + (xy2 - wz2) * point.y             + (xz2 + wy2) * point.z,
                (xy2 + wz2) * point.x           + (1.f - (x2s + z2s)) * point.y     + (yz2 - wx2) * point.z,
                (xz2 - wy2) * point.x           + (yz2 + wx2) * point.y             + (1.f - (x2s + y2s)) * point.z };
}

Quaternion Quaternion::operator*(Quaternion rhs) const {
    return Quaternion(w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y, 
        w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z, 
        w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x, 
        w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
}

void Quaternion::operator*=(Quaternion rhs) {
    *this = *this * rhs;
}



bool Quaternion::operator==(Quaternion other) const {
    return x == other.x
        && y == other.y
        && z == other.z
        && w == other.w;
}
bool Quaternion::operator!=(Quaternion other) const {
    return !(*this == other);
}

// determine whether the 

// it might be premature optimizaiton to
// pay too much attention to NetSyncis impl's



Quaternion FromEulerRad_Impl(
    Vector3f refVec,
    int dims) {

    float sx;
    float cx;
    float sy;
    float cy;
    float sz;
    float fVar1;
    float fVar2;
    float fVar3;
    float fVar4;
    float fVar5;

    cx = refVec.x * 0.5;
    sx = sinf(cx);
    cx = cosf(cx);

    cy = refVec.y * 0.5;
    sy = sinf(cy);
    cy = cosf(cy);

    fVar2 = refVec.z * 0.5;
    sz = sinf(fVar2);
    fVar2 = cosf(fVar2);

    switch (dims) {
    case 0:
        fVar5 = ((cy * fVar2 - 0.0) - sy * 0.0) - sz * 0.0;
        fVar3 = (sy * fVar2 + cy * 0.0 + sz * 0.0) - 0.0;
        fVar1 = (cy * sz + fVar2 * 0.0 + sy * 0.0) - 0.0;
        fVar4 = (fVar2 * 0.0 + cy * 0.0 + 0.0) - sy * sz;

        fVar2 = (cx * fVar4 + sx * fVar5 + fVar3 * 0.0) - fVar1 * 0.0;
        sz = (cx * fVar1 + fVar5 * 0.0 + fVar4 * 0.0) - sx * fVar3;
        cx = ((cx * fVar5 - sx * fVar4) - fVar3 * 0.0) - fVar1 * 0.0;

        return Quaternion(
            cx, sz, fVar2, 0
        );

        //break;
    case 1:
        fVar4 = (fVar2 * 0.0 + cy * 0.0 + sy * sz) - 0.0;
        fVar1 = (cy * sz + fVar2 * 0.0 + 0.0) - sy * 0.0;
        fVar5 = ((cy * fVar2 - 0.0) - sy * 0.0) - sz * 0.0;
        fVar3 = (sy * fVar2 + cy * 0.0 + 0.0) - sz * 0.0;

        fVar2 = (cx * fVar4 + sx * fVar5 + fVar3 * 0.0) - fVar1 * 0.0;
        sz = (cx * fVar1 + fVar5 * 0.0 + fVar4 * 0.0) - sx * fVar3;
        cx = ((cx * fVar5 - sx * fVar4) - fVar3 * 0.0) - fVar1 * 0.0;

        return Quaternion(
            cx, sz, fVar2, 0
        );

        //break;
    case 2:
        fVar4 = (sx * fVar2 + cx * 0.0 + sz * 0.0) - 0.0;
        fVar3 = (fVar2 * 0.0 + cx * 0.0 + 0.0) - sx * sz;
        fVar1 = (cx * sz + fVar2 * 0.0 + sx * 0.0) - 0.0;
        cx = ((cx * fVar2 - sx * 0.0) - 0.0) - sz * 0.0;
        sz = fVar3 * 0.0;
        sx = cx * 0.0;

        fVar2 = (cy * fVar4 + sx + sz) - sy * fVar1;
        cx = ((cy * cx - fVar4 * 0.0) - sy * fVar3) - fVar1 * 0.0;
        sz = (cy * fVar1 + sx + sy * fVar4) - sz;

        return Quaternion(
            cx, sz, fVar2, 0
        );

        //break;
    case 3:
        fVar1 = (cx * sz + fVar2 * 0.0 + 0.0) - sx * 0.0;
        fVar3 = (fVar2 * 0.0 + cx * 0.0 + sx * sz) - 0.0;
        fVar4 = (sx * fVar2 + cx * 0.0 + 0.0) - sz * 0.0;
        cx = ((cx * fVar2 - sx * 0.0) - 0.0) - sz * 0.0;
        sz = fVar3 * 0.0;
        sx = cx * 0.0;

        fVar2 = (cy * fVar4 + sx + sz) - sy * fVar1;
        cx = ((cy * cx - fVar4 * 0.0) - sy * fVar3) - fVar1 * 0.0;
        sz = (cy * fVar1 + sx + sy * fVar4) - sz;

        return Quaternion(
            cx, sz, fVar2, 0
        );

        //(Quaternion)CONCAT412(cx, CONCAT48(sz, (ulonglong)(uint)fVar2))

        //break;
    case 4:
        fVar3 = (sx * cy + cx * 0.0 + sy * 0.0) - 0.0;
        fVar1 = (cx * sy + cy * 0.0 + sx * 0.0) - 0.0;
        fVar4 = ((cx * cy - sx * 0.0) - sy * 0.0) - 0.0;
        cx = (cy * 0.0 + cx * 0.0 + 0.0) - sx * sy;
        sx = fVar1 * 0.0;
        sy = fVar3 * 0.0;

        return Quaternion(
            ((fVar2 * fVar4 - sy) - sx) - sz * cx,
            (fVar2 * cx + sz * fVar4 + sy) - sx,
            (fVar2 * fVar3 + fVar4 * 0.0 + sz * fVar1) - cx * 0.0,
            0
        );

        //*quatMemBuf = (Quaternion)
        //    CONCAT412(((fVar2 * fVar4 - sy) - sx) - sz * cx,
        //        CONCAT48((fVar2 * cx + sz * fVar4 + sy) - sx,
        //            (ulonglong)
        //            (uint)((fVar2 * fVar3 + fVar4 * 0.0 + sz * fVar1) - cx * 0.0)));
        //return quatMemBuf;
    case 5:
        fVar3 = (sx * cy + cx * 0.0 + 0.0) - sy * 0.0;
        fVar4 = ((cx * cy - sx * 0.0) - sy * 0.0) - 0.0;
        fVar1 = (cx * sy + cy * 0.0 + 0.0) - sx * 0.0;
        cx = (cy * 0.0 + cx * 0.0 + sx * sy) - 0.0;
        sx = fVar1 * 0.0;
        sy = fVar3 * 0.0;

        return Quaternion(
            ((fVar2 * fVar4 - sy) - sx) - sz * cx,
            (fVar2 * cx + sz * fVar4 + sy) - sx,
            (fVar2 * fVar3 + fVar4 * 0.0 + sz * fVar1) - cx * 0.0,
            0
        );

        //*quatMemBuf = (Quaternion)
        //    CONCAT412(((fVar2 * fVar4 - sy) - sx) - sz * cx,
        //        CONCAT48((fVar2 * cx + sz * fVar4 + sy) - sx,
        //            (ulonglong)
        //            (uint)((fVar2 * fVar3 + fVar4 * 0.0 + sz * fVar1) - cx * 0.0)))
        //    ;
    default:
        return Quaternion(0, 0, 0, 0);
        //goto switchD_180752161_caseD_6;
    }
    //*quatMemBuf = (Quaternion)CONCAT412(cx, CONCAT48(sz, (ulonglong)(uint)fVar2));
//switchD_180752161_caseD_6:
    //return quatMemBuf;
}

// https://stackoverflow.com/questions/12088610/conversion-between-euler-quaternion-like-in-unity3d-engine
Quaternion Quaternion::Euler(float x, float y, float z) {
    // float yaw, float pitch, float roll

    //Vector3f vec(x * 0.017453292f, y * 0.017453292f, z * 0.017453292f);
    //return FromEulerRad_Impl(Vector3f(x, y, z) * (PI / 180.f), 4);
    //return FromEulerRad_Impl(vec, 4);
        
    // degrees to radians
    y *= 0.017453292f;
    x *= 0.017453292f;
    z *= 0.017453292f;

    float rollOver2 = z * 0.5f;
    float sinRollOver2 = sin(rollOver2);
    float cosRollOver2 = cos(rollOver2);
    float pitchOver2 = x * 0.5f;
    float sinPitchOver2 = sin(pitchOver2);
    float cosPitchOver2 = cos(pitchOver2);
    float yawOver2 = y * 0.5f;
    float sinYawOver2 = sin(yawOver2);
    float cosYawOver2 = cos(yawOver2);

    Quaternion result = Quaternion::IDENTITY;

    result.w = cosYawOver2 * cosPitchOver2 * cosRollOver2 + sinYawOver2 * sinPitchOver2 * sinRollOver2;
    result.x = cosYawOver2 * sinPitchOver2 * cosRollOver2 + sinYawOver2 * cosPitchOver2 * sinRollOver2;
    result.y = sinYawOver2 * cosPitchOver2 * cosRollOver2 - cosYawOver2 * sinPitchOver2 * sinRollOver2;
    result.z = cosYawOver2 * cosPitchOver2 * sinRollOver2 - sinYawOver2 * sinPitchOver2 * cosRollOver2;

    return result;
}

// https://web.archive.org/web/20221126145919/https://gist.github.com/aeroson/043001ca12fe29ee911e

Quaternion Quaternion::LookRotation(Vector3f forward, Vector3f up) {
    // from http://answers.unity3d.com/questions/467614/what-is-the-source-code-of-quaternionlookrotation.html
    //private static MyQuaternion LookRotation(ref Vector3f forward, ref Vector3f up)
    {

        //forward = forward.Normalize(); // Vector3f.Normalize(forward);
        //forward.Normalize();

        //auto right = up.Cross(forward).Normalized(); // Vector3f.Normalize(Vector3f.Cross(up, forward));
        //up = forward.Cross(right);

        Vector3f vector = forward.Normal();
        Vector3f vector2 = up.Cross(vector).Normal();
        Vector3f vector3 = vector.Cross(vector2);

        auto m00 = vector2.x;
        auto m01 = vector2.y;
        auto m02 = vector2.z;
        auto m10 = vector3.x;
        auto m11 = vector3.y;
        auto m12 = vector3.z;
        auto m20 = vector.x;
        auto m21 = vector.y;
        auto m22 = vector.z;

        //float m00 = right.x;
        //float m01 = right.y;
        //float m02 = right.z;
        //float m10 = up.x;
        //float m11 = up.y;
        //float m12 = up.z;
        //float m20 = forward.x;
        //float m21 = forward.y;
        //float m22 = forward.z;

        auto num8 = (m00 + m11) + m22;
        Quaternion quaternion = Quaternion::IDENTITY;
        if (num8 > 0)
        {
            auto num = std::sqrt(num8 + 1);
            quaternion.w = num * 0.5f;
            num = 0.5f / num;
            quaternion.x = (m12 - m21) * num;
            quaternion.y = (m20 - m02) * num;
            quaternion.z = (m01 - m10) * num;
            return quaternion;
        }

        if ((m00 >= m11) && (m00 >= m22))
        {
            auto num7 = std::sqrt(((1 + m00) - m11) - m22);
            auto num4 = 0.5f / num7;
            quaternion.x = 0.5f * num7;
            quaternion.y = (m01 + m10) * num4;
            quaternion.z = (m02 + m20) * num4;
            quaternion.w = (m12 - m21) * num4;
            return quaternion;
        }

        if (m11 > m22)
        {
            auto num6 = std::sqrt(((1 + m11) - m00) - m22);
            auto num3 = 0.5f / num6;
            quaternion.x = (m10 + m01) * num3;
            quaternion.y = 0.5f * num6;
            quaternion.z = (m21 + m12) * num3;
            quaternion.w = (m20 - m02) * num3;
            return quaternion;
        }

        auto num5 = std::sqrt(((1 + m22) - m00) - m11);
        auto num2 = 0.5f / num5;
        quaternion.x = (m20 + m02) * num2;
        quaternion.y = (m21 + m12) * num2;
        quaternion.z = 0.5f * num5;
        quaternion.w = (m01 - m10) * num2;

        return quaternion;
    }
}

Quaternion Quaternion::Inverse(Quaternion rotation) {
    float lengthSq = rotation.LengthSquared();
    if (lengthSq != 0.0) {
        float i = 1.0f / lengthSq;
        return Quaternion(rotation.xyz() * -i, rotation.w * i);
    }
    return rotation;
}

std::ostream& operator<<(std::ostream& st, Quaternion quat) {
    return st << "(" << quat.x << ", " << quat.y << ", " << quat.z << ", " << quat.w << ")";
}
