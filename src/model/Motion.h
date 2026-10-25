#ifndef MOTION_H
#define MOTION_H

#include "../core/Types.h"
#include "../core/D3DXMath.h"
#include "../core/File.h"
#include "Object3D.h"
#include <string>

struct Bone
{
	int parentID;
	char name[32];
	D3DXMATRIX localTM;
	D3DXMATRIX TM;
	D3DXMATRIX inverseTM;
};

struct BoneFrame
{
	TMAnimation* frames;
	D3DXMATRIX TM;

	BoneFrame() : frames(nullptr) {}
};

class CMotion
{
public:
	CMotion();
	~CMotion();

	bool Load(const std::string& filename);
	void ReadTM(CFile& file, int boneCount, int frameCount);

	bool Save(const std::string& filename);
	void SaveTM(CFile& file);

	MotionAttribute* GetAttributes() const { return m_attributes; }
	int GetFrameCount() const { return m_frameCount; }
	int GetBoneCount() const { return m_boneCount; }
	Bone* GetBones() const { return m_bones; }

	void Animate(D3DXMATRIX* bones, float currentFrame, int nextFrame);

private:
	int m_ID;
	float m_perSlerp;
	int m_boneCount;
	int m_frameCount;
	D3DXVECTOR3* m_paths;
	MotionAttribute* m_attributes;
	D3DXVECTOR3 m_events[4];
	int m_eventCount;
	Bone* m_bones;
	TMAnimation* m_animations;
	BoneFrame* m_frames;
};

class CSkeleton
{
public:
	CSkeleton();
	~CSkeleton();

	bool Load(const std::string& filename);
	bool Save(const std::string& filename);

	void ResetBones(D3DXMATRIX* bones, D3DXMATRIX* invBones);

	int GetBoneCount() const { return m_boneCount; }
	bool MustSendVS() const { return m_sendVS; }
	Bone* GetBones() const { return m_bones; }

private:
	int m_ID;
	int m_boneCount;
	Bone* m_bones;
	bool m_sendVS;
	D3DXMATRIX m_localRH;
	D3DXMATRIX m_localLH;
	D3DXMATRIX m_localShield;
	D3DXMATRIX m_localKnuckle;
	D3DXVECTOR3 m_events[MAX_MDL_EVENT];
	int m_eventParentIDs[MAX_MDL_EVENT];
};

#endif // MOTION_H
