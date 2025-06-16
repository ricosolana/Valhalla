#pragma once

#include <cstdint>
#include <cmath>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/HelperMacros.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/bundled/fmt/ostream.h>
#include <quill/bundled/fmt/ranges.h>
#include <quill/sinks/ConsoleSink.h>
#include <ankerl/unordered_dense.h>

#include "CompileSettings.h"

#include "DataStream.h"


namespace avledet::util::CSU {
    template<typename T> requires std::is_arithmetic_v<T>
    struct Vector2 {
        T x, y;

        //static const Vector2<T> ZERO;
        //static const Vector2<T> UP;
        //static const Vector2<T> DOWN;
        //static const Vector2<T> FORWARD;

        constexpr Vector2() : x(0), y(0) { }
        constexpr Vector2(const T x, const T y) : x(x), y(y) { }
        //constexpr Vector2(Vector2<T> const& rhs) : x(rhs.x), y(rhs.y) {}

        //void operator=(Vector2<T> const& rhs) {
        //    x = rhs.x;
        //    y = rhs.y;
        //}


        // vector arithmetic
        Vector2<T> operator+(Vector2<T> const& rhs) const {
            return Vector2<T>(x + rhs.x, y + rhs.y);
        }

        Vector2<T> operator-(Vector2<T> const& rhs) const {
            return Vector2<T>(x - rhs.x, y - rhs.y);
        }

        Vector2<T> operator-() const {
            return Vector2<T>(-x, -y);
        }

        Vector2<T> operator*(Vector2<T> const& rhs) const {
            return Vector2<T>(x * rhs.x, y * rhs.y);
        }

        Vector2<T> operator/(Vector2<T> const& rhs) const {
            return Vector2<T>(x / rhs.x, y / rhs.y);
        }

        Vector2<T> operator*(float rhs) const {
            return Vector2<T>(x * rhs, y * rhs);
        }

        Vector2<T> operator/(float rhs) const {
            return Vector2<T>(x / rhs, y / rhs);
        }



        void operator+=(Vector2<T> const& rhs) {
            *this = *this + rhs;
        }

        void operator-=(Vector2<T> const& rhs) {
            *this = *this - rhs;
        }

        void operator*=(Vector2<T> const& rhs) {
            *this = *this * rhs;
        }

        void operator/=(Vector2<T> const& rhs) {
            *this = *this / rhs;
        }

        void operator*=(float rhs) {
            *this = *this * rhs;
        }

        void operator/=(float rhs) {
            *this = *this / rhs;
        }



        constexpr bool operator==(Vector2<T> const& rhs) const {
            return x == rhs.x && y == rhs.y;
        }

        constexpr bool operator!=(Vector2<T> const& rhs) const {
            return !(*this == rhs);
        }



        constexpr float dot(Vector2<T> const& rhs) const {
            return x * rhs.x + y * rhs.y;
        }

        constexpr float sq_magnitude() const {
            return x * x + y * y;
        }

        constexpr float magnitude() const {
            return std::sqrt(this->sq_magnitude());
        }

        constexpr float sq_distance_to(Vector2<T> const& rhs) const {
            return (x - rhs.x) * (x - rhs.x) + (y - rhs.y) * (y - rhs.y);
        }

        constexpr float distance_to(Vector2<T> const& rhs) const {
            return std::sqrt(this->sq_distance_to(rhs));
        }

        constexpr Vector2<T> normal() const {
            auto sq = this->sq_magnitude();

            if (sq > 1E-05f * 1E-05f) {
                return *this / std::sqrt(sq);
                //return *this * VUtils::Math::FISQRT(sqmagnitude);
            } else {
                return zero();
            }
        }

        static constexpr Vector2<T> zero() {
            return Vector2<T>(0, 0);
        }

        // todo quill / fmt formatters...
        friend std::ostream& operator<<(std::ostream& st, Vector2<T> const& vec) {
            return st << "(" << vec.x << ", " << vec.y << ")";
        }
    };

    

    template<typename T> requires std::is_arithmetic_v<T>
    struct Vector3 {
        T x, y, z;

        constexpr Vector3() : x(0), y(0), z(0) { }
        constexpr Vector3(const T x, const T y, const T z) : x(x), y(y), z(z) { }
        //constexpr Vector3(Vector3<T> const& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) { }

