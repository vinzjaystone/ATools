#include "raylib.h"
#include "core/Types.h"
#include "core/File.h"
#include "model/Object3D.h"
#include "model/TextureMng.h"
#include "model/ModelMng.h"
#include "viewer/Camera.h"
#include "viewer/Renderer.h"
#include "ui/UI.h"
#include <iostream>

int main(int argc, char* argv[])
{
	// Configure Window flags
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
	InitWindow(1280, 800, "Flyff .o3d Model Viewer & Editor");
	SetWindowMinSize(900, 600);
	SetTargetFPS(60);

	// Application state
	CObject3D* model = nullptr;
	int currentLOD = 0;
	float animSpeed = 1.0f;
	bool isPlaying = true;

	OrbitCamera camera;
	SceneRenderer renderer;
	RenderSettings settings;
	UserInterface ui;

	// Load model if path passed as command line argument
	if (argc > 1)
	{
		std::string initialPath = argv[1];
		CObject3D* newModel = new CObject3D();
		if (newModel->Load(initialPath))
		{
			model = newModel;
			camera.FrameModel(model->GetBounds());
			ui.ShowStatusMessage("Loaded: " + GetFileName(initialPath));
		}
		else
		{
			delete newModel;
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
			if (droppedFiles.count > 0)
			{
				std::string droppedPath = droppedFiles.paths[0];
				CObject3D* newModel = new CObject3D();
				if (newModel->Load(droppedPath))
				{
					if (model) delete model;
					model = newModel;
					currentLOD = 0;
					camera.FrameModel(model->GetBounds());
					ui.ShowStatusMessage("Loaded: " + GetFileName(droppedPath));
				}
				else
				{
					delete newModel;
					ui.ShowStatusMessage("Error loading dropped file: " + GetFileName(droppedPath), 4.0f, true);
				}
			}
			UnloadDroppedFiles(droppedFiles);
		}

		// Handle Open File request from UI
		std::string fileToOpen = ui.GetSelectedFileToOpen();
		if (!fileToOpen.empty())
		{
			CObject3D* newModel = new CObject3D();
			if (newModel->Load(fileToOpen))
			{
				if (model) delete model;
				model = newModel;
				currentLOD = 0;
				camera.FrameModel(model->GetBounds());
				ui.ShowStatusMessage("Loaded: " + GetFileName(fileToOpen));
			}
			else
			{
				delete newModel;
				ui.ShowStatusMessage("Failed to open file: " + GetFileName(fileToOpen), 4.0f, true);
			}
			ui.ClearFileRequests();
		}

		// Handle Save File request from UI
		std::string fileToSave = ui.GetSelectedFileToSave();
		if (!fileToSave.empty() && model)
		{
			if (model->Save(fileToSave))
				ui.ShowStatusMessage("Saved model successfully: " + GetFileName(fileToSave));
			else
				ui.ShowStatusMessage("Error saving model: " + GetFileName(fileToSave), 4.0f, true);
			ui.ClearFileRequests();
		}

		// Handle Keyboard Shortcuts
		if (!IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL))
		{
			if (IsKeyPressed(KEY_R))
			{
				if (model)
					camera.FrameModel(model->GetBounds());
				else
					camera.Reset();
			}
			if (IsKeyPressed(KEY_G))
				settings.showGrid = !settings.showGrid;
			if (IsKeyPressed(KEY_B))
				settings.showBounds = !settings.showBounds;
			if (IsKeyPressed(KEY_C))
				settings.showCollision = !settings.showCollision;
			if (IsKeyPressed(KEY_SPACE))
				isPlaying = !isPlaying;
			if (IsKeyPressed(KEY_ONE) && model && model->HasLOD())
				currentLOD = 0;
			if (IsKeyPressed(KEY_TWO) && model && model->HasLOD())
				currentLOD = 1;
			if (IsKeyPressed(KEY_THREE) && model && model->HasLOD())
				currentLOD = 2;
		}
		else
		{
			if (IsKeyPressed(KEY_O))
				ui.OpenFileDialog(false);
			if (IsKeyPressed(KEY_S) && model)
			{
				std::string fn = model->GetLoadedFilename();
				if (!fn.empty())
				{
					if (model->Save(fn))
						ui.ShowStatusMessage("Model saved: " + GetFileName(fn));
					else
						ui.ShowStatusMessage("Error saving model: " + GetFileName(fn), 4.0f, true);
				}
				else
					ui.OpenFileDialog(true);
			}
		}

		// Update 3D Orbit Camera (only when mouse is in viewport)
		camera.Update(ui.GetViewportRect());

		// Begin Frame Rendering
		BeginDrawing();
		ClearBackground(Color{ 30, 33, 40, 255 });

		// 1. Render 3D Viewport Scene
		renderer.Render(model, currentLOD, camera, ui.GetViewportRect(), settings);

		// 2. Render UI (Toolbar, Sidebar Panels, Inspector, Animation Controls, Status Bar)
		ui.UpdateAndDraw(model, currentLOD, camera, settings, animSpeed, isPlaying, deltaTime);

		EndDrawing();
	}

	// Cleanup
	if (model)
	{
		delete model;
		model = nullptr;
	}

	CTextureMng::Destroy();
	CModelMng::Destroy();

	CloseWindow();
	return 0;
}
