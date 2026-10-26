#ifndef FBX_EXPORTER_H
#define FBX_EXPORTER_H

#include "../core/Types.h"
#include "../core/D3DXMath.h"
#include "Object3D.h"
#include "Motion.h"
#include <string>
#include <vector>

class SceneManager;

struct FbxExportOptions
{
	bool exportAllAnimations;   // Export all discovered motions or active motion only
	bool exportCompositeScene;  // Export all parts in scene or active model only
	bool copyTextures;          // Copy textures to export folder
	int lodLevel;               // LOD level to export (default 0)

	FbxExportOptions()
		: exportAllAnimations(true),
		  exportCompositeScene(true),
		  copyTextures(true),
		  lodLevel(0)
	{
	}
};

class FbxExporter
{
public:
	static bool ExportFBX(const std::string& filepath, const SceneManager& scene,
	                      const FbxExportOptions& options = FbxExportOptions());

	static bool ExportModelFBX(const std::string& filepath, const CObject3D* model,
	                           const std::vector<CMotion*>& motions,
	                           const std::string& modelDirectory = "./");
};

#endif // FBX_EXPORTER_H
