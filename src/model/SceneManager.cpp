#include "SceneManager.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

std::string SceneManager::ExtractFileName(const std::string& path)
{
	if (path.empty()) return "";
	size_t lastSlash = path.find_last_of("/\\");
	if (lastSlash != std::string::npos)
		return path.substr(lastSlash + 1);
	return path;
}

SceneManager::SceneManager()
	: m_activePartIndex(0),
	  m_activeMotionPath(""),
	  m_activeSkeletonPath(""),
	  m_currentFrame(0.0f),
	  m_animSpeed(1.0f),
	  m_isPlaying(true),
	  m_loop(true)
{
}

SceneManager::~SceneManager()
{
	ClearAll();
}

bool SceneManager::LoadBaseModel(const std::string& filepath)
{
	CObject3D* newModel = new CObject3D();
	if (!newModel->Load(filepath))
	{
		delete newModel;
		return false;
	}

	ClearAll();

	ScenePart basePart;
	basePart.model = newModel;
	basePart.filepath = filepath;
	basePart.filename = ExtractFileName(filepath);
	basePart.visible = true;
	basePart.isBase = true;
	basePart.currentLOD = 0;
	basePart.attachBoneID = -1;

	m_parts.push_back(basePart);
	m_activePartIndex = 0;
	m_currentFrame = 0.0f;

	if (!m_activeSkeletonPath.empty())
		newModel->LoadSkeleton(m_activeSkeletonPath);

	if (!m_activeMotionPath.empty())
		newModel->LoadMotion(m_activeMotionPath);

	_rebuildBoneLinks();
	return true;
}

bool SceneManager::AppendModel(const std::string& filepath)
{
	CObject3D* newModel = new CObject3D();
	if (!newModel->Load(filepath))
	{
		delete newModel;
		return false;
	}

	ScenePart part;
	part.model = newModel;
	part.filepath = filepath;
	part.filename = ExtractFileName(filepath);
	part.visible = true;
	part.isBase = (m_parts.empty());
	part.currentLOD = 0;
	part.attachBoneID = -1;

	m_parts.push_back(part);
	m_activePartIndex = (int)m_parts.size() - 1;

	_rebuildBoneLinks();
	return true;
}

bool SceneManager::LoadMotion(const std::string& filepath)
{
	CObject3D* baseModel = GetBaseModel();
	if (!baseModel)
	{
		// Still record active motion path
		m_activeMotionPath = filepath;
		return true;
	}

	if (!baseModel->LoadMotion(filepath))
		return false;

	m_activeMotionPath = filepath;
	m_currentFrame = 0.0f;
	_rebuildBoneLinks();
	return true;
}

bool SceneManager::LoadSkeleton(const std::string& filepath)
{
	CObject3D* baseModel = GetBaseModel();
	if (!baseModel)
	{
		m_activeSkeletonPath = filepath;
		return true;
	}

	if (!baseModel->LoadSkeleton(filepath))
		return false;

	m_activeSkeletonPath = filepath;
	_rebuildBoneLinks();
	return true;
}

void SceneManager::UnloadMotion()
{
	m_activeMotionPath = "";
	CObject3D* baseModel = GetBaseModel();
	if (baseModel)
	{
		baseModel->UnloadMotion();
	}
	m_currentFrame = 0.0f;
}

void SceneManager::RemovePart(int index)
{
	if (index < 0 || index >= (int)m_parts.size())
		return;

	if (m_parts[index].model)
	{
		delete m_parts[index].model;
		m_parts[index].model = nullptr;
	}

	m_parts.erase(m_parts.begin() + index);

	if (m_parts.empty())
	{
		m_activePartIndex = 0;
	}
	else
	{
		// Ensure first part is designated as base
		m_parts[0].isBase = true;
		if (m_activePartIndex >= (int)m_parts.size())
			m_activePartIndex = (int)m_parts.size() - 1;
		_rebuildBoneLinks();
	}
}

void SceneManager::ClearAppendedParts()
{
	if (m_parts.size() <= 1)
		return;

	for (size_t i = 1; i < m_parts.size(); i++)
	{
		if (m_parts[i].model)
		{
			delete m_parts[i].model;
			m_parts[i].model = nullptr;
		}
	}

	m_parts.resize(1);
	m_activePartIndex = 0;
	_rebuildBoneLinks();
}

void SceneManager::ClearAll()
{
	for (auto& part : m_parts)
	{
		if (part.model)
		{
			delete part.model;
			part.model = nullptr;
		}
	}
	m_parts.clear();
	m_activePartIndex = 0;
	m_currentFrame = 0.0f;
}

ScenePart* SceneManager::GetPart(int index)
{
	if (index >= 0 && index < (int)m_parts.size())
		return &m_parts[index];
	return nullptr;
}

const ScenePart* SceneManager::GetPart(int index) const
{
	if (index >= 0 && index < (int)m_parts.size())
		return &m_parts[index];
	return nullptr;
}

void SceneManager::SetActivePartIndex(int index)
{
	if (index >= 0 && index < (int)m_parts.size())
		m_activePartIndex = index;
}

