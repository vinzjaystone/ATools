#ifndef DAE_EXPORTER_H
#define DAE_EXPORTER_H

#include "../core/Types.h"
#include "../core/D3DXMath.h"
#include "Object3D.h"
#include "Motion.h"
#include <string>
#include <vector>

class SceneManager;

struct DaeExportOptions
{
	bool exportAllAnimations;   // Export all discovered motions or active motion only
	bool exportCompositeScene;  // Export all parts in scene or active model only
	bool copyTextures;          // Copy textures to export folder
	int lodLevel;               // LOD level to export (default 0)

	DaeExportOptions()
		: exportAllAnimations(true),
		  exportCompositeScene(true),
		  copyTextures(true),
		  lodLevel(0)
	{
	}
};

class DaeExporter
{
public:
	static bool ExportDAE(const std::string& filepath, const SceneManager& scene,
	                      const DaeExportOptions& options = DaeExportOptions());

	static bool ExportModelDAE(const std::string& filepath, const CObject3D* model,
	                           const std::vector<CMotion*>& motions,
	                           const std::string& modelDirectory = "./");
};

#endif // DAE_EXPORTER_H
