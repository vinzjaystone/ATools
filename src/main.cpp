#include "raylib.h"
#include "core/Types.h"
#include "core/File.h"
#include "model/Object3D.h"
#include "model/SceneManager.h"
#include "model/FolderScanner.h"
#include "model/TextureMng.h"
#include "model/ModelMng.h"
#include "viewer/Camera.h"
#include "viewer/Renderer.h"
#include "ui/UI.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	// Configure Window flags
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
	InitWindow(1360, 840, "Flyff .o3d Model Viewer & Composite Editor");
	SetWindowMinSize(960, 600);
	SetTargetFPS(60);

	// Application state
	SceneManager scene;
	OrbitCamera camera;
	SceneRenderer renderer;
	RenderSettings settings;
	UserInterface ui;

	// Load initial model if path passed as command line argument
	if (argc > 1)
	{
		std::string initialPath = argv[1];
		if (scene.LoadBaseModel(initialPath))
		{
			camera.FrameModel(scene.GetCombinedBounds());
			ui.ShowStatusMessage("Loaded model: " + SceneManager::ExtractFileName(initialPath));

			// Point folder scanner to parent directory of loaded file
			fs::path p(initialPath);
			if (p.has_parent_path())
			{
				ui.GetScanner().SetCurrentFolder(p.parent_path().string());
			}
		}
		else
		{
			ui.ShowStatusMessage("Failed to load: " + initialPath, 4.0f, true);
		}
	}

	// Main Application Loop
	while (!WindowShouldClose())
	{
		float deltaTime = GetFrameTime();

		// Handle Drag & Drop files
		if (IsFileDropped())
		{
			FilePathList droppedFiles = LoadDroppedFiles();
			for (unsigned int i = 0; i < droppedFiles.count; i++)
			{
				std::string droppedPath = droppedFiles.paths[i];
				std::string ext = fs::path(droppedPath).extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				if (ext == ".o3d")
				{
					if (i == 0 && scene.GetPartCount() == 0)
					{
						if (scene.LoadBaseModel(droppedPath))
						{
							camera.FrameModel(scene.GetCombinedBounds());
							ui.ShowStatusMessage("Loaded base model: " + SceneManager::ExtractFileName(droppedPath));
						}
					}
					else
					{
						if (scene.AppendModel(droppedPath))
						{
							camera.FrameModel(scene.GetCombinedBounds());
							ui.ShowStatusMessage("Appended part: " + SceneManager::ExtractFileName(droppedPath));
						}
					}
				}
				else if (ext == ".mot")
				{
					if (scene.LoadMotion(droppedPath))
						ui.ShowStatusMessage("Loaded motion: " + SceneManager::ExtractFileName(droppedPath));
				}
				else if (ext == ".bon")
				{
					if (scene.LoadSkeleton(droppedPath))
						ui.ShowStatusMessage("Loaded skeleton: " + SceneManager::ExtractFileName(droppedPath));
				}

				// Update folder scanner to dropped file directory
				fs::path p(droppedPath);
				if (p.has_parent_path())
					ui.GetScanner().SetCurrentFolder(p.parent_path().string());
			}
			UnloadDroppedFiles(droppedFiles);
		}

		// Handle Keyboard Shortcuts
		bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
		bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

		if (!ctrlDown)
		{
			if (IsKeyPressed(KEY_F))
			{
				if (scene.GetPartCount() > 0)
					camera.FrameModel(scene.GetCombinedBounds());
			}
			if (IsKeyPressed(KEY_R))
				camera.Reset();
			if (IsKeyPressed(KEY_G))
				settings.showGrid = !settings.showGrid;
			if (IsKeyPressed(KEY_A))
				settings.showAxes = !settings.showAxes;
			if (IsKeyPressed(KEY_B))
				settings.showBounds = !settings.showBounds;
			if (IsKeyPressed(KEY_C))
				settings.showCollision = !settings.showCollision;
			if (IsKeyPressed(KEY_L))
				settings.enableLighting = !settings.enableLighting;
			if (IsKeyPressed(KEY_W))
				settings.fillMode = (settings.fillMode == FILL_WIREFRAME) ? FILL_SOLID : FILL_WIREFRAME;
			if (IsKeyPressed(KEY_SPACE))
				scene.TogglePlay();
			if (IsKeyPressed(KEY_LEFT))
				scene.StepFrame(-1);
			if (IsKeyPressed(KEY_RIGHT))
				scene.StepFrame(1);

			// LOD shortcuts for active part
			ScenePart* activePart = scene.GetPart(scene.GetActivePartIndex());
			if (activePart && activePart->model && activePart->model->HasLOD())
			{
				if (IsKeyPressed(KEY_ONE)) activePart->currentLOD = 0;
				if (IsKeyPressed(KEY_TWO)) activePart->currentLOD = 1;
				if (IsKeyPressed(KEY_THREE)) activePart->currentLOD = 2;
			}
		}
		else // Control is held
		{
			if (IsKeyPressed(KEY_O))
				ui.OpenFileDialog(DIALOG_OPEN_MODEL);
			if (IsKeyPressed(KEY_A) && shiftDown)
				ui.OpenFileDialog(DIALOG_APPEND_MODEL);
			if (IsKeyPressed(KEY_M))
				ui.OpenFileDialog(DIALOG_OPEN_MOTION);
			if (IsKeyPressed(KEY_S))
			{
				CObject3D* activeModel = scene.GetActiveModel();
				if (activeModel)
				{
					std::string fn = activeModel->GetLoadedFilename();
					if (!fn.empty())
					{
						if (activeModel->Save(fn))
							ui.ShowStatusMessage("Model saved: " + SceneManager::ExtractFileName(fn));
						else
							ui.ShowStatusMessage("Error saving model: " + SceneManager::ExtractFileName(fn), 4.0f, true);
					}
					else
						ui.OpenFileDialog(DIALOG_SAVE_MODEL);
				}
			}
		}

		// Update Animations
		scene.UpdateAnimation(deltaTime);

		// Update 3D Orbit Camera (only when mouse is inside viewport)
		camera.Update(ui.GetViewportRect());

		// Begin Frame Rendering
		BeginDrawing();
		ClearBackground(Color{ 22, 24, 28, 255 });

		// 1. Render 3D Viewport Scene with all composite parts
		renderer.RenderScene(scene, camera, ui.GetViewportRect(), settings);

		// 2. Render UI (Top bar, Left library/parts tabs, Right inspectors, Bottom timeline scrubber, Status bar)
		ui.UpdateAndDraw(scene, camera, settings, deltaTime);

		EndDrawing();
	}

	// Cleanup
	scene.ClearAll();
	CTextureMng::Destroy();
	CModelMng::Destroy();

	CloseWindow();
	return 0;
}
