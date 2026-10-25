#ifndef FILE_H
#define FILE_H

#include "Types.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

enum class FileMode
{
	NotOpen,
	ReadOnly,
	WriteOnly
};

class CFile
{
public:
	CFile();
	~CFile();

	bool Open(const std::string& filename, FileMode mode, bool error = true);
	void Close();

	// ReadOnly functions
	void Skip(int byteCount);
	void SetPos(int bytePos);
	int GetPos() const;
	int GetSize() const;
	const char* GetBuffer() const;
	bool IsOpen() const { return m_mode != FileMode::NotOpen; }

	void ReadBytes(void* dest, size_t byteCount)
	{
		if (!dest || byteCount == 0) return;
		if (m_cur && m_cur + byteCount <= m_buffer.data() + m_buffer.size())
		{
			std::memcpy(dest, m_cur, byteCount);
			m_cur += byteCount;
		}
		else if (m_cur)
		{
			size_t available = (m_buffer.data() + m_buffer.size() > m_cur) ?
			                   (size_t)(m_buffer.data() + m_buffer.size() - m_cur) : 0;
			std::memcpy(dest, m_cur, available);
			std::memset((char*)dest + available, 0, byteCount - available);
			m_cur = m_buffer.data() + m_buffer.size();
		}
	}

	template<class T> void Read(T& val)
	{
		ReadBytes(&val, sizeof(T));
	}

	template<class T> void Read(T* val, int count)
	{
		if (count > 0 && val)
			ReadBytes(val, sizeof(T) * (size_t)count);
	}

	// WriteOnly functions
	void WriteBytes(const void* src, size_t byteCount)
	{
		if (!src || byteCount == 0) return;
		const char* ptr = reinterpret_cast<const char*>(src);
		m_writeBuffer.insert(m_writeBuffer.end(), ptr, ptr + byteCount);
	}

	template<class T> void Write(const T& val)
	{
		WriteBytes(&val, sizeof(T));
	}

	template<class T> void Write(const T* val, int count)
	{
		if (count > 0 && val)
			WriteBytes(val, sizeof(T) * (size_t)count);
	}

private:
	FileMode m_mode;
	std::string m_filename;
	std::vector<char> m_buffer;
	std::vector<char> m_writeBuffer;
	const char* m_cur;
};

std::string GetFileName(const std::string& path);
std::string GetFileDirectory(const std::string& path);
std::string GetFileExtension(const std::string& path);

#endif // FILE_H
