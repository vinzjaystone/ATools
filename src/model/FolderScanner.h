#ifndef FOLDERSCANNER_H
#define FOLDERSCANNER_H

#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

struct ScannedFile
{
	std::string filename;
	std::string filepath;
	std::string relativePath;
	uintmax_t size;
	std::string extension;

	ScannedFile() : size(0) {}
};

class FolderScanner
{
public:
	FolderScanner();
	~FolderScanner();

	void Scan(const std::string& folderPath = "", bool recursive = false);
	void SetCurrentFolder(const std::string& folder);
	std::string GetCurrentFolder() const { return m_currentFolder; }

	void SetSearchFilter(const std::string& filter);
	std::string GetSearchFilter() const { return m_searchFilter; }

	void SetRecursive(bool recursive) { m_recursive = recursive; }
	bool IsRecursive() const { return m_recursive; }

	const std::vector<ScannedFile>& GetModels() const { return m_filteredModels; }
	const std::vector<ScannedFile>& GetMotions() const { return m_filteredMotions; }
	const std::vector<ScannedFile>& GetSkeletons() const { return m_filteredSkeletons; }

	int GetTotalModelCount() const { return (int)m_allModels.size(); }
	int GetTotalMotionCount() const { return (int)m_allMotions.size(); }
	int GetTotalSkeletonCount() const { return (int)m_allSkeletons.size(); }

	static std::string FormatFileSize(uintmax_t bytes);

private:
	std::string m_currentFolder;
	std::string m_searchFilter;
	bool m_recursive;

	std::vector<ScannedFile> m_allModels;
	std::vector<ScannedFile> m_allMotions;
	std::vector<ScannedFile> m_allSkeletons;

	std::vector<ScannedFile> m_filteredModels;
	std::vector<ScannedFile> m_filteredMotions;
	std::vector<ScannedFile> m_filteredSkeletons;

	void _applyFilter();
};

#endif // FOLDERSCANNER_H
