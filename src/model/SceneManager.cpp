#include "SceneManager.h"
#include "GltfExporter.h"
#include "DaeExporter.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>

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
	  m_currentAnimationIndex(-1),
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
	{
		newModel->LoadSkeleton(m_activeSkeletonPath);
	}
	else if (newModel->GetBoneCount() == 0)
	{
		// Auto-resolve character skeleton (.chr / .bon) if present in model directory
		namespace fs = std::filesystem;
		try
		{
			fs::path p(filepath);
			fs::path dir = p.parent_path();
			if (dir.empty()) dir = ".";

			std::string stem = p.stem().string();
			std::string stemLower = stem;
			std::transform(stemLower.begin(), stemLower.end(), stemLower.begin(), ::tolower);

			std::vector<std::string> candidates;
			candidates.push_back((dir / (stem + ".chr")).string());
			candidates.push_back((dir / (stem + ".bon")).string());

			if (stemLower.rfind("part_m", 0) == 0 || stemLower.find("male") != std::string::npos)
			{
				candidates.push_back((dir / "mvr_male.chr").string());
				candidates.push_back((dir / "mvr_male.bon").string());
			}
			else if (stemLower.rfind("part_f", 0) == 0 || stemLower.find("female") != std::string::npos)
			{
				candidates.push_back((dir / "mvr_female.chr").string());
				candidates.push_back((dir / "mvr_female.bon").string());
			}
			else if (stemLower.rfind("mvr_", 0) == 0)
			{
				candidates.push_back((dir / (stem + ".chr")).string());
			}

			for (const auto& cand : candidates)
			{
				if (fs::exists(cand))
				{
					if (newModel->LoadSkeleton(cand))
					{
						m_activeSkeletonPath = cand;
						break;
					}
				}
			}
		}
		catch (...) {}
	}

	// Auto-discover all available animations for this model/character
	DiscoverAvailableAnimations(filepath);

	if (!m_activeMotionPath.empty() && m_currentAnimationIndex < 0)
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

	// If the model is a rigid accessory/weapon (not skinned), auto-detect sensible default bone socket
	if (!newModel->HasSkin())
	{
		std::string fnLower = part.filename;
		std::transform(fnLower.begin(), fnLower.end(), fnLower.begin(), ::tolower);

		CObject3D* base = GetBaseModel();
		int boneCount = base ? base->GetEffectiveBoneCount() : 0;

		if (fnLower.find("shield") != std::string::npos || fnLower.find("lwea") != std::string::npos)
		{
			// Left Hand (LH) socket
			int lhBone = -1;
			if (base)
			{
				const Bone* bones = base->GetSkeleton() ? base->GetSkeleton()->GetBones() : (base->GetMotion() ? base->GetMotion()->GetBones() : nullptr);
				if (bones)
				{
					for (int b = 0; b < boneCount; b++)
					{
						std::string bName = bones[b].name;
						std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
						if (bName.find("l hand") != std::string::npos || bName.find("shield") != std::string::npos)
						{
							lhBone = b;
							break;
						}
					}
				}
			}
			part.attachBoneID = (lhBone >= 0) ? lhBone : 5;
		}
		else if (fnLower.find("wea") != std::string::npos || fnLower.find("sword") != std::string::npos ||
		         fnLower.find("axe") != std::string::npos || fnLower.find("bow") != std::string::npos ||
		         fnLower.find("wand") != std::string::npos || fnLower.find("staff") != std::string::npos ||
		         fnLower.find("knuckle") != std::string::npos)
		{
			// Right Hand (RH) socket
			int rhBone = -1;
			if (base)
			{
				const Bone* bones = base->GetSkeleton() ? base->GetSkeleton()->GetBones() : (base->GetMotion() ? base->GetMotion()->GetBones() : nullptr);
				if (bones)
				{
					for (int b = 0; b < boneCount; b++)
					{
						std::string bName = bones[b].name;
						std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
						if (bName.find("r hand") != std::string::npos || bName.find("weapon") != std::string::npos)
						{
							rhBone = b;
							break;
						}
					}
				}
			}
			part.attachBoneID = (rhBone >= 0) ? rhBone : 4;
		}
		else if (fnLower.find("hat") != std::string::npos || fnLower.find("cap") != std::string::npos ||
		         fnLower.find("helm") != std::string::npos || fnLower.find("mask") != std::string::npos)
		{
			// Head socket
			int headBone = -1;
			if (base)
			{
				const Bone* bones = base->GetSkeleton() ? base->GetSkeleton()->GetBones() : (base->GetMotion() ? base->GetMotion()->GetBones() : nullptr);
				if (bones)
				{
					for (int b = 0; b < boneCount; b++)
					{
						std::string bName = bones[b].name;
						std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
						if (bName.find("head") != std::string::npos)
						{
							headBone = b;
							break;
						}
					}
				}
			}
			part.attachBoneID = (headBone >= 0) ? headBone : 3;
		}
	}

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

	// Check if this matches an item in m_availableAnimations
	std::string fn = ExtractFileName(filepath);
	bool found = false;
	for (size_t i = 0; i < m_availableAnimations.size(); i++)
	{
		if (m_availableAnimations[i].filepath == filepath || m_availableAnimations[i].filename == fn)
		{
			m_currentAnimationIndex = (int)i;
			m_availableAnimations[i].frameCount = baseModel->GetMotion() ? baseModel->GetMotion()->GetFrameCount() : 0;
			found = true;
			break;
		}
	}

	if (!found)
	{
		ModelAnimationItem item;
		item.filepath = filepath;
		item.filename = fn;
		item.displayName = fn;
		item.frameCount = baseModel->GetMotion() ? baseModel->GetMotion()->GetFrameCount() : 0;
		m_availableAnimations.push_back(item);
		m_currentAnimationIndex = (int)m_availableAnimations.size() - 1;
	}

	_rebuildBoneLinks();
	return true;
}

