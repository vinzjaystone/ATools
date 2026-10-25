#include "File.h"
#include <iostream>

CFile::CFile()
	: m_mode(FileMode::NotOpen),
	  m_cur(nullptr)
{
}

CFile::~CFile()
{
	Close();
}

bool CFile::Open(const std::string& filename, FileMode mode, bool error)
{
	Close();
	m_filename = filename;

	if (mode == FileMode::ReadOnly)
	{
		FILE* fp = std::fopen(filename.c_str(), "rb");
		if (!fp)
		{
			if (error)
				std::cerr << "CFile: Can't open file for reading: " << filename << std::endl;
			return false;
		}

		std::fseek(fp, 0, SEEK_END);
		long size = std::ftell(fp);
		std::fseek(fp, 0, SEEK_SET);

		if (size < 0)
		{
			std::fclose(fp);
			return false;
		}

		m_buffer.resize(size);
		if (size > 0)
		{
			size_t readBytes = std::fread(m_buffer.data(), 1, size, fp);
			(void)readBytes;
		}
		std::fclose(fp);

		m_cur = m_buffer.data();
		m_mode = FileMode::ReadOnly;
		return true;
	}
	else if (mode == FileMode::WriteOnly)
	{
		m_writeBuffer.clear();
		m_mode = FileMode::WriteOnly;
		return true;
	}

	return false;
}

void CFile::Close()
{
	if (m_mode == FileMode::WriteOnly && !m_writeBuffer.empty() && !m_filename.empty())
	{
		FILE* fp = std::fopen(m_filename.c_str(), "wb");
		if (fp)
		{
			std::fwrite(m_writeBuffer.data(), 1, m_writeBuffer.size(), fp);
			std::fclose(fp);
		}
		else
		{
			std::cerr << "CFile: Can't open file for writing: " << m_filename << std::endl;
		}
	}

	m_mode = FileMode::NotOpen;
	m_buffer.clear();
	m_writeBuffer.clear();
	m_cur = nullptr;
}

void CFile::Skip(int byteCount)
{
	if (m_mode == FileMode::ReadOnly && m_cur)
	{
		m_cur += byteCount;
	}
}

void CFile::SetPos(int bytePos)
{
	if (m_mode == FileMode::ReadOnly)
	{
		if (bytePos >= 0 && bytePos <= (int)m_buffer.size())
			m_cur = m_buffer.data() + bytePos;
	}
}

int CFile::GetPos() const
{
	if (m_mode == FileMode::ReadOnly && m_cur)
		return (int)(m_cur - m_buffer.data());
	return 0;
}

int CFile::GetSize() const
{
	if (m_mode == FileMode::ReadOnly)
		return (int)m_buffer.size();
	return (int)m_writeBuffer.size();
}

const char* CFile::GetBuffer() const
{
	if (m_mode == FileMode::ReadOnly)
		return m_buffer.data();
	return m_writeBuffer.data();
}

std::string GetFileName(const std::string& path)
{
	size_t pos = path.find_last_of("/\\");
	if (pos != std::string::npos)
		return path.substr(pos + 1);
	return path;
}

std::string GetFileDirectory(const std::string& path)
{
	size_t pos = path.find_last_of("/\\");
	if (pos != std::string::npos)
		return path.substr(0, pos + 1);
	return "";
}

std::string GetFileExtension(const std::string& path)
{
	size_t pos = path.find_last_of('.');
	if (pos != std::string::npos)
		return path.substr(pos + 1);
	return "";
}
