#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "../core/Types.h"
#include "../model/Object3D.h"
#include "../model/SceneManager.h"
#include "../model/FolderScanner.h"
#include "../viewer/Camera.h"
#include "../viewer/Renderer.h"
#include <string>
#include <vector>

enum EFileDialogTarget
{
	DIALOG_OPEN_MODEL,
	DIALOG_APPEND_MODEL,
	DIALOG_SAVE_MODEL,
	DIALOG_OPEN_MOTION,
	DIALOG_OPEN_SKELETON,
	DIALOG_SELECT_FOLDER
};

struct FileDialogState
{
	bool active;
	EFileDialogTarget target;
	std::string currentPath;
	std::string selectedFile;
	char manualPathBuffer[512];
	std::vector<std::string> directories;
	std::vector<std::string> files;
	int scrollIndex;
	int selectedIndex;

	FileDialogState()
		: active(false),
		  target(DIALOG_OPEN_MODEL),
		  currentPath("./"),
		  selectedFile(""),
		  scrollIndex(0),
		  selectedIndex(-1)
	{
		std::memset(manualPathBuffer, 0, sizeof(manualPathBuffer));
	}
};

class UserInterface
{
public:
	UserInterface();
	~UserInterface();

	void UpdateAndDraw(SceneManager& scene, OrbitCamera& camera,
	                   RenderSettings& settings, float deltaTime);

	void ShowStatusMessage(const std::string& msg, float duration = 4.0f, bool isError = false);
	void OpenFileDialog(EFileDialogTarget target);

	Rectangle GetViewportRect() const { return m_viewportRect; }
	FolderScanner& GetScanner() { return m_scanner; }

private:
	Rectangle m_viewportRect;
	Rectangle m_topBarRect;
	Rectangle m_leftPanelRect;
	Rectangle m_rightPanelRect;
	Rectangle m_bottomBarRect;
	Rectangle m_statusBarRect;

	std::string m_statusMessage;
	float m_statusTimer;
	bool m_statusIsError;

	FileDialogState m_fileDialog;
	FolderScanner m_scanner;

	int m_leftTab;       // 0: Library / Scanner, 1: Scene Parts, 2: Model Stats & Hierarchy
	int m_rightTab;      // 0: Material Inspector, 1: Submesh Properties, 2: Animation & Skeleton
	int m_libraryTab;    // 0: Models (.o3d), 1: Motions (.mot), 2: Skeletons (.bon)

	bool m_appendMode;   // Global click action in library: false = Replace, true = Append
	char m_searchBuffer[128];
	int m_libraryScroll;
	int m_scenePartsScroll;
	int m_hierarchyScroll;
	int m_bonesScroll;

	// Material editing temp buffers
	char m_texNameBuffer[256];
	bool m_editingTexture;

	void _updateLayout(int screenWidth, int screenHeight, bool hasAnimation);
	void _drawTopBar(SceneManager& scene, OrbitCamera& camera, RenderSettings& settings);

	void _drawLeftPanel(SceneManager& scene, OrbitCamera& camera, RenderSettings& settings);
	void _drawLibraryTab(SceneManager& scene, OrbitCamera& camera);
	void _drawScenePartsTab(SceneManager& scene, OrbitCamera& camera);
	void _drawModelStatsTab(SceneManager& scene, RenderSettings& settings);

	void _drawRightPanel(SceneManager& scene, RenderSettings& settings);
	void _drawMaterialTab(CObject3D* model, int currentLOD, RenderSettings& settings);
	void _drawSubmeshTab(CObject3D* model, int currentLOD, RenderSettings& settings);
	void _drawAnimationTab(SceneManager& scene);

	void _drawBottomBar(SceneManager& scene, float deltaTime);
	void _drawStatusBar(const SceneManager& scene, const OrbitCamera& camera);
	void _drawFileDialog(SceneManager& scene, OrbitCamera& camera);

	void _refreshDirectoryContents();
};

#endif // UI_H
