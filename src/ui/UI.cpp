#define RAYGUI_IMPLEMENTATION
#include "../ui/UI.h"
#include "../../Lib/raylib/include/raygui.h"
#include "../model/TextureMng.h"
#include "../model/Motion.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

UserInterface::UserInterface()
	: m_statusMessage("Ready. Use toolbar or Library panel to load Flyff .o3d models and .mot animations."),
	  m_statusTimer(5.0f),
	  m_statusIsError(false),
	  m_leftTab(0),
	  m_rightTab(0),
	  m_libraryTab(0),
	  m_appendMode(false),
	  m_libraryScroll(0),
	  m_scenePartsScroll(0),
	  m_hierarchyScroll(0),
	  m_bonesScroll(0),
	  m_editingTexture(false)
{
	std::memset(m_searchBuffer, 0, sizeof(m_searchBuffer));
	std::memset(m_texNameBuffer, 0, sizeof(m_texNameBuffer));

	// Style configurations for raygui
	GuiSetStyle(DEFAULT, TEXT_SIZE, 12);
	GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0x22252cff);
	GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, 0xd0d5e0ff);
	GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, 0xedf0f5ff);

	// Initial scan of current directory
	m_scanner.Scan("./", false);
}

UserInterface::~UserInterface()
{
}

void UserInterface::ShowStatusMessage(const std::string& msg, float duration, bool isError)
{
	m_statusMessage = msg;
	m_statusTimer = duration;
	m_statusIsError = isError;
}

void UserInterface::OpenFileDialog(EFileDialogTarget target)
{
	m_fileDialog.active = true;
	m_fileDialog.target = target;
	m_fileDialog.selectedIndex = -1;
	m_fileDialog.scrollIndex = 0;
	m_fileDialog.selectedFile = "";

	if (m_fileDialog.currentPath.empty())
		m_fileDialog.currentPath = "./";

	_refreshDirectoryContents();
}

void UserInterface::_refreshDirectoryContents()
{
	m_fileDialog.directories.clear();
	m_fileDialog.files.clear();

	try
	{
		std::string path = m_fileDialog.currentPath;
		if (!fs::exists(path) || !fs::is_directory(path))
			path = "./";

		m_fileDialog.directories.push_back("..");

		for (const auto& entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied))
		{
			if (entry.is_directory())
			{
				m_fileDialog.directories.push_back(entry.path().filename().string());
			}
			else if (entry.is_regular_file())
			{
				std::string ext = entry.path().extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				bool accept = false;
				if (m_fileDialog.target == DIALOG_OPEN_MODEL || m_fileDialog.target == DIALOG_APPEND_MODEL || m_fileDialog.target == DIALOG_SAVE_MODEL)
					accept = (ext == ".o3d");
				else if (m_fileDialog.target == DIALOG_OPEN_MOTION)
					accept = (ext == ".mot");
				else if (m_fileDialog.target == DIALOG_OPEN_SKELETON)
					accept = (ext == ".bon");
				else if (m_fileDialog.target == DIALOG_SELECT_FOLDER)
					accept = false;

				if (accept)
					m_fileDialog.files.push_back(entry.path().filename().string());
			}
		}

		std::sort(m_fileDialog.directories.begin() + 1, m_fileDialog.directories.end());
		std::sort(m_fileDialog.files.begin(), m_fileDialog.files.end());
	}
	catch (const std::exception&)
	{
	}
}

void UserInterface::_updateLayout(int screenWidth, int screenHeight, bool hasAnimation)
{
	float topBarHeight = 44.0f;
	float statusBarHeight = 24.0f;
	float bottomAnimHeight = hasAnimation ? 48.0f : 0.0f;
	float leftPanelWidth = 320.0f;
	float rightPanelWidth = 330.0f;

	m_topBarRect = Rectangle{ 0, 0, (float)screenWidth, topBarHeight };
	m_statusBarRect = Rectangle{ 0, (float)screenHeight - statusBarHeight, (float)screenWidth, statusBarHeight };

	float mainAreaY = topBarHeight;
	float mainAreaHeight = (float)screenHeight - topBarHeight - statusBarHeight - bottomAnimHeight;

	m_leftPanelRect = Rectangle{ 0, mainAreaY, leftPanelWidth, mainAreaHeight + bottomAnimHeight };
	m_rightPanelRect = Rectangle{ (float)screenWidth - rightPanelWidth, mainAreaY, rightPanelWidth, mainAreaHeight + bottomAnimHeight };

	float vpX = leftPanelWidth;
	float vpWidth = (float)screenWidth - leftPanelWidth - rightPanelWidth;

	m_viewportRect = Rectangle{ vpX, mainAreaY, vpWidth, mainAreaHeight };
	m_bottomBarRect = Rectangle{ vpX, mainAreaY + mainAreaHeight, vpWidth, bottomAnimHeight };
}

void UserInterface::UpdateAndDraw(SceneManager& scene, OrbitCamera& camera,
                                  RenderSettings& settings, float deltaTime)
{
	int screenWidth = GetScreenWidth();
	int screenHeight = GetScreenHeight();

	if (m_statusTimer > 0.0f)
		m_statusTimer -= deltaTime;

	bool hasAnimation = (scene.GetTotalFrameCount() > 0);
	_updateLayout(screenWidth, screenHeight, hasAnimation);

	// 1. Draw Top Toolbar
	_drawTopBar(scene, camera, settings);

	// 2. Draw Left Panel (Library Scanner / Scene Parts / Model Stats)
	_drawLeftPanel(scene, camera, settings);

	// 3. Draw Right Panel (Material Inspector / Submesh / Animation & Skeleton)
	_drawRightPanel(scene, settings);

	// 4. Draw Bottom Animation Timeline (if animation exists)
	if (hasAnimation)
		_drawBottomBar(scene, deltaTime);

	// 5. Draw Bottom Status Bar
	_drawStatusBar(scene, camera);

	// 6. Draw Modal File Dialog (if open)
	if (m_fileDialog.active)
		_drawFileDialog(scene, camera);
}

