#include "FolderScanner.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

static std::string ToLower(const std::string& str)
{
	std::string res = str;
	std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
		return std::tolower(c);
	});
	return res;
}

FolderScanner::FolderScanner()
	: m_currentFolder("./"),
	  m_searchFilter(""),
	  m_recursive(false)
{
}

FolderScanner::~FolderScanner()
{
}

std::string FolderScanner::FormatFileSize(uintmax_t bytes)
{
	if (bytes < 1024)
		return std::to_string(bytes) + " B";
	else if (bytes < 1024 * 1024)
	{
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(1) << ((double)bytes / 1024.0) << " KB";
		return ss.str();
	}
	else
	{
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(1) << ((double)bytes / (1024.0 * 1024.0)) << " MB";
		return ss.str();
	}
}

void FolderScanner::SetCurrentFolder(const std::string& folder)
{
	if (!folder.empty())
	{
		m_currentFolder = folder;
		if (m_currentFolder.back() != '/' && m_currentFolder.back() != '\\')
			m_currentFolder += "/";
		Scan(m_currentFolder, m_recursive);
	}
}

void FolderScanner::SetSearchFilter(const std::string& filter)
{
	m_searchFilter = filter;
	_applyFilter();
}

void FolderScanner::Scan(const std::string& folderPath, bool recursive)
{
	if (!folderPath.empty())
	{
		m_currentFolder = folderPath;
		if (m_currentFolder.back() != '/' && m_currentFolder.back() != '\\')
			m_currentFolder += "/";
	}

	m_recursive = recursive;
	m_allModels.clear();
	m_allMotions.clear();
	m_allSkeletons.clear();

	try
	{
		if (!fs::exists(m_currentFolder) || !fs::is_directory(m_currentFolder))
			return;

		auto processEntry = [this](const fs::directory_entry& entry) {
			if (!entry.is_regular_file()) return;

			std::string ext = ToLower(entry.path().extension().string());
			if (ext == ".o3d" || ext == ".mot" || ext == ".ani" || ext == ".mo3" || ext == ".chr" || ext == ".bon")
			{
				ScannedFile file;
				file.filename = entry.path().filename().string();
				file.filepath = entry.path().string();
				file.relativePath = fs::relative(entry.path(), m_currentFolder).string();
				file.extension = ext;

				std::error_code ec;
				file.size = entry.file_size(ec);
				if (ec) file.size = 0;

				if (ext == ".o3d")
					m_allModels.push_back(file);
				else if (ext == ".mot" || ext == ".ani" || ext == ".mo3")
					m_allMotions.push_back(file);
				else if (ext == ".chr" || ext == ".bon")
					m_allSkeletons.push_back(file);
			}
		};

		if (m_recursive)
		{
			for (const auto& entry : fs::recursive_directory_iterator(m_currentFolder, fs::directory_options::skip_permission_denied))
			{
				processEntry(entry);
			}
		}
		else
		{
			for (const auto& entry : fs::directory_iterator(m_currentFolder, fs::directory_options::skip_permission_denied))
			{
				processEntry(entry);
			}
		}

		auto sortFunc = [](const ScannedFile& a, const ScannedFile& b) {
			return ToLower(a.filename) < ToLower(b.filename);
		};

		std::sort(m_allModels.begin(), m_allModels.end(), sortFunc);
		std::sort(m_allMotions.begin(), m_allMotions.end(), sortFunc);
		std::sort(m_allSkeletons.begin(), m_allSkeletons.end(), sortFunc);
	}
	catch (const std::exception&)
	{
		// Silently handle any filesystem permission errors
	}

	_applyFilter();
}

void FolderScanner::_applyFilter()
{
	m_filteredModels.clear();
	m_filteredMotions.clear();
	m_filteredSkeletons.clear();

	std::string filterLower = ToLower(m_searchFilter);

	auto matches = [&filterLower](const ScannedFile& f) {
		if (filterLower.empty()) return true;
		return ToLower(f.filename).find(filterLower) != std::string::npos ||
		       ToLower(f.relativePath).find(filterLower) != std::string::npos;
	};

	for (const auto& f : m_allModels)
		if (matches(f)) m_filteredModels.push_back(f);

	for (const auto& f : m_allMotions)
		if (matches(f)) m_filteredMotions.push_back(f);

	for (const auto& f : m_allSkeletons)
		if (matches(f)) m_filteredSkeletons.push_back(f);
}
