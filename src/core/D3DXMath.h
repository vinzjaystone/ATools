#ifndef D3DXMATH_H
#define D3DXMATH_H

#include "Types.h"

struct D3DXVECTOR2
{
	float x, y;

	D3DXVECTOR2() : x(0.0f), y(0.0f) {}
	D3DXVECTOR2(float _x, float _y) : x(_x), y(_y) {}

	D3DXVECTOR2 operator+(const D3DXVECTOR2& v) const { return D3DXVECTOR2(x + v.x, y + v.y); }
	D3DXVECTOR2 operator-(const D3DXVECTOR2& v) const { return D3DXVECTOR2(x - v.x, y - v.y); }
	D3DXVECTOR2 operator*(float f) const { return D3DXVECTOR2(x * f, y * f); }
	D3DXVECTOR2 operator/(float f) const { return D3DXVECTOR2(x / f, y / f); }
	D3DXVECTOR2& operator+=(const D3DXVECTOR2& v) { x += v.x; y += v.y; return *this; }
	D3DXVECTOR2& operator-=(const D3DXVECTOR2& v) { x -= v.x; y -= v.y; return *this; }
	D3DXVECTOR2& operator*=(float f) { x *= f; y *= f; return *this; }
	D3DXVECTOR2& operator/=(float f) { x /= f; y /= f; return *this; }
	bool operator==(const D3DXVECTOR2& v) const { return x == v.x && y == v.y; }
	bool operator!=(const D3DXVECTOR2& v) const { return !(*this == v); }
};

struct D3DXVECTOR3
{
	float x, y, z;

	D3DXVECTOR3() : x(0.0f), y(0.0f), z(0.0f) {}
	D3DXVECTOR3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

	D3DXVECTOR3 operator+(const D3DXVECTOR3& v) const { return D3DXVECTOR3(x + v.x, y + v.y, z + v.z); }
	D3DXVECTOR3 operator-(const D3DXVECTOR3& v) const { return D3DXVECTOR3(x - v.x, y - v.y, z - v.z); }
	D3DXVECTOR3 operator*(float f) const { return D3DXVECTOR3(x * f, y * f, z * f); }
	D3DXVECTOR3 operator/(float f) const { return D3DXVECTOR3(x / f, y / f, z / f); }
	D3DXVECTOR3 operator-() const { return D3DXVECTOR3(-x, -y, -z); }
	D3DXVECTOR3& operator+=(const D3DXVECTOR3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	D3DXVECTOR3& operator-=(const D3DXVECTOR3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	D3DXVECTOR3& operator*=(float f) { x *= f; y *= f; z *= f; return *this; }
	D3DXVECTOR3& operator/=(float f) { x /= f; y /= f; z /= f; return *this; }
	bool operator==(const D3DXVECTOR3& v) const { return x == v.x && y == v.y && z == v.z; }
	bool operator!=(const D3DXVECTOR3& v) const { return !(*this == v); }
};

struct D3DXVECTOR4
{
	float x, y, z, w;

	D3DXVECTOR4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	D3DXVECTOR4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

	D3DXVECTOR4 operator+(const D3DXVECTOR4& v) const { return D3DXVECTOR4(x + v.x, y + v.y, z + v.z, w + v.w); }
	D3DXVECTOR4 operator-(const D3DXVECTOR4& v) const { return D3DXVECTOR4(x - v.x, y - v.y, z - v.z, w - v.w); }
	D3DXVECTOR4 operator*(float f) const { return D3DXVECTOR4(x * f, y * f, z * f, w * f); }
};

struct D3DXQUATERNION
{
	float x, y, z, w;

	D3DXQUATERNION() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
	D3DXQUATERNION(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

struct D3DXMATRIX
{
	union
	{
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};

	D3DXMATRIX();
	D3DXMATRIX(float f11, float f12, float f13, float f14,
	           float f21, float f22, float f23, float f24,
	           float f31, float f32, float f33, float f34,
	           float f41, float f42, float f43, float f44);

	D3DXMATRIX operator*(const D3DXMATRIX& mat) const;
	D3DXMATRIX& operator*=(const D3DXMATRIX& mat);
};

struct Bounds
{
	D3DXVECTOR3 Min;
	D3DXVECTOR3 Max;

	Bounds() : Min(0.0f, 0.0f, 0.0f), Max(0.0f, 0.0f, 0.0f) {}
	Bounds(const D3DXVECTOR3& _min, const D3DXVECTOR3& _max) : Min(_min), Max(_max) {}
};

struct D3DCOLORVALUE
{
	float r, g, b, a;
};

struct D3DMATERIAL9
{
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Ambient;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Emissive;
	float Power;
};

// Math functions
D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* out);
D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX* out, const D3DXMATRIX* m1, const D3DXMATRIX* m2);
D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX* out, float x, float y, float z);
D3DXMATRIX* D3DXMatrixScaling(D3DXMATRIX* out, float sx, float sy, float sz);
D3DXMATRIX* D3DXMatrixRotationX(D3DXMATRIX* out, float angle);
D3DXMATRIX* D3DXMatrixRotationY(D3DXMATRIX* out, float angle);
D3DXMATRIX* D3DXMatrixRotationZ(D3DXMATRIX* out, float angle);
D3DXMATRIX* D3DXMatrixRotationQuaternion(D3DXMATRIX* out, const D3DXQUATERNION* q);
D3DXMATRIX* D3DXMatrixRotationYawPitchRoll(D3DXMATRIX* out, float yaw, float pitch, float roll);
D3DXMATRIX* D3DXMatrixTranspose(D3DXMATRIX* out, const D3DXMATRIX* in);
D3DXMATRIX* D3DXMatrixInverse(D3DXMATRIX* out, float* determinant, const D3DXMATRIX* in);

D3DXQUATERNION* D3DXQuaternionIdentity(D3DXQUATERNION* out);
D3DXQUATERNION* D3DXQuaternionRotationYawPitchRoll(D3DXQUATERNION* out, float yaw, float pitch, float roll);
D3DXQUATERNION* D3DXQuaternionRotationAxis(D3DXQUATERNION* out, const D3DXVECTOR3* axis, float angle);
D3DXQUATERNION* D3DXQuaternionSlerp(D3DXQUATERNION* out, const D3DXQUATERNION* q1, const D3DXQUATERNION* q2, float t);

D3DXVECTOR3* D3DXVec3Lerp(D3DXVECTOR3* out, const D3DXVECTOR3* v1, const D3DXVECTOR3* v2, float t);
D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DXMATRIX* m);
D3DXVECTOR3* D3DXVec3TransformNormal(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DXMATRIX* m);
D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* out, const D3DXVECTOR3* v1, const D3DXVECTOR3* v2);
float D3DXVec3Dot(const D3DXVECTOR3* v1, const D3DXVECTOR3* v2);
float D3DXVec3Length(const D3DXVECTOR3* v);
D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* out, const D3DXVECTOR3* v);

D3DXVECTOR4* D3DXVec4Transform(D3DXVECTOR4* out, const D3DXVECTOR4* v, const D3DXMATRIX* m);
D3DXVECTOR4* D3DXVec4Normalize(D3DXVECTOR4* out, const D3DXVECTOR4* v);

#endif // D3DXMATH_H