void UserInterface::_drawTopBar(SceneManager& scene, OrbitCamera& camera, RenderSettings& settings)
{
	DrawRectangleRec(m_topBarRect, Color{ 28, 30, 36, 255 });
	DrawLine(0, (int)m_topBarRect.height, GetScreenWidth(), (int)m_topBarRect.height, Color{ 48, 52, 62, 255 });

	float x = 10.0f;
	float y = 8.0f;
	float btnHeight = 28.0f;

	// File operations
	if (GuiButton(Rectangle{ x, y, 90, btnHeight }, "#01# Open Model"))
	{
		OpenFileDialog(DIALOG_OPEN_MODEL);
	}
	x += 96.0f;

	if (GuiButton(Rectangle{ x, y, 85, btnHeight }, "#201# + Append"))
	{
		OpenFileDialog(DIALOG_APPEND_MODEL);
	}
	x += 91.0f;

	CObject3D* activeModel = scene.GetActiveModel();
	if (GuiButton(Rectangle{ x, y, 65, btnHeight }, "#02# Save"))
	{
		if (activeModel)
		{
			std::string fn = activeModel->GetLoadedFilename();
			if (!fn.empty())
			{
				if (activeModel->Save(fn))
					ShowStatusMessage("Saved model: " + SceneManager::ExtractFileName(fn));
				else
					ShowStatusMessage("Error saving model: " + SceneManager::ExtractFileName(fn), 4.0f, true);
			}
			else
				OpenFileDialog(DIALOG_SAVE_MODEL);
		}
		else
		{
			ShowStatusMessage("No active model to save.", 3.0f, true);
		}
	}
	x += 71.0f;

	if (GuiButton(Rectangle{ x, y, 80, btnHeight }, "#03# Save As"))
	{
		if (activeModel)
			OpenFileDialog(DIALOG_SAVE_MODEL);
		else
			ShowStatusMessage("No active model to save.", 3.0f, true);
	}
	x += 86.0f;

	if (GuiButton(Rectangle{ x, y, 95, btnHeight }, "#132# Load Motion"))
	{
		OpenFileDialog(DIALOG_OPEN_MOTION);
	}
	x += 101.0f;

	// Separator
	DrawLine((int)x + 4, 8, (int)x + 4, 36, Color{ 60, 65, 78, 255 });
	x += 12.0f;

	// Camera & Framing
	if (GuiButton(Rectangle{ x, y, 80, btnHeight }, "#210# Frame (F)"))
	{
		if (scene.GetPartCount() > 0)
			camera.FrameModel(scene.GetCombinedBounds());
		else
			camera.Reset();
	}
	x += 86.0f;

	if (GuiButton(Rectangle{ x, y, 70, btnHeight }, "#198# Reset"))
	{
		camera.Reset();
	}
	x += 76.0f;

	// Separator
	DrawLine((int)x + 4, 8, (int)x + 4, 36, Color{ 60, 65, 78, 255 });
	x += 12.0f;

	// Viewport Overlays
	bool wire = (settings.fillMode == FILL_WIREFRAME);
	if (GuiToggle(Rectangle{ x, y, 40, btnHeight }, "Wire", &wire))
	{
		settings.fillMode = wire ? FILL_WIREFRAME : FILL_SOLID;
	}
	x += 44.0f;

	GuiToggle(Rectangle{ x, y, 45, btnHeight }, "Grid", &settings.showGrid);
	x += 49.0f;

	GuiToggle(Rectangle{ x, y, 45, btnHeight }, "Axes", &settings.showAxes);
	x += 49.0f;

	GuiToggle(Rectangle{ x, y, 50, btnHeight }, "Bounds", &settings.showBounds);
	x += 54.0f;

	GuiToggle(Rectangle{ x, y, 50, btnHeight }, "Bones", &settings.showBones);
	x += 54.0f;

	GuiToggle(Rectangle{ x, y, 45, btnHeight }, "Coll", &settings.showCollision);
	x += 49.0f;

	GuiToggle(Rectangle{ x, y, 45, btnHeight }, "Light", &settings.enableLighting);
	x += 49.0f;

	GuiToggle(Rectangle{ x, y, 45, btnHeight }, "Tex", &settings.enableTextures);
	x += 49.0f;

	// Background color toggle button
	if (GuiButton(Rectangle{ x, y, 32, btnHeight }, "#143#"))
	{
		if (settings.backgroundColor.r == 45)
			settings.backgroundColor = Color{ 15, 17, 22, 255 }; // Dark
		else if (settings.backgroundColor.r == 15)
			settings.backgroundColor = Color{ 90, 95, 110, 255 }; // Light grey
		else if (settings.backgroundColor.r == 90)
			settings.backgroundColor = Color{ 40, 60, 90, 255 }; // Blueish
		else
			settings.backgroundColor = Color{ 45, 48, 55, 255 }; // Default
	}
}

void UserInterface::_drawLeftPanel(SceneManager& scene, OrbitCamera& camera, RenderSettings& settings)
{
	DrawRectangleRec(m_leftPanelRect, Color{ 32, 35, 42, 255 });
	DrawLine((int)(m_leftPanelRect.x + m_leftPanelRect.width), (int)m_leftPanelRect.y,
	         (int)(m_leftPanelRect.x + m_leftPanelRect.width), (int)(m_leftPanelRect.y + m_leftPanelRect.height),
	         Color{ 48, 52, 62, 255 });

	// Tab header
	std::string tabPartsTitle = "Parts (" + std::to_string(scene.GetPartCount()) + ")";
	const char* tabNames[] = { "Library", tabPartsTitle.c_str(), "Hierarchy" };

	float tabWidth = m_leftPanelRect.width / 3.0f;
	for (int i = 0; i < 3; i++)
	{
		bool active = (m_leftTab == i);
		Rectangle tabRec{ m_leftPanelRect.x + i * tabWidth, m_leftPanelRect.y, tabWidth, 30 };
		if (GuiToggle(tabRec, tabNames[i], &active))
		{
			m_leftTab = i;
		}
	}

	if (m_leftTab == 0)
		_drawLibraryTab(scene, camera);
	else if (m_leftTab == 1)
		_drawScenePartsTab(scene, camera);
	else
		_drawModelStatsTab(scene, settings);
}

