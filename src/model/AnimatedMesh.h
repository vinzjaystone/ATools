#ifndef ANIMATEDMESH_H
#define ANIMATEDMESH_H

#include "Model.h"
#include "Object3D.h"
#include "Motion.h"

class CAnimatedMesh : public CModel
{
public:
	CAnimatedMesh();
	virtual ~CAnimatedMesh();

	virtual bool Load(const std::string& filename) override
	{
		return Load(filename, 0);
	}

	bool Load(const std::string& filename, int part);
	void SetMotion(const std::string& filename);
	void SetSkeleton(const std::string& filename);

	MotionAttribute* GetAttributes() const;

	CObject3D* GetElement(int part) {
		if (part >= 0 && part < MAX_ANIMATED_ELEMENTS)
			return m_elements[part].obj;
		return nullptr;
	}

	const CObject3D* GetElement(int part) const {
		if (part >= 0 && part < MAX_ANIMATED_ELEMENTS)
			return m_elements[part].obj;
		return nullptr;
	}

	CSkeleton* GetSkeleton() { return m_skeleton; }
	CMotion* GetMotion() { return m_motion; }
	D3DXMATRIX* GetBones() { return m_bones; }
	D3DXMATRIX* GetInvBones() { return m_invBones; }

	void SetElement(int part, CObject3D* obj) {
		if (part >= 0 && part < MAX_ANIMATED_ELEMENTS)
		{
			if (m_elements[part].obj)
				delete m_elements[part].obj;
			m_elements[part].obj = obj;
			_updateBounds();
		}
	}

	void Animate(float currentFrame, int nextFrame, int lod = 0);

private:
	struct Element
	{
		CObject3D* obj;
		Element() : obj(nullptr) {}
	};

	Element m_elements[MAX_ANIMATED_ELEMENTS];
	CSkeleton* m_skeleton;
	D3DXMATRIX* m_bones;
	D3DXMATRIX* m_invBones;
	CMotion* m_motion;

	void _updateBounds();
};

#endif // ANIMATEDMESH_H