void SceneManager::DiscoverAvailableAnimations(const std::string& modelFilepath)
{
	m_availableAnimations.clear();
	m_currentAnimationIndex = -1;

	namespace fs = std::filesystem;
	try
	{
		fs::path p(modelFilepath);
		fs::path dir = p.parent_path();
		if (dir.empty()) dir = ".";

		std::string stem = p.stem().string();
		std::string stemLower = stem;
		std::transform(stemLower.begin(), stemLower.end(), stemLower.begin(), ::tolower);

		std::string prefix = "";
		if (stemLower.rfind("part_m", 0) == 0 || stemLower.find("male") != std::string::npos)
		{
			prefix = "mvr_male";
		}
		else if (stemLower.rfind("part_f", 0) == 0 || stemLower.find("female") != std::string::npos)
		{
			prefix = "mvr_female";
		}
		else if (stemLower.rfind("part_", 0) == 0)
		{
			prefix = "mvr_male";
		}
		else if (stemLower.rfind("mvr_", 0) == 0)
		{
			prefix = stem;
		}
		else if (stemLower.rfind("item_", 0) == 0)
		{
			prefix = stem;
		}
		else
		{
			prefix = stem;
		}

		std::string prefixLower = prefix;
		std::transform(prefixLower.begin(), prefixLower.end(), prefixLower.begin(), ::tolower);

		std::vector<fs::path> matchedFiles;
		std::vector<fs::path> otherFiles;

		for (const auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied))
		{
			if (!entry.is_regular_file()) continue;

			std::string ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if (ext == ".ani" || ext == ".mot" || ext == ".mo3")
			{
				std::string fn = entry.path().filename().string();
				std::string fnLower = fn;
				std::transform(fnLower.begin(), fnLower.end(), fnLower.begin(), ::tolower);

				if (!prefixLower.empty() && (fnLower.rfind(prefixLower, 0) == 0 || fnLower.find(prefixLower) != std::string::npos))
				{
					matchedFiles.push_back(entry.path());
				}
				else
				{
					otherFiles.push_back(entry.path());
				}
			}
		}

		std::vector<fs::path>& targetList = (!matchedFiles.empty()) ? matchedFiles : otherFiles;

		auto sortFunc = [](const fs::path& a, const fs::path& b) {
			std::string sa = a.filename().string();
			std::string sb = b.filename().string();
			std::transform(sa.begin(), sa.end(), sa.begin(), ::tolower);
			std::transform(sb.begin(), sb.end(), sb.begin(), ::tolower);
			return sa < sb;
		};
		std::sort(targetList.begin(), targetList.end(), sortFunc);

		for (const auto& fp : targetList)
		{
			ModelAnimationItem item;
			item.filepath = fp.string();
			item.filename = fp.filename().string();

			std::string disp = fp.stem().string();
			std::string dispLower = disp;
			std::transform(dispLower.begin(), dispLower.end(), dispLower.begin(), ::tolower);

			std::vector<std::string> prefixesToStrip;
			if (!prefixLower.empty()) prefixesToStrip.push_back(prefixLower);
			prefixesToStrip.push_back("mvr_male");
			prefixesToStrip.push_back("mvr_female");
			prefixesToStrip.push_back("motion");

			for (const auto& p : prefixesToStrip)
			{
				if (dispLower.rfind(p + "_", 0) == 0)
				{
					disp = disp.substr(p.length() + 1);
					break;
				}
				else if (dispLower.rfind(p, 0) == 0 && dispLower.length() > p.length())
				{
					disp = disp.substr(p.length());
					if (!disp.empty() && disp[0] == '_') disp = disp.substr(1);
					break;
				}
			}

			if (disp.empty()) disp = fp.stem().string();
			item.displayName = disp;
			item.frameCount = 0;

			m_availableAnimations.push_back(item);
		}

		// Auto-select first or matching walk/idle/stand animation
		if (!m_availableAnimations.empty())
		{
			int defaultIndex = 0;
			for (size_t i = 0; i < m_availableAnimations.size(); i++)
			{
				std::string dLower = m_availableAnimations[i].displayName;
				std::transform(dLower.begin(), dLower.end(), dLower.begin(), ::tolower);
				if (dLower.find("walk") != std::string::npos || dLower.find("idle") != std::string::npos || dLower.find("stand") != std::string::npos)
				{
					defaultIndex = (int)i;
					break;
				}
			}
			SelectAnimation(defaultIndex);
		}
	}
	catch (...) {}
}