        void operator=(Vector3<T> const& other) {
            x = other.x;
            y = other.y;
            z = other.z;
        }



        // vector arithmetic
        Vector3<T> operator+(Vector3<T> const& rhs) const {
            return Vector3<T>(x + rhs.x, y + rhs.y, z + rhs.z);
        }

        Vector3<T> operator-(Vector3<T> const& rhs) const {
            return Vector3<T>(x - rhs.x, y - rhs.y, z - rhs.z);
        }

        Vector3<T> operator-() const {
            return Vector3<T>(-x, -y, -z);
        }

        Vector3<T> operator*(Vector3<T> const& rhs) const {
            return Vector3<T>(x * rhs.x, y * rhs.y, z * rhs.z);
        }

        Vector3<T> operator/(Vector3<T> const& rhs) const {
            return Vector3<T>(x / rhs.x, y / rhs.y, z / rhs.z);
        }

        Vector3<T> operator*(float rhs) const {
            return Vector3<T>(x * rhs, y * rhs, z * rhs);
        }

        Vector3<T> operator/(float rhs) const {
            return Vector3<T>(x / rhs, y / rhs, z / rhs);
        }



        void operator+=(Vector3<T> const& rhs) {
            *this = *this + rhs;
        }

        void operator-=(Vector3<T> const& rhs) {
            *this = *this - rhs;
        }

        void operator*=(Vector3<T> const& rhs) {
            *this = *this * rhs;
        }

        void operator/=(Vector3<T> const& rhs) {
            *this = *this / rhs;
        }

        void operator*=(float rhs) {
            *this = *this * rhs;
        }

        void operator/=(float rhs) {
            *this = *this / rhs;
        }



        constexpr bool operator==(Vector3<T> const& rhs) const {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }

        constexpr bool operator!=(Vector3<T> const& rhs) const {
            return !(*this == rhs);
        }



        constexpr Vector3<T> cross(Vector3<T> const& rhs) const {
            return Vector3<T>(
                y * rhs.z - z * rhs.y,
                z * rhs.x - x * rhs.z,
                x * rhs.y - y * rhs.x
            );
        }

        constexpr float dot(Vector3<T> const& rhs) const {
            return x * rhs.x
                + y * rhs.y
                + z * rhs.z;
        }

        constexpr float sq_magnitude() const {
            return x * x
                + y * y
                + z * z;
        }

        constexpr float magnitude() const {
            return std::sqrt(this->sq_magnitude());
        }

        constexpr float sq_distance_to(Vector3<T> const& rhs) const {
            return (x - rhs.x) * (x - rhs.x)
                + (y - rhs.y) * (y - rhs.y)
                + (z - rhs.z) * (z - rhs.z);
        }

        constexpr float distance_to(Vector3<T> const& rhs) const {
            return std::sqrt(this->sq_distance_to(rhs));
        }

        constexpr Vector3<T> normal() const {
            auto sq = this->sq_magnitude();

            if (sq > 1E-05f * 1E-05f * 1E-05f) {
                return *this / std::sqrt(sq);
                //return *this * VUtils::Math::FISQRT(sqmagnitude);
            } else {
                return zero();
            }
        }



        constexpr float sq_magnitude_h() const {
            return x * x
                + z * z;
        }

        constexpr float magnitude_h() const {
            return std::sqrt(this->sq_magnitude_h());
        }

        constexpr float sq_distance_to_h(Vector3<T> const& rhs) const {
            return (x - rhs.x) * (x - rhs.x)
                + (z - rhs.z) * (z - rhs.z);
        }

        //TODO name
        constexpr float HDistance(Vector3<T> const& rhs) const {
            return std::sqrt(sq_distance_to_h(rhs));
        }



        // TODO
        //  define these as constants instead of functions
        static constexpr Vector3<T> zero() {
            return Vector3<T>(0, 0, 0);
        }

        static constexpr Vector3<T> up() {
            return Vector3<T>(0, 1, 0);
        }

        static constexpr Vector3<T> down() {
            return Vector3<T>(0, -1, 0);
        }

        static constexpr Vector3<T> forward() {
            return Vector3<T>(0, 0, 1);
        }

