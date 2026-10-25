#include "Object3D.h"
#include "Motion.h"
#include "TextureMng.h"
#include <iostream>
#include <cfloat>

GMObject::GMObject()
	: ID(-1),
	  type(GMT_NORMAL),
	  usedBoneCount(0),
	  parentID(-1),
	  parentType(GMT_ERROR),
	  bounds(),
	  opacity(false),
	  bump(false),
	  rigid(false),
	  vertexCount(0),
	  vertexListCount(0),
	  faceListCount(0),
	  indexCount(0),
	  vertexList(nullptr),
	  vertices(nullptr),
	  indices(nullptr),
	  IIB(nullptr),
	  physiqueVertices(nullptr),
	  material(false),
	  materialCount(0),
	  materials(nullptr),
	  materialBlockCount(0),
	  materialBlocks(nullptr),
	  frames(nullptr),
	  light(false)
{
	std::memset(usedBones, 0, sizeof(usedBones));
	D3DXMatrixIdentity(&transform);
}

GMObject::~GMObject()
{
	Cleanup();
}

void GMObject::Cleanup()
{
	DeleteArray(frames);
	DeleteArray(materialBlocks);
	DeleteArray(materials);
	DeleteArray(indices);
	indices = nullptr;
	IIB = nullptr;
	DeleteArray(vertexList);
	DeleteArray(physiqueVertices);

	if (vertices)
	{
		if (type == GMT_SKIN)
			delete[] (SkinVertex*)vertices;
		else
			delete[] (NormalVertex*)vertices;
		vertices = nullptr;
	}

	vertexCount = 0;
	vertexListCount = 0;
	faceListCount = 0;
	indexCount = 0;
	materialCount = 0;
	materialBlockCount = 0;
}

CObject3D::CObject3D()
	: m_ID(-1),
	  m_bounds(),
	  m_scrlU(0.0f),
	  m_scrlV(0.0f),
	  m_perSlerp(0.5f),
	  m_frameCount(0),
	  m_eventCount(0),
	  m_LOD(false),
	  m_boneCount(0),
	  m_attributes(nullptr),
	  m_baseBones(nullptr),
	  m_baseInvBones(nullptr),
	  m_externBones(nullptr),
	  m_externInvBones(nullptr),
	  m_sendVS(false),
	  m_motion(nullptr),
	  m_hasSkin(false)
{
	m_forces[0] = D3DXVECTOR3();
	m_forces[1] = D3DXVECTOR3();
	m_forces[2] = D3DXVECTOR3();
	m_forces[3] = D3DXVECTOR3();
	for (int i = 0; i < 8; i++) m_events[i] = D3DXVECTOR3();
	m_collObj.type = GMT_ERROR;
	m_collObj.parentID = -1;
	m_collObj.parentType = GMT_ERROR;
	D3DXMatrixIdentity(&m_collObj.transform);

	for (int i = 0; i < MAX_GROUP; i++)
	{
		m_groups[i].objectCount = 0;
		m_groups[i].objects = nullptr;
		m_groups[i].updates = nullptr;
		m_groups[i].currentTextureEx = -1;
	}
}

CObject3D::~CObject3D()
{
	Cleanup();
}

void CObject3D::Cleanup()
{
	UnloadTextures();

	for (int i = 0; i < MAX_GROUP; i++)
	{
		if (m_groups[i].objects)
		{
			for (int j = 0; j < m_groups[i].objectCount; j++)
			{
				m_groups[i].objects[j].Cleanup();
			}
		}
	}

	if (m_groups[0].objects)
	{
		delete[] m_groups[0].objects;
		for (int i = 0; i < MAX_GROUP; i++)
			m_groups[i].objects = nullptr;
	}

	if (m_groups[0].updates)
	{
		delete[] m_groups[0].updates;
		for (int i = 0; i < MAX_GROUP; i++)
			m_groups[i].updates = nullptr;
	}

	for (int i = 0; i < MAX_GROUP; i++)
		m_groups[i].objectCount = 0;

	m_collObj.Cleanup();
	m_collObj.type = GMT_ERROR;

	DeleteArray(m_attributes);
	DeleteArray(m_baseBones);
	m_baseInvBones = nullptr;
	Delete(m_motion);

	m_frameCount = 0;
	m_boneCount = 0;
	m_hasSkin = false;
	m_LOD = false;
}

