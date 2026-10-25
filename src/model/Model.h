#ifndef MODEL_H
#define MODEL_H

#include "../core/Types.h"
#include "../core/D3DXMath.h"
#include <string>

class CModel
{
public:
	CModel();
	virtual ~CModel();

	virtual bool Load(const std::string& filename) = 0;
	virtual void MoveFrame();
	virtual void SetFrame(float currentFrame);

	int GetNextFrame() const;
	const Bounds& GetBounds() const { return m_bounds; }
	int GetFrameCount() const { return m_frameCount; }
	float GetCurrentFrame() const { return m_currentFrame; }
	void SetTextureEx(int ex) { m_textureEx = ex; }
	int GetTextureEx() const { return m_textureEx; }
	bool HasSkin() const { return m_hasSkin; }

protected:
	Bounds m_bounds;
	int m_frameCount;
	float m_currentFrame;
	int m_textureEx;
	float m_perSlerp;
	bool m_hasSkin;
};

#endif // MODEL_H