void UserInterface::_drawLibraryTab(SceneManager& scene, OrbitCamera& camera)
{
	float x = m_leftPanelRect.x + 8.0f;
	float y = m_leftPanelRect.y + 36.0f;
	float w = m_leftPanelRect.width - 16.0f;

	// Folder selector & refresh bar
	GuiLabel(Rectangle{ x, y, 50, 22 }, "Folder:");
	std::string folderDisplay = m_scanner.GetCurrentFolder();
	if (folderDisplay.length() > 22)
		folderDisplay = "..." + folderDisplay.substr(folderDisplay.length() - 22);

	GuiTextBox(Rectangle{ x + 48, y, w - 128, 22 }, (char*)folderDisplay.c_str(), 64, false);

	if (GuiButton(Rectangle{ x + w - 76, y, 40, 22 }, "Dir"))
	{
		OpenFileDialog(DIALOG_SELECT_FOLDER);
	}

	if (GuiButton(Rectangle{ x + w - 32, y, 32, 22 }, "#140#"))
	{
		m_scanner.Scan("", m_scanner.IsRecursive());
		ShowStatusMessage("Scanned directory: " + m_scanner.GetCurrentFolder());
	}
	y += 28.0f;

	// Search Filter input
	GuiLabel(Rectangle{ x, y, 45, 22 }, "Filter:");
	if (GuiTextBox(Rectangle{ x + 45, y, w - 45, 22 }, m_searchBuffer, sizeof(m_searchBuffer), true))
	{
		m_scanner.SetSearchFilter(m_searchBuffer);
	}
	y += 28.0f;

	// Mode toggle: Replace vs Append
	bool rec = m_scanner.IsRecursive();
	if (GuiCheckBox(Rectangle{ x, y + 2, 18, 18 }, "Subfolders", &rec))
	{
		m_scanner.SetRecursive(rec);
		m_scanner.Scan("", rec);
	}

	bool appMode = m_appendMode;
	if (GuiToggle(Rectangle{ x + 140, y, w - 140, 22 }, appMode ? "Mode: Append (+)" : "Mode: Replace", &appMode))
	{
		m_appendMode = appMode;
	}
	y += 28.0f;

	// Sub-tabs: Models / Motions / Skeletons
	std::string mTitle = "Models (" + std::to_string(m_scanner.GetModels().size()) + ")";
	std::string motTitle = "Motions (" + std::to_string(m_scanner.GetMotions().size()) + ")";
	std::string sklTitle = "Skel (" + std::to_string(m_scanner.GetSkeletons().size()) + ")";

	float subTabW = w / 3.0f;
	bool tab0 = (m_libraryTab == 0);
	bool tab1 = (m_libraryTab == 1);
	bool tab2 = (m_libraryTab == 2);

	if (GuiToggle(Rectangle{ x, y, subTabW, 24 }, mTitle.c_str(), &tab0)) m_libraryTab = 0;
	if (GuiToggle(Rectangle{ x + subTabW, y, subTabW, 24 }, motTitle.c_str(), &tab1)) m_libraryTab = 1;
	if (GuiToggle(Rectangle{ x + subTabW * 2, y, subTabW, 24 }, sklTitle.c_str(), &tab2)) m_libraryTab = 2;
	y += 28.0f;

	// File List scroll view
	float listHeight = m_leftPanelRect.height - (y - m_leftPanelRect.y) - 10.0f;
	DrawRectangle(x, y, w, listHeight, Color{ 22, 24, 28, 255 });
	DrawRectangleLines(x, y, w, listHeight, Color{ 48, 52, 62, 255 });

	float itemHeight = 24.0f;
	int visibleItems = (int)(listHeight / itemHeight);

	if (m_libraryTab == 0) // Models
	{
		const auto& models = m_scanner.GetModels();
		if (models.empty())
		{
			GuiLabel(Rectangle{ x + 10, y + 10, w - 20, 20 }, "No .o3d models found in folder.");
		}
		else
		{
			int maxScroll = std::max(0, (int)models.size() - visibleItems);
			if (m_libraryScroll > maxScroll) m_libraryScroll = maxScroll;

			// Handle mouse wheel scrolling when over list
			Vector2 mousePos = GetMousePosition();
			if (CheckCollisionPointRec(mousePos, Rectangle{ x, y, w, listHeight }))
			{
				float wheel = GetMouseWheelMove();
				if (wheel != 0)
				{
					m_libraryScroll -= (int)wheel * 2;
					if (m_libraryScroll < 0) m_libraryScroll = 0;
					if (m_libraryScroll > maxScroll) m_libraryScroll = maxScroll;
				}
			}

			for (int i = 0; i < visibleItems && (i + m_libraryScroll) < (int)models.size(); i++)
			{
				int idx = i + m_libraryScroll;
				const auto& item = models[idx];
				float itemY = y + i * itemHeight;

				Rectangle rowRec{ x + 2, itemY + 1, w - 60, itemHeight - 2 };
				std::string label = item.filename;
				if (label.length() > 24) label = label.substr(0, 22) + "..";

				if (GuiButton(rowRec, label.c_str()))
				{
					if (m_appendMode)
					{
						if (scene.AppendModel(item.filepath))
						{
							ShowStatusMessage("Appended model: " + item.filename);
							camera.FrameModel(scene.GetCombinedBounds());
						}
						else
							ShowStatusMessage("Failed to append: " + item.filename, 4.0f, true);
					}
					else
					{
						if (scene.LoadBaseModel(item.filepath))
						{
							ShowStatusMessage("Loaded base model: " + item.filename);
							camera.FrameModel(scene.GetCombinedBounds());
						}
						else
							ShowStatusMessage("Failed to load: " + item.filename, 4.0f, true);
					}
				}

				// Quick "+" append button
				if (GuiButton(Rectangle{ x + w - 54, itemY + 1, 24, itemHeight - 2 }, "+"))
				{
					if (scene.AppendModel(item.filepath))
					{
						ShowStatusMessage("Appended: " + item.filename);
						camera.FrameModel(scene.GetCombinedBounds());
					}
					else
						ShowStatusMessage("Failed to append: " + item.filename, 4.0f, true);
				}

				// Quick "Load" replace button
				if (GuiButton(Rectangle{ x + w - 28, itemY + 1, 26, itemHeight - 2 }, "#198#"))
				{
					if (scene.LoadBaseModel(item.filepath))
					{
						ShowStatusMessage("Loaded: " + item.filename);
						camera.FrameModel(scene.GetCombinedBounds());
					}
					else
						ShowStatusMessage("Failed to load: " + item.filename, 4.0f, true);
				}
			}
		}
	}
	else if (m_libraryTab == 1) // Motions
	{
		const auto& motions = m_scanner.GetMotions();
		if (motions.empty())
		{
			GuiLabel(Rectangle{ x + 10, y + 10, w - 20, 20 }, "No .mot motions found.");
		}
		else
		{
			int maxScroll = std::max(0, (int)motions.size() - visibleItems);
			if (m_libraryScroll > maxScroll) m_libraryScroll = maxScroll;

			Vector2 mousePos = GetMousePosition();
			if (CheckCollisionPointRec(mousePos, Rectangle{ x, y, w, listHeight }))
			{
				float wheel = GetMouseWheelMove();
				if (wheel != 0)
				{
					m_libraryScroll -= (int)wheel * 2;
					if (m_libraryScroll < 0) m_libraryScroll = 0;
					if (m_libraryScroll > maxScroll) m_libraryScroll = maxScroll;
				}
			}

			for (int i = 0; i < visibleItems && (i + m_libraryScroll) < (int)motions.size(); i++)
			{
				int idx = i + m_libraryScroll;
				const auto& item = motions[idx];
				float itemY = y + i * itemHeight;

				std::string label = item.filename;
				if (label.length() > 22) label = label.substr(0, 20) + "..";

				if (GuiButton(Rectangle{ x + 2, itemY + 1, w - 4, itemHeight - 2 }, ("#131# " + label).c_str()))
				{
					if (scene.LoadMotion(item.filepath))
					{
						ShowStatusMessage("Loaded motion: " + item.filename);
					}
					else
					{
						ShowStatusMessage("Failed to load motion: " + item.filename, 4.0f, true);
					}
				}
			}
		}
	}
	else if (m_libraryTab == 2) // Skeletons
	{
		const auto& skeletons = m_scanner.GetSkeletons();
		if (skeletons.empty())
		{
			GuiLabel(Rectangle{ x + 10, y + 10, w - 20, 20 }, "No .bon skeletons found.");
		}
		else
		{
			for (int i = 0; i < visibleItems && i < (int)skeletons.size(); i++)
			{
				const auto& item = skeletons[i];
				float itemY = y + i * itemHeight;

				if (GuiButton(Rectangle{ x + 2, itemY + 1, w - 4, itemHeight - 2 }, item.filename.c_str()))
				{
					if (scene.LoadSkeleton(item.filepath))
						ShowStatusMessage("Loaded skeleton: " + item.filename);
					else
						ShowStatusMessage("Failed to load skeleton: " + item.filename, 4.0f, true);
				}
			}
		}
	}
}