void CObject3D::LoadTextures(const std::string& modelPath)
{
	std::string searchDir = GetFileDirectory(modelPath);
	int groupCount = m_LOD ? MAX_GROUP : 1;

	for (int i = 0; i < groupCount; i++)
	{
		for (int j = 0; j < m_groups[i].objectCount; j++)
		{
			GMObject& obj = m_groups[i].objects[j];
			if (obj.material && obj.materials)
			{
				for (int k = 0; k < obj.materialCount; k++)
				{
					if (std::strlen(obj.materials[k].textureName) > 0)
					{
						obj.materials[k].texture = TextureMng->GetTexture(obj.materials[k].textureName, searchDir);
						obj.materials[k].textureLoaded = true;
					}
				}
			}
		}
	}
}

void CObject3D::UnloadTextures()
{
	int groupCount = m_LOD ? MAX_GROUP : 1;
	for (int i = 0; i < groupCount; i++)
	{
		if (m_groups[i].objects)
		{
			for (int j = 0; j < m_groups[i].objectCount; j++)
			{
				GMObject& obj = m_groups[i].objects[j];
				if (obj.material && obj.materials)
				{
					for (int k = 0; k < obj.materialCount; k++)
					{
						obj.materials[k].textureLoaded = false;
					}
				}
			}
		}
	}
}

bool CObject3D::Load(const std::string& filename)
{
	Cleanup();
	m_filename = filename;

	CFile file;
	if (!file.Open(filename, FileMode::ReadOnly))
		return false;

	char name[260];
	char nameLen = 0;
	file.Read(nameLen);

	if (nameLen > 0)
	{
		int lenToRead = (int)nameLen;
		if (lenToRead > 255) lenToRead = 255;
		file.Read(name, lenToRead);
		for (int i = 0; i < lenToRead; i++)
			name[i] = name[i] ^ (char)0xcd;
		name[lenToRead] = '\0';
	}
	else
	{
		name[0] = '\0';
	}

	int ver = 0;
	file.Read(ver);
	if (ver < VER_MESH)
	{
		std::cerr << "CObject3D: Version " << ver << " no longer supported in: " << filename << std::endl;
		file.Close();
		return false;
	}

	file.Read(m_ID);
	file.Read(m_forces[0]);
	file.Read(m_forces[1]);
	if (ver >= 22)
	{
		file.Read(m_forces[2]);
		file.Read(m_forces[3]);
	}
	file.Read(m_scrlU);
	file.Read(m_scrlV);
	file.Skip(16);

	file.Read(m_bounds);
	file.Read(m_perSlerp);
	file.Read(m_frameCount);

	file.Read(m_eventCount);
	if (m_eventCount > 0 && m_eventCount <= 8)
		file.Read(m_events, m_eventCount);

	int temp = 0;
	file.Read(temp);
	if (temp)
	{
		m_collObj.type = GMT_NORMAL;
		ReadGMObject(file, m_collObj);
	}

	file.Read(temp);
	m_LOD = (temp != 0);

	file.Read(m_boneCount);
	if (m_boneCount > 0)
	{
		m_baseBones = new D3DXMATRIX[m_boneCount * 2];
		m_baseInvBones = m_baseBones + m_boneCount;
		file.Read(m_baseBones, m_boneCount);
		file.Read(m_baseInvBones, m_boneCount);

		if (m_frameCount > 0)
		{
			m_motion = new CMotion();
			m_motion->ReadTM(file, m_boneCount, m_frameCount);
		}

		file.Read(temp);
		m_sendVS = (temp != 0);
	}

	const int groupCount = m_LOD ? MAX_GROUP : 1;
	int poolSize = 0;
	file.Read(poolSize);

	if (poolSize <= 0)
	{
		file.Close();
		return true;
	}

	GMObject* pool = new GMObject[poolSize];
	GMObject* curPool = pool;
	GMObject* obj = nullptr;
	int type = 0;

	for (int i = 0; i < groupCount; i++)
	{
		LODGroup& group = m_groups[i];
		file.Read(group.objectCount);
		group.objects = curPool;
		curPool += group.objectCount;

		for (int j = 0; j < group.objectCount; j++)
		{
			obj = &group.objects[j];

			file.Read(type);
			obj->type = (EGMType)(type & 0xffff);
			if (type & 0x80000000)
				obj->light = true;

			file.Read(obj->usedBoneCount);
			if (obj->usedBoneCount > 0 && obj->usedBoneCount <= MAX_BONES)
				file.Read(obj->usedBones, obj->usedBoneCount);

			file.Read(obj->ID);
			file.Read(obj->parentID);

			if (obj->parentID != -1)
			{
				int pType = 0;
				file.Read(pType);
				obj->parentType = (EGMType)pType;
			}

			file.Read(obj->transform);
			ReadGMObject(file, *obj);

			if (obj->type == GMT_SKIN)
				m_hasSkin = true;

			if (obj->type == GMT_NORMAL && m_frameCount > 0)
			{
				int frame = 0;
				file.Read(frame);
				if (frame)
				{
					obj->frames = new TMAnimation[m_frameCount];
					file.Read(obj->frames, m_frameCount);
				}
			}
		}
	}

	if (ver >= 21)
	{
		int attr = 0;
		file.Read(attr);
		if (attr == m_frameCount && m_frameCount > 0)
		{
			m_attributes = new MotionAttribute[m_frameCount];
			file.Read(m_attributes, m_frameCount);
		}
		else if (m_frameCount > 0)
		{
			m_attributes = new MotionAttribute[m_frameCount];
			std::memset(m_attributes, 0, sizeof(MotionAttribute) * m_frameCount);
		}
	}
	else if (m_frameCount > 0)
	{
		m_attributes = new MotionAttribute[m_frameCount];
		std::memset(m_attributes, 0, sizeof(MotionAttribute) * m_frameCount);
	}

	file.Close();

	// Allocate updates matrices
	D3DXMATRIX* matPool = new D3DXMATRIX[poolSize];
	D3DXMATRIX* curMat = matPool;
	for (int i = 0; i < groupCount; i++)
	{
		m_groups[i].updates = curMat;
		curMat += m_groups[i].objectCount;
		for (int j = 0; j < m_groups[i].objectCount; j++)
			D3DXMatrixIdentity(&m_groups[i].updates[j]);
	}

	LoadTextures(filename);
	return true;
}

