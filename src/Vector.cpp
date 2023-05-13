#include "Vector.h"
#include "VUtils.h"
#include "VUtilsMath.h"

std::ostream& operator<<(std::ostream& st, Vector2f vec) {
    return st << "(" << vec.x << ", " << vec.y << ")";
}

std::ostream& operator<<(std::ostream& st, Vector2i vec) {
    return st << "(" << vec.x << ", " << vec.y << ")";
}

std::ostream& operator<<(std::ostream& st, Vector3f vec) {
    return st << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
}
