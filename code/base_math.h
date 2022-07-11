/* TODO:
 * Math constants - PI etc.
 * Vector Operations
 * Projection and rejection
 * Cross Product
 * Trig functions
 * Matrices
 * Matrix Operations
 * Matrix Calculations - determinants, inverses etc.
 * Radians and angles
 * Quaternions
 * Remove need for math.h
 * 
 * Use SIMD where appropriate
*/ 
#include <math.h>

union V2 {
	struct {
		f32 x, y;
	};
};

inline V2 operator +(V2 a, V2 b) {
	return {a.x + b.x, a.y + b.y};
}


inline V2 operator -(V2 a, V2 b) {
	return {a.x - b.x, a.y - b.y};
}

inline V2 operator *(V2 a, f32 s) {
	return {a.x * s, a.y * s};
}

inline V2 operator /(V2 a, f32 s) {
	return {a.x / s, a.y / s};
}

inline b32 operator ==(V2 a, V2 b) {
	return ((a.x == b.x) && (a.y == b.y));
}

inline b32 operator !=(V2 a, V2 b) {
	return ((a.x != b.x) || (a.y != b.y));
}

inline f32 Magnitude(V2 a) {
	return (f32)sqrt((a.x * a.x) + (a.y * a.y));
}

inline V2 Normalize(V2 a) {
	return a / Magnitude(a);
}

inline f32 Dot(V2 a, V2 b) {
	return (a.x * b.x) + (a.y * b.y);
}

inline f32 CrossZ(V2 a, V2 b) {
	return (a.x * b.y) - (a.y * b.x);
}

union V3 {
	struct {
		f32 x, y, z;
	};
};

inline V3 operator +(V3 a, V3 b) {
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline V3 operator -(V3 a, V3 b) {
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline V3 operator *(V3 a, f32 s) {
	return {a.x * s, a.y * s, a.z * s};
}

inline V3 operator /(V3 a, f32 s) {
	return {a.x / s, a.y / s, a.z / s};
}

inline b32 operator ==(V3 a, V3 b) {
	return ((a.x == b.x) && (a.y == b.y) && (a.z == b.z));
}

inline b32 operator !=(V3 a, V3 b) {
	return ((a.x != b.x) || (a.y != b.y) || (a.z != b.z));
}

inline f32 Magnitude(V3 a) {
	return (f32)sqrt((a.x * a.x) + (a.y * a.y) + (a.z * a.z));
}

inline V3 Normalize(V3 a) {
	return a / Magnitude(a);
}

inline f32 Dot(V3 a, V3 b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

inline V3 Cross(V3 a, V3 b) {
	return V3{(a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x)};
}

union V4 {
	struct {
		f32 x, y, z, w;
	};

	struct {
		f32 x0, y0, x1, y1;
	};
	struct {
		u32 uX0, uY0, uX1, uY1;
	};
};

//TODO: operators for V4/homogenous coordinates