bool CObject3D::Save(const std::string& filename)
{
	CFile file;
	if (!file.Open(filename, FileMode::WriteOnly))
		return false;

	std::string baseName = GetFileName(filename);
	char nameLen = (char)baseName.size();
	file.Write(nameLen);

	for (size_t i = 0; i < baseName.size(); i++)
	{
		char c = (char)(baseName[i] ^ (char)0xcd);
		file.Write(c);
	}

	int ver = 22;
	file.Write(ver);
	file.Write(m_ID);
	file.WriteBytes(m_forces, sizeof(D3DXVECTOR3) * 4);
	file.Write(m_scrlU);
	file.Write(m_scrlV);

	char unused[16];
	std::memset(unused, 0, 16);
	file.WriteBytes(unused, 16);

	file.Write(m_bounds);
	file.Write(m_perSlerp);
	file.Write(m_frameCount);

	file.Write(m_eventCount);
	if (m_eventCount > 0 && m_eventCount <= 8)
		file.Write(m_events, m_eventCount);

	int temp = (m_collObj.type != GMT_ERROR) ? 1 : 0;
	file.Write(temp);
	if (temp)
		SaveGMObject(file, m_collObj);

	temp = m_LOD ? 1 : 0;
	file.Write(temp);

	file.Write(m_boneCount);
	if (m_boneCount > 0)
	{
		file.Write(m_baseBones, m_boneCount);
		file.Write(m_baseInvBones, m_boneCount);

		if (m_frameCount > 0 && m_motion)
			m_motion->SaveTM(file);

		temp = m_sendVS ? 1 : 0;
		file.Write(temp);
	}

	int poolSize = 0;
	const int groupCount = m_LOD ? MAX_GROUP : 1;
	for (int i = 0; i < groupCount; i++)
		poolSize += m_groups[i].objectCount;

	file.Write(poolSize);

	LODGroup* group = nullptr;
	GMObject* obj = nullptr;
	int type = 0;

	for (int i = 0; i < groupCount; i++)
	{
		group = &m_groups[i];
		file.Write(group->objectCount);

		for (int j = 0; j < group->objectCount; j++)
			group->objects[j].ID = j;

		for (int j = 0; j < group->objectCount; j++)
		{
			obj = &group->objects[j];

			type = obj->type & 0xffff;
			if (obj->light)
				type |= 0x80000000;

			file.Write(type);
			file.Write(obj->usedBoneCount);
			if (obj->usedBoneCount > 0)
				file.Write(obj->usedBones, obj->usedBoneCount);

			file.Write(obj->ID);
			file.Write(obj->parentID);

			if (obj->parentID != -1)
			{
				int pType = (int)obj->parentType;
				file.Write(pType);
			}

			file.Write(obj->transform);
			SaveGMObject(file, *obj);

			if (obj->type == GMT_NORMAL && m_frameCount > 0)
			{
				const int frame = (obj->frames != nullptr) ? 1 : 0;
				file.Write(frame);
				if (frame)
					file.Write(obj->frames, m_frameCount);
			}
		}
	}

	temp = (m_frameCount > 0) ? m_frameCount : 0;
	file.Write(temp);
	if (temp > 0 && m_attributes)
		file.Write(m_attributes, m_frameCount);

	file.Close();
	m_filename = filename;
	return true;
}

