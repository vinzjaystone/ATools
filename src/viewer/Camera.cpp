#include "Camera.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

OrbitCamera::OrbitCamera()
	: m_target{ 0.0f, 1.0f, 0.0f },
	  m_distance(10.0f),
	  m_yaw(180.0f),
	  m_pitch(25.0f),
	  m_fov(45.0f),
	  m_lastMousePos{ 0.0f, 0.0f },
	  m_isOrbiting(false),
	  m_isPanning(false)
{
}

void OrbitCamera::SetDistance(float d)
{
	m_distance = std::max(0.1f, std::min(d, 500.0f));
}

void OrbitCamera::SetPitch(float p)
{
	m_pitch = std::max(-89.0f, std::min(p, 89.0f));
}

void OrbitCamera::Reset()
{
	m_target = Vector3{ 0.0f, 1.0f, 0.0f };
	m_distance = 10.0f;
	m_yaw = 180.0f;
	m_pitch = 25.0f;
	m_fov = 45.0f;
}

void OrbitCamera::FrameModel(const Bounds& bounds)
{
	m_target.x = (bounds.Min.x + bounds.Max.x) * 0.5f;
	m_target.y = (bounds.Min.y + bounds.Max.y) * 0.5f;
	m_target.z = (bounds.Min.z + bounds.Max.z) * 0.5f;

	float dx = bounds.Max.x - bounds.Min.x;
	float dy = bounds.Max.y - bounds.Min.y;
	float dz = bounds.Max.z - bounds.Min.z;
	float maxDim = std::max(dx, std::max(dy, dz));

	if (maxDim < 0.1f)
		maxDim = 2.0f;

	m_distance = maxDim * 2.2f;
	m_yaw = 180.0f;
	m_pitch = 20.0f;
}

void OrbitCamera::Update(Rectangle viewportRect)
{
	Vector2 mousePos = GetMousePosition();
	bool mouseInViewport = CheckCollisionPointRec(mousePos, viewportRect);

	// Start orbit
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && mouseInViewport)
	{
		if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
			m_isPanning = true;
		else
			m_isOrbiting = true;
		m_lastMousePos = mousePos;
	}
	else if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) && mouseInViewport)
	{
		m_isPanning = true;
		m_lastMousePos = mousePos;
	}

	// Update dragging
	if (m_isOrbiting)
	{
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
		{
			Vector2 delta = Vector2Subtract(mousePos, m_lastMousePos);
			m_yaw -= delta.x * 0.4f;
			SetPitch(m_pitch + delta.y * 0.4f);
			m_lastMousePos = mousePos;
		}
		else
		{
			m_isOrbiting = false;
		}
	}

	if (m_isPanning)
	{
		if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
		   (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))))
		{
			Vector2 delta = Vector2Subtract(mousePos, m_lastMousePos);

			float phi = m_pitch * DEG2RAD;
			float theta = m_yaw * DEG2RAD;

			Vector3 forward = {
				-std::cos(phi) * std::sin(theta),
				-std::sin(phi),
				-std::cos(phi) * std::cos(theta)
			};

			Vector3 up = { 0.0f, 1.0f, 0.0f };
			Vector3 right = Vector3CrossProduct(forward, up);
			right = Vector3Normalize(right);
			Vector3 camUp = Vector3CrossProduct(right, forward);
			camUp = Vector3Normalize(camUp);

			float panSpeed = m_distance * 0.0018f;
			m_target = Vector3Add(m_target, Vector3Scale(right, -delta.x * panSpeed));
			m_target = Vector3Add(m_target, Vector3Scale(camUp, delta.y * panSpeed));

			m_lastMousePos = mousePos;
		}
		else
		{
			m_isPanning = false;
		}
	}

	// Zoom with mouse wheel
	if (mouseInViewport)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			float zoomFactor = 1.0f - wheel * 0.1f;
			SetDistance(m_distance * zoomFactor);
		}
	}
}

Camera3D OrbitCamera::GetRaylibCamera() const
{
	float phi = m_pitch * DEG2RAD;
	float theta = m_yaw * DEG2RAD;

	Vector3 eyeOffset = {
		m_distance * std::cos(phi) * std::sin(theta),
		m_distance * std::sin(phi),
		m_distance * std::cos(phi) * std::cos(theta)
	};

	Camera3D cam;
	cam.position = Vector3Add(m_target, eyeOffset);
	cam.target = m_target;
	cam.up = Vector3{ 0.0f, 1.0f, 0.0f };
	cam.fovy = m_fov;
	cam.projection = CAMERA_PERSPECTIVE;

	return cam;
}