void UserInterface::_drawScenePartsTab(SceneManager& scene, OrbitCamera& camera)
{
	float x = m_leftPanelRect.x + 8.0f;
	float y = m_leftPanelRect.y + 36.0f;
	float w = m_leftPanelRect.width - 16.0f;

	int partCount = scene.GetPartCount();
	GuiLabel(Rectangle{ x, y, w, 20 }, ("Loaded Parts in Scene (" + std::to_string(partCount) + "):").c_str());
	y += 24.0f;

	float listHeight = 180.0f;
	DrawRectangle(x, y, w, listHeight, Color{ 22, 24, 28, 255 });
	DrawRectangleLines(x, y, w, listHeight, Color{ 48, 52, 62, 255 });

	if (partCount == 0)
	{
		GuiLabel(Rectangle{ x + 10, y + 10, w - 20, 20 }, "No models loaded in scene.");
	}
	else
	{
		float itemH = 26.0f;
		for (int i = 0; i < partCount; i++)
		{
			ScenePart* part = scene.GetPart(i);
			if (!part) continue;

			float rowY = y + i * itemH;
			if (rowY + itemH > y + listHeight) break;

			// Highlight active selected part
			bool isActive = (i == scene.GetActivePartIndex());
			if (isActive)
				DrawRectangle((int)x + 2, (int)rowY + 1, (int)w - 4, (int)itemH - 2, Color{ 40, 70, 110, 255 });

			// Visibility checkbox
			bool vis = part->visible;
			if (GuiCheckBox(Rectangle{ x + 4, rowY + 4, 18, 18 }, "", &vis))
			{
				part->visible = vis;
			}

			// Part title
			std::string title = part->isBase ? "[Base] " : "[" + std::to_string(i) + "] ";
			title += part->filename;
			if (title.length() > 22) title = title.substr(0, 20) + "..";

			if (GuiButton(Rectangle{ x + 26, rowY + 2, w - 60, itemH - 4 }, title.c_str()))
			{
				scene.SetActivePartIndex(i);
			}

			// Remove part button (for non-base or all)
			if (GuiButton(Rectangle{ x + w - 30, rowY + 2, 24, itemH - 4 }, "#143#"))
			{
				scene.RemovePart(i);
				break;
			}
		}
	}
	y += listHeight + 8.0f;

	// Selected Part Configuration
	ScenePart* activePart = scene.GetPart(scene.GetActivePartIndex());
	if (activePart)
	{
		DrawRectangle(x, y, w, 150.0f, Color{ 26, 28, 34, 255 });
		DrawRectangleLines(x, y, w, 150.0f, Color{ 48, 52, 62, 255 });

		float py = y + 6.0f;
		GuiLabel(Rectangle{ x + 6, py, w - 12, 18 }, ("Selected: " + activePart->filename).c_str());
		py += 22.0f;

		// LOD Level for this part
		GuiLabel(Rectangle{ x + 6, py, 60, 20 }, "Part LOD:");
		bool lod0 = (activePart->currentLOD == 0);
		bool lod1 = (activePart->currentLOD == 1);
		bool lod2 = (activePart->currentLOD == 2);
		if (GuiToggle(Rectangle{ x + 70, py, 40, 20 }, "0", &lod0)) activePart->currentLOD = 0;
		if (GuiToggle(Rectangle{ x + 115, py, 40, 20 }, "1", &lod1)) activePart->currentLOD = 1;
		if (GuiToggle(Rectangle{ x + 160, py, 40, 20 }, "2", &lod2)) activePart->currentLOD = 2;
		py += 26.0f;

		// Bone Attachment Socket (for accessories, weapons, hats)
		GuiLabel(Rectangle{ x + 6, py, 80, 20 }, "Attachment:");
		CObject3D* baseModel = scene.GetBaseModel();
		int boneCount = baseModel ? baseModel->GetBoneCount() : 0;

		std::string socketName = "Root / Skinned";
		if (activePart->attachBoneID >= 0)
			socketName = "Bone #" + std::to_string(activePart->attachBoneID);

		if (GuiButton(Rectangle{ x + 90, py, w - 100, 22 }, socketName.c_str()))
		{
			// Cycle attachment bone socket
			if (boneCount > 0)
			{
				activePart->attachBoneID++;
				if (activePart->attachBoneID >= boneCount)
					activePart->attachBoneID = -1; // wrap back to root
			}
		}
		py += 28.0f;

		// Quick socket reset button
		if (activePart->attachBoneID >= 0)
		{
			if (GuiButton(Rectangle{ x + 90, py, w - 100, 20 }, "Reset to Root"))
				activePart->attachBoneID = -1;
		}

		y += 158.0f;
	}

	// Action buttons
	if (GuiButton(Rectangle{ x, y, (w - 6) / 2, 26 }, "#201# Append Part"))
	{
		OpenFileDialog(DIALOG_APPEND_MODEL);
	}

	if (GuiButton(Rectangle{ x + (w - 6) / 2 + 6, y, (w - 6) / 2, 26 }, "#143# Clear Parts"))
	{
		scene.ClearAppendedParts();
	}
	y += 30.0f;

	if (GuiButton(Rectangle{ x, y, w, 26 }, "#210# Frame All Parts (F)"))
	{
		camera.FrameModel(scene.GetCombinedBounds());
	}
}

