#pragma once
#include <cmath>
#include <algorithm>

namespace netcode {
namespace math {

struct MyVec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // Constructors
    MyVec3() = default;
    MyVec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Operator Overloads
    MyVec3 operator+(const MyVec3& other) const {
        return MyVec3(x + other.x, y + other.y, z + other.z);
    }
    MyVec3& operator+=(const MyVec3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }
    MyVec3 operator-(const MyVec3& other) const {
        return MyVec3(x - other.x, y - other.y, z - other.z);
    }
    MyVec3& operator-=(const MyVec3& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }
    MyVec3 operator*(float scalar) const {
        return MyVec3(x * scalar, y * scalar, z * scalar);
    }
    MyVec3& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }
    MyVec3 operator/(float scalar) const {
        // Consider handling division by zero if necessary
        return MyVec3(x / scalar, y / scalar, z / scalar);
    }
    MyVec3& operator/=(float scalar) {
        x /= scalar; y /= scalar; z /= scalar;
        return *this;
    }

    bool operator==(const MyVec3& other) const {
        // Use epsilon comparison for floating-point numbers
        const float epsilon = 0.0001f;
        return (std::fabs(x - other.x) < epsilon) &&
               (std::fabs(y - other.y) < epsilon) &&
               (std::fabs(z - other.z) < epsilon);
    }
    bool operator!=(const MyVec3& other) const {
        return !(*this == other);
    }
};

inline MyVec3 Lerp(const MyVec3& start, const MyVec3& end, float alpha) {
    float t = std::max(0.0f, std::min(1.0f, alpha));
    return start + (end - start) * t;
}

inline float Dot(const MyVec3& a, const MyVec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float MagnitudeSquared(const MyVec3& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float Magnitude(const MyVec3& v) {
    return std::sqrt(MagnitudeSquared(v));
}

inline MyVec3 Normalize(const MyVec3& v) {
    float mag = Magnitude(v);
    if (mag > 0.0001f) {
        return v / mag;
    }
    return MyVec3(0,0,0);
}

} // namespace math
} // namespace netcode