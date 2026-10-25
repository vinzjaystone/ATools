#include "TextureMng.h"
#include "../core/File.h"
#include <iostream>
#include <sys/stat.h>

CTextureMng* CTextureMng::s_instance = nullptr;

static bool FileExists(const std::string& path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

CTextureMng* CTextureMng::Instance()
{
	if (!s_instance)
		s_instance = new CTextureMng();
	return s_instance;
}

void CTextureMng::Destroy()
{
	if (s_instance)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

CTextureMng::CTextureMng()
	: m_modelTexturePath(""),
	  m_defaultsCreated(false)
{
	m_defaultWhite = Texture2D{ 0, 0, 0, 0, 0 };
	m_defaultGrid = Texture2D{ 0, 0, 0, 0, 0 };
}

CTextureMng::~CTextureMng()
{
	Clear();
}

void CTextureMng::_initDefaults()
{
	if (m_defaultsCreated) return;

	// Create 1x1 white texture
	Image whiteImg = GenImageColor(2, 2, WHITE);
	m_defaultWhite = LoadTextureFromImage(whiteImg);
	UnloadImage(whiteImg);

	// Create 32x32 subtle checkerboard grid texture
	Image gridImg = GenImageChecked(64, 64, 8, 8, Color{ 220, 220, 220, 255 }, Color{ 180, 180, 180, 255 });
	m_defaultGrid = LoadTextureFromImage(gridImg);
	UnloadImage(gridImg);

	m_defaultsCreated = true;
}

Texture2D CTextureMng::GetDefaultWhiteTexture()
{
	_initDefaults();
	return m_defaultWhite;
}

Texture2D CTextureMng::GetDefaultGridTexture()
{
	_initDefaults();
	return m_defaultGrid;
}

std::string CTextureMng::_findTexturePath(const std::string& filename, const std::string& searchDir)
{
	if (filename.empty()) return "";

	std::string base = GetFileName(filename);
	std::string ext = GetFileExtension(base);
	std::string nameNoExt = base.substr(0, base.length() - (ext.empty() ? 0 : ext.length() + 1));

	std::vector<std::string> searchDirs;
	if (!searchDir.empty()) searchDirs.push_back(searchDir);
	if (!m_modelTexturePath.empty()) searchDirs.push_back(m_modelTexturePath);
	searchDirs.push_back("");
	searchDirs.push_back("Model/");
	searchDirs.push_back("Model/Texture/");
	searchDirs.push_back("Texture/");
	searchDirs.push_back("model/");
	searchDirs.push_back("model/texture/");

	std::vector<std::string> exts;
	if (!ext.empty()) exts.push_back("." + ext);
	exts.push_back(".png");
	exts.push_back(".PNG");
	exts.push_back(".tga");
	exts.push_back(".TGA");
	exts.push_back(".bmp");
	exts.push_back(".BMP");
	exts.push_back(".dds");
	exts.push_back(".DDS");
	exts.push_back(".jpg");
	exts.push_back(".JPG");

	for (const auto& dir : searchDirs)
	{
		std::string d = dir;
		if (!d.empty() && d.back() != '/' && d.back() != '\\')
			d += "/";

		// Try original exact filename
		std::string full = d + filename;
		if (FileExists(full)) return full;

		full = d + base;
		if (FileExists(full)) return full;

		// Try variations of extension
		for (const auto& e : exts)
		{
			std::string candidate = d + nameNoExt + e;
			if (FileExists(candidate)) return candidate;
		}
	}

	return "";
}

Texture2D CTextureMng::GetTexture(const std::string& filename, const std::string& searchDir)
{
	_initDefaults();

	if (filename.empty())
		return m_defaultWhite;

	auto it = m_textures.find(filename);
	if (it != m_textures.end())
		return it->second;

	std::string fullPath = _findTexturePath(filename, searchDir);
	if (!fullPath.empty())
	{
		Texture2D tex = LoadTexture(fullPath.c_str());
		if (tex.id > 0)
		{
			SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
			SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
			m_textures[filename] = tex;
			return tex;
		}
	}

	return m_defaultWhite;
}

void CTextureMng::Clear()
{
	for (auto& pair : m_textures)
	{
		if (pair.second.id > 0)
			UnloadTexture(pair.second);
	}
	m_textures.clear();

	if (m_defaultWhite.id > 0)
	{
		UnloadTexture(m_defaultWhite);
		m_defaultWhite = Texture2D{ 0, 0, 0, 0, 0 };
	}
	if (m_defaultGrid.id > 0)
	{
		UnloadTexture(m_defaultGrid);
		m_defaultGrid = Texture2D{ 0, 0, 0, 0, 0 };
	}
	m_defaultsCreated = false;
}
