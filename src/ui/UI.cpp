#include "UI.h"
#include "raymath.h"
#include "../core/File.h"
#include "../model/TextureMng.h"
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstdio>
#include <iostream>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Helper function to check if a file is an .o3d file
static bool IsO3DFile(const std::string& filename)
{
	std::string ext = GetFileExtension(filename);
	std::string extLower = ext;
	std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);
	return extLower == "o3d";
}

UserInterface::UserInterface()
	: m_statusMessage("Ready. Flyff .o3d Model Viewer & Editor"),
	  m_statusTimer(5.0f),
	  m_statusIsError(false),
	  m_leftTab(0),
	  m_rightTab(0),
	  m_editingTexture(false)
{
	std::memset(m_texNameBuffer, 0, sizeof(m_texNameBuffer));

	// Initialize raygui default style
	GuiSetStyle(DEFAULT, TEXT_SIZE, 13);
	GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0x22252cff);
	GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x2e323bff);
	GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, 0x3d424eff);
	GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, 0x1f2228ff);
	GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xe0e0e0ff);
	GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, 0xffffffff);
	GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, 0x80c0ffff);
	GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, 0x4a4f5cff);
	GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, 0x6080a0ff);
	GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, 0x3a6090ff);
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

void UserInterface::_updateLayout(int screenWidth, int screenHeight, bool hasAnimation)
{
	float topBarHeight = 38.0f;
	float statusBarHeight = 26.0f;
	float bottomBarHeight = hasAnimation ? 48.0f : 0.0f;
	float leftPanelWidth = 270.0f;
	float rightPanelWidth = 290.0f;

	m_topBarRect = Rectangle{ 0, 0, (float)screenWidth, topBarHeight };
	m_statusBarRect = Rectangle{ 0, (float)screenHeight - statusBarHeight, (float)screenWidth, statusBarHeight };

	float mainHeight = (float)screenHeight - topBarHeight - statusBarHeight - bottomBarHeight;

	m_leftPanelRect = Rectangle{ 0, topBarHeight, leftPanelWidth, mainHeight + bottomBarHeight };
	m_rightPanelRect = Rectangle{ (float)screenWidth - rightPanelWidth, topBarHeight, rightPanelWidth, mainHeight + bottomBarHeight };

	float viewportX = leftPanelWidth;
	float viewportWidth = (float)screenWidth - leftPanelWidth - rightPanelWidth;

	m_viewportRect = Rectangle{ viewportX, topBarHeight, viewportWidth, mainHeight };
	m_bottomBarRect = Rectangle{ viewportX, topBarHeight + mainHeight, viewportWidth, bottomBarHeight };
}

void UserInterface::UpdateAndDraw(CObject3D*& model, int& currentLOD, OrbitCamera& camera,
                                  RenderSettings& settings, float& animSpeed, bool& isPlaying,
                                  float deltaTime)
{
	int screenWidth = GetScreenWidth();
	int screenHeight = GetScreenHeight();

	bool hasAnimation = (model != nullptr && model->GetFrameCount() > 0);
	_updateLayout(screenWidth, screenHeight, hasAnimation);

	// Update status message timer
	if (m_statusTimer > 0.0f)
	{
		m_statusTimer -= deltaTime;
		if (m_statusTimer <= 0.0f)
			m_statusMessage = "Ready.";
	}

	// 1. Draw Top Toolbar
	_drawTopBar(model, currentLOD, camera, settings);

	// 2. Draw Left Panel (Model Info & Hierarchy)
	_drawLeftPanel(model, currentLOD, settings);

	// 3. Draw Right Panel (Inspector & Editing Tools)
	_drawRightPanel(model, currentLOD, settings);

	// 4. Draw Bottom Animation Bar (if animated)
	if (hasAnimation)
		_drawBottomBar(model, currentLOD, animSpeed, isPlaying, deltaTime);

	// 5. Draw Status Bar
	_drawStatusBar();

	// 6. Draw Modal File Dialog (if open)
	if (m_fileDialog.active)
		_drawFileDialog();
}

