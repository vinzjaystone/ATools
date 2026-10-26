#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "Object3D.h"
#include "Motion.h"
#include <string>
#include <vector>

struct ModelAnimationItem
{
	std::string filename;
	std::string displayName;
	std::string filepath;
	int frameCount;

	ModelAnimationItem() : frameCount(0) {}
};

struct ScenePart
{
	CObject3D* model;
	std::string filepath;
	std::string filename;
	bool visible;
	int currentLOD;
	int attachBoneID; // -1: Root / Skinned to skeleton; >= 0: Attached to bone socket
	bool isBase;
	D3DXVECTOR3 offset;
	D3DXVECTOR3 rotation;
	float scale;

	ScenePart()
		: model(nullptr),
		  filepath(""),
		  filename(""),
		  visible(true),
		  currentLOD(0),
		  attachBoneID(-1),
		  isBase(false),
		  offset(0.0f, 0.0f, 0.0f),
		  rotation(0.0f, 0.0f, 0.0f),
		  scale(1.0f)
	{
	}
};

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	bool LoadBaseModel(const std::string& filepath);
	bool AppendModel(const std::string& filepath);
	bool LoadMotion(const std::string& filepath);
	bool LoadSkeleton(const std::string& filepath);
	void UnloadMotion();
	void UnloadSkeleton();

	// Multi-animation management
	void DiscoverAvailableAnimations(const std::string& modelFilepath);
	bool SelectAnimation(int index);
	bool SelectAnimationByName(const std::string& animName);
	bool NextAnimation();
	bool PrevAnimation();

	const std::vector<ModelAnimationItem>& GetAvailableAnimations() const { return m_availableAnimations; }
	int GetCurrentAnimationIndex() const { return m_currentAnimationIndex; }
	std::string GetCurrentAnimationDisplayName() const;

	void RemovePart(int index);
	void ClearAppendedParts();
	void ClearAll();

	int GetPartCount() const { return (int)m_parts.size(); }
	ScenePart* GetPart(int index);
	const ScenePart* GetPart(int index) const;

	int GetActivePartIndex() const { return m_activePartIndex; }
	void SetActivePartIndex(int index);

	CObject3D* GetActiveModel();
	const CObject3D* GetActiveModel() const;

	CObject3D* GetBaseModel();
	const CObject3D* GetBaseModel() const;

	CMotion* GetActiveMotion();
	const CMotion* GetActiveMotion() const;

	std::string GetActiveMotionPath() const { return m_activeMotionPath; }
	std::string GetActiveSkeletonPath() const { return m_activeSkeletonPath; }

	int GetTotalFrameCount() const;
	float GetCurrentFrame() const { return m_currentFrame; }
	void SetCurrentFrame(float frame);

	float GetAnimSpeed() const { return m_animSpeed; }
	void SetAnimSpeed(float speed) { m_animSpeed = speed; }

	bool IsPlaying() const { return m_isPlaying; }
	void SetPlaying(bool playing)
	{
		m_isPlaying = playing;
		if (m_isPlaying)
		{
			int total = GetTotalFrameCount();
			if (total > 0 && m_currentFrame >= (float)(total - 1))
			{
				m_currentFrame = 0.0f;
			}
		}
	}
	void TogglePlay() { SetPlaying(!m_isPlaying); }

	bool IsLooping() const { return m_loop; }
	void SetLooping(bool loop) { m_loop = loop; }
	void ToggleLoop() { m_loop = !m_loop; }

	void StepFrame(int delta);

	void UpdateAnimation(float deltaTime);
	void SyncBones();
	Bounds GetCombinedBounds() const;

	bool ExportToGLTF(const std::string& filepath, bool exportAllAnimations = true) const;
	bool ExportToDAE(const std::string& filepath, bool exportAllAnimations = true) const;

	// Helper to extract file name from path
	static std::string ExtractFileName(const std::string& path);

private:
	std::vector<ScenePart> m_parts;
	int m_activePartIndex;
	std::string m_activeMotionPath;
	std::string m_activeSkeletonPath;

	std::vector<ModelAnimationItem> m_availableAnimations;
	int m_currentAnimationIndex;

	float m_currentFrame;
	float m_animSpeed;
	bool m_isPlaying;
	bool m_loop;

	void _rebuildBoneLinks();
};

#endif // SCENEMANAGER_H