void UserInterface::_drawModelStatsTab(SceneManager& scene, RenderSettings& settings)
{
	float x = m_leftPanelRect.x + 8.0f;
	float y = m_leftPanelRect.y + 36.0f;
	float w = m_leftPanelRect.width - 16.0f;

	CObject3D* model = scene.GetActiveModel();
	if (!model)
	{
		GuiLabel(Rectangle{ x, y, w, 20 }, "No active model loaded.");
		return;
	}

	// General Stats Box
	DrawRectangle(x, y, w, 110.0f, Color{ 22, 24, 28, 255 });
	DrawRectangleLines(x, y, w, 110.0f, Color{ 48, 52, 62, 255 });

	float sy = y + 4.0f;
	GuiLabel(Rectangle{ x + 6, sy, w - 12, 18 }, ("File: " + SceneManager::ExtractFileName(model->GetLoadedFilename())).c_str());
	sy += 18.0f;

	int totalVerts = 0, totalTris = 0, totalSubObjs = 0;
	const LODGroup* grp = model->GetGroup(0);
	if (grp)
	{
		totalSubObjs = grp->objectCount;
		for (int i = 0; i < grp->objectCount; i++)
		{
			totalVerts += grp->objects[i].vertexCount;
			totalTris += grp->objects[i].faceListCount;
		}
	}

	GuiLabel(Rectangle{ x + 6, sy, w - 12, 18 }, ("Sub-Objects: " + std::to_string(totalSubObjs) + " | Bones: " + std::to_string(model->GetBoneCount())).c_str());
	sy += 18.0f;
	GuiLabel(Rectangle{ x + 6, sy, w - 12, 18 }, ("Vertices: " + std::to_string(totalVerts) + " | Triangles: " + std::to_string(totalTris)).c_str());
	sy += 18.0f;

	const Bounds& b = model->GetBounds();
	std::ostringstream bss;
	bss << std::fixed << std::setprecision(1) << "Bounds: [" << b.Min.x << "," << b.Min.y << "," << b.Min.z << "]";
	GuiLabel(Rectangle{ x + 6, sy, w - 12, 18 }, bss.str().c_str());

	y += 118.0f;

	// Sub-Object Hierarchy Tree
	GuiLabel(Rectangle{ x, y, w, 20 }, "Sub-Object Hierarchy:");
	y += 22.0f;

	float treeHeight = m_leftPanelRect.height - (y - m_leftPanelRect.y) - 10.0f;
	DrawRectangle(x, y, w, treeHeight, Color{ 22, 24, 28, 255 });
	DrawRectangleLines(x, y, w, treeHeight, Color{ 48, 52, 62, 255 });

	if (grp && grp->objects)
	{
		float rowH = 22.0f;
		for (int i = 0; i < grp->objectCount; i++)
		{
			float rowY = y + i * rowH;
			if (rowY + rowH > y + treeHeight) break;

			const GMObject& obj = grp->objects[i];
			bool isSel = (settings.selectedObjIndex == i);

			if (isSel)
				DrawRectangle((int)x + 2, (int)rowY + 1, (int)w - 4, (int)rowH - 2, Color{ 40, 80, 130, 255 });

			std::string objTitle = "#" + std::to_string(i) + " (" + (obj.type == GMT_SKIN ? "Skin" : "Mesh") + ")";
			objTitle += " V:" + std::to_string(obj.vertexCount) + " F:" + std::to_string(obj.faceListCount);

			if (GuiButton(Rectangle{ x + 4, rowY + 1, w - 8, rowH - 2 }, objTitle.c_str()))
			{
				settings.selectedObjIndex = (settings.selectedObjIndex == i) ? -1 : i;
				settings.selectedBlockIndex = -1;
			}
		}
	}
}

void UserInterface::_drawRightPanel(SceneManager& scene, RenderSettings& settings)
{
	DrawRectangleRec(m_rightPanelRect, Color{ 32, 35, 42, 255 });
	DrawLine((int)m_rightPanelRect.x, (int)m_rightPanelRect.y,
	         (int)m_rightPanelRect.x, (int)(m_rightPanelRect.y + m_rightPanelRect.height),
	         Color{ 48, 52, 62, 255 });

	// Right Tab header
	const char* rTabNames[] = { "Materials", "Submesh", "Motion" };
	float tabWidth = m_rightPanelRect.width / 3.0f;
	for (int i = 0; i < 3; i++)
	{
		bool active = (m_rightTab == i);
		Rectangle tabRec{ m_rightPanelRect.x + i * tabWidth, m_rightPanelRect.y, tabWidth, 30 };
		if (GuiToggle(tabRec, rTabNames[i], &active))
		{
			m_rightTab = i;
		}
	}

	CObject3D* model = scene.GetActiveModel();
	ScenePart* activePart = scene.GetPart(scene.GetActivePartIndex());
	int currentLOD = activePart ? activePart->currentLOD : 0;

	if (m_rightTab == 0)
		_drawMaterialTab(model, currentLOD, settings);
	else if (m_rightTab == 1)
		_drawSubmeshTab(model, currentLOD, settings);
	else
		_drawAnimationTab(scene);
}

