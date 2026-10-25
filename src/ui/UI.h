#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "../core/Types.h"
#include "../model/Object3D.h"
#include "../viewer/Camera.h"
#include "../viewer/Renderer.h"
#include <string>
#include <vector>

struct FileDialogState
{
	bool active;
	bool isSaveMode;
	std::string currentPath;
	std::string selectedFile;
	char manualPathBuffer[512];
	std::vector<std::string> directories;
	std::vector<std::string> files;
	int scrollIndex;
	int selectedIndex;

	FileDialogState()
		: active(false),
		  isSaveMode(false),
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

	void UpdateAndDraw(CObject3D*& model, int& currentLOD, OrbitCamera& camera,
	                   RenderSettings& settings, float& animSpeed, bool& isPlaying,
	                   float deltaTime);

	void ShowStatusMessage(const std::string& msg, float duration = 4.0f, bool isError = false);
	void OpenFileDialog(bool isSave);

	std::string GetSelectedFileToOpen() const { return m_fileToOpen; }
	std::string GetSelectedFileToSave() const { return m_fileToSave; }
	void ClearFileRequests() { m_fileToOpen.clear(); m_fileToSave.clear(); }

	Rectangle GetViewportRect() const { return m_viewportRect; }

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
	std::string m_fileToOpen;
	std::string m_fileToSave;

	int m_leftTab;  // 0: Model Info, 1: Sub-Objects
	int m_rightTab; // 0: Material Inspector, 1: Model Tools, 2: Environment

	// Material editing temp buffers
	char m_texNameBuffer[256];
	bool m_editingTexture;

	void _updateLayout(int screenWidth, int screenHeight, bool hasAnimation);
	void _drawTopBar(CObject3D*& model, int& currentLOD, OrbitCamera& camera,
	                 RenderSettings& settings);
	void _drawLeftPanel(const CObject3D* model, int currentLOD, RenderSettings& settings);
	void _drawRightPanel(CObject3D* model, int currentLOD, RenderSettings& settings);
	void _drawBottomBar(CObject3D* model, int currentLOD, float& animSpeed,
	                    bool& isPlaying, float deltaTime);
	void _drawStatusBar();
	void _drawFileDialog();

	void _refreshDirectoryContents();
};

#endif // UI_H