bool SceneManager::SelectAnimation(int index)
{
	if (index < 0 || index >= (int)m_availableAnimations.size())
		return false;

	m_currentAnimationIndex = index;
	const std::string& animPath = m_availableAnimations[index].filepath;
	return LoadMotion(animPath);
}

bool SceneManager::SelectAnimationByName(const std::string& animName)
{
	std::string target = animName;
	std::transform(target.begin(), target.end(), target.begin(), ::tolower);

	for (size_t i = 0; i < m_availableAnimations.size(); i++)
	{
		std::string fn = m_availableAnimations[i].filename;
		std::string dn = m_availableAnimations[i].displayName;
		std::transform(fn.begin(), fn.end(), fn.begin(), ::tolower);
		std::transform(dn.begin(), dn.end(), dn.begin(), ::tolower);

		if (fn == target || dn == target || fn.find(target) != std::string::npos)
		{
			return SelectAnimation((int)i);
		}
	}
	return false;
}

bool SceneManager::NextAnimation()
{
	if (m_availableAnimations.empty()) return false;
	int next = (m_currentAnimationIndex + 1) % (int)m_availableAnimations.size();
	return SelectAnimation(next);
}

bool SceneManager::PrevAnimation()
{
	if (m_availableAnimations.empty()) return false;
	int prev = (m_currentAnimationIndex - 1 + (int)m_availableAnimations.size()) % (int)m_availableAnimations.size();
	return SelectAnimation(prev);
}

std::string SceneManager::GetCurrentAnimationDisplayName() const
{
	if (m_currentAnimationIndex >= 0 && m_currentAnimationIndex < (int)m_availableAnimations.size())
		return m_availableAnimations[m_currentAnimationIndex].displayName;
	if (!m_activeMotionPath.empty())
		return ExtractFileName(m_activeMotionPath);
	return "(None)";
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

void SceneManager::UnloadSkeleton()
{
	m_activeSkeletonPath = "";
	CObject3D* baseModel = GetBaseModel();
	if (baseModel)
	{
		baseModel->UnloadSkeleton();
	}
	_rebuildBoneLinks();
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
	m_availableAnimations.clear();
	m_currentAnimationIndex = -1;
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
			if (m_loop)
				m_currentFrame = (float)(total - 1);
			else
			{
				m_currentFrame = 0.0f;
				m_isPlaying = false;
			}
		}
	}

	// Animate base model and all visible parts
	CObject3D* base = GetBaseModel();
	if (base)
	{
		int nextFrame = (int)m_currentFrame + 1;
		if (m_loop)
		{
			if (total > 0) nextFrame %= total;
		}
		else
		{
			if (total > 0) nextFrame = std::min(total - 1, nextFrame);
		}

		base->Animate(m_currentFrame, nextFrame, m_parts[0].currentLOD);

		D3DXMATRIX* baseBones = base->GetEffectiveBones();
		D3DXMATRIX* baseInvBones = base->GetEffectiveInvBones();
		int baseBoneCount = base->GetEffectiveBoneCount();

		for (size_t i = 1; i < m_parts.size(); i++)
		{
			ScenePart& part = m_parts[i];
			if (part.model && part.visible)
			{
				if (baseBones && baseInvBones)
					part.model->SetExternBones(baseBones, baseInvBones, baseBoneCount);

				part.model->Animate(m_currentFrame, nextFrame, part.currentLOD);
			}
		}
	}
}

bool SceneManager::ExportToGLTF(const std::string& filepath, bool exportAllAnimations) const
{
	GltfExportOptions opts;
	opts.exportAllAnimations = exportAllAnimations;
	opts.embedTextures = true;
	opts.exportCompositeScene = true;
	return GltfExporter::ExportGLB(filepath, *this, opts);
}

bool SceneManager::ExportToDAE(const std::string& filepath, bool exportAllAnimations) const
{
	DaeExportOptions opts;
	opts.exportAllAnimations = exportAllAnimations;
	opts.exportCompositeScene = true;
	opts.copyTextures = true;
	return DaeExporter::ExportDAE(filepath, *this, opts);
}

void SceneManager::SyncBones()
{
	_rebuildBoneLinks();
}

void SceneManager::_rebuildBoneLinks()
{
	CObject3D* base = GetBaseModel();
	if (!base) return;

	D3DXMATRIX* baseBones = base->GetEffectiveBones();
	D3DXMATRIX* baseInvBones = base->GetEffectiveInvBones();
	int baseBoneCount = base->GetEffectiveBoneCount();

	for (size_t i = 1; i < m_parts.size(); i++)
	{
		ScenePart& part = m_parts[i];
		if (part.model)
		{
			if (baseBones && baseInvBones)
				part.model->SetExternBones(baseBones, baseInvBones, baseBoneCount);
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