CObject3D* SceneManager::GetActiveModel()
{
	if (m_activePartIndex >= 0 && m_activePartIndex < (int)m_parts.size())
		return m_parts[m_activePartIndex].model;
	return nullptr;
}

const CObject3D* SceneManager::GetActiveModel() const
{
	if (m_activePartIndex >= 0 && m_activePartIndex < (int)m_parts.size())
		return m_parts[m_activePartIndex].model;
	return nullptr;
}

CObject3D* SceneManager::GetBaseModel()
{
	if (!m_parts.empty())
		return m_parts[0].model;
	return nullptr;
}

const CObject3D* SceneManager::GetBaseModel() const
{
	if (!m_parts.empty())
		return m_parts[0].model;
	return nullptr;
}

CMotion* SceneManager::GetActiveMotion()
{
	CObject3D* base = GetBaseModel();
	return base ? base->GetMotion() : nullptr;
}

const CMotion* SceneManager::GetActiveMotion() const
{
	const CObject3D* base = GetBaseModel();
	return base ? base->GetMotion() : nullptr;
}

int SceneManager::GetTotalFrameCount() const
{
	const CObject3D* base = GetBaseModel();
	if (base)
	{
		if (base->GetMotion())
			return base->GetMotion()->GetFrameCount();
		if (base->GetFrameCount() > 0)
			return base->GetFrameCount();
	}
	return 0;
}

void SceneManager::SetCurrentFrame(float frame)
{
	int total = GetTotalFrameCount();
	if (total > 0)
	{
		m_currentFrame = std::max(0.0f, std::min((float)(total - 1), frame));
	}
	else
	{
		m_currentFrame = 0.0f;
	}
}

void SceneManager::StepFrame(int delta)
{
	int total = GetTotalFrameCount();
	if (total <= 0) return;

	int cur = (int)m_currentFrame + delta;
	if (cur < 0)
		cur = total - 1;
	else if (cur >= total)
		cur = 0;

	m_currentFrame = (float)cur;
}

void SceneManager::UpdateAnimation(float deltaTime)
{
	int total = GetTotalFrameCount();
	if (total > 1 && m_isPlaying)
	{
		m_currentFrame += m_animSpeed * 30.0f * deltaTime; // 30 FPS standard Flyff motion
		if (m_currentFrame >= (float)total)
		{
			if (m_loop)
			{
				m_currentFrame = std::fmod(m_currentFrame, (float)total);
			}
			else
			{
				m_currentFrame = (float)(total - 1);
				m_isPlaying = false;
			}
		}
		else if (m_currentFrame < 0.0f)
		{
			m_currentFrame = (float)(total - 1);
		}
	}

	// Animate base model and all visible parts
	CObject3D* base = GetBaseModel();
	if (base)
	{
		int nextFrame = (int)m_currentFrame + 1;
		if (total > 0) nextFrame %= total;

		base->Animate(m_currentFrame, nextFrame, m_parts[0].currentLOD);

		D3DXMATRIX* baseBones = base->GetBaseBones();
		D3DXMATRIX* baseInvBones = base->GetBaseInvBones();

		for (size_t i = 1; i < m_parts.size(); i++)
		{
			ScenePart& part = m_parts[i];
			if (part.model && part.visible)
			{
				if (baseBones && baseInvBones)
					part.model->SetExternBones(baseBones, baseInvBones);

				part.model->Animate(m_currentFrame, nextFrame, part.currentLOD);
			}
		}
	}
}

void SceneManager::SyncBones()
{
	_rebuildBoneLinks();
}

void SceneManager::_rebuildBoneLinks()
{
	CObject3D* base = GetBaseModel();
	if (!base) return;

	D3DXMATRIX* baseBones = base->GetBaseBones();
	D3DXMATRIX* baseInvBones = base->GetBaseInvBones();

	for (size_t i = 1; i < m_parts.size(); i++)
	{
		ScenePart& part = m_parts[i];
		if (part.model && baseBones && baseInvBones)
		{
			part.model->SetExternBones(baseBones, baseInvBones);
		}
	}
}

Bounds SceneManager::GetCombinedBounds() const
{
	D3DXVECTOR3 bMin(FLT_MAX, FLT_MAX, FLT_MAX);
	D3DXVECTOR3 bMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	bool hasAny = false;

	for (const auto& part : m_parts)
	{
		if (part.model && part.visible)
		{
			const Bounds& b = part.model->GetBounds();
			bMin.x = std::min(bMin.x, b.Min.x);
			bMin.y = std::min(bMin.y, b.Min.y);
			bMin.z = std::min(bMin.z, b.Min.z);
			bMax.x = std::max(bMax.x, b.Max.x);
			bMax.y = std::max(bMax.y, b.Max.y);
			bMax.z = std::max(bMax.z, b.Max.z);
			hasAny = true;
		}
	}

	Bounds res;
	if (hasAny)
	{
		res.Min = bMin;
		res.Max = bMax;
	}
	else
	{
		res.Min = D3DXVECTOR3(-1.0f, -1.0f, -1.0f);
		res.Max = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
	}
	return res;
}