void UserInterface::_drawTopBar(CObject3D*& model, int& currentLOD, OrbitCamera& camera,
                                RenderSettings& settings)
{
	DrawRectangleRec(m_topBarRect, Color{ 30, 33, 40, 255 });
	DrawLine(0, (int)m_topBarRect.height, GetScreenWidth(), (int)m_topBarRect.height, Color{ 55, 60, 72, 255 });

	float x = 10.0f;
	float y = 5.0f;
	float btnH = 28.0f;

	// Open Button
	if (GuiButton(Rectangle{ x, y, 75, btnH }, "#001#Open..."))
	{
		OpenFileDialog(false);
	}
	x += 80.0f;

	// Save Button
	if (GuiButton(Rectangle{ x, y, 65, btnH }, "#002#Save"))
	{
		if (model)
		{
			std::string fn = model->GetLoadedFilename();
			if (!fn.empty())
			{
				if (model->Save(fn))
					ShowStatusMessage("Model saved successfully: " + GetFileName(fn), 4.0f, false);
				else
					ShowStatusMessage("Error saving model: " + GetFileName(fn), 4.0f, true);
			}
			else
			{
				OpenFileDialog(true);
			}
		}
	}
	x += 70.0f;

	// Save As Button
	if (GuiButton(Rectangle{ x, y, 80, btnH }, "#003#Save As..."))
	{
		if (model)
			OpenFileDialog(true);
	}
	x += 90.0f;

	// Separator
	DrawLine((int)x, 6, (int)x, 32, Color{ 60, 65, 78, 255 });
	x += 10.0f;

	// Reset Camera / Frame Model Button
	if (GuiButton(Rectangle{ x, y, 90, btnH }, "#028#Frame [R]"))
	{
		if (model)
			camera.FrameModel(model->GetBounds());
		else
			camera.Reset();
	}
	x += 98.0f;

	// Separator
	DrawLine((int)x, 6, (int)x, 32, Color{ 60, 65, 78, 255 });
	x += 10.0f;

	// Fill Mode: Solid / Wire / Points
	int fillModeInt = (int)settings.fillMode;
	if (GuiToggleGroup(Rectangle{ x, y, 55, btnH }, "Solid;Wire;Point", &fillModeInt))
	{
		settings.fillMode = (EFillMode)fillModeInt;
	}
	x += 175.0f;

	// Overlays: Grid, Axes, Bounds, Collision, Bones
	GuiToggle(Rectangle{ x, y, 48, btnH }, "Grid", &settings.showGrid);
	x += 52.0f;

	GuiToggle(Rectangle{ x, y, 48, btnH }, "Axes", &settings.showAxes);
	x += 52.0f;

	GuiToggle(Rectangle{ x, y, 58, btnH }, "Bounds", &settings.showBounds);
	x += 62.0f;

	GuiToggle(Rectangle{ x, y, 45, btnH }, "Coll", &settings.showCollision);
	x += 49.0f;

	GuiToggle(Rectangle{ x, y, 52, btnH }, "Bones", &settings.showBones);
	x += 62.0f;

	// Separator
	DrawLine((int)x, 6, (int)x, 32, Color{ 60, 65, 78, 255 });
	x += 10.0f;

	// LOD Selector (if model has LOD)
	if (model && model->HasLOD())
	{
		DrawText("LOD:", (int)x, (int)y + 6, 12, Color{ 180, 185, 200, 255 });
		x += 35.0f;

		int lodChoice = currentLOD;
		if (GuiToggleGroup(Rectangle{ x, y, 32, btnH }, "0;1;2", &lodChoice))
		{
			currentLOD = lodChoice;
		}
	}
}

