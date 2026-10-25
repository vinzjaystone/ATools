CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -Wno-stringop-truncation -Wno-class-memaccess -Isrc -ILib/raylib/include
LDFLAGS ?= -LLib/raylib/lib Lib/raylib/lib/libraylib.a -lX11 -lm -lpthread -ldl -lrt

TARGET = model_viewer

SRCS = src/core/D3DXMath.cpp \
       src/core/File.cpp \
       src/model/Object3D.cpp \
       src/model/Motion.cpp \
       src/model/TextureMng.cpp \
       src/model/Model.cpp \
       src/model/AnimatedMesh.cpp \
       src/model/ModelMng.cpp \
       src/model/SceneManager.cpp \
       src/model/GltfExporter.cpp \
       src/model/FolderScanner.cpp \
       src/viewer/Camera.cpp \
       src/viewer/Renderer.cpp \
       src/ui/UI.cpp \
       src/main.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
