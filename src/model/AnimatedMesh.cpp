#include "AnimatedMesh.h"
#include "ModelMng.h"
#include <iostream>
#include <cfloat>

CAnimatedMesh::CAnimatedMesh()
	: m_skeleton(nullptr),
	  m_bones(nullptr),
	  m_invBones(nullptr),
	  m_motion(nullptr)
{
}

CAnimatedMesh::~CAnimatedMesh()
{
	for (int i = 0; i < MAX_ANIMATED_ELEMENTS; i++)
	{
		Delete(m_elements[i].obj);
	}

	Delete(m_motion);
	Delete(m_skeleton);
	DeleteArray(m_bones);
	m_invBones = nullptr;
}

bool CAnimatedMesh::Load(const std::string& filename, int part)
{
	if (part < 0 || part >= MAX_ANIMATED_ELEMENTS)
		return false;

	CObject3D* obj = new CObject3D();
	if (!obj->Load(filename))
	{
		delete obj;
		return false;
	}

	if (m_elements[part].obj)
		delete m_elements[part].obj;

	m_elements[part].obj = obj;

	if (obj->HasSkin())
		m_hasSkin = true;

	if (!m_skeleton && !m_motion && obj->GetFrameCount() > 0)
		m_frameCount = obj->GetFrameCount();

	_updateBounds();
	return true;
}

void CAnimatedMesh::SetMotion(const std::string& filename)
{
	Delete(m_motion);
	m_motion = new CMotion();
	if (!m_motion->Load(filename))
	{
		Delete(m_motion);
		return;
	}

	m_frameCount = m_motion->GetFrameCount();
	m_currentFrame = 0.0f;
}

void CAnimatedMesh::SetSkeleton(const std::string& filename)
{
	Delete(m_skeleton);
	DeleteArray(m_bones);

	m_skeleton = new CSkeleton();
	if (!m_skeleton->Load(filename))
	{
		Delete(m_skeleton);
		return;
	}

	int boneCount = m_skeleton->GetBoneCount();
	if (boneCount > 0)
	{
		m_bones = new D3DXMATRIX[boneCount * 2];
		m_invBones = m_bones + boneCount;
		m_skeleton->ResetBones(m_bones, m_invBones);

		for (int i = 0; i < MAX_ANIMATED_ELEMENTS; i++)
		{
			if (m_elements[i].obj)
				m_elements[i].obj->SetExternBones(m_bones, m_invBones);
		}
	}
}

MotionAttribute* CAnimatedMesh::GetAttributes() const
{
	if (m_motion)
		return m_motion->GetAttributes();
	if (m_elements[0].obj)
		return m_elements[0].obj->GetAttributes();
	return nullptr;
}

void CAnimatedMesh::_updateBounds()
{
	D3DXVECTOR3 bbMin(FLT_MAX, FLT_MAX, FLT_MAX);
	D3DXVECTOR3 bbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	bool hasBounds = false;

	for (int i = 0; i < MAX_ANIMATED_ELEMENTS; i++)
	{
		if (m_elements[i].obj)
		{
			const Bounds& b = m_elements[i].obj->GetBounds();
			bbMin.x = std::min(bbMin.x, b.Min.x);
			bbMin.y = std::min(bbMin.y, b.Min.y);
			bbMin.z = std::min(bbMin.z, b.Min.z);
			bbMax.x = std::max(bbMax.x, b.Max.x);
			bbMax.y = std::max(bbMax.y, b.Max.y);
			bbMax.z = std::max(bbMax.z, b.Max.z);
			hasBounds = true;
		}
	}

	if (hasBounds)
	{
		m_bounds.Min = bbMin;
		m_bounds.Max = bbMax;
	}
}

void CAnimatedMesh::Animate(float currentFrame, int nextFrame, int lod)
{
	if (m_skeleton && m_motion && m_bones)
	{
		m_motion->Animate(m_bones, currentFrame, nextFrame);
	}

	for (int i = 0; i < MAX_ANIMATED_ELEMENTS; i++)
	{
		if (m_elements[i].obj)
		{
			m_elements[i].obj->Animate(currentFrame, nextFrame, lod);
		}
	}
}
