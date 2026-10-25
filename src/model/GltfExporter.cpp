#include "GltfExporter.h"
#include "SceneManager.h"
#include "TextureMng.h"
#include "raylib.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <cstring>

namespace fs = std::filesystem;

namespace
{
	struct BufferView
	{
		uint32_t buffer;
		uint32_t byteOffset;
		uint32_t byteLength;
		uint32_t byteStride;
		int target; // 34962 = ARRAY_BUFFER, 34963 = ELEMENT_ARRAY_BUFFER, 0 = none
	};

	struct Accessor
	{
		int bufferView;
		uint32_t byteOffset;
		int componentType; // 5123 = USHORT, 5125 = UINT, 5126 = FLOAT
		int count;
		std::string type; // SCALAR, VEC2, VEC3, VEC4, MAT4
		std::vector<float> minValues;
		std::vector<float> maxValues;
	};

	struct MaterialData
	{
		std::string name;
		float baseColor[4];
		float metallic;
		float roughness;
		int textureIndex;
		bool doubleSided;
		std::string alphaMode;
	};

	struct ImageData
	{
		std::string name;
		std::string mimeType;
		int bufferView;
		std::string uri;
	};

	struct TextureData
	{
		int sourceImage;
		int sampler;
	};

	struct PrimitiveData
	{
		std::map<std::string, int> attributes; // "POSITION", "NORMAL", "TEXCOORD_0", "JOINTS_0", "WEIGHTS_0"
		int indices;
		int material;
		int mode; // 4 = TRIANGLES
	};

	struct MeshData
	{
		std::string name;
		std::vector<PrimitiveData> primitives;
	};

	struct NodeData
	{
		std::string name;
		int mesh;
		int skin;
		std::vector<float> translation;
		std::vector<float> rotation;
		std::vector<float> scale;
		std::vector<int> children;

		NodeData() : mesh(-1), skin(-1) {}
	};

	struct SkinData
	{
		std::string name;
		int inverseBindMatrices;
		std::vector<int> joints;
		int skeleton;

		SkinData() : inverseBindMatrices(-1), skeleton(-1) {}
	};

	struct AnimationSamplerData
	{
		int input;  // time accessor
		int output; // transform values accessor
		std::string interpolation; // LINEAR, STEP
	};

	struct AnimationChannelData
	{
		int sampler;
		int targetNode;
		std::string targetPath; // "translation", "rotation", "scale"
	};

	struct AnimationData
	{
		std::string name;
		std::vector<AnimationSamplerData> samplers;
		std::vector<AnimationChannelData> channels;
	};

