# Flyff O3D Model Viewer & Editor (Linux / raylib)

A lightweight, standalone 3D model viewer and property editor for **Flyff (Fly For Fun) `.o3d`** models on Linux.

This project extracts and modernizes the Flyff 3D model pipeline from the legacy Windows/Qt/DirectX 9 ATools suite into a self-contained, cross-platform C++ application powered by **raylib** and **raygui**.

---

## Features

### 3D Viewport & Rendering (raylib)
- **Interactive Orbit Camera**: Orbit (Right-click drag), Pan (Middle-click drag or Shift + Right-click drag), Zoom (Mouse wheel), Smooth auto-framing (`F`).
- **Render Modes & Overlays**:
  - Textured mesh rendering with alpha blending and dynamic lighting.
  - Wireframe overlay mode (`W`).
  - Normal vector visualization (`N`).
  - Skeletal bone hierarchy & joint rendering (`B`).
  - 3D Bounding Box visualization (`C`).
  - Coordinate axes (X = Red, Y = Green, Z = Blue) (`A`) and ground reference grid (`G`).
  - Dynamic viewport background color toggle.
- **Skeletal & Mesh Animation**: Support for Flyff bone animations (`.mot`) with playback, pause, and speed controls.

### Model Inspection & Hierarchy
- **Real-Time Statistics**: Total vertex count, triangle count, submesh count, bone count, bounding box min/max, format version, and flags.
- **Mesh Hierarchy Browser**: Browse all sub-objects in the `.o3d` file with vertex and triangle counts, and inspect/edit them individually.

### Model & Material Property Editing
- **Object Attributes**: Edit object name, visibility, collision flag, pickable flag, billboard type (None, Y-Axis, Full), and alpha opacity.
- **Material Editor**:
  - Edit Ambient, Diffuse, and Specular RGBA color channels with real-time visual feedback.
  - View and edit assigned texture filenames.
  - Toggle Two-Sided polygon rendering.
  - UV Scrolling animation parameters ($U, V$ speeds).
- **Material Effects / Blocks**: Inspect and edit blending effects and blend amounts (e.g., additive blending, glow, specular effects).

### Exact `.o3d` Binary Serialization
- 100% byte-accurate serialization and deserialization matching the original Flyff `.o3d` specification across all versions (v17–v23).
- Zero data loss when loading, modifying properties, and saving models back to disk.

---

## Directory Structure

```
ATools/
├── Lib/
│   └── raylib/            # Bundled raylib & raygui headers and x86_64 static library
│       ├── include/       # raylib.h, raymath.h, rlgl.h, raygui.h
│       └── lib/           # libraylib.a (built for Linux x86_64)
├── src/
│   ├── core/              # Foundational types, binary streaming, and math
│   │   ├── Types.h        # Cross-platform basic types & definitions
│   │   ├── File.h/.cpp    # High-fidelity binary stream serialization
│   │   └── D3DXMath.h/.cpp# Complete D3D9 vector/matrix/quaternion math library
│   ├── model/             # Flyff 3D model, motion, and texture management
│   │   ├── Object3D.h/.cpp# Flyff .o3d mesh format parser and serializer
│   │   ├── Motion.h/.cpp  # Skeleton and bone animation parser
│   │   ├── Model.h/.cpp   # Composite model & sub-object container
│   │   ├── AnimatedMesh.h/.cpp # Keyframe interpolation and bone skinning
│   │   ├── TextureMng.h/.cpp   # Raylib Texture2D resource caching
│   │   └── ModelMng.h/.cpp# Model registry and caching
│   ├── viewer/            # Viewport & 3D rendering pipeline
│   │   ├── Camera.h/.cpp  # Smooth orbit/pan/zoom camera controller
│   │   └── Renderer.h/.cpp# Raylib/OpenGL drawing routines, overlays, & debug views
│   ├── ui/                # Graphical user interface (raygui)
│   │   └── UI.h/.cpp      # Toolbar, side panels, inspector, & file browser modal
│   └── main.cpp           # Application entry point and interactive main loop
├── sample_cube.o3d        # Sample test model
├── Makefile               # Standard Makefile build
├── CMakeLists.txt         # CMake build configuration
└── README.md
```

---

## Build Instructions

### Prerequisites
- GCC / G++ (supporting C++17)
- Standard system libraries: `libglfw`, `libX11`, `pthread`, `dl`, `m`

On Debian/Ubuntu systems:
```bash
sudo apt-get install build-essential libglfw3-dev libx11-dev
```

### Building with `make` (Recommended)
```bash
make -j$(nproc)
```
This produces the standalone `model_viewer` executable in the root directory.

### Building with `cmake`
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

To clean the build:
```bash
make clean
```

---

## Usage

### Launching the Application
Run the executable directly:
```bash
./model_viewer
```
Or open an `.o3d` model directly from the command line:
```bash
./model_viewer sample_cube.o3d
```

---

## Controls & Keybindings

### Mouse Controls
| Action | Input |
|---|---|
| **Orbit Camera** | Right Mouse Button + Drag |
| **Pan Camera** | Middle Mouse Button + Drag _or_ Shift + Right Mouse Button + Drag |
| **Zoom / Dolly** | Mouse Wheel Scroll |
| **Select / Interact** | Left Mouse Button |

### Keyboard Shortcuts
| Key | Action |
|---|---|
| `Ctrl + O` | Open File Dialog |
| `Ctrl + S` | Save Current Model (`.o3d`) |
| `Ctrl + Shift + S` | Save As New File (`.o3d`) |
| `F` | Focus / Frame Camera on Model |
| `R` | Reset Camera to Default View |
| `W` | Toggle Wireframe Overlay |
| `N` | Toggle Normal Vectors |
| `B` | Toggle Bones / Skeleton Display |
| `G` | Toggle Ground Grid |
| `A` | Toggle Coordinate Axes |
| `C` | Toggle Bounding Box / Collision Bounds |
| `L` | Toggle Viewport Lighting |
| `Space` | Play / Pause Animation |
| `Escape` | Deselect Object / Close Dialogs |

---

## UI Guide

- **Top Toolbar**: Fast access to file operations (Open, Save, Save As), camera reset, viewport overlay toggles (Wireframe, Normals, Bones, Grid, Axes, Bounds, Lighting), and background color cycling.
- **Left Panel (Model Inspector)**:
  - **General Info**: Format version, object count, bone count, animated status, and total polygon/vertex stats.
  - **Hierarchy Tree**: Click any submesh to select and inspect it in the right panel.
  - **Model Settings**: Global collision toggle and texture coordinate scrolling parameters ($U, V$).
- **Right Panel (Submesh & Material Editor)**:
  - **Object Attributes**: Name, ID, Parent ID, Visibility, Collision, Billboard mode, and Alpha Opacity slider.
  - **Material Properties**: Submesh material selector, RGBA color pickers (Ambient, Diffuse, Specular), Two-Sided toggle, and Texture filename editor.
  - **Material Effect Blocks**: Blend effects and blend amount slider.
- **Built-in File Browser**: Seamlessly browse local directories to load and save `.o3d` files directly within the raylib UI.
