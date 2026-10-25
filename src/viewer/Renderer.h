#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include "../core/Types.h"
#include "../core/D3DXMath.h"
#include "../model/Object3D.h"
#include "../model/SceneManager.h"
#include "Camera.h"

struct RenderSettings
{
	bool showGrid;
	bool showAxes;
	bool showBounds;
	bool showSubBounds;
	bool showCollision;
	bool showBones;
	bool enableLighting;
	bool enableTextures;
	EFillMode fillMode;
	Color backgroundColor;
	int selectedObjIndex;
	int selectedBlockIndex;

	RenderSettings()
		: showGrid(true),
		  showAxes(true),
		  showBounds(false),
		  showSubBounds(false),
		  showCollision(false),
		  showBones(false),
		  enableLighting(true),
		  enableTextures(true),
		  fillMode(FILL_SOLID),
		  backgroundColor(Color{ 45, 48, 55, 255 }),
		  selectedObjIndex(-1),
		  selectedBlockIndex(-1)
	{
	}
};

class SceneRenderer
{
public:
	SceneRenderer();
	~SceneRenderer();

	// Render full scene with composite parts
	void RenderScene(const SceneManager& scene, const OrbitCamera& camera,
	                 Rectangle viewportRect, const RenderSettings& settings);

	// Render single standalone model
	void Render(const CObject3D* model, int lod, const OrbitCamera& camera,
	            Rectangle viewportRect, const RenderSettings& settings);

	void DrawGrid(int slices, float spacing);
	void DrawAxes(float length);
	void DrawBoundingBox(const Bounds& bounds, Color color);
	void DrawBoundingBoxOriented(const Bounds& bounds, const D3DXMATRIX& mat, Color color);
	void DrawCollisionMesh(const GMObject* collObj, const RenderSettings& settings);
	void DrawBones(const CObject3D* model);

private:
	void _drawModelInternal(const CObject3D* model, int lod, const D3DXMATRIX& rootTransform,
	                        const RenderSettings& settings, bool isSelectedPart);

	void _drawGMObject(const GMObject& obj, const D3DXMATRIX& transform, int lod,
	                   const RenderSettings& settings, const CObject3D* model,
	                   int objIdx, bool isSelectedPart);

	void _drawSkinGMObject(const GMObject& obj, const D3DXMATRIX& rootTransform, int lod,
	                       const RenderSettings& settings, const CObject3D* model,
	                       int objIdx, bool isSelectedPart);
};

#endif // RENDERER_H
