#include "D3DXMath.h"

D3DXMATRIX::D3DXMATRIX()
{
	D3DXMatrixIdentity(this);
}

D3DXMATRIX::D3DXMATRIX(float f11, float f12, float f13, float f14,
                       float f21, float f22, float f23, float f24,
                       float f31, float f32, float f33, float f34,
                       float f41, float f42, float f43, float f44)
{
	_11 = f11; _12 = f12; _13 = f13; _14 = f14;
	_21 = f21; _22 = f22; _23 = f23; _24 = f24;
	_31 = f31; _32 = f32; _33 = f33; _34 = f34;
	_41 = f41; _42 = f42; _43 = f43; _44 = f44;
}

D3DXMATRIX D3DXMATRIX::operator*(const D3DXMATRIX& mat) const
{
	D3DXMATRIX res;
	D3DXMatrixMultiply(&res, this, &mat);
	return res;
}

D3DXMATRIX& D3DXMATRIX::operator*=(const D3DXMATRIX& mat)
{
	D3DXMatrixMultiply(this, this, &mat);
	return *this;
}

D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* out)
{
	if (!out) return nullptr;
	out->_11 = 1.0f; out->_12 = 0.0f; out->_13 = 0.0f; out->_14 = 0.0f;
	out->_21 = 0.0f; out->_22 = 1.0f; out->_23 = 0.0f; out->_24 = 0.0f;
	out->_31 = 0.0f; out->_32 = 0.0f; out->_33 = 1.0f; out->_34 = 0.0f;
	out->_41 = 0.0f; out->_42 = 0.0f; out->_43 = 0.0f; out->_44 = 1.0f;
	return out;
}

D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX* out, const D3DXMATRIX* m1, const D3DXMATRIX* m2)
{
	if (!out || !m1 || !m2) return nullptr;
	D3DXMATRIX temp;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			temp.m[i][j] = m1->m[i][0] * m2->m[0][j] +
			               m1->m[i][1] * m2->m[1][j] +
			               m1->m[i][2] * m2->m[2][j] +
			               m1->m[i][3] * m2->m[3][j];
		}
	}
	*out = temp;
	return out;
}

D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX* out, float x, float y, float z)
{
	if (!out) return nullptr;
	D3DXMatrixIdentity(out);
	out->_41 = x;
	out->_42 = y;
	out->_43 = z;
	return out;
}

D3DXMATRIX* D3DXMatrixScaling(D3DXMATRIX* out, float sx, float sy, float sz)
{
	if (!out) return nullptr;
	D3DXMatrixIdentity(out);
	out->_11 = sx;
	out->_22 = sy;
	out->_33 = sz;
	return out;
}

D3DXMATRIX* D3DXMatrixRotationX(D3DXMATRIX* out, float angle)
{
	if (!out) return nullptr;
	D3DXMatrixIdentity(out);
	float c = std::cos(angle);
	float s = std::sin(angle);
	out->_22 = c;
	out->_23 = s;
	out->_32 = -s;
	out->_33 = c;
	return out;
}

D3DXMATRIX* D3DXMatrixRotationY(D3DXMATRIX* out, float angle)
{
	if (!out) return nullptr;
	D3DXMatrixIdentity(out);
	float c = std::cos(angle);
	float s = std::sin(angle);
	out->_11 = c;
	out->_13 = -s;
	out->_31 = s;
	out->_33 = c;
	return out;
}

D3DXMATRIX* D3DXMatrixRotationZ(D3DXMATRIX* out, float angle)
{
	if (!out) return nullptr;
	D3DXMatrixIdentity(out);
	float c = std::cos(angle);
	float s = std::sin(angle);
	out->_11 = c;
	out->_12 = s;
	out->_21 = -s;
	out->_22 = c;
	return out;
}

D3DXMATRIX* D3DXMatrixRotationQuaternion(D3DXMATRIX* out, const D3DXQUATERNION* q)
{
	if (!out || !q) return nullptr;
	float xx = q->x * q->x;
	float yy = q->y * q->y;
	float zz = q->z * q->z;
	float xy = q->x * q->y;
	float xz = q->x * q->z;
	float yz = q->y * q->z;
	float wx = q->w * q->x;
	float wy = q->w * q->y;
	float wz = q->w * q->z;

	out->_11 = 1.0f - 2.0f * (yy + zz);
	out->_12 = 2.0f * (xy + wz);
	out->_13 = 2.0f * (xz - wy);
	out->_14 = 0.0f;

	out->_21 = 2.0f * (xy - wz);
	out->_22 = 1.0f - 2.0f * (xx + zz);
	out->_23 = 2.0f * (yz + wx);
	out->_24 = 0.0f;

	out->_31 = 2.0f * (xz + wy);
	out->_32 = 2.0f * (yz - wx);
	out->_33 = 1.0f - 2.0f * (xx + yy);
	out->_34 = 0.0f;

	out->_41 = 0.0f;
	out->_42 = 0.0f;
	out->_43 = 0.0f;
	out->_44 = 1.0f;

	return out;
}

