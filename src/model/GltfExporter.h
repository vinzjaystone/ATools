#ifndef GLTF_EXPORTER_H
#define GLTF_EXPORTER_H

#include "../core/Types.h"
#include "../core/D3DXMath.h"
#include "Object3D.h"
#include "Motion.h"
#include <string>
#include <vector>

class SceneManager;

struct GltfExportOptions
{
	bool exportAllAnimations;   // Export all discovered motions or active motion only
	bool embedTextures;         // Embed textures into GLB / base64
	bool exportCompositeScene;  // Export all parts in scene or active model only
	int lodLevel;               // LOD level to export (default 0)

	GltfExportOptions()
		: exportAllAnimations(true),
		  embedTextures(true),
		  exportCompositeScene(true),
		  lodLevel(0)
	{
	}
};

class GltfExporter
{
public:
	static bool ExportGLB(const std::string& filepath, const SceneManager& scene,
	                      const GltfExportOptions& options = GltfExportOptions());

	static bool ExportGLTF(const std::string& filepath, const SceneManager& scene,
	                       const GltfExportOptions& options = GltfExportOptions());

	static bool ExportModelGLB(const std::string& filepath, const CObject3D* model,
	                           const std::vector<CMotion*>& motions,
	                           const std::string& modelDirectory = "./");
};

#endif // GLTF_EXPORTER_H