void UserInterface::_drawLeftPanel(const CObject3D* model, int currentLOD, RenderSettings& settings)
{
	DrawRectangleRec(m_leftPanelRect, Color{ 35, 38, 46, 255 });
	DrawLine((int)m_leftPanelRect.width, (int)m_leftPanelRect.y,
	         (int)m_leftPanelRect.width, (int)(m_leftPanelRect.y + m_leftPanelRect.height),
	         Color{ 55, 60, 72, 255 });

	float x = m_leftPanelRect.x + 8.0f;
	float y = m_leftPanelRect.y + 6.0f;
	float w = m_leftPanelRect.width - 16.0f;

	// Tab bar: Model Info / Sub-Objects
	int tab = m_leftTab;
	if (GuiToggleGroup(Rectangle{ x, y, w / 2.0f, 24 }, "Model Info;Sub-Objects", &tab))
		m_leftTab = tab;
	y += 30.0f;

	if (m_leftTab == 0) // Model Info Tab
	{
		DrawRectangle(x, y, w, 22, Color{ 48, 52, 64, 255 });
		DrawText("METADATA & GEOMETRY", x + 6, y + 5, 11, Color{ 200, 210, 230, 255 });
		y += 28.0f;

		if (model)
		{
			std::string fname = GetFileName(model->GetLoadedFilename());
			if (fname.empty()) fname = "(Untitled .o3d)";

			DrawText(TextFormat("File: %s", fname.c_str()), x + 4, y, 11, Color{ 220, 220, 220, 255 });
			y += 18.0f;

			DrawText(TextFormat("Model ID: %d", model->GetID()), x + 4, y, 11, Color{ 200, 200, 200, 255 });
			y += 18.0f;

			const LODGroup* group = model->GetGroup(currentLOD);
			int totalVerts = 0;
			int totalFaces = 0;
			int totalMatBlocks = 0;
			int objCount = group ? group->objectCount : 0;

			if (group)
			{
				for (int i = 0; i < group->objectCount; i++)
				{
					totalVerts += group->objects[i].vertexCount;
					totalFaces += group->objects[i].faceListCount;
					totalMatBlocks += group->objects[i].materialBlockCount;
				}
			}

			DrawText(TextFormat("Sub-Objects: %d", objCount), x + 4, y, 11, Color{ 180, 220, 255, 255 });
			y += 18.0f;
			DrawText(TextFormat("Total Vertices: %d", totalVerts), x + 4, y, 11, Color{ 180, 220, 255, 255 });
			y += 18.0f;
			DrawText(TextFormat("Total Triangles: %d", totalFaces), x + 4, y, 11, Color{ 180, 220, 255, 255 });
			y += 18.0f;
			DrawText(TextFormat("Material Blocks: %d", totalMatBlocks), x + 4, y, 11, Color{ 180, 220, 255, 255 });
			y += 18.0f;
			DrawText(TextFormat("Bones: %d", model->GetBoneCount()), x + 4, y, 11, Color{ 180, 220, 255, 255 });
			y += 18.0f;
			DrawText(TextFormat("Anim Frames: %d", model->GetFrameCount()), x + 4, y, 11, Color{ 180, 220, 255, 255 });
			y += 18.0f;
			DrawText(TextFormat("Has LOD: %s", model->HasLOD() ? "Yes (3 levels)" : "No"), x + 4, y, 11, Color{ 180, 220, 255, 255 });
			y += 18.0f;
			DrawText(TextFormat("Collision: %s", model->GetCollisionObj() ? "Present" : "None"), x + 4, y, 11, Color{ 180, 220, 255, 255 });
			y += 24.0f;

			// Bounding Box info
			DrawRectangle(x, y, w, 22, Color{ 48, 52, 64, 255 });
			DrawText("BOUNDING BOX", x + 6, y + 5, 11, Color{ 200, 210, 230, 255 });
			y += 28.0f;

			const Bounds& b = model->GetBounds();
			DrawText(TextFormat("Min: (%.2f, %.2f, %.2f)", b.Min.x, b.Min.y, b.Min.z), x + 4, y, 11, Color{ 190, 190, 190, 255 });
			y += 18.0f;
			DrawText(TextFormat("Max: (%.2f, %.2f, %.2f)", b.Max.x, b.Max.y, b.Max.z), x + 4, y, 11, Color{ 190, 190, 190, 255 });
			y += 18.0f;
			float dx = b.Max.x - b.Min.x;
			float dy = b.Max.y - b.Min.y;
			float dz = b.Max.z - b.Min.z;
			DrawText(TextFormat("Size: %.2f x %.2f x %.2f", dx, dy, dz), x + 4, y, 11, Color{ 120, 220, 180, 255 });
			y += 24.0f;

			// UV Scroll info
			DrawRectangle(x, y, w, 22, Color{ 48, 52, 64, 255 });
			DrawText("UV SCROLL SPEEDS", x + 6, y + 5, 11, Color{ 200, 210, 230, 255 });
			y += 28.0f;
			DrawText(TextFormat("Scroll U: %.4f | Scroll V: %.4f", model->GetScrollU(), model->GetScrollV()), x + 4, y, 11, Color{ 190, 190, 190, 255 });
		}
		else
		{
			DrawText("No model loaded.", x + 4, y, 12, Color{ 150, 150, 150, 255 });
			y += 20.0f;
			DrawText("Click 'Open...' to load a Flyff", x + 4, y, 11, Color{ 120, 120, 120, 255 });
			y += 16.0f;
			DrawText(".o3d 3D model file.", x + 4, y, 11, Color{ 120, 120, 120, 255 });
		}
	}
	else // Sub-Objects (GMObjects) Tab
	{
		DrawRectangle(x, y, w, 22, Color{ 48, 52, 64, 255 });
		DrawText("SUB-OBJECTS HIERARCHY", x + 6, y + 5, 11, Color{ 200, 210, 230, 255 });
		y += 28.0f;

		if (model)
		{
			const LODGroup* group = model->GetGroup(currentLOD);
			if (group && group->objects && group->objectCount > 0)
			{
				for (int i = 0; i < group->objectCount; i++)
				{
					const GMObject& obj = group->objects[i];
					bool isSel = (settings.selectedObjIndex == i);

					Rectangle itemRect = { x, y, w, 40 };
					if (isSel)
						DrawRectangleRec(itemRect, Color{ 50, 80, 120, 255 });
					else
						DrawRectangleRec(itemRect, Color{ 40, 44, 54, 255 });

					DrawRectangleLinesEx(itemRect, 1, isSel ? Color{ 80, 140, 220, 255 } : Color{ 50, 55, 68, 255 });

					const char* typeStr = (obj.type == GMT_SKIN) ? "Skin" : (obj.type == GMT_BONE ? "Bone" : "Normal");
					DrawText(TextFormat("[%d] GMObject (%s)", i, typeStr), x + 6, y + 4, 11, isSel ? WHITE : Color{ 220, 220, 220, 255 });
					DrawText(TextFormat("Verts: %d | Tris: %d | Blocks: %d", obj.vertexCount, obj.faceListCount, obj.materialBlockCount),
					         x + 6, y + 20, 10, Color{ 160, 170, 185, 255 });

					if (CheckCollisionPointRec(GetMousePosition(), itemRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
					{
						settings.selectedObjIndex = i;
						settings.selectedBlockIndex = 0;
					}

					y += 44.0f;
					if (y > m_leftPanelRect.y + m_leftPanelRect.height - 40)
						break;
				}
			}
			else
			{
				DrawText("No sub-objects in this LOD.", x + 4, y, 11, Color{ 150, 150, 150, 255 });
			}
		}
		else
		{
			DrawText("No model loaded.", x + 4, y, 11, Color{ 150, 150, 150, 255 });
		}
	}
}

void UserInterface::_drawRightPanel(CObject3D* model, int currentLOD, RenderSettings& settings)
{
	DrawRectangleRec(m_rightPanelRect, Color{ 35, 38, 46, 255 });
	DrawLine((int)m_rightPanelRect.x, (int)m_rightPanelRect.y,
	         (int)m_rightPanelRect.x, (int)(m_rightPanelRect.y + m_rightPanelRect.height),
	         Color{ 55, 60, 72, 255 });

	float x = m_rightPanelRect.x + 8.0f;
	float y = m_rightPanelRect.y + 6.0f;
	float w = m_rightPanelRect.width - 16.0f;

	// Tabs: Material Inspector / Model Tools / View
	int tab = m_rightTab;
	if (GuiToggleGroup(Rectangle{ x, y, w / 3.0f, 24 }, "Inspector;Tools;View", &tab))
		m_rightTab = tab;
	y += 30.0f;

	if (m_rightTab == 0) // Material & Effects Inspector
	{
		DrawRectangle(x, y, w, 22, Color{ 48, 52, 64, 255 });
		DrawText("MATERIAL & EFFECTS INSPECTOR", x + 6, y + 5, 11, Color{ 200, 210, 230, 255 });
		y += 28.0f;

		if (model)
		{
			LODGroup* group = model->GetGroup(currentLOD);
			if (group && group->objectCount > 0)
			{
				if (settings.selectedObjIndex < 0 || settings.selectedObjIndex >= group->objectCount)
					settings.selectedObjIndex = 0;

				GMObject& selObj = group->objects[settings.selectedObjIndex];

				// Object Selector
				DrawText(TextFormat("Object (%d / %d):", settings.selectedObjIndex + 1, group->objectCount), x + 4, y + 4, 11, Color{ 200, 200, 200, 255 });
				if (GuiButton(Rectangle{ x + 150, y, 32, 22 }, "<"))
				{
					if (settings.selectedObjIndex > 0)
					{
						settings.selectedObjIndex--;
						settings.selectedBlockIndex = 0;
					}
				}
				if (GuiButton(Rectangle{ x + 186, y, 32, 22 }, ">"))
				{
					if (settings.selectedObjIndex < group->objectCount - 1)
					{
						settings.selectedObjIndex++;
						settings.selectedBlockIndex = 0;
					}
				}
				y += 28.0f;

				// Material Block Selector
				if (selObj.materialBlockCount > 0)
				{
					if (settings.selectedBlockIndex < 0 || settings.selectedBlockIndex >= selObj.materialBlockCount)
						settings.selectedBlockIndex = 0;

					MaterialBlock& block = selObj.materialBlocks[settings.selectedBlockIndex];

					DrawText(TextFormat("Block (%d / %d):", settings.selectedBlockIndex + 1, selObj.materialBlockCount), x + 4, y + 4, 11, Color{ 200, 200, 200, 255 });
					if (GuiButton(Rectangle{ x + 150, y, 32, 22 }, "<"))
					{
						if (settings.selectedBlockIndex > 0)
							settings.selectedBlockIndex--;
					}
					if (GuiButton(Rectangle{ x + 186, y, 32, 22 }, ">"))
					{
						if (settings.selectedBlockIndex < selObj.materialBlockCount - 1)
							settings.selectedBlockIndex++;
					}
					y += 30.0f;

					DrawText(TextFormat("Triangles in Block: %d", block.primitiveCount), x + 4, y, 10, Color{ 160, 180, 200, 255 });
					y += 18.0f;

					// Effects Checkboxes
					DrawRectangle(x, y, w, 20, Color{ 42, 46, 56, 255 });
					DrawText("Effect Flags", x + 6, y + 4, 10, Color{ 180, 190, 210, 255 });
					y += 24.0f;

					// 2-Sided Toggle
					bool is2Side = (block.effect & XE_2SIDE) != 0;
					if (GuiCheckBox(Rectangle{ x + 6, y, 18, 18 }, "2-Sided Rendering (XE_2SIDE)", &is2Side))
					{
						if (is2Side) block.effect |= XE_2SIDE;
						else block.effect &= ~XE_2SIDE;
					}
					y += 22.0f;

					// Opacity Toggle
					bool isOpacity = (block.effect & XE_OPACITY) != 0;
					if (GuiCheckBox(Rectangle{ x + 6, y, 18, 18 }, "Alpha Opacity (XE_OPACITY)", &isOpacity))
					{
						if (isOpacity)
						{
							block.effect |= XE_OPACITY;
							if (block.amount == 0) block.amount = 255;
						}
						else block.effect &= ~XE_OPACITY;
					}
					y += 22.0f;

					// Opacity Amount Slider
					if (isOpacity)
					{
						float amountVal = (float)block.amount;
						DrawText(TextFormat("Alpha Amount: %d", block.amount), x + 24, y, 10, Color{ 200, 200, 200, 255 });
						y += 14.0f;
						if (GuiSlider(Rectangle{ x + 24, y, w - 30, 16 }, "0", "255", &amountVal, 0, 255))
						{
							block.amount = (int)amountVal;
						}
						y += 22.0f;
					}

					// Reflect Toggle
					bool isReflect = (block.effect & XE_REFLECT) != 0;
					if (GuiCheckBox(Rectangle{ x + 6, y, 18, 18 }, "Reflection (XE_REFLECT)", &isReflect))
					{
						if (isReflect) block.effect |= XE_REFLECT;
						else block.effect &= ~XE_REFLECT;
					}
					y += 22.0f;

					// Self-Illuminate Toggle
					bool isSelfIllum = (block.effect & XE_SELF_ILLUMINATE) != 0;
					if (GuiCheckBox(Rectangle{ x + 6, y, 18, 18 }, "Self-Illuminate (XE_SELF_ILLUM)", &isSelfIllum))
					{
						if (isSelfIllum) block.effect |= XE_SELF_ILLUMINATE;
						else block.effect &= ~XE_SELF_ILLUMINATE;
					}
					y += 22.0f;

					// Highlight Toggle
					bool isHighlight = (block.effect & XE_HIGHLIGHT_OBJ) != 0;
					if (GuiCheckBox(Rectangle{ x + 6, y, 18, 18 }, "Highlight / Additive (XE_HIGHLIGHT)", &isHighlight))
					{
						if (isHighlight) block.effect |= XE_HIGHLIGHT_OBJ;
						else block.effect &= ~XE_HIGHLIGHT_OBJ;
					}
					y += 28.0f;

					// Texture Assignment
					DrawRectangle(x, y, w, 20, Color{ 42, 46, 56, 255 });
					DrawText("Texture Assignment", x + 6, y + 4, 10, Color{ 180, 190, 210, 255 });
					y += 24.0f;

					if (selObj.material && selObj.materials && block.materialID < selObj.materialCount)
					{
						ModelMaterial& mat = selObj.materials[block.materialID];
						DrawText(TextFormat("Current: %s", (mat.textureName[0] ? mat.textureName : "(none)")), x + 6, y, 11, Color{ 220, 220, 120, 255 });
						y += 20.0f;

						if (!m_editingTexture)
						{
							if (GuiButton(Rectangle{ x + 6, y, 120, 24 }, "Edit Texture Name"))
							{
								m_editingTexture = true;
								std::strncpy(m_texNameBuffer, mat.textureName, sizeof(m_texNameBuffer) - 1);
							}
						}
						else
						{
							GuiTextBox(Rectangle{ x + 6, y, w - 80, 24 }, m_texNameBuffer, sizeof(m_texNameBuffer), true);
							if (GuiButton(Rectangle{ x + w - 70, y, 65, 24 }, "Apply"))
							{
								std::strncpy(mat.textureName, m_texNameBuffer, sizeof(mat.textureName) - 1);
								model->LoadTextures(model->GetLoadedFilename());
								m_editingTexture = false;
								ShowStatusMessage("Texture name updated: " + std::string(mat.textureName));
							}
						}
					}
				}
				else
				{
					DrawText("No material blocks on this object.", x + 4, y, 11, Color{ 150, 150, 150, 255 });
				}
			}
		}
		else
		{
			DrawText("No model loaded.", x + 4, y, 11, Color{ 150, 150, 150, 255 });
		}
	}
	else if (m_rightTab == 1) // Model Tools Tab
	{
		DrawRectangle(x, y, w, 22, Color{ 48, 52, 64, 255 });
		DrawText("MODEL EDITING TOOLS", x + 6, y + 5, 11, Color{ 200, 210, 230, 255 });
		y += 30.0f;

		if (model)
		{
			// Flip Winding Button
			if (GuiButton(Rectangle{ x + 4, y, w - 8, 28 }, "#021#Flip Winding (CW / CCW)"))
			{
				model->ChangeCW(currentLOD, settings.selectedObjIndex);
				ShowStatusMessage("Triangle winding flipped (CW/CCW)");
			}
			y += 34.0f;

			// Flip All Objects Winding
			if (GuiButton(Rectangle{ x + 4, y, w - 8, 28 }, "Flip ALL Objects Winding"))
			{
				model->ChangeCW(-1, -1);
				ShowStatusMessage("All objects triangle winding flipped");
			}
			y += 38.0f;

			// Auto Collision Button
			if (GuiButton(Rectangle{ x + 4, y, w - 8, 28 }, "#023#Auto Generate Collision"))
			{
				model->CollisionAuto(currentLOD);
				settings.showCollision = true;
				ShowStatusMessage("Collision mesh auto-generated successfully");
			}
			y += 38.0f;

			// Recalculate Bounds Button
			if (GuiButton(Rectangle{ x + 4, y, w - 8, 28 }, "#024#Recalculate Bounds"))
			{
				model->RecalculateBounds();
				ShowStatusMessage("Model bounds recalculated");
			}
			y += 42.0f;

			// Model ID Editor
			DrawRectangle(x, y, w, 20, Color{ 42, 46, 56, 255 });
			DrawText("Model Properties", x + 6, y + 4, 10, Color{ 180, 190, 210, 255 });
			y += 24.0f;

			int id = model->GetID();
			DrawText(TextFormat("Model ID: %d", id), x + 6, y + 4, 11, Color{ 220, 220, 220, 255 });
			if (GuiButton(Rectangle{ x + 150, y, 30, 20 }, "-"))
			{
				model->SetID(id - 1);
			}
			if (GuiButton(Rectangle{ x + 184, y, 30, 20 }, "+"))
			{
				model->SetID(id + 1);
			}
			y += 28.0f;

			// Scroll U Editor
			float scrlU = model->GetScrollU();
			DrawText(TextFormat("Scroll U: %.3f", scrlU), x + 6, y + 4, 11, Color{ 220, 220, 220, 255 });
			if (GuiButton(Rectangle{ x + 120, y, 30, 20 }, "-0.01"))
			{
				model->SetScrollU(scrlU - 0.01f);
			}
			if (GuiButton(Rectangle{ x + 154, y, 30, 20 }, "+0.01"))
			{
				model->SetScrollU(scrlU + 0.01f);
			}
			if (GuiButton(Rectangle{ x + 188, y, 26, 20 }, "0"))
			{
				model->SetScrollU(0.0f);
			}
			y += 28.0f;

			// Scroll V Editor
			float scrlV = model->GetScrollV();
			DrawText(TextFormat("Scroll V: %.3f", scrlV), x + 6, y + 4, 11, Color{ 220, 220, 220, 255 });
			if (GuiButton(Rectangle{ x + 120, y, 30, 20 }, "-0.01"))
			{
				model->SetScrollV(scrlV - 0.01f);
			}
			if (GuiButton(Rectangle{ x + 154, y, 30, 20 }, "+0.01"))
			{
				model->SetScrollV(scrlV + 0.01f);
			}
			if (GuiButton(Rectangle{ x + 188, y, 26, 20 }, "0"))
			{
				model->SetScrollV(0.0f);
			}
		}
		else
		{
			DrawText("No model loaded.", x + 4, y, 11, Color{ 150, 150, 150, 255 });
		}
	}
	else // View & Environment Tab
	{
		DrawRectangle(x, y, w, 22, Color{ 48, 52, 64, 255 });
		DrawText("VIEWPORT & ENVIRONMENT", x + 6, y + 5, 11, Color{ 200, 210, 230, 255 });
		y += 30.0f;

		// Lighting toggle
		GuiCheckBox(Rectangle{ x + 6, y, 18, 18 }, "Enable Shading / Lighting", &settings.enableLighting);
		y += 24.0f;

		// Textures toggle
		GuiCheckBox(Rectangle{ x + 6, y, 18, 18 }, "Enable Textures", &settings.enableTextures);
		y += 30.0f;

		// Background Color Presets
		DrawRectangle(x, y, w, 20, Color{ 42, 46, 56, 255 });
		DrawText("Background Color Presets", x + 6, y + 4, 10, Color{ 180, 190, 210, 255 });
		y += 26.0f;

		if (GuiButton(Rectangle{ x + 6, y, w - 12, 24 }, "Dark Slate (Default)"))
			settings.backgroundColor = Color{ 45, 48, 55, 255 };
		y += 28.0f;

		if (GuiButton(Rectangle{ x + 6, y, w - 12, 24 }, "Deep Navy"))
			settings.backgroundColor = Color{ 20, 28, 45, 255 };
		y += 28.0f;

		if (GuiButton(Rectangle{ x + 6, y, w - 12, 24 }, "Neutral Gray"))
			settings.backgroundColor = Color{ 140, 145, 155, 255 };
		y += 28.0f;

		if (GuiButton(Rectangle{ x + 6, y, w - 12, 24 }, "Pure Black"))
			settings.backgroundColor = Color{ 10, 10, 12, 255 };
		y += 28.0f;

		if (GuiButton(Rectangle{ x + 6, y, w - 12, 24 }, "Flyff Sky Blue"))
			settings.backgroundColor = Color{ 90, 140, 195, 255 };
	}
}

void UserInterface::_drawBottomBar(CObject3D* model, int currentLOD, float& animSpeed,
                                   bool& isPlaying, float deltaTime)
{
	DrawRectangleRec(m_bottomBarRect, Color{ 32, 35, 42, 255 });
	DrawLine((int)m_bottomBarRect.x, (int)m_bottomBarRect.y,
	         (int)(m_bottomBarRect.x + m_bottomBarRect.width), (int)m_bottomBarRect.y,
	         Color{ 55, 60, 72, 255 });

	int frameCount = model ? model->GetFrameCount() : 0;
	if (frameCount <= 0) return;

	static float animFrame = 0.0f;

	if (isPlaying)
	{
		animFrame += deltaTime * 30.0f * animSpeed;
		if (animFrame >= (float)frameCount)
			animFrame = std::fmod(animFrame, (float)frameCount);
	}

	int currFrame = (int)animFrame;
	int nextFrame = (currFrame + 1) % frameCount;

	if (model)
		model->Animate(animFrame, nextFrame, currentLOD);

	float x = m_bottomBarRect.x + 10.0f;
	float y = m_bottomBarRect.y + 10.0f;

	// Play/Pause button
	if (GuiButton(Rectangle{ x, y, 32, 28 }, isPlaying ? "#132#" : "#131#"))
	{
		isPlaying = !isPlaying;
	}
	x += 38.0f;

	// Stop button
	if (GuiButton(Rectangle{ x, y, 32, 28 }, "#133#"))
	{
		isPlaying = false;
		animFrame = 0.0f;
	}
	x += 38.0f;

	// Step Back button
	if (GuiButton(Rectangle{ x, y, 28, 28 }, "<"))
	{
		animFrame = (currFrame > 0) ? (float)(currFrame - 1) : (float)(frameCount - 1);
		isPlaying = false;
	}
	x += 32.0f;

	// Step Forward button
	if (GuiButton(Rectangle{ x, y, 28, 28 }, ">"))
	{
		animFrame = (currFrame + 1 >= frameCount) ? 0.0f : (float)(currFrame + 1);
		isPlaying = false;
	}
	x += 36.0f;

	// Frame Info text
	DrawText(TextFormat("Frame: %d / %d", currFrame, frameCount - 1), (int)x, (int)y + 7, 12, Color{ 220, 220, 220, 255 });
	x += 105.0f;

	// Frame Scrubber Slider
	float sliderW = m_bottomBarRect.width - (x - m_bottomBarRect.x) - 150.0f;
	if (sliderW > 60.0f)
	{
		float frameVal = animFrame;
		if (GuiSlider(Rectangle{ x, y + 4, sliderW, 20 }, "0", TextFormat("%d", frameCount - 1), &frameVal, 0, (float)(frameCount - 1)))
		{
			animFrame = frameVal;
			isPlaying = false;
		}
		x += sliderW + 15.0f;
	}

	// Speed label & slider
	DrawText("Speed:", (int)x, (int)y + 7, 11, Color{ 180, 180, 180, 255 });
	x += 42.0f;
	GuiSlider(Rectangle{ x, y + 4, 75, 20 }, "", TextFormat("%.1fx", animSpeed), &animSpeed, 0.2f, 2.5f);
}

void UserInterface::_drawStatusBar()
{
	DrawRectangleRec(m_statusBarRect, Color{ 25, 27, 33, 255 });
	DrawLine(0, (int)m_statusBarRect.y, GetScreenWidth(), (int)m_statusBarRect.y, Color{ 48, 52, 62, 255 });

	Color msgColor = m_statusIsError ? Color{ 255, 90, 90, 255 } : Color{ 140, 220, 160, 255 };
	DrawText(m_statusMessage.c_str(), 10, (int)m_statusBarRect.y + 6, 12, msgColor);

	const char* hints = "Right-Drag: Orbit | Middle-Drag: Pan | Wheel: Zoom | R: Frame | Solid/Wire: View modes";
	int textW = MeasureText(hints, 11);
	DrawText(hints, GetScreenWidth() - textW - 12, (int)m_statusBarRect.y + 7, 11, Color{ 140, 145, 160, 255 });
}

void UserInterface::_refreshDirectoryContents()
{
	m_fileDialog.directories.clear();
	m_fileDialog.files.clear();

	DIR* dir = opendir(m_fileDialog.currentPath.c_str());
	if (!dir)
	{
		m_fileDialog.currentPath = "./";
		dir = opendir(m_fileDialog.currentPath.c_str());
	}

	if (dir)
	{
		struct dirent* entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			std::string name = entry->d_name;
			if (name == ".") continue;

			std::string full = m_fileDialog.currentPath;
			if (full.back() != '/') full += "/";
			full += name;

			struct stat st;
			if (stat(full.c_str(), &st) == 0)
			{
				if (S_ISDIR(st.st_mode))
					m_fileDialog.directories.push_back(name);
				else if (IsO3DFile(name) || !m_fileDialog.isSaveMode)
					m_fileDialog.files.push_back(name);
			}
		}
		closedir(dir);

		std::sort(m_fileDialog.directories.begin(), m_fileDialog.directories.end());
		std::sort(m_fileDialog.files.begin(), m_fileDialog.files.end());
	}
}

void UserInterface::OpenFileDialog(bool isSave)
{
	m_fileDialog.active = true;
	m_fileDialog.isSaveMode = isSave;
	m_fileDialog.selectedIndex = -1;
	m_fileDialog.selectedFile = "";
	std::strncpy(m_fileDialog.manualPathBuffer, m_fileDialog.currentPath.c_str(), sizeof(m_fileDialog.manualPathBuffer) - 1);
	_refreshDirectoryContents();
}

void UserInterface::_drawFileDialog()
{
	// Dim background
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{ 0, 0, 0, 160 });

	float dlgW = 600.0f;
	float dlgH = 460.0f;
	float dlgX = (GetScreenWidth() - dlgW) * 0.5f;
	float dlgY = (GetScreenHeight() - dlgH) * 0.5f;

	Rectangle dlgRect = { dlgX, dlgY, dlgW, dlgH };

	// Dialog background
	DrawRectangleRec(dlgRect, Color{ 36, 40, 48, 255 });
	DrawRectangleLinesEx(dlgRect, 2, Color{ 65, 75, 95, 255 });

	// Title bar
	DrawRectangle(dlgX, dlgY, dlgW, 32, Color{ 48, 54, 66, 255 });
	const char* title = m_fileDialog.isSaveMode ? "Save Flyff .o3d Model" : "Open Flyff .o3d Model";
	DrawText(title, (int)dlgX + 12, (int)dlgY + 8, 14, Color{ 230, 235, 245, 255 });

	// Close button
	if (GuiButton(Rectangle{ dlgX + dlgW - 28, dlgY + 4, 24, 24 }, "X"))
	{
		m_fileDialog.active = false;
	}

	float x = dlgX + 16.0f;
	float y = dlgY + 42.0f;
	float w = dlgW - 32.0f;

	// Current Path display
	DrawText("Folder:", (int)x, (int)y + 4, 11, Color{ 180, 185, 200, 255 });
	GuiTextBox(Rectangle{ x + 50, y, w - 120, 24 }, m_fileDialog.manualPathBuffer, sizeof(m_fileDialog.manualPathBuffer), true);

	if (GuiButton(Rectangle{ x + w - 60, y, 60, 24 }, "Go"))
	{
		m_fileDialog.currentPath = m_fileDialog.manualPathBuffer;
		_refreshDirectoryContents();
	}
	y += 32.0f;

	// File / Directory List box
	Rectangle listRect = { x, y, w, 280 };
	DrawRectangleRec(listRect, Color{ 24, 26, 32, 255 });
	DrawRectangleLinesEx(listRect, 1, Color{ 55, 60, 75, 255 });

	float itemY = y + 4.0f;
	int itemIndex = 0;

	// List directories
	for (const auto& d : m_fileDialog.directories)
	{
		if (itemY > y + listRect.height - 24.0f) break;

		Rectangle itemR = { x + 4, itemY, w - 8, 20 };
		bool isHover = CheckCollisionPointRec(GetMousePosition(), itemR);

		if (isHover)
		{
			DrawRectangleRec(itemR, Color{ 50, 60, 80, 255 });
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				if (d == "..")
				{
					// Go up one directory
					std::string cp = m_fileDialog.currentPath;
					if (cp.length() > 1 && cp.back() == '/') cp.pop_back();
					size_t p = cp.find_last_of("/\\");
					if (p != std::string::npos)
						m_fileDialog.currentPath = cp.substr(0, p + 1);
					else
						m_fileDialog.currentPath = "./";
				}
				else
				{
					if (m_fileDialog.currentPath.back() != '/') m_fileDialog.currentPath += "/";
					m_fileDialog.currentPath += d + "/";
				}
				std::strncpy(m_fileDialog.manualPathBuffer, m_fileDialog.currentPath.c_str(), sizeof(m_fileDialog.manualPathBuffer) - 1);
				_refreshDirectoryContents();
				break;
			}
		}

		DrawText(TextFormat("[DIR] %s", d.c_str()), (int)x + 8, (int)itemY + 4, 11, Color{ 140, 190, 255, 255 });
		itemY += 22.0f;
		itemIndex++;
	}

	// List files
	for (const auto& f : m_fileDialog.files)
	{
		if (itemY > y + listRect.height - 24.0f) break;

		Rectangle itemR = { x + 4, itemY, w - 8, 20 };
		bool isSel = (m_fileDialog.selectedFile == f);
		bool isHover = CheckCollisionPointRec(GetMousePosition(), itemR);

		if (isSel)
			DrawRectangleRec(itemR, Color{ 40, 90, 160, 255 });
		else if (isHover)
			DrawRectangleRec(itemR, Color{ 45, 52, 68, 255 });

		if (isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			m_fileDialog.selectedFile = f;
			std::string fullPath = m_fileDialog.currentPath;
			if (fullPath.back() != '/') fullPath += "/";
			fullPath += f;
			std::strncpy(m_fileDialog.manualPathBuffer, fullPath.c_str(), sizeof(m_fileDialog.manualPathBuffer) - 1);
		}

		Color col = IsO3DFile(f) ? Color{ 220, 240, 180, 255 } : Color{ 180, 180, 180, 255 };
		DrawText(f.c_str(), (int)x + 20, (int)itemY + 4, 11, isSel ? WHITE : col);
		itemY += 22.0f;
		itemIndex++;
	}

	y += 292.0f;

	// Bottom action bar
	std::string actionName = m_fileDialog.isSaveMode ? "Save" : "Open";

	if (GuiButton(Rectangle{ dlgX + dlgW - 190, y, 80, 28 }, actionName.c_str()) ||
	    (IsKeyPressed(KEY_ENTER) && m_fileDialog.active))
	{
		std::string chosenPath = m_fileDialog.manualPathBuffer;
		if (!chosenPath.empty())
		{
			if (m_fileDialog.isSaveMode)
				m_fileToSave = chosenPath;
			else
				m_fileToOpen = chosenPath;
			m_fileDialog.active = false;
		}
	}

	if (GuiButton(Rectangle{ dlgX + dlgW - 100, y, 80, 28 }, "Cancel") ||
	    (IsKeyPressed(KEY_ESCAPE) && m_fileDialog.active))
	{
		m_fileDialog.active = false;
	}
}
