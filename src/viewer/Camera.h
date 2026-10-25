#ifndef CAMERA_H
#define CAMERA_H

#include "raylib.h"
#include "../core/D3DXMath.h"

class OrbitCamera
{
public:
	OrbitCamera();

	void Update(Rectangle viewportRect);
	void Reset();
	void FrameModel(const Bounds& bounds);

	Camera3D GetRaylibCamera() const;

	Vector3 GetTarget() const { return m_target; }
	void SetTarget(Vector3 t) { m_target = t; }

	float GetDistance() const { return m_distance; }
	void SetDistance(float d);

	float GetYaw() const { return m_yaw; }
	void SetYaw(float y) { m_yaw = y; }

	float GetPitch() const { return m_pitch; }
	void SetPitch(float p);

	float GetFOV() const { return m_fov; }
	void SetFOV(float f) { m_fov = f; }

private:
	Vector3 m_target;
	float m_distance;
	float m_yaw;   // degrees
	float m_pitch; // degrees
	float m_fov;   // degrees
	Vector2 m_lastMousePos;
	bool m_isOrbiting;
	bool m_isPanning;
};

#endif // CAMERA_H
