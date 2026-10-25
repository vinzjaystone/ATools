#include "Model.h"
#include <cstring>
#include <cmath>

CModel::CModel()
	: m_frameCount(0),
	  m_currentFrame(0.0f),
	  m_textureEx(0),
	  m_perSlerp(0.5f),
	  m_hasSkin(false)
{
	std::memset(&m_bounds, 0, sizeof(m_bounds));
}

CModel::~CModel()
{
}

void CModel::MoveFrame()
{
	if (m_frameCount > 0)
	{
		m_currentFrame += m_perSlerp;
		if ((int)m_currentFrame >= m_frameCount)
			m_currentFrame -= (float)m_frameCount;
	}
}

void CModel::SetFrame(float currentFrame)
{
	if (m_frameCount > 0)
	{
		m_currentFrame = currentFrame;
		while (m_currentFrame >= (float)m_frameCount)
			m_currentFrame -= (float)m_frameCount;
		while (m_currentFrame < 0.0f)
			m_currentFrame += (float)m_frameCount;
	}
	else
	{
		m_currentFrame = 0.0f;
	}
}

int CModel::GetNextFrame() const
{
	if (m_frameCount <= 0) return 0;
	int next = (int)m_currentFrame + 1;
	if (next >= m_frameCount)
		next = 0;
	return next;
}
