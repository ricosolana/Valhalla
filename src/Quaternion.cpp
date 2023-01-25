#include <cmath>
#include <stdexcept>

#include "Quaternion.h"
#include "Vector.h"
#include "VUtils.h"

const Quaternion Quaternion::IDENTITY = { 0, 0, 0, 1 };

Quaternion::Quaternion(float x, float y, float z, float w) 
    : x(x), y(y), z(z), w(w) {}

Vector3 Quaternion::operator*(const Vector3& other) const {
    float dx = x * 2.f;
    float dy = y * 2.f;
    float dz = z * 2.f;

    float xx = x * dx;
    float yy = y * dy;
    float zz = z * dz;

    float xy = x * dy;
    float xz = x * dz;
    float yz = y * dz;

    float wx = w * dx;
    float wy = w * dy;
    float wz = w * dz;

    return { (1.f - (yy + zz)) * other.x + (xy - w) * other.y + (xz + wy) * other.z,
        (xy + wz) * other.x + (1.f - (xx + zz)) * other.y + (yz - wx) * other.z,
        (xz - wy) * other.x + (yz + wx) * other.y + (1.f - (xx + yy)) * other.z };
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
    const Vector3& vec,
    int dims) {

    Quaternion quat = Quaternion::IDENTITY;

    float sx;
    float cx;
    float sy;
    float cy;
    float sz;
    float fVar1;
    float fVar2;
    float fVar3;
    float fVar4;
    float cz;

    // zero out the w component
    quat.w = 0;

    sx = sinf(vec.x * .5f);
    cx = cosf(vec.x * .5f);

    sy = sinf(vec.y * .5f);
    cy = cosf(vec.y * .5f);

    sz = sinf(vec.z * .5f);
    cz = cosf(vec.z * .5f);

    switch (dims) {
    case 0:
        fVar4 = ((cy * cz));
        fVar2 = (sy * cz);
        fVar1 = (cy * sz);
        fVar3 = -sy * sz;

        quat.x = cx * fVar4 - sx * fVar3;
        quat.y = (cx * fVar1) - sx * fVar2;
        quat.z = cx * fVar3 + sx * fVar4;

        return;
    case 1:
        fVar3 = sy * sz;
        fVar1 = cy * sz;
        fVar4 = cy * cz;
        fVar2 = sy * cz;

        quat.x = cx * fVar4 - sx * fVar3;
        quat.y = (cx * fVar1) - sx * fVar2;
        quat.z = cx * fVar3 + sx * fVar4;

        return;
    case 2:
        fVar3 = sx * cz;
        fVar2 = -sx * sz;
        fVar1 = cx * sz;

        cx = cx * cz;

        quat.x = (cy * cx) - sy * fVar2;
        quat.y = (cy * fVar1 + sy * fVar3);
        quat.z = (cy * fVar3) - sy * fVar1;

        return;
    case 3:
        fVar1 = cx * sz;
        fVar2 = sx * sz;
        fVar3 = sx * cz;

        quat.x = (cy * cx * cz) - sy * fVar2;
        quat.y = cy * fVar1 + sy * fVar3;
        quat.z = (cy * fVar3) - sy * fVar1;

        return;
    case 4:
        fVar2 = sx * cy;
        fVar1 = cx * sy;
        fVar3 = cx * cy;

        cx = -sx * sy;

        quat.x = (cz * fVar3) - sz * cx;
        quat.y = cz * cx + sz * fVar3;
        quat.z = cz * fVar2 + sz * fVar1;

        return;
    case 5:
        fVar2 = (sx * cy);
        fVar3 = ((cx * cy));
        fVar1 = (cx * sy);

        cx = (sx * sy);

        quat.x = (cz * fVar3) - sz * cx;
        quat.y = cz * cx + sz * fVar3;
        quat.z = cz * fVar2 + sz * fVar1;

        return;
    default:
        return;
    }


}

Quaternion Quaternion::Euler(float x, float y, float z) {
    return FromEulerRad_Impl(Vector3(x, y, z) * (PI / 180.f), 4);
}

Quaternion Quaternion::LookRotation(Vector3 forward, Vector3 upwards) {
    throw std::runtime_error("not implemented");
}