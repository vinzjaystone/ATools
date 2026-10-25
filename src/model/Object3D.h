#ifndef OBJECT3D_H
#define OBJECT3D_H

#include "../core/Types.h"
#include "../core/D3DXMath.h"
#include "../core/File.h"
#include "raylib.h"
#include <string>
#include <vector>

struct MotionAttribute
{
	uint type;
	int soundID;
	float frame;
};

struct TMAnimation
{
	D3DXQUATERNION rot;
	D3DXVECTOR3 pos;
};

class CMotion;
class CSkeleton;

struct SkinVertex
{
	D3DXVECTOR3 p;
	float w1, w2;
	ushort id1, id2;
	D3DXVECTOR3 n;
	D3DXVECTOR2 t;
};

struct NormalVertex
{
	D3DXVECTOR3 p;
	D3DXVECTOR3 n;
	D3DXVECTOR2 t;
};

struct CollisionVertex
{
	D3DXVECTOR3 p;
	uint c;
};

struct ModelMaterial
{
	D3DMATERIAL9 material;
	char textureName[256];
	Texture2D texture;
	bool textureLoaded;

	ModelMaterial()
	{
		std::memset(&material, 0, sizeof(material));
		std::memset(textureName, 0, sizeof(textureName));
		texture = Texture2D{ 0, 0, 0, 0, 0 };
		textureLoaded = false;
	}
};

struct MaterialBlock
{
	int startVertex;
	int primitiveCount;
	int materialID;
	uint effect;
	int amount;
	int usedBoneCount;
	int usedBones[MAX_BONES];

	MaterialBlock()
	{
		startVertex = 0;
		primitiveCount = 0;
		materialID = 0;
		effect = XE_NONE;
		amount = 255;
		usedBoneCount = 0;
		std::memset(usedBones, 0, sizeof(usedBones));
	}
};

struct GMObject
{
	int ID;
	EGMType type;
	int usedBoneCount;
	int usedBones[MAX_BONES];
	int parentID;
	EGMType parentType;
	D3DXMATRIX transform;
	Bounds bounds;
	bool opacity, bump, rigid;
	int vertexCount, vertexListCount, faceListCount, indexCount;
	D3DXVECTOR3* vertexList;
	void* vertices;
	ushort* indices, *IIB;
	int* physiqueVertices;
	bool material;
	int materialCount;
	ModelMaterial* materials;
	int materialBlockCount;
	MaterialBlock* materialBlocks;
	TMAnimation* frames;
	bool light;

	GMObject();
	~GMObject();
	void Cleanup();
};

struct LODGroup
{
	int objectCount;
	GMObject* objects;
	D3DXMATRIX* updates;
	int currentTextureEx;

	LODGroup()
	{
		objectCount = 0;
		objects = nullptr;
		updates = nullptr;
		currentTextureEx = -1;
	}
};

class CObject3D
{
public:
	CObject3D();
	~CObject3D();

	void Cleanup();

	bool Load(const std::string& filename);
	bool Save(const std::string& filename);

	void ReadGMObject(CFile& file, GMObject& obj);
	void SaveGMObject(CFile& file, GMObject& obj);

	int GetFrameCount() const { return m_frameCount; }
	void SetFrameCount(int fc) { m_frameCount = fc; }

	int GetBoneCount() const { return m_boneCount; }
	const Bounds& GetBounds() const { return m_bounds; }
	bool HasLOD() const { return m_LOD; }
	bool HasSkin() const { return m_hasSkin; }
	bool HasAnimation() const { return m_frameCount > 0 || m_motion != nullptr; }

	int GetID() const { return m_ID; }
	void SetID(int id) { m_ID = id; }

	float GetScrollU() const { return m_scrlU; }
	float GetScrollV() const { return m_scrlV; }
	void SetScrollU(float u) { m_scrlU = u; }
	void SetScrollV(float v) { m_scrlV = v; }

	D3DXVECTOR3 GetForce(int idx) const { return (idx >= 0 && idx < 4) ? m_forces[idx] : D3DXVECTOR3(); }
	void SetForce(int idx, const D3DXVECTOR3& f) { if (idx >= 0 && idx < 4) m_forces[idx] = f; }

	LODGroup* GetGroup(int lod) {
		if (lod < 0 || lod >= MAX_GROUP) lod = 0;
		return m_LOD ? &m_groups[lod] : &m_groups[0];
	}

	const LODGroup* GetGroup(int lod) const {
		if (lod < 0 || lod >= MAX_GROUP) lod = 0;
		return m_LOD ? &m_groups[lod] : &m_groups[0];
	}

	GMObject* GetCollisionObj() { return (m_collObj.type != GMT_ERROR) ? &m_collObj : nullptr; }
	const GMObject* GetCollisionObj() const { return (m_collObj.type != GMT_ERROR) ? &m_collObj : nullptr; }

	int GetEffectiveBoneCount() const;

	MotionAttribute* GetAttributes() const { return m_attributes; }
	CMotion* GetMotion() const { return m_motion; }
	CSkeleton* GetSkeleton() const { return m_skeleton; }
	D3DXMATRIX* GetBaseBones() const { return m_baseBones; }
	D3DXMATRIX* GetBaseInvBones() const { return m_baseInvBones; }
	D3DXMATRIX* GetEffectiveBones() const {
		if (m_externBones) return m_externBones;
		return m_baseBones;
	}
	D3DXMATRIX* GetEffectiveInvBones() const {
		if (m_baseInvBones) return m_baseInvBones;
		return m_externInvBones;
	}

	void SetExternBones(D3DXMATRIX* bones, D3DXMATRIX* invBones, int boneCount = 0)
	{
		m_externBones = bones;
		m_externInvBones = invBones;
		m_externBoneCount = boneCount;
	}

	// External animation & skeleton loading
	bool LoadMotion(const std::string& filename);
	bool LoadSkeleton(const std::string& filename);
	void UnloadMotion();
	void UnloadSkeleton();

	void Animate(float currentFrame, int nextFrame, int lod = 0);
	void LoadTextures(const std::string& modelPath);
	void UnloadTextures();

	// Editing operations
	void ChangeCW(int groupIndex = -1, int objIndex = -1);
	void CollisionAuto(int lod = 0);
	void RecalculateBounds();

	std::string GetLoadedFilename() const { return m_filename; }

private:
	std::string m_filename;
	int m_ID;
	D3DXVECTOR3 m_forces[4];
	Bounds m_bounds;
	float m_scrlU, m_scrlV;
	float m_perSlerp;
	int m_frameCount;
	int m_eventCount;
	D3DXVECTOR3 m_events[8];
	GMObject m_collObj;
	bool m_LOD;
	int m_boneCount;
	LODGroup m_groups[MAX_GROUP];
	MotionAttribute* m_attributes;
	D3DXMATRIX* m_baseBones;
	D3DXMATRIX* m_baseInvBones;
	D3DXMATRIX* m_externBones;
	D3DXMATRIX* m_externInvBones;
	int m_externBoneCount;
	bool m_sendVS;
	CMotion* m_motion;
	CSkeleton* m_skeleton;
	bool m_hasSkin;
};

#endif // OBJECT3D_H
