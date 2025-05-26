#pragma once
#include <cmath>
#include <algorithm>

namespace netcode {
namespace math {

/**
 * @brief A 3D vector class with basic vector operations
 * 
 * MyVec3 provides common vector operations like addition, subtraction,
 * scalar multiplication, normalization etc. It uses float components
 * and implements operator overloading for intuitive vector math.
 */
struct MyVec3 {
    float x = 0.0f;  ///< X component of the vector
    float y = 0.0f;  ///< Y component of the vector 
    float z = 0.0f;  ///< Z component of the vector

    /// Default constructor initializes to zero vector
    MyVec3() = default;

    /**
     * @brief Constructs a vector with given x,y,z components
     * @param x X component
     * @param y Y component
     * @param z Z component
     */
    MyVec3(float x, float y, float z) : x(x), y(y), z(z) {}

    /**
     * @brief Adds two vectors
     * @param other Vector to add
     * @return New vector containing the sum
     */
    MyVec3 operator+(const MyVec3& other) const {
        return MyVec3(x + other.x, y + other.y, z + other.z);
    }

    /**
     * @brief Adds another vector to this one
     * @param other Vector to add
     * @return Reference to modified vector
     */
    MyVec3& operator+=(const MyVec3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    /**
     * @brief Subtracts two vectors
     * @param other Vector to subtract
     * @return New vector containing the difference
     */
    MyVec3 operator-(const MyVec3& other) const {
        return MyVec3(x - other.x, y - other.y, z - other.z);
    }

    /**
     * @brief Subtracts another vector from this one
     * @param other Vector to subtract
     * @return Reference to modified vector
     */
    MyVec3& operator-=(const MyVec3& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }

    /**
     * @brief Multiplies vector by a scalar
     * @param scalar Value to multiply by
     * @return New scaled vector
     */
    MyVec3 operator*(float scalar) const {
        return MyVec3(x * scalar, y * scalar, z * scalar);
    }

    /**
     * @brief Multiplies this vector by a scalar
     * @param scalar Value to multiply by
     * @return Reference to modified vector
     */
    MyVec3& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    /**
     * @brief Divides vector by a scalar
     * @param scalar Value to divide by
     * @return New scaled vector
     */
    MyVec3 operator/(float scalar) const {
        // Consider handling division by zero if necessary
        return MyVec3(x / scalar, y / scalar, z / scalar);
    }

    /**
     * @brief Divides this vector by a scalar
     * @param scalar Value to divide by
     * @return Reference to modified vector
     */
    MyVec3& operator/=(float scalar) {
        x /= scalar; y /= scalar; z /= scalar;
        return *this;
    }

    /**
     * @brief Compares two vectors for equality
     * @param other Vector to compare with
     * @return true if vectors are equal within epsilon
     */
    bool operator==(const MyVec3& other) const {
        // Use epsilon comparison for floating-point numbers
        const float epsilon = 0.0001f;
        return (std::fabs(x - other.x) < epsilon) &&
               (std::fabs(y - other.y) < epsilon) &&
               (std::fabs(z - other.z) < epsilon);
    }

    /**
     * @brief Compares two vectors for inequality
     * @param other Vector to compare with
     * @return true if vectors are not equal within epsilon
     */
    bool operator!=(const MyVec3& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Linearly interpolates between two vectors
 * @param start Starting vector
 * @param end Ending vector
 * @param alpha Interpolation factor (0-1)
 * @return Interpolated vector
 */
inline MyVec3 Lerp(const MyVec3& start, const MyVec3& end, float alpha) {
    float t = std::max(0.0f, std::min(1.0f, alpha));
    return start + (end - start) * t;
}

/**
 * @brief Calculates dot product of two vectors
 * @param a First vector
 * @param b Second vector
 * @return Dot product result
 */
inline float Dot(const MyVec3& a, const MyVec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * @brief Calculates squared magnitude of a vector
 * @param v Input vector
 * @return Squared magnitude
 */
inline float MagnitudeSquared(const MyVec3& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

/**
 * @brief Calculates magnitude of a vector
 * @param v Input vector
 * @return Vector magnitude
 */
inline float Magnitude(const MyVec3& v) {
    return std::sqrt(MagnitudeSquared(v));
}

/**
 * @brief Returns normalized (unit length) version of vector
 * @param v Input vector
 * @return Normalized vector, or zero vector if input magnitude is near zero
 */
inline MyVec3 Normalize(const MyVec3& v) {
    float mag = Magnitude(v);
    if (mag > 0.0001f) {
        return v / mag;
    }
    return MyVec3(0,0,0);
}

} // namespace math
} // namespace netcode