void UserInterface::_drawMaterialTab(CObject3D* model, int currentLOD, RenderSettings& settings)
{
	float x = m_rightPanelRect.x + 8.0f;
	float y = m_rightPanelRect.y + 36.0f;
	float w = m_rightPanelRect.width - 16.0f;

	if (!model)
	{
		GuiLabel(Rectangle{ x, y, w, 20 }, "No active model selected.");
		return;
	}

	LODGroup* grp = model->GetGroup(currentLOD);
	if (!grp || settings.selectedObjIndex < 0 || settings.selectedObjIndex >= grp->objectCount)
	{
		GuiLabel(Rectangle{ x, y, w, 20 }, "Select a sub-object from hierarchy");
		GuiLabel(Rectangle{ x, y + 20, w, 20 }, "to inspect and edit its materials.");

		// Global Texture Scroll Controls
		y += 60.0f;
		GuiGroupBox(Rectangle{ x, y, w, 90 }, "Texture UV Scrolling");
		float u = model->GetScrollU();
		float v = model->GetScrollV();

		GuiLabel(Rectangle{ x + 10, y + 20, 60, 20 }, "Scroll U:");
		if (GuiSlider(Rectangle{ x + 70, y + 20, w - 120, 20 }, "-1.0", "1.0", &u, -1.0f, 1.0f))
			model->SetScrollU(u);

		GuiLabel(Rectangle{ x + 10, y + 50, 60, 20 }, "Scroll V:");
		if (GuiSlider(Rectangle{ x + 70, y + 50, w - 120, 20 }, "-1.0", "1.0", &v, -1.0f, 1.0f))
			model->SetScrollV(v);

		return;
	}

	GMObject& obj = grp->objects[settings.selectedObjIndex];

	// Material Selector
	GuiLabel(Rectangle{ x, y, w, 18 }, ("Submesh #" + std::to_string(settings.selectedObjIndex) + " Materials (" + std::to_string(obj.materialCount) + "):").c_str());
	y += 22.0f;

	int matIdx = 0;
	if (obj.materialBlocks && obj.materialBlockCount > 0)
		matIdx = obj.materialBlocks[0].materialID;

	if (obj.materials && matIdx < obj.materialCount)
	{
		ModelMaterial& mat = obj.materials[matIdx];

		// Ambient Color
		GuiGroupBox(Rectangle{ x, y, w, 80 }, "Ambient Color (RGBA)");
		GuiSlider(Rectangle{ x + 25, y + 16, w - 60, 14 }, "R", "", &mat.material.Ambient.r, 0.0f, 1.0f);
		GuiSlider(Rectangle{ x + 25, y + 34, w - 60, 14 }, "G", "", &mat.material.Ambient.g, 0.0f, 1.0f);
		GuiSlider(Rectangle{ x + 25, y + 52, w - 60, 14 }, "B", "", &mat.material.Ambient.b, 0.0f, 1.0f);
		y += 88.0f;

		// Diffuse Color
		GuiGroupBox(Rectangle{ x, y, w, 80 }, "Diffuse Color (RGBA)");
		GuiSlider(Rectangle{ x + 25, y + 16, w - 60, 14 }, "R", "", &mat.material.Diffuse.r, 0.0f, 1.0f);
		GuiSlider(Rectangle{ x + 25, y + 34, w - 60, 14 }, "G", "", &mat.material.Diffuse.g, 0.0f, 1.0f);
		GuiSlider(Rectangle{ x + 25, y + 52, w - 60, 14 }, "B", "", &mat.material.Diffuse.b, 0.0f, 1.0f);
		y += 88.0f;

		// Texture file edit
		GuiGroupBox(Rectangle{ x, y, w, 68 }, "Texture File");
		if (!m_editingTexture)
		{
			std::strncpy(m_texNameBuffer, mat.textureName, sizeof(m_texNameBuffer) - 1);
		}

		if (GuiTextBox(Rectangle{ x + 10, y + 20, w - 75, 24 }, m_texNameBuffer, sizeof(m_texNameBuffer), true))
		{
			m_editingTexture = true;
		}

		if (GuiButton(Rectangle{ x + w - 60, y + 20, 50, 24 }, "Apply"))
		{
			std::strncpy(mat.textureName, m_texNameBuffer, sizeof(mat.textureName) - 1);
			model->LoadTextures(model->GetLoadedFilename());
			m_editingTexture = false;
			ShowStatusMessage("Updated texture: " + std::string(mat.textureName));
		}
		y += 76.0f;

		// Material Block Effects
		if (obj.materialBlocks && obj.materialBlockCount > 0)
		{
			MaterialBlock& blk = obj.materialBlocks[0];
			GuiGroupBox(Rectangle{ x, y, w, 60 }, "Effects & Blending");

			bool twoSided = (blk.effect & XE_2SIDE) != 0;
			if (GuiCheckBox(Rectangle{ x + 10, y + 18, 18, 18 }, "Two-Sided", &twoSided))
			{
				if (twoSided) blk.effect |= XE_2SIDE;
				else blk.effect &= ~XE_2SIDE;
			}

			bool highlight = (blk.effect & XE_HIGHLIGHT_OBJ) != 0;
			if (GuiCheckBox(Rectangle{ x + 110, y + 18, 18, 18 }, "Additive Glow", &highlight))
			{
				if (highlight) blk.effect |= XE_HIGHLIGHT_OBJ;
				else blk.effect &= ~XE_HIGHLIGHT_OBJ;
			}
		}
	}
}

void UserInterface::_drawSubmeshTab(CObject3D* model, int currentLOD, RenderSettings& settings)
{
	float x = m_rightPanelRect.x + 8.0f;
	float y = m_rightPanelRect.y + 36.0f;
	float w = m_rightPanelRect.width - 16.0f;

	if (!model)
	{
		GuiLabel(Rectangle{ x, y, w, 20 }, "No active model.");
		return;
	}

	LODGroup* grp = model->GetGroup(currentLOD);
	if (!grp || settings.selectedObjIndex < 0 || settings.selectedObjIndex >= grp->objectCount)
	{
		GuiLabel(Rectangle{ x, y, w, 20 }, "Select a submesh to edit properties.");
		return;
	}

	GMObject& obj = grp->objects[settings.selectedObjIndex];

	GuiGroupBox(Rectangle{ x, y, w, 180 }, ("Submesh #" + std::to_string(settings.selectedObjIndex)).c_str());
	float py = y + 20.0f;

	GuiLabel(Rectangle{ x + 10, py, 60, 20 }, "Type:");
	GuiLabel(Rectangle{ x + 80, py, 120, 20 }, (obj.type == GMT_SKIN ? "Skinned Mesh" : "Normal Mesh"));
	py += 24.0f;

	GuiLabel(Rectangle{ x + 10, py, 60, 20 }, "Vertices:");
	GuiLabel(Rectangle{ x + 80, py, 120, 20 }, std::to_string(obj.vertexCount).c_str());
	py += 24.0f;

	GuiLabel(Rectangle{ x + 10, py, 60, 20 }, "Faces:");
	GuiLabel(Rectangle{ x + 80, py, 120, 20 }, std::to_string(obj.faceListCount).c_str());
	py += 24.0f;

	bool rigid = obj.rigid;
	if (GuiCheckBox(Rectangle{ x + 10, py, 18, 18 }, "Rigid Body", &rigid))
		obj.rigid = rigid;
	py += 24.0f;

	bool bump = obj.bump;
	if (GuiCheckBox(Rectangle{ x + 10, py, 18, 18 }, "Bump Mapping", &bump))
		obj.bump = bump;
	py += 30.0f;

	if (GuiButton(Rectangle{ x + 10, py, w - 20, 24 }, "Invert Faces (CW/CCW)"))
	{
		model->ChangeCW(currentLOD, settings.selectedObjIndex);
		ShowStatusMessage("Inverted faces on submesh #" + std::to_string(settings.selectedObjIndex));
	}
}

void UserInterface::_drawAnimationTab(SceneManager& scene)
{
	float x = m_rightPanelRect.x + 8.0f;
	float y = m_rightPanelRect.y + 36.0f;
	float w = m_rightPanelRect.width - 16.0f;

	GuiGroupBox(Rectangle{ x, y, w, 110 }, "Motion & Animation");
	float py = y + 18.0f;

	int totalFrames = scene.GetTotalFrameCount();
	std::string motPath = scene.GetActiveMotionPath();
	std::string motName = motPath.empty() ? "(Internal / None)" : SceneManager::ExtractFileName(motPath);

	GuiLabel(Rectangle{ x + 10, py, w - 20, 18 }, ("Active Motion: " + motName).c_str());
	py += 20.0f;
	GuiLabel(Rectangle{ x + 10, py, w - 20, 18 }, ("Keyframes: " + std::to_string(totalFrames)).c_str());
	py += 22.0f;

	if (GuiButton(Rectangle{ x + 10, py, (w - 26) / 2, 24 }, "#132# Load .mot"))
	{
		OpenFileDialog(DIALOG_OPEN_MOTION);
	}

	if (GuiButton(Rectangle{ x + (w - 26) / 2 + 16, py, (w - 26) / 2, 24 }, "#143# Unload"))
	{
		scene.UnloadMotion();
		ShowStatusMessage("Unloaded active motion.");
	}
	y += 120.0f;

	// Skeleton Bones Inspector
	CObject3D* baseModel = scene.GetBaseModel();
	int boneCount = baseModel ? baseModel->GetBoneCount() : 0;

	GuiLabel(Rectangle{ x, y, w, 20 }, ("Skeleton Bones (" + std::to_string(boneCount) + "):").c_str());
	y += 22.0f;

	float bonesHeight = m_rightPanelRect.height - (y - m_rightPanelRect.y) - 10.0f;
	DrawRectangle(x, y, w, bonesHeight, Color{ 22, 24, 28, 255 });
	DrawRectangleLines(x, y, w, bonesHeight, Color{ 48, 52, 62, 255 });

	if (boneCount == 0)
	{
		GuiLabel(Rectangle{ x + 10, y + 10, w - 20, 20 }, "No skeleton bones in model.");
	}
	else
	{
		const CMotion* mot = baseModel->GetMotion();
		const Bone* boneList = mot ? mot->GetBones() : nullptr;

		float rowH = 20.0f;
		for (int i = 0; i < boneCount; i++)
		{
			float rowY = y + i * rowH;
			if (rowY + rowH > y + bonesHeight) break;

			std::string boneLabel = "Bone #" + std::to_string(i);
			if (boneList && boneList[i].name[0] != '\0')
				boneLabel += " (" + std::string(boneList[i].name) + ")";

			GuiLabel(Rectangle{ x + 6, rowY + 1, w - 12, rowH - 2 }, boneLabel.c_str());
		}
	}
}

