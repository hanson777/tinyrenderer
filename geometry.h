#pragma once
#include <cassert>
#include <cmath>
#include <iostream>

template <int n> struct vec {
    double data[n] = { 0 };
    double& operator[](const int i) {
        assert(i >= 0 && i < n);
        return data[i];
    }
    double operator[](const int i) const {
        assert(i >= 0 && i < n);
        return data[i];
    }
};

template <> struct vec<2> {
    double x = 0, y = 0;
    double& operator[](const int i) {
        assert(i >= 0 && i < 2);
        return i == 0 ? x : y;
    }
    double operator[](const int i) const {
        assert(i >= 0 && i < 2);
        return i == 0 ? x : y;
    }
};

template <> struct vec<3> {
    double x = 0, y = 0, z = 0;
    double& operator[](const int i) {
        assert(i >= 0 && i < 3);
        return i ? (1 == i ? y : z) : x;
    }
    double operator[](const int i) const {
        assert(i >= 0 && i < 3);
        return i ? (1 == i ? y : z) : x;
    }
};

template <> struct vec<4> {
    double x = 0, y = 0, z = 0, w = 0;
    double& operator[](const int i) {
        assert(i >= 0 && i < 4);
        return i ? (1 == i ? y : (2 == i ? z : w)) : x;
    }
    double operator[](const int i) const {
        assert(i >= 0 && i < 4);
        return i ? (1 == i ? y : (2 == i ? z : w)) : x;
    }
};

template <int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    for (int i = 0; i < n; i++)
        out << v[i] << " ";
    return out;
}

template <int n> vec<n> operator+(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> result = rhs;
    for (int i = 0; i < n; i++) {
        result[i] += lhs[i];
    }
    return result;
}

template <int n> vec<n> operator-(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> result = rhs;
    for (int i = 0; i < n; i++) {
        result[i] -= lhs[i];
    }
    return result;
}

template <int n> double dot(const vec<n>& lhs, const vec<n>& rhs) {
    double result = 0;
    for (int i = 0; i < n; i++)
        result += lhs[i] * rhs[i];
    return result;
}

template <int n> vec<n> operator*(const vec<n>& lhs, const double scalar) {
    vec<n> result = lhs;
    for (int i = 0; i < n; i++) {
        result[i] *= scalar;
    }
    return result;
}

template <int n> vec<n> operator*(const double& scalar, const vec<n>& rhs) { return rhs * scalar; }

template <int n> vec<n> operator/(const vec<n>& lhs, const double scalar) { return lhs * (1.0 / scalar); }

template <int n> double length(const vec<n>& v) { return std::sqrt(dot(v, v)); }

template <int n> vec<n> norm(const vec<n> v) { return v / (length(v)); }

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;

inline vec3 cross(const vec3& v1, const vec3& v2) {
    return { v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x };
}
