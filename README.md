# Flyff O3D Model Viewer & Composite Editor (Linux / raylib)

A high-performance, standalone 3D model viewer, animation player, and composite character editor for **Flyff (Fly For Fun) `.o3d`** models, **`.chr` / `.bon`** character skeletons, and **`.mot` / `.ani`** motions on Linux.

Powered by **raylib** and **raygui**, completely free of Qt and Direct3D 9 dependencies.

---

## Features

### 1. Folder Scanner & Model Library Browser
- **Folder Scanner**: Point to any folder (or current directory) and instantly scan for all `.o3d` models, `.mot` / `.ani` animations, and `.chr` / `.bon` skeletons.
- **Subfolder Search**: Toggle recursive folder scanning to discover assets across nested game directories (e.g., `Model/`, `Item/`, `Ctrl/`).
- **Live Search & Filter**: Real-time keyword filter to quickly find specific models (e.g., `head`, `hair`, `hat`, `sword`, `mvr_`).
- **One-Click Loading & Dual Modes**:
  - **Replace Mode**: Click any `.o3d` model to load and display it as the primary base model.
  - **Append Mode**: Click any `.o3d` model (or click the `+` button) to append it to the current scene without replacing existing models!
  - **Motion Library**: Click any `.mot` or `.ani` file to immediately apply the animation to the loaded models.
  - **Skeleton Library**: Click any `.chr` or `.bon` file to load or bind a character skeleton.

### 2. Multi-Part Composite Character & Scene Manager
- **Assemble Composite Models**: Load a base body (e.g., character or monster) and append additional `.o3d` parts:
  - Head, Hair, Hats, Helmets
  - Upper / Lower Armor, Gloves, Boots
  - Weapons (Swords, Shields, Bows, Wands)
  - Cloaks, Wings, and Accessories
- **Auto-Skeleton Resolution**: Automatically detects and attaches the corresponding character skeleton (`mvr_male.chr`, `mvr_female.chr`, or `<name>.chr`) when loading character body parts.
- **Parts Manager Panel**:
  - Per-part **Visibility toggle** (`[x]`).
  - Active part selection for individual material and submesh editing.
  - **Bone Socket Attachments**: Attach rigid weapons or accessories to specific skeleton sockets (Root, Head, Right Hand (`RH`), Left Hand (`LH`), Back, etc.) or standard skeletal skinning.
  - Per-part LOD level control (`LOD 0`, `1`, `2`).
  - Individual part removal (`X`) and `Clear Appended Parts`.
- **Combined Auto-Framing**: Frame Camera (`F`) automatically calculates the bounding box of all visible parts combined.

### 3. Motion & Animation Player (`.mot`, `.ani`, `.chr`, `.bon`)
- **Auto-Discovery of All Available Animations**:
  - When loading a character model or part (e.g. `mvr_male.o3d`, `part_m_*.o3d`, `mvr_clockwork.o3d`), the viewer automatically discovers and loads **all available animations** in the directory (`walk`, `run`, `idle`, `attack1`, `attack2`, `jump`, `sit`, `die`, etc.).
  - Select and play any animation with a single click from the **Character Animations** panel or cycle seamlessly through animations with `[` / `]` or `Ctrl + Left` / `Right`.
- **Animation Controls**:
  - Play / Pause toggle (`Space`).
  - Frame Step buttons (`<` and `>` arrow keys).
  - First frame rewind (`|<`).
  - **Quick Motion Switcher**: `[Prev Motion]` and `[Next Motion]` buttons in the bottom timeline.
  - **Interactive Timeline Scrubber**: Drag slider to scrub across keyframes with real-time model and bone posing!
  - **Playback Speed & Loop Control**: Adjust playback speed from `0.1x` to `3.0x` and toggle animation looping (`Loop` vs `Once`).

### 4. glTF 2.0 / GLB 3D Export
- **One-Click glTF 2.0 / GLB Export**:
  - Export loaded models and composite character assemblies to standard, self-contained **`.glb`** / **`.gltf`** files.
  - Compatible with **Blender**, **Godot**, **Unity**, **Unreal Engine**, **Three.js**, **Windows 3D Viewer**, and web model viewers.
- **Includes All Geometry, Skeleton & Animations**:
  - Exports vertex positions, normals, UV texture coordinates, joint indices (`JOINTS_0`), and skin weights (`WEIGHTS_0`).
  - Exports full bone armature hierarchy with inverse bind matrices.
  - Exports **all discovered animations** as named glTF animation tracks with linear quaternion and translation keyframing.
  - Embeds standard PBR materials and PNG textures directly inside the single `.glb` file.
  - Accessible via Top Bar (`#07# Export glTF`), Keyboard Shortcut (`Ctrl + E`), or CLI (`./model_viewer --export-glb <in.o3d> <out.glb>`).
  - Playback Speed Slider ($0.1\times \dots 3.0\times$).
  - Loop toggle (Loop playback vs Single pass).
  - Keyframe counter display: `Frame: X / Total: Y`.
- **Synchronized Skeletal Skinning**:
  - Appended parts automatically inherit the animated skeleton in real time, moving together seamlessly in sync with the motion.
- **Skeleton Bones Inspector**: Browse all skeleton joints with IDs, parent linkages, and joint names (e.g., `Bip01 Pelvis`, `Bip01 Spine`, `Bip01 Head`, `RH`, `LH`).