        friend std::ostream& operator<<(std::ostream& st, Vector3<T> const& vec) {
            return st << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
        }
    };

#if VH_IS_ON(VH_USE_MODS)
    //void init_api(sol::table api_table);
#endif

    using Vector2f = Vector2<float>;
    using Vector2i = Vector2<std::int32_t>;
    using Vector2s = Vector2<std::int16_t>;

    using Vector3f = Vector3<float>;
}


// Serializers / Deserializers
namespace avledet::util {
    template <class Num>
    struct Streamer<avledet::util::CSU::Vector2<Num>> {
        void operator()(Writer& writer, avledet::util::CSU::Vector2<Num> const& value) {
            writer.write(value.x);
            writer.write(value.y);
        }

        decltype(auto) operator()(Reader& reader) {
            auto x = reader.read<Num>();
            auto y = reader.read<Num>();
            return avledet::util::CSU::Vector2<Num>(x, y);
        }
    };

    template <class Num>
    struct Streamer<avledet::util::CSU::Vector3<Num>> {
        void operator()(Writer& writer, avledet::util::CSU::Vector3<Num> const& value) {
            writer.write(value.x);
            writer.write(value.y);
            writer.write(value.z);
        }
        
        decltype(auto) operator()(Reader& reader) {
            auto x = reader.read<Num>();
            auto y = reader.read<Num>();
            auto z = reader.read<Num>();
            return avledet::util::CSU::Vector3<Num>(x, y, z);
        }
    };
}


// hashable
template <>
struct ankerl::unordered_dense::hash<avledet::util::CSU::Vector2i> {
    using is_avalanching = void;

    static_assert(std::has_unique_object_representations_v<avledet::util::CSU::Vector2i>);

    auto operator()(avledet::util::CSU::Vector2i const& value) const noexcept -> std::uint64_t {
        return ankerl::unordered_dense::detail::wyhash::hash(&value, sizeof(value));
    }
};

template <>
struct ankerl::unordered_dense::hash<avledet::util::CSU::Vector2s> {
    using is_avalanching = void;

    static_assert(std::has_unique_object_representations_v<avledet::util::CSU::Vector2s>);

    auto operator()(avledet::util::CSU::Vector2s v) const noexcept -> std::uint64_t {
        return ankerl::unordered_dense::detail::wyhash::hash(&v, sizeof(v));
    }
};

template <>
struct ankerl::unordered_dense::hash<avledet::util::CSU::Vector3f> {
    using is_avalanching = void;

    //static_assert(std::has_unique_object_representations_v<avledet::util::CSU::Vector3f>);

    auto operator()(avledet::util::CSU::Vector3f const& value) const /*noexcept*/ -> std::uint64_t {
        //return ankerl::unordered_dense::detail::wyhash::hash(&value, sizeof(value));

        using namespace ankerl::unordered_dense::detail::wyhash;
        std::uint64_t x = *reinterpret_cast<const std::uint32_t*>(&value.x);
        std::uint64_t y = *reinterpret_cast<const std::uint32_t*>(&value.y);
        std::uint64_t z = *reinterpret_cast<const std::uint32_t*>(&value.z);
        return mix((x << 32) | y, z);
    }
};



// Quill loggable
//std::ostream& operator<<(std::ostream& st, avledet::util::CSU::Vector2f const& vec);
//std::ostream& operator<<(std::ostream& st, avledet::util::CSU::Vector2i const& vec);
//std::ostream& operator<<(std::ostream& st, avledet::util::CSU::Vector2s const& vec);
//std::ostream& operator<<(std::ostream& st, avledet::util::CSU::Vector3f const& vec);



// Quill spec
QUILL_LOGGABLE_DEFERRED_FORMAT(avledet::util::CSU::Vector2f)
QUILL_LOGGABLE_DEFERRED_FORMAT(avledet::util::CSU::Vector2i)
QUILL_LOGGABLE_DEFERRED_FORMAT(avledet::util::CSU::Vector2s)
QUILL_LOGGABLE_DEFERRED_FORMAT(avledet::util::CSU::Vector3f)



// Compatibility usings
//  TODO remove once migrations are completed
using Vector2f = avledet::util::CSU::Vector2f;
using Vector2i = avledet::util::CSU::Vector2i;
using Vector2s = avledet::util::CSU::Vector2s;
using Vector3f = avledet::util::CSU::Vector3f;