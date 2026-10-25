#include "Motion.h"
#include <iostream>

CMotion::CMotion()
	: m_ID(-1),
	  m_perSlerp(0.5f),
	  m_boneCount(0),
	  m_frameCount(0),
	  m_paths(nullptr),
	  m_attributes(nullptr),
	  m_eventCount(0),
	  m_bones(nullptr),
	  m_animations(nullptr),
	  m_frames(nullptr)
{
	std::memset(&m_events, 0, sizeof(m_events));
}

CMotion::~CMotion()
{
	DeleteArray(m_attributes);
	DeleteArray(m_paths);
	DeleteArray(m_bones);
	DeleteArray(m_animations);
	DeleteArray(m_frames);
}

void CMotion::Animate(D3DXMATRIX* bones, float currentFrame_, int nextFrame)
{
	if (!bones || !m_frames || m_boneCount <= 0 || m_frameCount <= 0)
		return;

	TMAnimation* frame = nullptr;
	TMAnimation* next = nullptr;
	D3DXQUATERNION slerp;
	D3DXVECTOR3 lerp;
	D3DXMATRIX m1, m2;
	const int currentFrame = (int)currentFrame_;
	const float slp = currentFrame_ - (float)currentFrame;

	int cFrame = currentFrame % m_frameCount;
	int nFrame = nextFrame % m_frameCount;

	for (int i = 0; i < m_boneCount; i++)
	{
		if (m_frames[i].frames)
		{
			frame = &m_frames[i].frames[cFrame];
			next = &m_frames[i].frames[nFrame];

			D3DXQuaternionSlerp(&slerp, &frame->rot, &next->rot, slp);
			D3DXVec3Lerp(&lerp, &frame->pos, &next->pos, slp);
			D3DXMatrixTranslation(&m1, lerp.x, lerp.y, lerp.z);
			D3DXMatrixRotationQuaternion(&m2, &slerp);
			m2 = m2 * m1;

			if (m_bones && m_bones[i].parentID != -1)
				m2 = m2 * bones[m_bones[i].parentID];
		}
		else
		{
			m2 = m_frames[i].TM;

			if (m_bones && m_bones[i].parentID != -1)
				m2 = m2 * bones[m_bones[i].parentID];
		}

		bones[i] = m2;
	}
}

bool CMotion::Load(const std::string& filename)
{
	CFile file;
	if (!file.Open(filename, FileMode::ReadOnly))
		return false;

	int ver = 0;
	file.Read(ver);
	if (ver < VER_MOTION)
	{
		std::cerr << "Motion version not supported: " << filename << std::endl;
		file.Close();
		return false;
	}

	file.Read(m_ID);
	file.Read(m_perSlerp);
	file.Skip(32);

	file.Read(m_boneCount);
	file.Read(m_frameCount);

	int temp = 0;
	file.Read(temp);
	if (temp)
	{
		m_paths = new D3DXVECTOR3[m_frameCount];
		file.Read(m_paths, m_frameCount);
	}

	ReadTM(file, m_boneCount, m_frameCount);

	if (m_frameCount > 0)
	{
		m_attributes = new MotionAttribute[m_frameCount];
		file.Read(m_attributes, m_frameCount);
	}

	file.Read(m_eventCount);
	if (m_eventCount > 0 && m_eventCount <= 4)
		file.Read(m_events, m_eventCount);

	file.Close();
	return true;
}

bool CMotion::Save(const std::string& filename)
{
	CFile file;
	if (!file.Open(filename, FileMode::WriteOnly))
		return false;

	int ver = VER_MOTION;
	file.Write(ver);
	file.Write(m_ID);
	file.Write(m_perSlerp);

	char unused[32];
	std::memset(unused, 0, 32);
	file.WriteBytes(unused, 32);

	file.Write(m_boneCount);
	file.Write(m_frameCount);

	const int temp = (m_paths != nullptr) ? 1 : 0;
	file.Write(temp);
	if (temp)
		file.Write(m_paths, m_frameCount);

	SaveTM(file);

	if (m_frameCount > 0 && m_attributes)
		file.Write(m_attributes, m_frameCount);

	file.Write(m_eventCount);
	if (m_eventCount > 0 && m_eventCount <= 4)
		file.Write(m_events, m_eventCount);

	file.Close();
	return true;
}

void CMotion::ReadTM(CFile& file, int boneCount, int frameCount)
{
	m_boneCount = boneCount;
	m_frameCount = frameCount;

	if (boneCount <= 0) return;

	m_bones = new Bone[boneCount];
	std::memset(m_bones, 0, sizeof(Bone) * m_boneCount);

	int len = 0;
	Bone* bone = nullptr;
	for (int i = 0; i < boneCount; i++)
	{
		bone = &m_bones[i];
		file.Read(len);
		if (len > 0 && len < 32)
			file.Read(bone->name, len);
		else if (len >= 32)
		{
			file.Read(bone->name, 31);
			file.Skip(len - 31);
			bone->name[31] = '\0';
		}
		file.Read(bone->inverseTM);
		file.Read(bone->localTM);
		file.Read(bone->parentID);
	}

	int aniCount = 0;
	file.Read(aniCount);

	if (aniCount > 0)
		m_animations = new TMAnimation[aniCount];

	if (m_frameCount > 0 && !m_attributes)
	{
		m_attributes = new MotionAttribute[m_frameCount];
		std::memset(m_attributes, 0, sizeof(MotionAttribute) * m_frameCount);
	}

	m_frames = new BoneFrame[m_boneCount];

	int frame = 0;
	TMAnimation* ani = m_animations;
	for (int i = 0; i < m_boneCount; i++)
	{
		file.Read(frame);
		if (frame == 1 && ani && m_frameCount > 0)
		{
			m_frames[i].frames = ani;
			file.Read(ani, m_frameCount);
			ani += m_frameCount;
		}
		else
		{
			m_frames[i].frames = nullptr;
			file.Read(m_frames[i].TM);
		}
	}
}