void CObject3D::ReadGMObject(CFile& file, GMObject& obj)
{
	file.Read(obj.bounds);

	int temp = 0;
	file.Read(temp);
	obj.opacity = (temp != 0);
	file.Read(temp);
	obj.bump = (temp != 0);
	file.Read(temp);
	obj.rigid = (temp != 0);

	file.Skip(28);

	file.Read(obj.vertexListCount);
	file.Read(obj.vertexCount);
	file.Read(obj.faceListCount);
	file.Read(obj.indexCount);

	if (obj.vertexListCount > 0)
	{
		obj.vertexList = new D3DXVECTOR3[obj.vertexListCount];
		file.Read(obj.vertexList, obj.vertexListCount);
	}

	if (obj.vertexCount > 0)
	{
		if (obj.type == GMT_SKIN)
		{
			obj.vertices = new SkinVertex[obj.vertexCount];
			file.Read((SkinVertex*)obj.vertices, obj.vertexCount);
		}
		else
		{
			obj.vertices = new NormalVertex[obj.vertexCount];
			file.Read((NormalVertex*)obj.vertices, obj.vertexCount);
		}
	}

	if (obj.indexCount > 0 || obj.vertexCount > 0)
	{
		obj.indices = new ushort[obj.indexCount + obj.vertexCount];
		obj.IIB = obj.indices + obj.indexCount;

		if (obj.indexCount > 0)
			file.Read(obj.indices, obj.indexCount);
		if (obj.vertexCount > 0)
			file.Read(obj.IIB, obj.vertexCount);
	}

	file.Read(temp);
	if (temp && obj.vertexListCount > 0)
	{
		obj.physiqueVertices = new int[obj.vertexListCount];
		file.Read(obj.physiqueVertices, obj.vertexListCount);
	}

	file.Read(temp);
	obj.material = (temp != 0);
	if (obj.material)
	{
		file.Read(obj.materialCount);
		if (obj.materialCount <= 0)
			obj.materialCount = 1;

		obj.materials = new ModelMaterial[obj.materialCount];
		for (int i = 0; i < obj.materialCount; i++)
		{
			ModelMaterial* mat = &obj.materials[i];
			file.Read(mat->material);

			int len = 0;
			file.Read(len);
			if (len > 0 && len < 256)
			{
				file.Read(mat->textureName, len);
				mat->textureName[std::min(len, 255)] = '\0';
			}
			else if (len >= 256)
			{
				file.Read(mat->textureName, 255);
				file.Skip(len - 255);
				mat->textureName[255] = '\0';
			}
		}
	}

	file.Read(obj.materialBlockCount);
	if (obj.materialBlockCount > 0)
	{
		obj.materialBlocks = new MaterialBlock[obj.materialBlockCount];
		file.Read(obj.materialBlocks, obj.materialBlockCount);
	}
}