### 4. 3D Viewport & Rendering Pipeline
- **Interactive Orbit Camera**: Orbit (Right-click drag), Pan (Middle-click drag or Shift + Right-click drag), Zoom (Mouse wheel), Smooth auto-framing (`F`).
- **Render Modes & Overlays**:
  - Textured rendering with alpha blending and dynamic lighting.
  - Wireframe overlay mode (`W`).
  - Skeletal bone hierarchy & joint rendering (`B`).
  - 3D Bounding Box visualization (`C`).
  - Coordinate axes (X = Red, Y = Green, Z = Blue) (`A`) and ground reference grid (`G`).
  - Viewport background color toggle button.

### 5. Model & Material Property Inspector
- **Submesh & Material Editor**:
  - Ambient, Diffuse, Specular RGBA color pickers.
  - Texture filename text edit and reload.
  - Two-Sided polygon rendering toggle.
  - UV Scrolling animation parameters ($U, V$ speeds).
  - Material Effect Blocks (Additive glow, opacity, blending).
- **100% Byte-Accurate Binary Serialization**:
  - Full load and save compatibility preserving the original Flyff `.o3d`, `.chr`, `.bon`, and `.mot` format specifications.

---

## Directory Structure

```
ATools/
├── Lib/
│   └── raylib/            # Bundled raylib & raygui headers and static library
│       ├── include/       # raylib.h, raymath.h, rlgl.h, raygui.h
│       └── lib/           # libraylib.a (built for Linux x86_64)
├── src/
│   ├── core/              # Foundational types, binary streaming, and math
│   │   ├── Types.h        # Cross-platform basic types & definitions
│   │   ├── File.h/.cpp    # High-fidelity binary stream serialization
│   │   └── D3DXMath.h/.cpp# Complete D3D9 vector/matrix/quaternion math library
│   ├── model/             # Flyff 3D model, composite scene, motion, and textures
│   │   ├── Object3D.h/.cpp# Flyff .o3d mesh format parser and serializer
│   │   ├── Motion.h/.cpp  # Skeleton and bone animation parser (.chr / .bon / .mot / .ani)
│   │   ├── SceneManager.h/.cpp # Multi-part composite character & scene manager
│   │   ├── FolderScanner.h/.cpp# Directory scanner and file filter
│   │   ├── TextureMng.h/.cpp   # Raylib Texture2D resource caching
│   │   └── AnimatedMesh.h/.cpp # Keyframe interpolation and bone skinning
│   ├── viewer/            # Viewport & 3D rendering pipeline
│   │   ├── Camera.h/.cpp  # Smooth orbit/pan/zoom camera controller
│   │   └── Renderer.h/.cpp# Multi-part drawing routines, overlays, & debug views
│   ├── ui/                # Graphical user interface (raygui)
│   │   └── UI.h/.cpp      # Library browser, parts manager, timeline, inspector
│   └── main.cpp           # Application entry point and interactive main loop
├── samples/               # Sample models and animations for testing
│   ├── model_body.o3d     # Base torso model
│   ├── model_body.chr     # Skeleton definition
│   ├── mvr_male.chr       # Character skeleton file
│   ├── model_head.o3d     # Head part model
│   ├── part_hat.o3d       # Hat accessory model
│   ├── wea_sword.o3d      # Sword weapon model
│   └── motion_walk.mot    # Walk cycle skeletal animation
├── Makefile               # Standard Makefile build
├── CMakeLists.txt         # CMake build configuration
└── README.md
```

---

## Build Instructions

### Prerequisites
- GCC / G++ (supporting C++17)
- Standard system runtime libraries: `libm`, `libpthread`, `libdl`, `librt`, `libX11`

### Building with `make` (Recommended)
```bash
make -j$(nproc)
```
This produces the standalone `model_viewer` executable.

To clean the build:
```bash
make clean
```

---

## Usage

### Launching the Application
```bash
./model_viewer
```
Or open a model directly from the command line:
```bash
./model_viewer samples/model_body.o3d
```

### Drag & Drop Support
You can drag and drop `.o3d` models, `.mot` / `.ani` animations, or `.chr` / `.bon` skeletons directly onto the application window from your file manager!

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
| `Ctrl + O` | Open Model (`.o3d`) Dialog |
| `Ctrl + Shift + A` | Append Model to Scene Dialog |
| `Ctrl + M` | Load Motion (`.mot` / `.ani`) Dialog |
| `Ctrl + K` | Load Skeleton (`.chr` / `.bon`) Dialog |
| `Ctrl + S` | Save Active Model (`.o3d`) |
| `Ctrl + E` | Export to glTF 2.0 / GLB (`.glb`) |
| `[` / `]` | Previous / Next Model Animation |
| `Ctrl + Left / Right` | Previous / Next Model Animation |
| `Space` | Play / Pause Animation |
| `Left / Right` | Step Animation 1 Frame Backward / Forward |
| `F` | Focus / Frame Camera on All Visible Models |
| `R` | Reset Camera to Default View |
| `W` | Toggle Wireframe Overlay |
| `G` | Toggle Ground Grid |
| `A` | Toggle Coordinate Axes |
| `B` | Toggle Bounding Box |
| `C` | Toggle Collision Mesh |
| `L` | Toggle Viewport Lighting |
| `1` / `2` / `3` | Switch Active Part LOD Level (0, 1, 2) |