D3DXMATRIX* D3DXMatrixTranspose(D3DXMATRIX* out, const D3DXMATRIX* in)
{
	if (!out || !in) return nullptr;
	D3DXMATRIX temp;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			temp.m[i][j] = in->m[j][i];
	*out = temp;
	return out;
}

D3DXMATRIX* D3DXMatrixInverse(D3DXMATRIX* out, float* determinant, const D3DXMATRIX* in)
{
	if (!out || !in) return nullptr;

	float a0 = in->m[0][0] * in->m[1][1] - in->m[0][1] * in->m[1][0];
	float a1 = in->m[0][0] * in->m[1][2] - in->m[0][2] * in->m[1][0];
	float a2 = in->m[0][0] * in->m[1][3] - in->m[0][3] * in->m[1][0];
	float a3 = in->m[0][1] * in->m[1][2] - in->m[0][2] * in->m[1][1];
	float a4 = in->m[0][1] * in->m[1][3] - in->m[0][3] * in->m[1][1];
	float a5 = in->m[0][2] * in->m[1][3] - in->m[0][3] * in->m[1][2];
	float b0 = in->m[2][0] * in->m[3][1] - in->m[2][1] * in->m[3][0];
	float b1 = in->m[2][0] * in->m[3][2] - in->m[2][2] * in->m[3][0];
	float b2 = in->m[2][0] * in->m[3][3] - in->m[2][3] * in->m[3][0];
	float b3 = in->m[2][1] * in->m[3][2] - in->m[2][2] * in->m[3][1];
	float b4 = in->m[2][1] * in->m[3][3] - in->m[2][3] * in->m[3][1];
	float b5 = in->m[2][2] * in->m[3][3] - in->m[2][3] * in->m[3][2];

	float det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;

	if (determinant)
		*determinant = det;

	if (std::abs(det) < 1e-8f)
	{
		D3DXMatrixIdentity(out);
		return nullptr;
	}

	float invDet = 1.0f / det;
	D3DXMATRIX temp;

	temp.m[0][0] = ( in->m[1][1] * b5 - in->m[1][2] * b4 + in->m[1][3] * b3) * invDet;
	temp.m[0][1] = (-in->m[0][1] * b5 + in->m[0][2] * b4 - in->m[0][3] * b3) * invDet;
	temp.m[0][2] = ( in->m[3][1] * a5 - in->m[3][2] * a4 + in->m[3][3] * a3) * invDet;
	temp.m[0][3] = (-in->m[2][1] * a5 + in->m[2][2] * a4 - in->m[2][3] * a3) * invDet;

	temp.m[1][0] = (-in->m[1][0] * b5 + in->m[1][2] * b2 - in->m[1][3] * b1) * invDet;
	temp.m[1][1] = ( in->m[0][0] * b5 - in->m[0][2] * b2 + in->m[0][3] * b1) * invDet;
	temp.m[1][2] = (-in->m[3][0] * a5 + in->m[3][2] * a2 - in->m[3][3] * a1) * invDet;
	temp.m[1][3] = ( in->m[2][0] * a5 - in->m[2][2] * a2 + in->m[2][3] * a1) * invDet;

	temp.m[2][0] = ( in->m[1][0] * b4 - in->m[1][1] * b2 + in->m[1][3] * b0) * invDet;
	temp.m[2][1] = (-in->m[0][0] * b4 + in->m[0][1] * b2 - in->m[0][3] * b0) * invDet;
	temp.m[2][2] = ( in->m[3][0] * a4 - in->m[3][1] * a2 + in->m[3][3] * a0) * invDet;
	temp.m[2][3] = (-in->m[2][0] * a4 + in->m[2][1] * a2 - in->m[2][3] * a0) * invDet;

	temp.m[3][0] = (-in->m[1][0] * b3 + in->m[1][1] * b1 - in->m[1][2] * b0) * invDet;
	temp.m[3][1] = ( in->m[0][0] * b3 - in->m[0][1] * b1 + in->m[0][2] * b0) * invDet;
	temp.m[3][2] = (-in->m[3][0] * a3 + in->m[3][1] * a1 - in->m[3][2] * a0) * invDet;
	temp.m[3][3] = ( in->m[2][0] * a3 - in->m[2][1] * a1 + in->m[2][2] * a0) * invDet;

	*out = temp;
	return out;
}

D3DXQUATERNION* D3DXQuaternionIdentity(D3DXQUATERNION* out)
{
	if (!out) return nullptr;
	out->x = 0.0f;
	out->y = 0.0f;
	out->z = 0.0f;
	out->w = 1.0f;
	return out;
}