void CMotion::SaveTM(CFile& file)
{
	Bone* bone = nullptr;
	for (int i = 0; i < m_boneCount; i++)
	{
		bone = &m_bones[i];
		const int len = (int)std::strlen(bone->name);
		file.Write(len + 1);
		file.Write(bone->name, len);
		file.Write('\0');
		file.Write(bone->inverseTM);
		file.Write(bone->localTM);
		file.Write(bone->parentID);
	}

	int aniCount = 0;
	for (int i = 0; i < m_boneCount; i++)
		if (m_frames[i].frames)
			aniCount += m_frameCount;

	file.Write(aniCount);

	TMAnimation* ani = m_animations;
	for (int i = 0; i < m_boneCount; i++)
	{
		const int frame = (m_frames[i].frames != nullptr) ? 1 : 0;
		file.Write(frame);
		if (frame == 1 && ani)
		{
			file.Write(ani, m_frameCount);
			ani += m_frameCount;
		}
		else
			file.Write(m_frames[i].TM);
	}
}

CSkeleton::CSkeleton()
	: m_ID(-1),
	  m_boneCount(0),
	  m_bones(nullptr),
	  m_sendVS(false)
{
	D3DXMatrixIdentity(&m_localRH);
	D3DXMatrixIdentity(&m_localLH);
	D3DXMatrixIdentity(&m_localShield);
	D3DXMatrixIdentity(&m_localKnuckle);
	std::memset(m_events, 0, sizeof(m_events));
	std::memset(m_eventParentIDs, 0, sizeof(m_eventParentIDs));
}

CSkeleton::~CSkeleton()
{
	DeleteArray(m_bones);
}

void CSkeleton::ResetBones(D3DXMATRIX* bones, D3DXMATRIX* invBones)
{
	for (int i = 0; i < m_boneCount; i++)
	{
		bones[i] = m_bones[i].TM;
		invBones[i] = m_bones[i].inverseTM;
	}
}

bool CSkeleton::Load(const std::string& filename)
{
	CFile file;
	if (!file.Open(filename, FileMode::ReadOnly))
		return false;

	int ver = 0;
	file.Read(ver);
	if (ver < VER_BONE)
	{
		std::cerr << "Skeleton version not supported: " << filename << std::endl;
		file.Close();
		return false;
	}

	file.Read(m_ID);
	file.Read(m_boneCount);

	if (m_boneCount <= 0)
	{
		file.Close();
		return true;
	}

	int len = 0;
	Bone* bone = nullptr;
	m_bones = new Bone[m_boneCount];
	for (int i = 0; i < m_boneCount; i++)
	{
		bone = &m_bones[i];
		file.Read(len);
		if (len > 0 && len < 32)
			file.Read(bone->name, len);
		else if (len >= 32)
		{
			file.Read(bone->name, 31);
			file.Skip(len - 31);
			bone->name[31] = '\0';
		}
		file.Read(bone->TM);
		file.Read(bone->inverseTM);
		file.Read(bone->localTM);
		file.Read(bone->parentID);
	}

	int temp = 0;
	file.Read(temp);
	m_sendVS = (temp != 0);

	file.Read(m_localRH);
	file.Read(m_localShield);
	file.Read(m_localKnuckle);

	if (ver == 5)
	{
		file.Read(m_events, 4);
		file.Read(m_eventParentIDs, 4);
	}
	else if (ver >= 6)
	{
		file.Read(m_events, MAX_MDL_EVENT);
		file.Read(m_eventParentIDs, MAX_MDL_EVENT);
	}

	if (ver == 7)
		file.Read(m_localLH);

	file.Close();
	return true;
}

bool CSkeleton::Save(const std::string& filename)
{
	CFile file;
	if (!file.Open(filename, FileMode::WriteOnly))
		return false;

	int ver = VER_BONE;
	file.Write(ver);
	file.Write(m_ID);
	file.Write(m_boneCount);

	Bone* bone = nullptr;
	for (int i = 0; i < m_boneCount; i++)
	{
		bone = &m_bones[i];
		const int len = (int)std::strlen(bone->name);
		file.Write(len + 1);
		file.Write(bone->name, len);
		file.Write('\0');
		file.Write(bone->TM);
		file.Write(bone->inverseTM);
		file.Write(bone->localTM);
		file.Write(bone->parentID);
	}

	const int sendVS = m_sendVS ? 1 : 0;
	file.Write(sendVS);

	file.Write(m_localRH);
	file.Write(m_localShield);
	file.Write(m_localKnuckle);
	file.Write(m_events, MAX_MDL_EVENT);
	file.Write(m_eventParentIDs, MAX_MDL_EVENT);
	file.Write(m_localLH);

	file.Close();
	return true;
}