void CObject3D::SaveGMObject(CFile& file, GMObject& obj)
{
	file.Write(obj.bounds);

	int temp = obj.opacity ? 1 : 0;
	file.Write(temp);
	temp = obj.bump ? 1 : 0;
	file.Write(temp);
	temp = obj.rigid ? 1 : 0;
	file.Write(temp);

	char unused[28];
	std::memset(unused, 0, 28);
	file.WriteBytes(unused, 28);

	file.Write(obj.vertexListCount);
	file.Write(obj.vertexCount);
	file.Write(obj.faceListCount);
	file.Write(obj.indexCount);

	if (obj.vertexListCount > 0 && obj.vertexList)
		file.Write(obj.vertexList, obj.vertexListCount);

	if (obj.vertexCount > 0 && obj.vertices)
	{
		if (obj.type == GMT_SKIN)
			file.Write((SkinVertex*)obj.vertices, obj.vertexCount);
		else
			file.Write((NormalVertex*)obj.vertices, obj.vertexCount);
	}

	if (obj.indexCount > 0 && obj.indices)
		file.Write(obj.indices, obj.indexCount);
	if (obj.vertexCount > 0 && obj.IIB)
		file.Write(obj.IIB, obj.vertexCount);

	temp = (obj.physiqueVertices != nullptr) ? 1 : 0;
	file.Write(temp);
	if (temp && obj.physiqueVertices)
		file.Write(obj.physiqueVertices, obj.vertexListCount);

	temp = obj.material ? 1 : 0;
	file.Write(temp);
	if (temp && obj.materials)
	{
		file.Write(obj.materialCount);

		for (int i = 0; i < obj.materialCount; i++)
		{
			ModelMaterial* mat = &obj.materials[i];
			file.Write(mat->material);

			int len = (int)std::strlen(mat->textureName);
			file.Write(len + 1);
			if (len > 0)
				file.Write(mat->textureName, len);
			char nullTerm = '\0';
			file.Write(nullTerm);
		}
	}

	file.Write(obj.materialBlockCount);
	if (obj.materialBlockCount > 0 && obj.materialBlocks)
		file.Write(obj.materialBlocks, obj.materialBlockCount);
}

bool CObject3D::LoadMotion(const std::string& filename)
{
	Delete(m_motion);
	m_motion = new CMotion();
	if (!m_motion->Load(filename))
	{
		Delete(m_motion);
		return false;
	}

	m_frameCount = m_motion->GetFrameCount();

	if (m_boneCount == 0 && m_motion->GetBoneCount() > 0)
	{
		m_boneCount = m_motion->GetBoneCount();
		m_baseBones = new D3DXMATRIX[m_boneCount * 2];
		m_baseInvBones = m_baseBones + m_boneCount;
		for (int i = 0; i < m_boneCount; i++)
		{
			D3DXMatrixIdentity(&m_baseBones[i]);
			D3DXMatrixIdentity(&m_baseInvBones[i]);
		}
	}

	return true;
}

bool CObject3D::LoadSkeleton(const std::string& filename)
{
	CSkeleton skl;
	if (!skl.Load(filename))
		return false;

	int boneCount = skl.GetBoneCount();
	if (boneCount > 0)
	{
		DeleteArray(m_baseBones);
		m_boneCount = boneCount;
		m_baseBones = new D3DXMATRIX[m_boneCount * 2];
		m_baseInvBones = m_baseBones + m_boneCount;
		skl.ResetBones(m_baseBones, m_baseInvBones);
	}
	return true;
}

void CObject3D::UnloadMotion()
{
	Delete(m_motion);
	m_frameCount = 0;
}

void CObject3D::Animate(float currentFrame_, int nextFrame, int lod)
{
	D3DXMATRIX* activeBones = m_baseBones ? m_baseBones : m_externBones;

	if (m_motion && activeBones)
	{
		m_motion->Animate(activeBones, currentFrame_, nextFrame);
	}

	LODGroup* group = GetGroup(lod);
	if (!group || !group->updates || group->objectCount <= 0)
		return;

	TMAnimation* frame = nullptr;
	TMAnimation* next = nullptr;
	D3DXQUATERNION slerp;
	D3DXVECTOR3 lerp;
	D3DXMATRIX m1, m2;
	D3DXMATRIX* updates = group->updates;
	D3DXMATRIX* parent = nullptr;
	GMObject* obj = nullptr;

	const int currentFrame = (int)currentFrame_;
	const float slp = currentFrame_ - (float)currentFrame;

	int cFrame = (m_frameCount > 0) ? (currentFrame % m_frameCount) : 0;
	int nFrame = (m_frameCount > 0) ? (nextFrame % m_frameCount) : 0;

	for (int i = 0; i < group->objectCount; i++)
	{
		obj = &group->objects[i];
		if (obj->parentType == GMT_BONE)
		{
			parent = activeBones;
		}
		else
		{
			parent = updates;
		}

		if (m_frameCount > 0 && obj->frames)
		{
			frame = &obj->frames[cFrame];
			next = &obj->frames[nFrame];

			D3DXQuaternionSlerp(&slerp, &frame->rot, &next->rot, slp);
			D3DXVec3Lerp(&lerp, &frame->pos, &next->pos, slp);
			D3DXMatrixTranslation(&m1, lerp.x, lerp.y, lerp.z);
			D3DXMatrixRotationQuaternion(&m2, &slerp);
			updates[i] = m2 * m1;

			if (obj->parentID != -1 && parent)
				updates[i] = updates[i] * parent[obj->parentID];
		}
		else
		{
			if (obj->parentID != -1 && parent)
				updates[i] = obj->transform * parent[obj->parentID];
			else
				updates[i] = obj->transform;
		}
	}
}