D3DXQUATERNION* D3DXQuaternionSlerp(D3DXQUATERNION* out, const D3DXQUATERNION* q1, const D3DXQUATERNION* q2, float t)
{
	if (!out || !q1 || !q2) return nullptr;

	float dot = q1->x * q2->x + q1->y * q2->y + q1->z * q2->z + q1->w * q2->w;
	D3DXQUATERNION target = *q2;

	if (dot < 0.0f)
	{
		dot = -dot;
		target.x = -target.x;
		target.y = -target.y;
		target.z = -target.z;
		target.w = -target.w;
	}

	if (dot > 0.9995f)
	{
		out->x = q1->x + t * (target.x - q1->x);
		out->y = q1->y + t * (target.y - q1->y);
		out->z = q1->z + t * (target.z - q1->z);
		out->w = q1->w + t * (target.w - q1->w);
	}
	else
	{
		float theta_0 = std::acos(dot);
		float theta = theta_0 * t;
		float sin_theta = std::sin(theta);
		float sin_theta_0 = std::sin(theta_0);

		float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
		float s1 = sin_theta / sin_theta_0;

		out->x = (s0 * q1->x) + (s1 * target.x);
		out->y = (s0 * q1->y) + (s1 * target.y);
		out->z = (s0 * q1->z) + (s1 * target.z);
		out->w = (s0 * q1->w) + (s1 * target.w);
	}

	float len = std::sqrt(out->x * out->x + out->y * out->y + out->z * out->z + out->w * out->w);
	if (len > 1e-6f)
	{
		out->x /= len;
		out->y /= len;
		out->z /= len;
		out->w /= len;
	}
	return out;
}

D3DXVECTOR3* D3DXVec3Lerp(D3DXVECTOR3* out, const D3DXVECTOR3* v1, const D3DXVECTOR3* v2, float t)
{
	if (!out || !v1 || !v2) return nullptr;
	out->x = v1->x + t * (v2->x - v1->x);
	out->y = v1->y + t * (v2->y - v1->y);
	out->z = v1->z + t * (v2->z - v1->z);
	return out;
}

D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DXMATRIX* m)
{
	if (!out || !v || !m) return nullptr;
	float x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31 + m->_41;
	float y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32 + m->_42;
	float z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33 + m->_43;
	float w = v->x * m->_14 + v->y * m->_24 + v->z * m->_34 + m->_44;

	if (std::abs(w) > 1e-8f)
	{
		out->x = x / w;
		out->y = y / w;
		out->z = z / w;
	}
	else
	{
		out->x = x;
		out->y = y;
		out->z = z;
	}
	return out;
}

D3DXVECTOR3* D3DXVec3TransformNormal(D3DXVECTOR3* out, const D3DXVECTOR3* v, const D3DXMATRIX* m)
{
	if (!out || !v || !m) return nullptr;
	float x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31;
	float y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32;
	float z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33;
	out->x = x;
	out->y = y;
	out->z = z;
	return out;
}

D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* out, const D3DXVECTOR3* v1, const D3DXVECTOR3* v2)
{
	if (!out || !v1 || !v2) return nullptr;
	float x = v1->y * v2->z - v1->z * v2->y;
	float y = v1->z * v2->x - v1->x * v2->z;
	float z = v1->x * v2->y - v1->y * v2->x;
	out->x = x;
	out->y = y;
	out->z = z;
	return out;
}

float D3DXVec3Dot(const D3DXVECTOR3* v1, const D3DXVECTOR3* v2)
{
	if (!v1 || !v2) return 0.0f;
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

float D3DXVec3Length(const D3DXVECTOR3* v)
{
	if (!v) return 0.0f;
	return std::sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* out, const D3DXVECTOR3* v)
{
	if (!out || !v) return nullptr;
	float len = D3DXVec3Length(v);
	if (len > 1e-8f)
	{
		out->x = v->x / len;
		out->y = v->y / len;
		out->z = v->z / len;
	}
	else
	{
		*out = *v;
	}
	return out;
}

D3DXVECTOR4* D3DXVec4Transform(D3DXVECTOR4* out, const D3DXVECTOR4* v, const D3DXMATRIX* m)
{
	if (!out || !v || !m) return nullptr;
	float x = v->x * m->_11 + v->y * m->_21 + v->z * m->_31 + v->w * m->_41;
	float y = v->x * m->_12 + v->y * m->_22 + v->z * m->_32 + v->w * m->_42;
	float z = v->x * m->_13 + v->y * m->_23 + v->z * m->_33 + v->w * m->_43;
	float w = v->x * m->_14 + v->y * m->_24 + v->z * m->_34 + v->w * m->_44;
	out->x = x;
	out->y = y;
	out->z = z;
	out->w = w;
	return out;
}

D3DXVECTOR4* D3DXVec4Normalize(D3DXVECTOR4* out, const D3DXVECTOR4* v)
{
	if (!out || !v) return nullptr;
	float len = std::sqrt(v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w);
	if (len > 1e-8f)
	{
		out->x = v->x / len;
		out->y = v->y / len;
		out->z = v->z / len;
		out->w = v->w / len;
	}
	else
	{
		*out = *v;
	}
	return out;
}
