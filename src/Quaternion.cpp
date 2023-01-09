#include "Quaternion.h"

const Quaternion Quaternion::IDENTITY = { 0, 0, 0, 1 };

Quaternion::Quaternion(float x, float y, float z, float w) 
    : x(x), y(y), z(z), w(w) {}

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