void CObject3D::ChangeCW(int groupIndex, int objIndex)
{
	int startGroup = 0;
	int endGroup = m_LOD ? MAX_GROUP : 1;

	if (groupIndex >= 0 && groupIndex < endGroup)
	{
		startGroup = groupIndex;
		endGroup = groupIndex + 1;
	}

	for (int i = startGroup; i < endGroup; i++)
	{
		LODGroup& group = m_groups[i];
		int startObj = 0;
		int endObj = group.objectCount;

		if (objIndex >= 0 && objIndex < group.objectCount)
		{
			startObj = objIndex;
			endObj = objIndex + 1;
		}

		for (int j = startObj; j < endObj; j++)
		{
			GMObject& obj = group.objects[j];
			if (obj.indices && obj.indexCount >= 3)
			{
				for (int k = 0; k < obj.indexCount / 3; k++)
				{
					std::swap(obj.indices[k * 3], obj.indices[k * 3 + 2]);
				}
			}
		}
	}

	if (m_collObj.type != GMT_ERROR && m_collObj.indices && m_collObj.indexCount >= 3)
	{
		for (int k = 0; k < m_collObj.indexCount / 3; k++)
		{
			std::swap(m_collObj.indices[k * 3], m_collObj.indices[k * 3 + 2]);
		}
	}
}

void CObject3D::CollisionAuto(int lod)
{
	LODGroup* group = GetGroup(lod);
	if (!group || group->objectCount <= 0)
		return;

	m_collObj.Cleanup();

	bool normalObj = false;
	for (int i = 0; i < group->objectCount; i++)
	{
		if (group->objects[i].type == GMT_NORMAL && !group->objects[i].light)
		{
			normalObj = true;
			break;
		}
	}

	if (!normalObj)
		return;

	m_collObj.type = GMT_NORMAL;
	m_collObj.parentType = GMT_ERROR;
	m_collObj.parentID = -1;
	D3DXMatrixIdentity(&m_collObj.transform);

	for (int i = 0; i < group->objectCount; i++)
	{
		GMObject& obj = group->objects[i];
		if (obj.type == GMT_NORMAL && !obj.light)
		{
			m_collObj.vertexCount += obj.vertexCount;
			m_collObj.vertexListCount += obj.vertexListCount;
			m_collObj.faceListCount += obj.faceListCount;
			m_collObj.indexCount += obj.indexCount;
		}
	}

	if (m_collObj.vertexCount <= 0 || m_collObj.indexCount <= 0)
		return;

	m_collObj.vertices = new NormalVertex[m_collObj.vertexCount];
	NormalVertex* vertices = (NormalVertex*)m_collObj.vertices;
	m_collObj.indices = new ushort[m_collObj.indexCount + m_collObj.vertexCount];
	m_collObj.IIB = m_collObj.indices + m_collObj.indexCount;
	m_collObj.vertexList = new D3DXVECTOR3[m_collObj.vertexListCount];

	NormalVertex* curVertices = vertices;
	ushort* curIndices = m_collObj.indices;
	ushort* curIIB = m_collObj.IIB;
	D3DXVECTOR3* curVertexList = m_collObj.vertexList;
	ushort indexOffset = 0;

	for (int i = 0; i < group->objectCount; i++)
	{
		GMObject& obj = group->objects[i];
		if (obj.type == GMT_NORMAL && !obj.light)
		{
			NormalVertex* srcVertices = (NormalVertex*)obj.vertices;
			for (int j = 0; j < obj.vertexCount; j++)
			{
				curVertices[j] = srcVertices[j];
				D3DXVec3TransformCoord(&curVertices[j].p, &srcVertices[j].p, &group->updates[i]);
			}
			curVertices += obj.vertexCount;

			for (int j = 0; j < obj.indexCount; j++)
			{
				curIndices[j] = obj.indices[j] + indexOffset;
			}
			curIndices += obj.indexCount;
			indexOffset += obj.vertexCount;

			std::memcpy(curIIB, obj.IIB, obj.vertexCount * sizeof(ushort));
			curIIB += obj.vertexCount;

			for (int j = 0; j < obj.vertexListCount; j++)
			{
				D3DXVec3TransformCoord(&curVertexList[j], &obj.vertexList[j], &group->updates[i]);
			}
			curVertexList += obj.vertexListCount;
		}
	}

	m_collObj.materialBlockCount = 1;
	m_collObj.materialBlocks = new MaterialBlock[1];
	m_collObj.materialBlocks[0].startVertex = 0;
	m_collObj.materialBlocks[0].primitiveCount = m_collObj.faceListCount;

	D3DXVECTOR3 bbMin(FLT_MAX, FLT_MAX, FLT_MAX);
	D3DXVECTOR3 bbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int i = 0; i < m_collObj.vertexListCount; i++)
	{
		const D3DXVECTOR3& v = m_collObj.vertexList[i];
		bbMin.x = std::min(bbMin.x, v.x);
		bbMin.y = std::min(bbMin.y, v.y);
		bbMin.z = std::min(bbMin.z, v.z);
		bbMax.x = std::max(bbMax.x, v.x);
		bbMax.y = std::max(bbMax.y, v.y);
		bbMax.z = std::max(bbMax.z, v.z);
	}

	m_collObj.bounds.Min = bbMin;
	m_collObj.bounds.Max = bbMax;
}