void UserInterface::_drawBottomBar(SceneManager& scene, float deltaTime)
{
	DrawRectangleRec(m_bottomBarRect, Color{ 26, 28, 34, 255 });
	DrawLine((int)m_bottomBarRect.x, (int)m_bottomBarRect.y,
	         (int)(m_bottomBarRect.x + m_bottomBarRect.width), (int)m_bottomBarRect.y,
	         Color{ 48, 52, 62, 255 });

	float x = m_bottomBarRect.x + 10.0f;
	float y = m_bottomBarRect.y + 10.0f;
	float h = 28.0f;

	// Rewind to first frame
	if (GuiButton(Rectangle{ x, y, 30, h }, "#129#"))
	{
		scene.SetCurrentFrame(0.0f);
	}
	x += 34.0f;

	// Step backward
	if (GuiButton(Rectangle{ x, y, 30, h }, "#118#"))
	{
		scene.StepFrame(-1);
	}
	x += 34.0f;

	// Play / Pause toggle
	bool isPlaying = scene.IsPlaying();
	const char* playIcon = isPlaying ? "#132#" : "#131#";
	if (GuiButton(Rectangle{ x, y, 36, h }, playIcon))
	{
		scene.TogglePlay();
	}
	x += 40.0f;

	// Step forward
	if (GuiButton(Rectangle{ x, y, 30, h }, "#119#"))
	{
		scene.StepFrame(1);
	}
	x += 36.0f;

	// Timeline Slider
	int totalFrames = scene.GetTotalFrameCount();
	float currentFrame = scene.GetCurrentFrame();

	float sliderWidth = m_bottomBarRect.width - 380.0f;
	if (sliderWidth < 120.0f) sliderWidth = 120.0f;

	float sliderVal = currentFrame;
	if (GuiSlider(Rectangle{ x, y + 4, sliderWidth, 20 }, "0", std::to_string(totalFrames - 1).c_str(),
	              &sliderVal, 0.0f, (float)(totalFrames - 1)))
	{
		scene.SetCurrentFrame(sliderVal);
	}
	x += sliderWidth + 10.0f;

	// Frame counter text
	std::string frameText = std::to_string((int)currentFrame) + " / " + std::to_string(totalFrames - 1);
	GuiLabel(Rectangle{ x, y + 4, 70, 20 }, frameText.c_str());
	x += 74.0f;

	// Playback Speed Slider
	float speed = scene.GetAnimSpeed();
	GuiLabel(Rectangle{ x, y + 4, 40, 20 }, "Spd:");
	if (GuiSlider(Rectangle{ x + 35, y + 4, 60, 20 }, "", "3x", &speed, 0.1f, 3.0f))
	{
		scene.SetAnimSpeed(speed);
	}
	x += 105.0f;

	// Loop checkbox
	bool loop = scene.IsLooping();
	if (GuiCheckBox(Rectangle{ x, y + 4, 18, 18 }, "Loop", &loop))
	{
		scene.SetLooping(loop);
	}
}

void UserInterface::_drawStatusBar(const SceneManager& scene, const OrbitCamera& camera)
{
	DrawRectangleRec(m_statusBarRect, Color{ 22, 24, 28, 255 });
	DrawLine(0, (int)m_statusBarRect.y, GetScreenWidth(), (int)m_statusBarRect.y, Color{ 42, 45, 54, 255 });

	float x = 10.0f;
	float y = m_statusBarRect.y + 3.0f;

	// Status Message
	Color msgCol = m_statusIsError ? Color{ 255, 100, 100, 255 } : Color{ 200, 210, 230, 255 };
	DrawText(m_statusMessage.c_str(), (int)x, (int)y, 11, msgCol);

	// Right-aligned stats
	int fps = GetFPS();
	std::string stats = "Parts: " + std::to_string(scene.GetPartCount()) +
	                    " | Dist: " + std::to_string((int)camera.GetDistance()) +
	                    " | FPS: " + std::to_string(fps);

	int textW = MeasureText(stats.c_str(), 11);
	DrawText(stats.c_str(), GetScreenWidth() - textW - 12, (int)y, 11, Color{ 150, 160, 180, 255 });
}

