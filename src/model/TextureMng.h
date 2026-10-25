#ifndef TEXTUREMNG_H
#define TEXTUREMNG_H

#include "raylib.h"
#include <string>
#include <map>

class CTextureMng
{
public:
	static CTextureMng* Instance();
	static void Destroy();

	Texture2D GetTexture(const std::string& filename, const std::string& searchDir = "");
	void SetModelTexturePath(const std::string& path) { m_modelTexturePath = path; }
	std::string GetModelTexturePath() const { return m_modelTexturePath; }

	Texture2D GetDefaultWhiteTexture();
	Texture2D GetDefaultGridTexture();

	void Clear();

private:
	CTextureMng();
	~CTextureMng();

	static CTextureMng* s_instance;
	std::string m_modelTexturePath;
	std::map<std::string, Texture2D> m_textures;
	Texture2D m_defaultWhite;
	Texture2D m_defaultGrid;
	bool m_defaultsCreated;

	void _initDefaults();
	std::string _findTexturePath(const std::string& filename, const std::string& searchDir);
};

#define TextureMng CTextureMng::Instance()

#endif // TEXTUREMNG_H