void CObject3D::RecalculateBounds()
{
	D3DXVECTOR3 overallMin(FLT_MAX, FLT_MAX, FLT_MAX);
	D3DXVECTOR3 overallMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	bool hasPoints = false;

	const int groupCount = m_LOD ? MAX_GROUP : 1;
	for (int i = 0; i < groupCount; i++)
	{
		LODGroup& group = m_groups[i];
		for (int j = 0; j < group.objectCount; j++)
		{
			GMObject& obj = group.objects[j];
			D3DXVECTOR3 objMin(FLT_MAX, FLT_MAX, FLT_MAX);
			D3DXVECTOR3 objMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			bool objHasPoints = false;

			if (obj.vertexListCount > 0 && obj.vertexList)
			{
				for (int k = 0; k < obj.vertexListCount; k++)
				{
					const D3DXVECTOR3& v = obj.vertexList[k];
					objMin.x = std::min(objMin.x, v.x);
					objMin.y = std::min(objMin.y, v.y);
					objMin.z = std::min(objMin.z, v.z);
					objMax.x = std::max(objMax.x, v.x);
					objMax.y = std::max(objMax.y, v.y);
					objMax.z = std::max(objMax.z, v.z);
					objHasPoints = true;
				}
			}
			else if (obj.vertexCount > 0 && obj.vertices)
			{
				for (int k = 0; k < obj.vertexCount; k++)
				{
					D3DXVECTOR3 v;
					if (obj.type == GMT_SKIN)
						v = ((SkinVertex*)obj.vertices)[k].p;
					else
						v = ((NormalVertex*)obj.vertices)[k].p;

					objMin.x = std::min(objMin.x, v.x);
					objMin.y = std::min(objMin.y, v.y);
					objMin.z = std::min(objMin.z, v.z);
					objMax.x = std::max(objMax.x, v.x);
					objMax.y = std::max(objMax.y, v.y);
					objMax.z = std::max(objMax.z, v.z);
					objHasPoints = true;
				}
			}

			if (objHasPoints)
			{
				obj.bounds.Min = objMin;
				obj.bounds.Max = objMax;

				if (i == 0) // Compute overall bounds from LOD 0
				{
					overallMin.x = std::min(overallMin.x, objMin.x);
					overallMin.y = std::min(overallMin.y, objMin.y);
					overallMin.z = std::min(overallMin.z, objMin.z);
					overallMax.x = std::max(overallMax.x, objMax.x);
					overallMax.y = std::max(overallMax.y, objMax.y);
					overallMax.z = std::max(overallMax.z, objMax.z);
					hasPoints = true;
				}
			}
		}
	}

	if (hasPoints)
	{
		m_bounds.Min = overallMin;
		m_bounds.Max = overallMax;
	}
}
