#pragma once

#include <Vector.h>

namespace avledet::util::CSU {

    class Quaternion {
    public:
        static const Quaternion IDENTITY;

        float x, y, z, w;

        Quaternion();
        Quaternion(float x, float y, float z, float w);    
        Quaternion(Vector3f v, float w);
    
        //Quaternion(const Quaternion &other) 
        //    : x(other.x), y(other.y), z(other.z), w(other.w) {}
    
        float length_squared() const;
        Vector3f xyz() const;
        Vector3f euler_angles() const;
        float dot(Quaternion b) const;
    
        Vector3f operator*(Vector3f other) const;
        Quaternion operator*(Quaternion rhs) const;
    
        void operator*=(Quaternion rhs);
    
        bool operator==(Quaternion other) const;
        bool operator!=(Quaternion other) const;
    
        // Returns a Quaternion rotation accepting degrees in z -> x -> y (applied in order)
        static Quaternion euler(float x, float y, float z);
        // Returns a Quaternion rotation accepting degrees in vector z -> x -> y (applied in order)
        static Quaternion euler(Vector3f angles) {
            return euler(angles.x, angles.y, angles.z);
        }
        static Quaternion look_rotation(Vector3f forward, Vector3f upwards);
        //static Quaternion look_rotation(Vector3f forward) {
        //    return look_rotation(forward, Vector3f::Up());
        //}
        static Quaternion inverse(Quaternion rotation);
    
        static Vector3f Internal_ToEulerRad(Quaternion rotation);
    
        static Vector3f normalize_angles(Vector3f angles);
        static float normalize_angles(float angle);
    };

}// namespace avledet::util

template <>
struct avledet::util::Streamer<avledet::util::CSU::Quaternion> {
    void operator()(Writer& writer, avledet::util::CSU::Quaternion const& value) {
        writer.write(value.x);
        writer.write(value.y);
        writer.write(value.z);
        writer.write(value.w);
    }

    decltype(auto) operator()(Reader& reader) {
        return avledet::util::CSU::Quaternion(
            reader.read<float>(),
            reader.read<float>(),
            reader.read<float>(),
            reader.read<float>()
        );
    }
};

std::ostream& operator<<(std::ostream& st, avledet::util::CSU::Quaternion quat);

using Quaternion = avledet::util::CSU::Quaternion;