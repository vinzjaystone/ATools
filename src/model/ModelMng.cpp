#include "ModelMng.h"
#include "Object3D.h"
#include "Motion.h"
#include "../core/File.h"
#include <algorithm>

CModelMng* CModelMng::s_instance = nullptr;

CModelMng* CModelMng::Instance()
{
	if (!s_instance)
		s_instance = new CModelMng();
	return s_instance;
}

void CModelMng::Destroy()
{
	if (s_instance)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

CModelMng::CModelMng()
	: m_modelPath("")
{
}

CModelMng::~CModelMng()
{
	Clear();
}

void CModelMng::Clear()
{
	for (auto& pair : m_objects)
		delete pair.second;
	m_objects.clear();

	for (auto& pair : m_skeletons)
		delete pair.second;
	m_skeletons.clear();

	for (auto& pair : m_motions)
		delete pair.second;
	m_motions.clear();
}

static std::string ToLower(const std::string& str)
{
	std::string s = str;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

int CModelMng::GetModelPart(const std::string& filename)
{
	std::string name = ToLower(GetFileName(filename));

	if (name.find("head") != std::string::npos)
		return PARTS_HEAD;
	if (name.find("upper") != std::string::npos)
		return PARTS_UPPER_BODY;
	if (name.find("lower") != std::string::npos)
		return PARTS_LOWER_BODY;
	if (name.find("hand") != std::string::npos || name.find("arm") != std::string::npos)
		return PARTS_HAND;
	if (name.find("foot") != std::string::npos || name.find("feet") != std::string::npos)
		return PARTS_FOOT;
	if (name.find("hair") != std::string::npos)
		return PARTS_HAIR;
	if (name.find("cap") != std::string::npos || name.find("hat") != std::string::npos || name.find("helm") != std::string::npos)
		return PARTS_CAP;
	if (name.find("cloak") != std::string::npos)
		return PARTS_CLOAK;
	if (name.find("shield") != std::string::npos)
		return PARTS_SHIELD;
	if (name.find("wing") != std::string::npos)
		return PARTS_WING;
	if (name.find("rweapon") != std::string::npos || name.find("weap") != std::string::npos)
		return PARTS_RWEAPON;
	if (name.find("lweapon") != std::string::npos)
		return PARTS_LWEAPON;

	return PARTS_HEAD;
}

CObject3D* CModelMng::GetObject3D(const std::string& filename)
{
	auto it = m_objects.find(filename);
	if (it != m_objects.end())
		return it->second;

	std::string fullPath = m_modelPath + filename;
	CObject3D* obj = new CObject3D();
	if (obj->Load(fullPath))
	{
		m_objects[filename] = obj;
		return obj;
	}

	delete obj;
	return nullptr;
}

CSkeleton* CModelMng::GetSkeleton(const std::string& filename)
{
	auto it = m_skeletons.find(filename);
	if (it != m_skeletons.end())
		return it->second;

	std::string fullPath = m_modelPath + filename;
	CSkeleton* skl = new CSkeleton();
	if (skl->Load(fullPath))
	{
		m_skeletons[filename] = skl;
		return skl;
	}

	delete skl;
	return nullptr;
}

CMotion* CModelMng::GetMotion(const std::string& filename)
{
	auto it = m_motions.find(filename);
	if (it != m_motions.end())
		return it->second;

	std::string fullPath = m_modelPath + filename;
	CMotion* mot = new CMotion();
	if (mot->Load(fullPath))
	{
		m_motions[filename] = mot;
		return mot;
	}

	delete mot;
	return nullptr;
}
