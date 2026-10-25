#ifndef MODELMNG_H
#define MODELMNG_H

#include "../core/Types.h"
#include "../core/Defines.h"
#include <string>
#include <map>

class CObject3D;
class CSkeleton;
class CMotion;

class CModelMng
{
public:
	static CModelMng* Instance();
	static void Destroy();

	void SetModelPath(const std::string& path) { m_modelPath = path; }
	std::string GetModelPath() const { return m_modelPath; }

	int GetModelPart(const std::string& filename);

	CObject3D* GetObject3D(const std::string& filename);
	CSkeleton* GetSkeleton(const std::string& filename);
	CMotion* GetMotion(const std::string& filename);

	void Clear();

private:
	CModelMng();
	~CModelMng();

	static CModelMng* s_instance;
	std::string m_modelPath;
	std::map<std::string, CObject3D*> m_objects;
	std::map<std::string, CSkeleton*> m_skeletons;
	std::map<std::string, CMotion*> m_motions;
};

#define ModelMng CModelMng::Instance()

#endif // MODELMNG_H