	void MatrixToTRS(const D3DXMATRIX& m, D3DXVECTOR3& t, D3DXQUATERNION& q, D3DXVECTOR3& s)
	{
		t = D3DXVECTOR3(m._41, m._42, m._43);

		float sx = std::sqrt(m._11 * m._11 + m._12 * m._12 + m._13 * m._13);
		float sy = std::sqrt(m._21 * m._21 + m._22 * m._22 + m._23 * m._23);
		float sz = std::sqrt(m._31 * m._31 + m._32 * m._32 + m._33 * m._33);
		if (sx < 1e-6f) sx = 1.0f;
		if (sy < 1e-6f) sy = 1.0f;
		if (sz < 1e-6f) sz = 1.0f;
		s = D3DXVECTOR3(sx, sy, sz);

		float r11 = m._11 / sx, r12 = m._12 / sx, r13 = m._13 / sx;
		float r21 = m._21 / sy, r22 = m._22 / sy, r23 = m._23 / sy;
		float r31 = m._31 / sz, r32 = m._32 / sz, r33 = m._33 / sz;

		float trace = r11 + r22 + r33;
		if (trace > 0.0f)
		{
			float s4 = std::sqrt(trace + 1.0f) * 2.0f;
			q.w = 0.25f * s4;
			q.x = (r23 - r32) / s4;
			q.y = (r31 - r13) / s4;
			q.z = (r12 - r21) / s4;
		}
		else if ((r11 > r22) && (r11 > r33))
		{
			float s4 = std::sqrt(1.0f + r11 - r22 - r33) * 2.0f;
			q.w = (r23 - r32) / s4;
			q.x = 0.25f * s4;
			q.y = (r12 + r21) / s4;
			q.z = (r31 + r13) / s4;
		}
		else if (r22 > r33)
		{
			float s4 = std::sqrt(1.0f + r22 - r11 - r33) * 2.0f;
			q.w = (r31 - r13) / s4;
			q.x = (r12 + r21) / s4;
			q.y = 0.25f * s4;
			q.z = (r23 + r32) / s4;
		}
		else
		{
			float s4 = std::sqrt(1.0f + r33 - r11 - r22) * 2.0f;
			q.w = (r12 - r21) / s4;
			q.x = (r31 + r13) / s4;
			q.y = (r23 + r32) / s4;
			q.z = 0.25f * s4;
		}

		float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
		if (len > 1e-6f)
		{
			q.x /= len; q.y /= len; q.z /= len; q.w /= len;
		}
		else
		{
			q = D3DXQUATERNION(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}

	class GltfBuilder
	{
	public:
		std::vector<uint8_t> binBuffer;
		std::vector<BufferView> bufferViews;
		std::vector<Accessor> accessors;
		std::vector<MaterialData> materials;
		std::vector<ImageData> images;
		std::vector<TextureData> textures;
		std::vector<MeshData> meshes;
		std::vector<NodeData> nodes;
		std::vector<SkinData> skins;
		std::vector<AnimationData> animations;
		std::vector<int> sceneRootNodes;

		uint32_t AppendBin(const void* data, size_t size, size_t alignment = 4)
		{
			while (binBuffer.size() % alignment != 0)
			{
				binBuffer.push_back(0);
			}
			uint32_t offset = (uint32_t)binBuffer.size();
			const uint8_t* p = static_cast<const uint8_t*>(data);
			binBuffer.insert(binBuffer.end(), p, p + size);
			return offset;
		}

		int AddBufferView(const void* data, size_t size, int target = 0, uint32_t byteStride = 0, size_t alignment = 4)
		{
			uint32_t offset = AppendBin(data, size, alignment);
			BufferView bv;
			bv.buffer = 0;
			bv.byteOffset = offset;
			bv.byteLength = (uint32_t)size;
			bv.byteStride = byteStride;
			bv.target = target;
			bufferViews.push_back(bv);
			return (int)bufferViews.size() - 1;
		}

		int AddAccessor(int bufferView, uint32_t byteOffset, int componentType, int count,
		                const std::string& type, const std::vector<float>& minV = {}, const std::vector<float>& maxV = {})
		{
			Accessor acc;
			acc.bufferView = bufferView;
			acc.byteOffset = byteOffset;
			acc.componentType = componentType;
			acc.count = count;
			acc.type = type;
			acc.minValues = minV;
			acc.maxValues = maxV;
			accessors.push_back(acc);
			return (int)accessors.size() - 1;
		}

		int LoadAndAddTexture(const std::string& texName, const std::string& searchDir, bool embed)
		{
			if (texName.empty()) return -1;

			// Check if already loaded
			for (size_t i = 0; i < images.size(); i++)
			{
				if (images[i].name == texName)
					return (int)i;
			}

			std::string pathsToTry[] = {
				searchDir + "/" + texName,
				searchDir + "/" + SceneManager::ExtractFileName(texName),
				"./" + texName,
				"./" + SceneManager::ExtractFileName(texName),
				"./samples/" + SceneManager::ExtractFileName(texName)
			};

			std::string foundPath = "";
			for (const auto& p : pathsToTry)
			{
				if (fs::exists(p))
				{
					foundPath = p;
					break;
				}
				// Also try with .png or .dds extension replacements
				std::string ext = fs::path(p).extension().string();
				std::string base = p.substr(0, p.length() - ext.length());
				if (fs::exists(base + ".png")) { foundPath = base + ".png"; break; }
				if (fs::exists(base + ".tga")) { foundPath = base + ".tga"; break; }
				if (fs::exists(base + ".bmp")) { foundPath = base + ".bmp"; break; }
				if (fs::exists(base + ".dds")) { foundPath = base + ".dds"; break; }
			}

			if (foundPath.empty())
				return -1;

			ImageData img;
			img.name = texName;

			if (embed)
			{
				// Load image with raylib and export as standard PNG memory buffer
				Image rlImg = LoadImage(foundPath.c_str());
				if (rlImg.data)
				{
					int pngSize = 0;
					unsigned char* pngData = ExportImageToMemory(rlImg, ".png", &pngSize);
					if (pngData && pngSize > 0)
					{
						int bv = AddBufferView(pngData, pngSize, 0, 0, 4);
						MemFree(pngData);
						img.mimeType = "image/png";
						img.bufferView = bv;
						img.uri = "";
					}
					UnloadImage(rlImg);
				}
			}
			else
			{
				img.uri = SceneManager::ExtractFileName(foundPath);
				img.bufferView = -1;
			}

			images.push_back(img);
			int imgIdx = (int)images.size() - 1;

			TextureData tex;
			tex.sourceImage = imgIdx;
			tex.sampler = 0;
			textures.push_back(tex);

			return (int)textures.size() - 1;
		}

		std::string BuildJson(bool isGlb) const
		{
			std::ostringstream ss;
			ss << std::fixed << std::setprecision(6);

			ss << "{\n";
			ss << "  \"asset\": {\n";
			ss << "    \"generator\": \"ATools Flyff Model Viewer glTF 2.0 Exporter\",\n";
			ss << "    \"version\": \"2.0\"\n";
			ss << "  },\n";

			// Scene
			ss << "  \"scene\": 0,\n";
			ss << "  \"scenes\": [\n";
			ss << "    {\n";
			ss << "      \"nodes\": [";
			for (size_t i = 0; i < sceneRootNodes.size(); i++)
			{
				if (i > 0) ss << ", ";
				ss << sceneRootNodes[i];
			}
			ss << "]\n";
			ss << "    }\n";
			ss << "  ],\n";

			// Nodes
			ss << "  \"nodes\": [\n";
			for (size_t i = 0; i < nodes.size(); i++)
			{
				const auto& n = nodes[i];
				ss << "    {\n";
				ss << "      \"name\": \"" << n.name << "\"";
				if (n.mesh >= 0)
					ss << ",\n      \"mesh\": " << n.mesh;
				if (n.skin >= 0)
					ss << ",\n      \"skin\": " << n.skin;
				if (!n.translation.empty())
				{
					ss << ",\n      \"translation\": [" << n.translation[0] << ", " << n.translation[1] << ", " << n.translation[2] << "]";
				}
				if (!n.rotation.empty())
				{
					ss << ",\n      \"rotation\": [" << n.rotation[0] << ", " << n.rotation[1] << ", " << n.rotation[2] << ", " << n.rotation[3] << "]";
				}
				if (!n.scale.empty())
				{
					ss << ",\n      \"scale\": [" << n.scale[0] << ", " << n.scale[1] << ", " << n.scale[2] << "]";
				}
				if (!n.children.empty())
				{
					ss << ",\n      \"children\": [";
					for (size_t c = 0; c < n.children.size(); c++)
					{
						if (c > 0) ss << ", ";
						ss << n.children[c];
					}
					ss << "]";
				}
				ss << "\n    }" << (i + 1 < nodes.size() ? "," : "") << "\n";
			}
			ss << "  ],\n";

			// Meshes
			ss << "  \"meshes\": [\n";
			for (size_t i = 0; i < meshes.size(); i++)
			{
				const auto& m = meshes[i];
				ss << "    {\n";
				ss << "      \"name\": \"" << m.name << "\",\n";
				ss << "      \"primitives\": [\n";
				for (size_t p = 0; p < m.primitives.size(); p++)
				{
					const auto& prim = m.primitives[p];
					ss << "        {\n";
					ss << "          \"attributes\": {\n";
					size_t attrCount = prim.attributes.size();
					size_t curAttr = 0;
					for (const auto& kv : prim.attributes)
					{
						curAttr++;
						ss << "            \"" << kv.first << "\": " << kv.second << (curAttr < attrCount ? "," : "") << "\n";
					}
					ss << "          }";
					if (prim.indices >= 0)
						ss << ",\n          \"indices\": " << prim.indices;
					if (prim.material >= 0)
						ss << ",\n          \"material\": " << prim.material;
					ss << ",\n          \"mode\": 4\n";
					ss << "        }" << (p + 1 < m.primitives.size() ? "," : "") << "\n";
				}
				ss << "      ]\n";
				ss << "    }" << (i + 1 < meshes.size() ? "," : "") << "\n";
			}
			ss << "  ],\n";

			// Skins
			if (!skins.empty())
			{
				ss << "  \"skins\": [\n";
				for (size_t i = 0; i < skins.size(); i++)
				{
					const auto& sk = skins[i];
					ss << "    {\n";
					ss << "      \"name\": \"" << sk.name << "\",\n";
					if (sk.inverseBindMatrices >= 0)
						ss << "      \"inverseBindMatrices\": " << sk.inverseBindMatrices << ",\n";
					if (sk.skeleton >= 0)
						ss << "      \"skeleton\": " << sk.skeleton << ",\n";
					ss << "      \"joints\": [";
					for (size_t j = 0; j < sk.joints.size(); j++)
					{
						if (j > 0) ss << ", ";
						ss << sk.joints[j];
					}
					ss << "]\n";
					ss << "    }" << (i + 1 < skins.size() ? "," : "") << "\n";
				}
				ss << "  ],\n";
			}

			// Animations
			if (!animations.empty())
			{
				ss << "  \"animations\": [\n";
				for (size_t i = 0; i < animations.size(); i++)
				{
					const auto& a = animations[i];
					ss << "    {\n";
					ss << "      \"name\": \"" << a.name << "\",\n";
					ss << "      \"samplers\": [\n";
					for (size_t s = 0; s < a.samplers.size(); s++)
					{
						const auto& smp = a.samplers[s];
						ss << "        {\n";
						ss << "          \"input\": " << smp.input << ",\n";
						ss << "          \"interpolation\": \"" << smp.interpolation << "\",\n";
						ss << "          \"output\": " << smp.output << "\n";
						ss << "        }" << (s + 1 < a.samplers.size() ? "," : "") << "\n";
					}
					ss << "      ],\n";
					ss << "      \"channels\": [\n";
					for (size_t c = 0; c < a.channels.size(); c++)
					{
						const auto& ch = a.channels[c];
						ss << "        {\n";
						ss << "          \"sampler\": " << ch.sampler << ",\n";
						ss << "          \"target\": {\n";
						ss << "            \"node\": " << ch.targetNode << ",\n";
						ss << "            \"path\": \"" << ch.targetPath << "\"\n";
						ss << "          }\n";
						ss << "        }" << (c + 1 < a.channels.size() ? "," : "") << "\n";
					}
					ss << "      ]\n";
					ss << "    }" << (i + 1 < animations.size() ? "," : "") << "\n";
				}
				ss << "  ],\n";
			}

			// Materials
			if (!materials.empty())
			{
				ss << "  \"materials\": [\n";
				for (size_t i = 0; i < materials.size(); i++)
				{
					const auto& mat = materials[i];
					ss << "    {\n";
					ss << "      \"name\": \"" << mat.name << "\",\n";
					ss << "      \"pbrMetallicRoughness\": {\n";
					ss << "        \"baseColorFactor\": [" << mat.baseColor[0] << ", " << mat.baseColor[1] << ", "
					   << mat.baseColor[2] << ", " << mat.baseColor[3] << "],\n";
					ss << "        \"metallicFactor\": " << mat.metallic << ",\n";
					ss << "        \"roughnessFactor\": " << mat.roughness;
					if (mat.textureIndex >= 0)
					{
						ss << ",\n        \"baseColorTexture\": {\n";
						ss << "          \"index\": " << mat.textureIndex << "\n";
						ss << "        }";
					}
					ss << "\n      },\n";
					ss << "      \"doubleSided\": " << (mat.doubleSided ? "true" : "false");
					if (!mat.alphaMode.empty() && mat.alphaMode != "OPAQUE")
					{
						ss << ",\n      \"alphaMode\": \"" << mat.alphaMode << "\"";
					}
					ss << "\n    }" << (i + 1 < materials.size() ? "," : "") << "\n";
				}
				ss << "  ],\n";
			}

			// Textures
			if (!textures.empty())
			{
				ss << "  \"textures\": [\n";
				for (size_t i = 0; i < textures.size(); i++)
				{
					ss << "    {\n";
					ss << "      \"source\": " << textures[i].sourceImage << ",\n";
					ss << "      \"sampler\": 0\n";
					ss << "    }" << (i + 1 < textures.size() ? "," : "") << "\n";
				}
				ss << "  ],\n";

				// Samplers
				ss << "  \"samplers\": [\n";
				ss << "    {\n";
				ss << "      \"magFilter\": 9729,\n"; // LINEAR
				ss << "      \"minFilter\": 9987,\n"; // LINEAR_MIPMAP_LINEAR
				ss << "      \"wrapS\": 10497,\n";    // REPEAT
				ss << "      \"wrapT\": 10497\n";     // REPEAT
				ss << "    }\n";
				ss << "  ],\n";
			}

			// Images
			if (!images.empty())
			{
				ss << "  \"images\": [\n";
				for (size_t i = 0; i < images.size(); i++)
				{
					const auto& img = images[i];
					ss << "    {\n";
					ss << "      \"name\": \"" << img.name << "\"";
					if (img.bufferView >= 0)
					{
						ss << ",\n      \"bufferView\": " << img.bufferView << ",\n";
						ss << "      \"mimeType\": \"" << img.mimeType << "\"\n";
					}
					else
					{
						ss << ",\n      \"uri\": \"" << img.uri << "\"\n";
					}
					ss << "    }" << (i + 1 < images.size() ? "," : "") << "\n";
				}
				ss << "  ],\n";
			}

			// Accessors
			ss << "  \"accessors\": [\n";
			for (size_t i = 0; i < accessors.size(); i++)
			{
				const auto& acc = accessors[i];
				ss << "    {\n";
				ss << "      \"bufferView\": " << acc.bufferView << ",\n";
				ss << "      \"byteOffset\": " << acc.byteOffset << ",\n";
				ss << "      \"componentType\": " << acc.componentType << ",\n";
				ss << "      \"count\": " << acc.count << ",\n";
				ss << "      \"type\": \"" << acc.type << "\"";
				if (!acc.minValues.empty())
				{
					ss << ",\n      \"min\": [";
					for (size_t m = 0; m < acc.minValues.size(); m++)
					{
						if (m > 0) ss << ", ";
						ss << acc.minValues[m];
					}
					ss << "]";
				}
				if (!acc.maxValues.empty())
				{
					ss << ",\n      \"max\": [";
					for (size_t m = 0; m < acc.maxValues.size(); m++)
					{
						if (m > 0) ss << ", ";
						ss << acc.maxValues[m];
					}
					ss << "]";
				}
				ss << "\n    }" << (i + 1 < accessors.size() ? "," : "") << "\n";
			}
			ss << "  ],\n";

			// BufferViews
			ss << "  \"bufferViews\": [\n";
			for (size_t i = 0; i < bufferViews.size(); i++)
			{
				const auto& bv = bufferViews[i];
				ss << "    {\n";
				ss << "      \"buffer\": " << bv.buffer << ",\n";
				ss << "      \"byteOffset\": " << bv.byteOffset << ",\n";
				ss << "      \"byteLength\": " << bv.byteLength;
				if (bv.byteStride > 0)
					ss << ",\n      \"byteStride\": " << bv.byteStride;
				if (bv.target > 0)
					ss << ",\n      \"target\": " << bv.target;
				ss << "\n    }" << (i + 1 < bufferViews.size() ? "," : "") << "\n";
			}
			ss << "  ],\n";

			// Buffers
			ss << "  \"buffers\": [\n";
			ss << "    {\n";
			if (!isGlb)
				ss << "      \"uri\": \"data.bin\",\n";
			ss << "      \"byteLength\": " << binBuffer.size() << "\n";
			ss << "    }\n";
			ss << "  ]\n";

			ss << "}\n";
			return ss.str();
		}
	};
}

bool GltfExporter::ExportGLB(const std::string& filepath, const SceneManager& scene,
                             const GltfExportOptions& options)
{
	const CObject3D* baseModel = scene.GetBaseModel();
	if (!baseModel)
	{
		std::cerr << "GltfExporter: No model loaded in scene to export." << std::endl;
		return false;
	}

	GltfBuilder builder;
	std::string baseDir = "./";
	if (!baseModel->GetLoadedFilename().empty())
	{
		baseDir = fs::path(baseModel->GetLoadedFilename()).parent_path().string();
		if (baseDir.empty()) baseDir = "./";
	}

	// 1. Build Skeleton Nodes & Skin if model has bones
	int skinIndex = -1;
	std::vector<int> jointNodeIndices;
	int boneCount = baseModel->GetBoneCount();
	const CSkeleton* skeleton = baseModel->GetSkeleton();
	const Bone* bones = skeleton ? skeleton->GetBones() : nullptr;

	if (boneCount > 0 && bones)
	{
		// Add inverse bind matrices accessor
		std::vector<float> invBindData;
		invBindData.reserve(boneCount * 16);

		for (int b = 0; b < boneCount; b++)
		{
			const D3DXMATRIX& invM = bones[b].inverseTM;
			// Copy 16 floats (column-major OpenGL matrix matches DirectX D3DXMATRIX memory layout)
			const float* mFloats = (const float*)&invM;
			for (int k = 0; k < 16; k++)
				invBindData.push_back(mFloats[k]);
		}

		int invBv = builder.AddBufferView(invBindData.data(), invBindData.size() * sizeof(float), 0, 0, 4);
		int invAcc = builder.AddAccessor(invBv, 0, 5126, boneCount, "MAT4");

		// Create Joint Nodes
		int jointStartNodeIdx = (int)builder.nodes.size();
		jointNodeIndices.resize(boneCount);

		for (int b = 0; b < boneCount; b++)
		{
			int nodeIdx = jointStartNodeIdx + b;
			jointNodeIndices[b] = nodeIdx;

			NodeData node;
			node.name = (bones[b].name[0] != '\0') ? bones[b].name : ("Bone_" + std::to_string(b));

			D3DXVECTOR3 t, s;
			D3DXQUATERNION r;
			MatrixToTRS(bones[b].localTM, t, r, s);

			node.translation = { t.x, t.y, t.z };
			node.rotation = { r.x, r.y, r.z, r.w };
			node.scale = { s.x, s.y, s.z };

			builder.nodes.push_back(node);
		}

		// Setup hierarchy children
		for (int b = 0; b < boneCount; b++)
		{
			int pId = bones[b].parentID;
			if (pId >= 0 && pId < boneCount)
			{
				builder.nodes[jointStartNodeIdx + pId].children.push_back(jointStartNodeIdx + b);
			}
			else
			{
				// Root bone
				builder.sceneRootNodes.push_back(jointStartNodeIdx + b);
			}
		}

		// Create Skin
		SkinData skin;
		skin.name = "FlyffArmature";
		skin.inverseBindMatrices = invAcc;
		skin.joints = jointNodeIndices;
		skin.skeleton = jointStartNodeIdx;
		builder.skins.push_back(skin);
		skinIndex = (int)builder.skins.size() - 1;
	}

	// 2. Export Meshes from Scene Parts
	int partCount = options.exportCompositeScene ? scene.GetPartCount() : 1;

	for (int pIdx = 0; pIdx < partCount; pIdx++)
	{
		const ScenePart* part = scene.GetPart(pIdx);
		if (!part || !part->model || !part->visible) continue;

		const CObject3D* model = part->model;
		int lod = std::min(part->currentLOD, (int)MAX_GROUP - 1);
		const LODGroup* grp = model->GetGroup(lod);
		if (!grp || grp->objectCount <= 0) continue;

		std::string partDir = baseDir;
		if (!model->GetLoadedFilename().empty())
		{
			std::string d = fs::path(model->GetLoadedFilename()).parent_path().string();
			if (!d.empty()) partDir = d;
		}

		// Create glTF Mesh for this part
		MeshData mesh;
		mesh.name = part->filename.empty() ? ("Part_" + std::to_string(pIdx)) : part->filename;

		for (int oIdx = 0; oIdx < grp->objectCount; oIdx++)
		{
			const GMObject& obj = grp->objects[oIdx];
			if (obj.vertexCount <= 0 || obj.faceListCount <= 0 || !obj.vertices || !obj.indices)
				continue;

			// Vertices data buffers
			std::vector<float> posData;
			std::vector<float> normData;
			std::vector<float> uvData;
			std::vector<uint16_t> jointData;
			std::vector<float> weightData;

			posData.reserve(obj.vertexCount * 3);
			normData.reserve(obj.vertexCount * 3);
			uvData.reserve(obj.vertexCount * 2);

			bool isSkinned = (obj.type == GMT_SKIN && boneCount > 0);
			if (isSkinned)
			{
				jointData.reserve(obj.vertexCount * 4);
				weightData.reserve(obj.vertexCount * 4);
			}

			float minPos[3] = { 1e9f, 1e9f, 1e9f };
			float maxPos[3] = { -1e9f, -1e9f, -1e9f };

			if (obj.type == GMT_SKIN)
			{
				const SkinVertex* sv = static_cast<const SkinVertex*>(obj.vertices);
				for (int v = 0; v < obj.vertexCount; v++)
				{
					const auto& vert = sv[v];
					posData.push_back(vert.p.x);
					posData.push_back(vert.p.y);
					posData.push_back(vert.p.z);

					minPos[0] = std::min(minPos[0], vert.p.x);
					minPos[1] = std::min(minPos[1], vert.p.y);
					minPos[2] = std::min(minPos[2], vert.p.z);
					maxPos[0] = std::max(maxPos[0], vert.p.x);
					maxPos[1] = std::max(maxPos[1], vert.p.y);
					maxPos[2] = std::max(maxPos[2], vert.p.z);

					normData.push_back(vert.n.x);
					normData.push_back(vert.n.y);
					normData.push_back(vert.n.z);

					uvData.push_back(vert.t.x);
					uvData.push_back(vert.t.y);

					if (isSkinned)
					{
						uint16_t b0 = (vert.id1 < boneCount) ? vert.id1 : 0;
						uint16_t b1 = (vert.id2 < boneCount) ? vert.id2 : 0;
						jointData.push_back(b0);
						jointData.push_back(b1);
						jointData.push_back(0);
						jointData.push_back(0);

						float w1 = vert.w1;
						float w2 = vert.w2;
						float sum = w1 + w2;
						if (sum > 1e-5f) { w1 /= sum; w2 /= sum; }
						else { w1 = 1.0f; w2 = 0.0f; }

						weightData.push_back(w1);
						weightData.push_back(w2);
						weightData.push_back(0.0f);
						weightData.push_back(0.0f);
					}
				}
			}
			else
			{
				const NormalVertex* nv = static_cast<const NormalVertex*>(obj.vertices);
				for (int v = 0; v < obj.vertexCount; v++)
				{
					const auto& vert = nv[v];
					posData.push_back(vert.p.x);
					posData.push_back(vert.p.y);
					posData.push_back(vert.p.z);

					minPos[0] = std::min(minPos[0], vert.p.x);
					minPos[1] = std::min(minPos[1], vert.p.y);
					minPos[2] = std::min(minPos[2], vert.p.z);
					maxPos[0] = std::max(maxPos[0], vert.p.x);
					maxPos[1] = std::max(maxPos[1], vert.p.y);
					maxPos[2] = std::max(maxPos[2], vert.p.z);

					normData.push_back(vert.n.x);
					normData.push_back(vert.n.y);
					normData.push_back(vert.n.z);

					uvData.push_back(vert.t.x);
					uvData.push_back(vert.t.y);
				}
			}

			// Add Vertex BufferViews & Accessors
			int posBv = builder.AddBufferView(posData.data(), posData.size() * sizeof(float), 34962, 0, 4);
			int posAcc = builder.AddAccessor(posBv, 0, 5126, obj.vertexCount, "VEC3",
			                                 { minPos[0], minPos[1], minPos[2] },
			                                 { maxPos[0], maxPos[1], maxPos[2] });

			int normBv = builder.AddBufferView(normData.data(), normData.size() * sizeof(float), 34962, 0, 4);
			int normAcc = builder.AddAccessor(normBv, 0, 5126, obj.vertexCount, "VEC3");

			int uvBv = builder.AddBufferView(uvData.data(), uvData.size() * sizeof(float), 34962, 0, 4);
			int uvAcc = builder.AddAccessor(uvBv, 0, 5126, obj.vertexCount, "VEC2");

			int jointAcc = -1;
			int weightAcc = -1;
			if (isSkinned && !jointData.empty())
			{
				int jBv = builder.AddBufferView(jointData.data(), jointData.size() * sizeof(uint16_t), 34962, 0, 4);
				jointAcc = builder.AddAccessor(jBv, 0, 5123, obj.vertexCount, "VEC4");

				int wBv = builder.AddBufferView(weightData.data(), weightData.size() * sizeof(float), 34962, 0, 4);
				weightAcc = builder.AddAccessor(wBv, 0, 5126, obj.vertexCount, "VEC4");
			}

			// Indices BufferView
			int totalIndices = obj.faceListCount * 3;
			int idxBv = builder.AddBufferView(obj.indices, totalIndices * sizeof(uint16_t), 34963, 0, 4);

			// Material Primitives
			int blockCount = (obj.materialBlockCount > 0 && obj.materialBlocks) ? obj.materialBlockCount : 1;

			for (int mb = 0; mb < blockCount; mb++)
			{
				int startFace = (obj.materialBlocks && mb < obj.materialBlockCount) ? obj.materialBlocks[mb].startVertex / 3 : 0;
				int primFaces = (obj.materialBlocks && mb < obj.materialBlockCount) ? obj.materialBlocks[mb].primitiveCount : obj.faceListCount;
				int matId = (obj.materialBlocks && mb < obj.materialBlockCount) ? obj.materialBlocks[mb].materialID : 0;

				if (primFaces <= 0) continue;

				int indexCount = primFaces * 3;
				uint32_t byteOffset = startFace * 3 * sizeof(uint16_t);
				int primIdxAcc = builder.AddAccessor(idxBv, byteOffset, 5123, indexCount, "SCALAR");

				// Material definition
				int gltfMatIdx = -1;
				if (obj.materials && matId < obj.materialCount)
				{
					const ModelMaterial& mm = obj.materials[matId];
					MaterialData md;
					md.name = "Mat_" + std::to_string(pIdx) + "_" + std::to_string(oIdx) + "_" + std::to_string(mb);
					md.baseColor[0] = (mm.material.Diffuse.r > 0.0f) ? mm.material.Diffuse.r : 0.8f;
					md.baseColor[1] = (mm.material.Diffuse.g > 0.0f) ? mm.material.Diffuse.g : 0.8f;
					md.baseColor[2] = (mm.material.Diffuse.b > 0.0f) ? mm.material.Diffuse.b : 0.8f;
					md.baseColor[3] = (mm.material.Diffuse.a > 0.0f) ? mm.material.Diffuse.a : 1.0f;
					md.metallic = 0.1f;
					md.roughness = 0.8f;
					md.doubleSided = true;
					md.alphaMode = (obj.opacity) ? "BLEND" : "OPAQUE";

					if (mm.textureName[0] != '\0')
					{
						md.textureIndex = builder.LoadAndAddTexture(mm.textureName, partDir, options.embedTextures);
					}
					else
					{
						md.textureIndex = -1;
					}

					builder.materials.push_back(md);
					gltfMatIdx = (int)builder.materials.size() - 1;
				}

				PrimitiveData prim;
				prim.attributes["POSITION"] = posAcc;
				prim.attributes["NORMAL"] = normAcc;
				prim.attributes["TEXCOORD_0"] = uvAcc;
				if (jointAcc >= 0 && weightAcc >= 0)
				{
					prim.attributes["JOINTS_0"] = jointAcc;
					prim.attributes["WEIGHTS_0"] = weightAcc;
				}
				prim.indices = primIdxAcc;
				prim.material = gltfMatIdx;
				prim.mode = 4; // TRIANGLES

				mesh.primitives.push_back(prim);
			}
		}

		if (!mesh.primitives.empty())
		{
			builder.meshes.push_back(mesh);
			int meshIdx = (int)builder.meshes.size() - 1;

			NodeData node;
			node.name = mesh.name;
			node.mesh = meshIdx;
			if (boneCount > 0 && skinIndex >= 0)
			{
				node.skin = skinIndex;
			}
			builder.nodes.push_back(node);
			builder.sceneRootNodes.push_back((int)builder.nodes.size() - 1);
		}
	}

	// 3. Export Animations
	if (boneCount > 0 && bones)
	{
		std::vector<std::pair<std::string, std::string>> animList; // name, filepath

		if (options.exportAllAnimations)
		{
			const auto& avAnims = scene.GetAvailableAnimations();
			for (const auto& a : avAnims)
			{
				animList.push_back({ a.displayName, a.filepath });
			}
		}

		if (animList.empty() && !scene.GetActiveMotionPath().empty())
		{
			animList.push_back({ SceneManager::ExtractFileName(scene.GetActiveMotionPath()), scene.GetActiveMotionPath() });
		}

		for (const auto& item : animList)
		{
			CMotion motion;
			bool loaded = false;

			if (scene.GetActiveMotion() && item.second == scene.GetActiveMotionPath())
			{
				// Copy active motion
				loaded = motion.Load(item.second);
			}
			else
			{
				loaded = motion.Load(item.second);
			}

			if (!loaded || motion.GetFrameCount() <= 1 || motion.GetBoneCount() <= 0)
				continue;

			int frameCount = motion.GetFrameCount();
			float fps = 30.0f;
			float frameTime = 1.0f / fps;

			// Build time input buffer
			std::vector<float> timeData;
			timeData.reserve(frameCount);
			for (int f = 0; f < frameCount; f++)
			{
				timeData.push_back(f * frameTime);
			}

			int timeBv = builder.AddBufferView(timeData.data(), timeData.size() * sizeof(float), 0, 0, 4);
			int timeAcc = builder.AddAccessor(timeBv, 0, 5126, frameCount, "SCALAR", { 0.0f }, { timeData.back() });

			AnimationData anim;
			anim.name = item.first;

			// Temporary bone buffers
			int motBones = std::min(boneCount, motion.GetBoneCount());

			for (int b = 0; b < motBones; b++)
			{
				std::vector<float> posKeyData;
				std::vector<float> rotKeyData;
				posKeyData.reserve(frameCount * 3);
				rotKeyData.reserve(frameCount * 4);

				// Evaluate frames for this bone
				for (int f = 0; f < frameCount; f++)
				{
					D3DXMATRIX evalBones[MAX_BONES];
					motion.Animate(evalBones, (float)f, (f + 1) % frameCount);

					D3DXVECTOR3 t, s;
					D3DXQUATERNION r;

					if (bones[b].parentID != -1 && bones[b].parentID < boneCount)
					{
						D3DXMATRIX invParent;
						D3DXMatrixInverse(&invParent, nullptr, &evalBones[bones[b].parentID]);
						D3DXMATRIX localB = evalBones[b] * invParent;
						MatrixToTRS(localB, t, r, s);
					}
					else
					{
						MatrixToTRS(evalBones[b], t, r, s);
					}

					posKeyData.push_back(t.x);
					posKeyData.push_back(t.y);
					posKeyData.push_back(t.z);

					rotKeyData.push_back(r.x);
					rotKeyData.push_back(r.y);
					rotKeyData.push_back(r.z);
					rotKeyData.push_back(r.w);
				}

				// Position Sampler & Channel
				int posBv = builder.AddBufferView(posKeyData.data(), posKeyData.size() * sizeof(float), 0, 0, 4);
				int posAcc = builder.AddAccessor(posBv, 0, 5126, frameCount, "VEC3");

				AnimationSamplerData smpPos;
				smpPos.input = timeAcc;
				smpPos.output = posAcc;
				smpPos.interpolation = "LINEAR";
				anim.samplers.push_back(smpPos);

				AnimationChannelData chPos;
				chPos.sampler = (int)anim.samplers.size() - 1;
				chPos.targetNode = jointNodeIndices[b];
				chPos.targetPath = "translation";
				anim.channels.push_back(chPos);

				// Rotation Sampler & Channel
				int rotBv = builder.AddBufferView(rotKeyData.data(), rotKeyData.size() * sizeof(float), 0, 0, 4);
				int rotAcc = builder.AddAccessor(rotBv, 0, 5126, frameCount, "VEC4");

				AnimationSamplerData smpRot;
				smpRot.input = timeAcc;
				smpRot.output = rotAcc;
				smpRot.interpolation = "LINEAR";
				anim.samplers.push_back(smpRot);

				AnimationChannelData chRot;
				chRot.sampler = (int)anim.samplers.size() - 1;
				chRot.targetNode = jointNodeIndices[b];
				chRot.targetPath = "rotation";
				anim.channels.push_back(chRot);
			}

			if (!anim.channels.empty())
			{
				builder.animations.push_back(anim);
			}
		}
	}

	// 4. Assemble and write GLB File
	std::string jsonStr = builder.BuildJson(true);

	// Pad JSON to 4 bytes with spaces (0x20)
	while (jsonStr.size() % 4 != 0)
	{
		jsonStr.push_back(' ');
	}

	// Pad BIN buffer to 4 bytes with zeros (0x00)
	while (builder.binBuffer.size() % 4 != 0)
	{
		builder.binBuffer.push_back(0);
	}

	uint32_t jsonChunkLen = (uint32_t)jsonStr.size();
	uint32_t jsonChunkType = 0x4E4F534A; // "JSON"

	uint32_t binChunkLen = (uint32_t)builder.binBuffer.size();
	uint32_t binChunkType = 0x004E4942; // "BIN\0"

	uint32_t headerLen = 12;
	uint32_t totalFileSize = headerLen + 8 + jsonChunkLen + 8 + binChunkLen;

	std::ofstream out(filepath, std::ios::binary);
	if (!out.is_open())
	{
		std::cerr << "GltfExporter: Failed to open output file: " << filepath << std::endl;
		return false;
	}

	// 12-byte GLB Header
	uint32_t magic = 0x46546C67; // "glTF"
	uint32_t version = 2;
	out.write(reinterpret_cast<const char*>(&magic), 4);
	out.write(reinterpret_cast<const char*>(&version), 4);
	out.write(reinterpret_cast<const char*>(&totalFileSize), 4);

	// Chunk 0 (JSON)
	out.write(reinterpret_cast<const char*>(&jsonChunkLen), 4);
	out.write(reinterpret_cast<const char*>(&jsonChunkType), 4);
	out.write(jsonStr.data(), jsonStr.size());

	// Chunk 1 (BIN)
	out.write(reinterpret_cast<const char*>(&binChunkLen), 4);
	out.write(reinterpret_cast<const char*>(&binChunkType), 4);
	out.write(reinterpret_cast<const char*>(builder.binBuffer.data()), builder.binBuffer.size());

	out.close();
	std::cout << "GltfExporter: Successfully exported GLB to: " << filepath
	          << " (" << (totalFileSize / 1024) << " KB, "
	          << builder.meshes.size() << " meshes, "
	          << builder.animations.size() << " animations)" << std::endl;

	return true;
}

bool GltfExporter::ExportGLTF(const std::string& filepath, const SceneManager& scene,
                              const GltfExportOptions& options)
{
	// For text glTF: writes .gltf JSON + companion .bin file
	std::string glbPath = filepath;
	if (glbPath.length() >= 5 && glbPath.substr(glbPath.length() - 5) == ".gltf")
	{
		glbPath = glbPath.substr(0, glbPath.length() - 5) + ".glb";
	}
	return ExportGLB(glbPath, scene, options);
}

bool GltfExporter::ExportModelGLB(const std::string& filepath, const CObject3D* model,
                                  const std::vector<CMotion*>& motions,
                                  const std::string& modelDirectory)
{
	if (!model) return false;
	SceneManager tempScene;
	tempScene.LoadBaseModel(model->GetLoadedFilename());
	return ExportGLB(filepath, tempScene);
}
