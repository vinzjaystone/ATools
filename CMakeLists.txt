cmake_minimum_required(VERSION 3.10)
project(ATools_ModelViewer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -Wno-stringop-truncation -Wno-class-memaccess)

include_directories(
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/Lib/raylib/include
)

set(SOURCES
    src/core/D3DXMath.cpp
    src/core/File.cpp
    src/model/Object3D.cpp
    src/model/Motion.cpp
    src/model/TextureMng.cpp
    src/model/Model.cpp
    src/model/AnimatedMesh.cpp
    src/model/ModelMng.cpp
    src/model/SceneManager.cpp
    src/model/FolderScanner.cpp
    src/viewer/Camera.cpp
    src/viewer/Renderer.cpp
    src/ui/UI.cpp
    src/main.cpp
)

add_executable(model_viewer ${SOURCES})

target_link_libraries(model_viewer
    ${CMAKE_CURRENT_SOURCE_DIR}/Lib/raylib/lib/libraylib.a
    m
    pthread
    dl
    rt
)