void UserInterface::_drawFileDialog(SceneManager& scene, OrbitCamera& camera)
{
	float boxW = 540.0f;
	float boxH = 400.0f;
	float boxX = (GetScreenWidth() - boxW) / 2.0f;
	float boxY = (GetScreenHeight() - boxH) / 2.0f;

	// Dim background
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{ 0, 0, 0, 160 });

	// Modal panel
	DrawRectangle(boxX, boxY, boxW, boxH, Color{ 30, 33, 40, 255 });
	DrawRectangleLines(boxX, boxY, boxW, boxH, Color{ 60, 70, 90, 255 });

	const char* title = "Select File";
	if (m_fileDialog.target == DIALOG_OPEN_MODEL) title = "Open Flyff 3D Model (.o3d)";
	else if (m_fileDialog.target == DIALOG_APPEND_MODEL) title = "Append Model to Scene (.o3d)";
	else if (m_fileDialog.target == DIALOG_SAVE_MODEL) title = "Save Flyff 3D Model (.o3d)";
	else if (m_fileDialog.target == DIALOG_OPEN_MOTION) title = "Load Animation Motion (.mot)";
	else if (m_fileDialog.target == DIALOG_OPEN_SKELETON) title = "Load Skeleton (.bon)";
	else if (m_fileDialog.target == DIALOG_SELECT_FOLDER) title = "Select Directory to Scan";

	DrawRectangle(boxX, boxY, boxW, 30, Color{ 40, 45, 58, 255 });
	DrawText(title, boxX + 12, boxY + 8, 12, Color{ 230, 235, 245, 255 });

	// Close 'X' button
	if (GuiButton(Rectangle{ boxX + boxW - 26, boxY + 4, 22, 22 }, "X"))
	{
		m_fileDialog.active = false;
	}

	float px = boxX + 12.0f;
	float py = boxY + 38.0f;
	float pw = boxW - 24.0f;

	// Path header & Up button
	GuiLabel(Rectangle{ px, py, 40, 22 }, "Path:");
	GuiTextBox(Rectangle{ px + 40, py, pw - 85, 22 }, (char*)m_fileDialog.currentPath.c_str(), 256, false);

	if (GuiButton(Rectangle{ px + pw - 40, py, 40, 22 }, "Up"))
	{
		fs::path p(m_fileDialog.currentPath);
		if (p.has_parent_path())
		{
			m_fileDialog.currentPath = p.parent_path().string();
			_refreshDirectoryContents();
		}
	}
	py += 30.0f;

	// Directories and Files List View
	float listW = (pw - 8.0f) / 2.0f;
	float listH = 240.0f;

	GuiLabel(Rectangle{ px, py, listW, 18 }, "Directories:");
	GuiLabel(Rectangle{ px + listW + 8, py, listW, 18 }, "Files:");
	py += 20.0f;

	// Directory List
	DrawRectangle(px, py, listW, listH, Color{ 20, 22, 26, 255 });
	DrawRectangleLines(px, py, listW, listH, Color{ 50, 55, 68, 255 });

	float itemH = 22.0f;
	for (size_t i = 0; i < m_fileDialog.directories.size(); i++)
	{
		float dy = py + i * itemH;
		if (dy + itemH > py + listH) break;

		std::string dName = "[Dir] " + m_fileDialog.directories[i];
		if (GuiButton(Rectangle{ px + 2, dy + 1, listW - 4, itemH - 2 }, dName.c_str()))
		{
			if (m_fileDialog.directories[i] == "..")
			{
				fs::path p(m_fileDialog.currentPath);
				if (p.has_parent_path())
					m_fileDialog.currentPath = p.parent_path().string();
			}
			else
			{
				m_fileDialog.currentPath = (fs::path(m_fileDialog.currentPath) / m_fileDialog.directories[i]).string();
			}
			_refreshDirectoryContents();
			break;
		}
	}

	// File List
	float fx = px + listW + 8.0f;
	DrawRectangle(fx, py, listW, listH, Color{ 20, 22, 26, 255 });
	DrawRectangleLines(fx, py, listW, listH, Color{ 50, 55, 68, 255 });

	for (size_t i = 0; i < m_fileDialog.files.size(); i++)
	{
		float fy = py + i * itemH;
		if (fy + itemH > py + listH) break;

		bool isSel = ((int)i == m_fileDialog.selectedIndex);
		if (isSel)
			DrawRectangle((int)fx + 2, (int)fy + 1, (int)listW - 4, (int)itemH - 2, Color{ 40, 70, 110, 255 });

		if (GuiButton(Rectangle{ fx + 2, fy + 1, listW - 4, itemH - 2 }, m_fileDialog.files[i].c_str()))
		{
			m_fileDialog.selectedIndex = (int)i;
			m_fileDialog.selectedFile = m_fileDialog.files[i];
		}
	}
	py += listH + 10.0f;

	// File Name input (for Save or Open)
	GuiLabel(Rectangle{ px, py, 70, 24 }, "Filename:");
	char fnBuffer[256];
	std::memset(fnBuffer, 0, sizeof(fnBuffer));
	std::strncpy(fnBuffer, m_fileDialog.selectedFile.c_str(), sizeof(fnBuffer) - 1);

	if (GuiTextBox(Rectangle{ px + 75, py, pw - 235, 24 }, fnBuffer, sizeof(fnBuffer), true))
	{
		m_fileDialog.selectedFile = fnBuffer;
	}

	// OK / Select Button
	const char* okLabel = (m_fileDialog.target == DIALOG_SAVE_MODEL) ? "Save" : "Select";
	if (GuiButton(Rectangle{ px + pw - 150, py, 70, 24 }, okLabel))
	{
		std::string finalPath;
		if (m_fileDialog.target == DIALOG_SELECT_FOLDER)
		{
			finalPath = m_fileDialog.currentPath;
			m_scanner.SetCurrentFolder(finalPath);
			ShowStatusMessage("Selected folder: " + finalPath);
		}
		else if (!m_fileDialog.selectedFile.empty())
		{
			finalPath = (fs::path(m_fileDialog.currentPath) / m_fileDialog.selectedFile).string();

			if (m_fileDialog.target == DIALOG_OPEN_MODEL)
			{
				if (scene.LoadBaseModel(finalPath))
				{
					ShowStatusMessage("Loaded model: " + m_fileDialog.selectedFile);
					camera.FrameModel(scene.GetCombinedBounds());
				}
				else
					ShowStatusMessage("Failed to load: " + m_fileDialog.selectedFile, 4.0f, true);
			}
			else if (m_fileDialog.target == DIALOG_APPEND_MODEL)
			{
				if (scene.AppendModel(finalPath))
				{
					ShowStatusMessage("Appended model: " + m_fileDialog.selectedFile);
					camera.FrameModel(scene.GetCombinedBounds());
				}
				else
					ShowStatusMessage("Failed to append: " + m_fileDialog.selectedFile, 4.0f, true);
			}
			else if (m_fileDialog.target == DIALOG_OPEN_MOTION)
			{
				if (scene.LoadMotion(finalPath))
					ShowStatusMessage("Loaded motion: " + m_fileDialog.selectedFile);
				else
					ShowStatusMessage("Failed to load motion: " + m_fileDialog.selectedFile, 4.0f, true);
			}
			else if (m_fileDialog.target == DIALOG_OPEN_SKELETON)
			{
				if (scene.LoadSkeleton(finalPath))
					ShowStatusMessage("Loaded skeleton: " + m_fileDialog.selectedFile);
				else
					ShowStatusMessage("Failed to load skeleton: " + m_fileDialog.selectedFile, 4.0f, true);
			}
			else if (m_fileDialog.target == DIALOG_SAVE_MODEL)
			{
				CObject3D* activeModel = scene.GetActiveModel();
				if (activeModel)
				{
					if (activeModel->Save(finalPath))
						ShowStatusMessage("Saved model: " + m_fileDialog.selectedFile);
					else
						ShowStatusMessage("Failed to save: " + m_fileDialog.selectedFile, 4.0f, true);
				}
			}
		}

		m_fileDialog.active = false;
	}

	// Cancel Button
	if (GuiButton(Rectangle{ px + pw - 70, py, 70, 24 }, "Cancel"))
	{
		m_fileDialog.active = false;
	}
}
