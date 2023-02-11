#include <cmath>
#include <stdexcept>

#include "Quaternion.h"
#include "Vector.h"
#include "VUtils.h"

const Quaternion Quaternion::IDENTITY = { 0, 0, 0, 1 };

Quaternion::Quaternion(float x, float y, float z, float w) 
    : x(x), y(y), z(z), w(w) {}

Vector3 Quaternion::operator*(const Vector3& point) const {
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

Quaternion Quaternion::operator*(const Quaternion &rhs) const {
    return Quaternion(w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y, w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z, w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x, w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
}




bool Quaternion::operator==(const Quaternion& other) const {
    return x == other.x
        && y == other.y
        && z == other.z
        && w == other.w;
}
bool Quaternion::operator!=(const Quaternion & other) const {
    return !(*this == other);
}

// determine whether the 

// it might be premature optimizaiton to
// pay too much attention to NetSyncis impl's



Quaternion FromEulerRad_Impl(
    const Vector3& refVec,
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

    //Vector3 vec(x * 0.017453292f, y * 0.017453292f, z * 0.017453292f);
    //return FromEulerRad_Impl(Vector3(x, y, z) * (PI / 180.f), 4);
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

Quaternion Quaternion::LookRotation(Vector3 forward, Vector3 upwards) {
    throw std::runtime_error("not implemented");
}