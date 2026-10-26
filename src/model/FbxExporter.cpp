#include "FbxExporter.h"
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
#include <cstdint>

namespace fs = std::filesystem;

namespace
{
	const int64_t KTIME_ONE_SECOND = 46186158000LL;
	const int64_t KTIME_PER_FRAME_30FPS = 1539538600LL; // KTIME_ONE_SECOND / 30

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

	void MatrixToEulerXYZ(const D3DXMATRIX& m, float& outRx, float& outRy, float& outRz)
	{
		float sx = std::sqrt(m._11 * m._11 + m._12 * m._12 + m._13 * m._13);
		float sy = std::sqrt(m._21 * m._21 + m._22 * m._22 + m._23 * m._23);
		float sz = std::sqrt(m._31 * m._31 + m._32 * m._32 + m._33 * m._33);
		if (sx < 1e-6f) sx = 1.0f;
		if (sy < 1e-6f) sy = 1.0f;
		if (sz < 1e-6f) sz = 1.0f;

		float r11 = m._11 / sx, r12 = m._12 / sx, r13 = m._13 / sx;
		float r21 = m._21 / sy, r22 = m._22 / sy, r23 = m._23 / sy;
		float r33 = m._33 / sz;

		float rad2deg = 57.29577951308232f;

		if (r13 < 0.99999f)
		{
			if (r13 > -0.99999f)
			{
				outRy = std::asin(r13);
				outRx = std::atan2(-r23, r33);
				outRz = std::atan2(-r12, r11);
			}
			else
			{
				outRy = -3.14159265f * 0.5f;
				outRx = -std::atan2(r21, r22);
				outRz = 0.0f;
			}
		}
		else
		{
			outRy = 3.14159265f * 0.5f;
			outRx = std::atan2(r21, r22);
			outRz = 0.0f;
		}

		outRx *= rad2deg;
		outRy *= rad2deg;
		outRz *= rad2deg;
	}

	std::string MatrixToFbxString(const D3DXMATRIX& mat)
	{
		// FBX 4x4 row-major matrix string
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(6);
		// Transpose DirectX matrix for standard FBX
		ss << mat._11 << "," << mat._21 << "," << mat._31 << "," << mat._41 << ","
		   << mat._12 << "," << mat._22 << "," << mat._32 << "," << mat._42 << ","
		   << mat._13 << "," << mat._23 << "," << mat._33 << "," << mat._43 << ","
		   << mat._14 << "," << mat._24 << "," << mat._34 << "," << mat._44;
		return ss.str();
	}

	std::string SanitizeFbxName(const std::string& name)
	{
		std::string s = name;
		for (char& c : s)
		{
			if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
				c = '_';
		}
		if (s.empty()) s = "object";
		return s;
	}
}

bool FbxExporter::ExportFBX(const std::string& filepath, const SceneManager& scene,
                            const FbxExportOptions& options)
{
	const CObject3D* baseModel = scene.GetBaseModel();
	if (!baseModel)
	{
		std::cerr << "FbxExporter: No base model loaded in scene to export." << std::endl;
		return false;
	}

	std::string outDir = fs::path(filepath).parent_path().string();
	if (outDir.empty()) outDir = "./";

	std::string baseDir = "./";
	if (!baseModel->GetLoadedFilename().empty())
	{
		baseDir = fs::path(baseModel->GetLoadedFilename()).parent_path().string();
		if (baseDir.empty()) baseDir = "./";
	}

	std::ofstream out(filepath);
	if (!out.is_open())
	{
		std::cerr << "FbxExporter: Failed to open output file: " << filepath << std::endl;
		return false;
	}

	int64_t idCounter = 100000000;
	auto nextId = [&]() -> int64_t { return ++idCounter; };

	int boneCount = baseModel->GetBoneCount();
	const CSkeleton* skeleton = baseModel->GetSkeleton();
	const Bone* bones = skeleton ? skeleton->GetBones() : nullptr;

	std::vector<int64_t> boneNodeIds(boneCount);
	std::vector<std::string> boneNames(boneCount);

	for (int b = 0; b < boneCount; b++)
	{
		boneNodeIds[b] = nextId();
		std::string bName = (bones && bones[b].name[0] != '\0') ? bones[b].name : ("Bone_" + std::to_string(b));
		boneNames[b] = SanitizeFbxName(bName);
	}

	// 1. Collect Materials & Textures
	struct FbxMatInfo
	{
		int64_t matId;
		int64_t texId;
		std::string matName;
		std::string texFilename;
		float diffuse[3];
		float ambient[3];
	};

	std::vector<FbxMatInfo> materials;
	std::map<std::string, int64_t> textureMap;

	int partCount = options.exportCompositeScene ? scene.GetPartCount() : 1;

	for (int pIdx = 0; pIdx < partCount; pIdx++)
	{
		const ScenePart* part = scene.GetPart(pIdx);
		if (!part || !part->model || !part->visible) continue;

		const CObject3D* model = part->model;
		int lod = std::min(part->currentLOD, (int)MAX_GROUP - 1);
		const LODGroup* grp = model->GetGroup(lod);
		if (!grp) continue;

		std::string partDir = baseDir;
		if (!model->GetLoadedFilename().empty())
		{
			std::string d = fs::path(model->GetLoadedFilename()).parent_path().string();
			if (!d.empty()) partDir = d;
		}

		for (int oIdx = 0; oIdx < grp->objectCount; oIdx++)
		{
			const GMObject& obj = grp->objects[oIdx];
			for (int mIdx = 0; mIdx < obj.materialCount; mIdx++)
			{
				const ModelMaterial& mm = obj.materials[mIdx];
				FbxMatInfo mi;
				mi.matId = nextId();
				mi.texId = 0;
				mi.matName = "Mat_p" + std::to_string(pIdx) + "_o" + std::to_string(oIdx) + "_m" + std::to_string(mIdx);

				mi.diffuse[0] = (mm.material.Diffuse.r > 0.0f) ? mm.material.Diffuse.r : 0.8f;
				mi.diffuse[1] = (mm.material.Diffuse.g > 0.0f) ? mm.material.Diffuse.g : 0.8f;
				mi.diffuse[2] = (mm.material.Diffuse.b > 0.0f) ? mm.material.Diffuse.b : 0.8f;

				mi.ambient[0] = (mm.material.Ambient.r > 0.0f) ? mm.material.Ambient.r : 0.2f;
				mi.ambient[1] = (mm.material.Ambient.g > 0.0f) ? mm.material.Ambient.g : 0.2f;
				mi.ambient[2] = (mm.material.Ambient.b > 0.0f) ? mm.material.Ambient.b : 0.2f;

				if (mm.textureName[0] != '\0')
				{
					std::string rawTexName = mm.textureName;
					if (textureMap.find(rawTexName) == textureMap.end())
					{
						int64_t tId = nextId();
						textureMap[rawTexName] = tId;

						if (options.copyTextures)
						{
							std::string searchPaths[] = {
								partDir + "/" + rawTexName,
								partDir + "/" + SceneManager::ExtractFileName(rawTexName),
								"./" + rawTexName,
								"./" + SceneManager::ExtractFileName(rawTexName),
								"./samples/" + SceneManager::ExtractFileName(rawTexName)
							};

							std::string foundPath = "";
							for (const auto& sp : searchPaths)
							{
								if (fs::exists(sp)) { foundPath = sp; break; }
								std::string ext = fs::path(sp).extension().string();
								std::string base = sp.substr(0, sp.length() - ext.length());
								if (fs::exists(base + ".png")) { foundPath = base + ".png"; break; }
								if (fs::exists(base + ".tga")) { foundPath = base + ".tga"; break; }
								if (fs::exists(base + ".bmp")) { foundPath = base + ".bmp"; break; }
								if (fs::exists(base + ".dds")) { foundPath = base + ".dds"; break; }
							}

							if (!foundPath.empty())
							{
								std::string baseName = fs::path(foundPath).stem().string();
								std::string destPng = outDir + "/" + baseName + ".png";
								if (!fs::exists(destPng))
								{
									Image rlImg = LoadImage(foundPath.c_str());
									if (rlImg.data)
									{
										ExportImage(rlImg, destPng.c_str());
										UnloadImage(rlImg);
									}
								}
							}
						}
					}
					mi.texId = textureMap[rawTexName];
					mi.texFilename = fs::path(rawTexName).stem().string() + ".png";
				}

				materials.push_back(mi);
			}
		}
	}

	// 2. Prepare Meshes and Deformers
	struct FbxGeomEntry
	{
		int partIdx;
		int objIdx;
		int64_t geomId;
		int64_t modelId;
		int64_t skinId;
		bool isSkinned;
		std::string name;
		std::vector<int64_t> clusterIds; // per bone
		std::vector<int64_t> matIds;
	};

	std::vector<FbxGeomEntry> meshes;

	for (int pIdx = 0; pIdx < partCount; pIdx++)
	{
		const ScenePart* part = scene.GetPart(pIdx);
		if (!part || !part->model || !part->visible) continue;

		const CObject3D* model = part->model;
		int lod = std::min(part->currentLOD, (int)MAX_GROUP - 1);
		const LODGroup* grp = model->GetGroup(lod);
		if (!grp || grp->objectCount <= 0) continue;

		for (int oIdx = 0; oIdx < grp->objectCount; oIdx++)
		{
			const GMObject& obj = grp->objects[oIdx];
			if (obj.vertexCount <= 0 || obj.faceListCount <= 0 || !obj.vertices || !obj.indices)
				continue;

			FbxGeomEntry ge;
			ge.partIdx = pIdx;
			ge.objIdx = oIdx;
			ge.geomId = nextId();
			ge.modelId = nextId();
			ge.name = SanitizeFbxName(part->filename.empty() ? ("Part_" + std::to_string(pIdx)) : part->filename) + "_Mesh" + std::to_string(oIdx);
			ge.isSkinned = (obj.type == GMT_SKIN && boneCount > 0);

			if (ge.isSkinned)
			{
				ge.skinId = nextId();
				ge.clusterIds.resize(boneCount);
				for (int b = 0; b < boneCount; b++)
					ge.clusterIds[b] = nextId();
			}
			else
			{
				ge.skinId = 0;
			}

			meshes.push_back(ge);
		}
	}

	// 3. Prepare Animation Data
	struct FbxAnimStackInfo
	{
		int64_t stackId;
		int64_t layerId;
		std::string name;
		std::string filepath;
		int frameCount;
		int64_t stopKTime;
		struct BoneCurveNodes
		{
			int64_t tNodeId;
			int64_t rNodeId;
			int64_t txCurveId, tyCurveId, tzCurveId;
			int64_t rxCurveId, ryCurveId, rzCurveId;
		};
		std::vector<BoneCurveNodes> boneCurves;
	};

	std::vector<FbxAnimStackInfo> animStacks;

	if (boneCount > 0 && bones)
	{
		std::vector<std::pair<std::string, std::string>> animList;

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
			if (!motion.Load(item.second) || motion.GetFrameCount() <= 1 || motion.GetBoneCount() <= 0)
				continue;

			FbxAnimStackInfo asi;
			asi.stackId = nextId();
			asi.layerId = nextId();
			asi.name = SanitizeFbxName(item.first);
			asi.filepath = item.second;
			asi.frameCount = motion.GetFrameCount();
			asi.stopKTime = (asi.frameCount - 1) * KTIME_PER_FRAME_30FPS;

			int motBones = std::min(boneCount, motion.GetBoneCount());
			asi.boneCurves.resize(motBones);

			for (int b = 0; b < motBones; b++)
			{
				asi.boneCurves[b].tNodeId = nextId();
				asi.boneCurves[b].rNodeId = nextId();
				asi.boneCurves[b].txCurveId = nextId();
				asi.boneCurves[b].tyCurveId = nextId();
				asi.boneCurves[b].tzCurveId = nextId();
				asi.boneCurves[b].rxCurveId = nextId();
				asi.boneCurves[b].ryCurveId = nextId();
				asi.boneCurves[b].rzCurveId = nextId();
			}

			animStacks.push_back(asi);
		}
	}

	// 4. Output FBX ASCII Content
	out << "; FBX 7.4.0 project file\n";
	out << "; ----------------------------------------------------\n\n";

	out << "FBXHeaderExtension:  {\n";
	out << "	FBXHeaderVersion: 1003\n";
	out << "	FBXVersion: 7400\n";
	out << "	CreationTimeStamp:  {\n";
	out << "		Version: 1000\n";
	out << "		Year: 2026\n";
	out << "		Month: 10\n";
	out << "		Day: 26\n";
	out << "		Hour: 0\n";
	out << "		Minute: 0\n";
	out << "		Second: 0\n";
	out << "		Millisecond: 0\n";
	out << "	}\n";
	out << "	Creator: \"ATools Flyff Model Viewer FBX Exporter\"\n";
	out << "}\n\n";

	out << "GlobalSettings:  {\n";
	out << "	Version: 1000\n";
	out << "	Properties70:  {\n";
	out << "		P: \"UpAxis\", \"int\", \"Integer\", \"\",1\n";
	out << "		P: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n";
	out << "		P: \"FrontAxis\", \"int\", \"Integer\", \"\",2\n";
	out << "		P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",1\n";
	out << "		P: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n";
	out << "		P: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n";
	out << "		P: \"OriginalUpAxis\", \"int\", \"Integer\", \"\",1\n";
	out << "		P: \"OriginalUpAxisSign\", \"int\", \"Integer\", \"\",1\n";
	out << "		P: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1.0\n";
	out << "		P: \"OriginalUnitScaleFactor\", \"double\", \"Number\", \"\",1.0\n";
	out << "	}\n";
	out << "}\n\n";

	out << "Documents:  {\n";
	out << "	Count: 1\n";
	out << "	Document: 1000000, \"Scene\", \"Scene\" {\n";
	out << "		Properties70:  {\n";
	out << "			P: \"SourceObject\", \"object\", \"\", \"\"\n";
	if (!animStacks.empty())
		out << "			P: \"ActiveAnimStackName\", \"KString\", \"\", \"\", \"" << animStacks[0].name << "\"\n";
	out << "		}\n";
	out << "		RootNode: 0\n";
	out << "	}\n";
	out << "}\n\n";

	out << "References:  {\n";
	out << "}\n\n";

	// Definitions Section
	int totalObjects = (int)meshes.size() * 2 + boneCount + (int)materials.size() + (int)textureMap.size();
	for (const auto& m : meshes)
	{
		if (m.isSkinned) totalObjects += 1 + boneCount;
	}
	for (const auto& a : animStacks)
	{
		totalObjects += 2 + (int)a.boneCurves.size() * 8; // Stack + Layer + CurveNodes + Curves
	}

	out << "Definitions:  {\n";
	out << "	Version: 100\n";
	out << "	Count: " << totalObjects << "\n";
	out << "	ObjectType: \"GlobalSettings\" {\n";
	out << "		Count: 1\n";
	out << "	}\n";
	out << "	ObjectType: \"Model\" {\n";
	out << "		Count: " << ((int)meshes.size() + boneCount) << "\n";
	out << "	}\n";
	out << "	ObjectType: \"Geometry\" {\n";
	out << "		Count: " << meshes.size() << "\n";
	out << "	}\n";
	out << "	ObjectType: \"Material\" {\n";
	out << "		Count: " << materials.size() << "\n";
	out << "	}\n";
	out << "	ObjectType: \"Texture\" {\n";
	out << "		Count: " << textureMap.size() << "\n";
	out << "	}\n";
	out << "	ObjectType: \"Deformer\" {\n";
	int defCount = 0;
	for (const auto& m : meshes) if (m.isSkinned) defCount += 1 + boneCount;
	out << "		Count: " << defCount << "\n";
	out << "	}\n";
	if (!animStacks.empty())
	{
		out << "	ObjectType: \"AnimationStack\" {\n";
		out << "		Count: " << animStacks.size() << "\n";
		out << "	}\n";
		out << "	ObjectType: \"AnimationLayer\" {\n";
		out << "		Count: " << animStacks.size() << "\n";
		out << "	}\n";
	}
	out << "}\n\n";

	// Objects Section
	out << "Objects:  {\n";

	// 4.1 Geometries
	for (const auto& ge : meshes)
	{
		const ScenePart* part = scene.GetPart(ge.partIdx);
		const LODGroup* grp = part->model->GetGroup(part->currentLOD);
		const GMObject& obj = grp->objects[ge.objIdx];

		out << "	Geometry: " << ge.geomId << ", \"Geometry::" << ge.name << "\", \"Mesh\" {\n";

		// Vertices
		out << "		Vertices: *" << (obj.vertexCount * 3) << " {\n";
		out << "			a: ";
		out << std::fixed << std::setprecision(6);
		for (int v = 0; v < obj.vertexCount; v++)
		{
			D3DXVECTOR3 p = (obj.type == GMT_SKIN) ? static_cast<const SkinVertex*>(obj.vertices)[v].p
			                                       : static_cast<const NormalVertex*>(obj.vertices)[v].p;
			if (v > 0) out << ",";
			out << p.x << "," << p.y << "," << p.z;
		}
		out << "\n		}\n";

		// PolygonVertexIndex
		int totalIndices = obj.faceListCount * 3;
		out << "		PolygonVertexIndex: *" << totalIndices << " {\n";
		out << "			a: ";
		for (int f = 0; f < obj.faceListCount; f++)
		{
			int i0 = obj.indices[f * 3];
			int i1 = obj.indices[f * 3 + 1];
			int i2 = obj.indices[f * 3 + 2];
			int lastIdx = -i2 - 1; // Negative complement marks polygon end in FBX

			if (f > 0) out << ",";
			out << i0 << "," << i1 << "," << lastIdx;
		}
		out << "\n		}\n";

		// Normals (ByPolygonVertex)
		out << "		LayerElementNormal: 0 {\n";
		out << "			Version: 101\n";
		out << "			Name: \"\"\n";
		out << "			MappingInformationType: \"ByPolygonVertex\"\n";
		out << "			ReferenceInformationType: \"Direct\"\n";
		out << "			Normals: *" << (totalIndices * 3) << " {\n";
		out << "				a: ";
		for (int f = 0; f < obj.faceListCount; f++)
		{
			for (int k = 0; k < 3; k++)
			{
				int vIdx = obj.indices[f * 3 + k];
				D3DXVECTOR3 n = (obj.type == GMT_SKIN) ? static_cast<const SkinVertex*>(obj.vertices)[vIdx].n
				                                       : static_cast<const NormalVertex*>(obj.vertices)[vIdx].n;
				if (f > 0 || k > 0) out << ",";
				out << n.x << "," << n.y << "," << n.z;
			}
		}
		out << "\n			}\n";
		out << "		}\n";

		// UVs (ByPolygonVertex)
		out << "		LayerElementUV: 0 {\n";
		out << "			Version: 101\n";
		out << "			Name: \"UVMap\"\n";
		out << "			MappingInformationType: \"ByPolygonVertex\"\n";
		out << "			ReferenceInformationType: \"Direct\"\n";
		out << "			UV: *" << (totalIndices * 2) << " {\n";
		out << "				a: ";
		for (int f = 0; f < obj.faceListCount; f++)
		{
			for (int k = 0; k < 3; k++)
			{
				int vIdx = obj.indices[f * 3 + k];
				D3DXVECTOR2 t = (obj.type == GMT_SKIN) ? static_cast<const SkinVertex*>(obj.vertices)[vIdx].t
				                                       : static_cast<const NormalVertex*>(obj.vertices)[vIdx].t;
				if (f > 0 || k > 0) out << ",";
				out << t.x << "," << t.y;
			}
		}
		out << "\n			}\n";
		out << "		}\n";

		// Material per Polygon
		out << "		LayerElementMaterial: 0 {\n";
		out << "			Version: 101\n";
		out << "			Name: \"\"\n";
		out << "			MappingInformationType: \"ByPolygon\"\n";
		out << "			ReferenceInformationType: \"IndexToDirect\"\n";
		out << "			Materials: *" << obj.faceListCount << " {\n";
		out << "				a: ";
		for (int f = 0; f < obj.faceListCount; f++)
		{
			int matId = 0;
			if (obj.materialBlocks && obj.materialBlockCount > 0)
			{
				for (int mb = 0; mb < obj.materialBlockCount; mb++)
				{
					int sFace = obj.materialBlocks[mb].startVertex / 3;
					int pFaces = obj.materialBlocks[mb].primitiveCount;
					if (f >= sFace && f < sFace + pFaces)
					{
						matId = obj.materialBlocks[mb].materialID;
						break;
					}
				}
			}
			if (f > 0) out << ",";
			out << matId;
		}
		out << "\n			}\n";
		out << "		}\n";

		// Layer definition
		out << "		Layer: 0 {\n";
		out << "			Version: 100\n";
		out << "			LayerElement:  {\n";
		out << "				Type: \"LayerElementNormal\"\n";
		out << "				TypedIndex: 0\n";
		out << "			}\n";
		out << "			LayerElement:  {\n";
		out << "				Type: \"LayerElementMaterial\"\n";
		out << "				TypedIndex: 0\n";
		out << "			}\n";
		out << "			LayerElement:  {\n";
		out << "				Type: \"LayerElementUV\"\n";
		out << "				TypedIndex: 0\n";
		out << "			}\n";
		out << "		}\n";

		out << "	}\n";
	}

	// 4.2 Mesh Models
	for (const auto& ge : meshes)
	{
		out << "	Model: " << ge.modelId << ", \"Model::" << ge.name << "\", \"Mesh\" {\n";
		out << "		Version: 232\n";
		out << "		Properties70:  {\n";
		out << "			P: \"InheritType\", \"enum\", \"\", \"\",1\n";
		out << "			P: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\",0,0,0\n";
		out << "			P: \"Lcl Rotation\", \"Lcl Rotation\", \"\", \"A\",0,0,0\n";
		out << "			P: \"Lcl Scaling\", \"Lcl Scaling\", \"\", \"A\",1,1,1\n";
		out << "		}\n";
		out << "		Culling: \"CullingOff\"\n";
		out << "	}\n";
	}

	// 4.3 Bone Models (LimbNode)
	for (int b = 0; b < boneCount; b++)
	{
		D3DXVECTOR3 t, s;
		D3DXQUATERNION r;
		MatrixToTRS(bones[b].localTM, t, r, s);

		float rx, ry, rz;
		MatrixToEulerXYZ(bones[b].localTM, rx, ry, rz);

		out << "	Model: " << boneNodeIds[b] << ", \"Model::" << boneNames[b] << "\", \"LimbNode\" {\n";
		out << "		Version: 232\n";
		out << "		Properties70:  {\n";
		out << "			P: \"InheritType\", \"enum\", \"\", \"\",1\n";
		out << "			P: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\"," << t.x << "," << t.y << "," << t.z << "\n";
		out << "			P: \"Lcl Rotation\", \"Lcl Rotation\", \"\", \"A\"," << rx << "," << ry << "," << rz << "\n";
		out << "			P: \"Lcl Scaling\", \"Lcl Scaling\", \"\", \"A\"," << s.x << "," << s.y << "," << s.z << "\n";
		out << "		}\n";
		out << "	}\n";
	}

	// 4.4 Deformers (Skin & Clusters)
	for (const auto& ge : meshes)
	{
		if (!ge.isSkinned) continue;

		const ScenePart* part = scene.GetPart(ge.partIdx);
		const LODGroup* grp = part->model->GetGroup(part->currentLOD);
		const GMObject& obj = grp->objects[ge.objIdx];
		const SkinVertex* sv = static_cast<const SkinVertex*>(obj.vertices);

		out << "	Deformer: " << ge.skinId << ", \"Deformer::Skin\", \"Skin\" {\n";
		out << "		Version: 101\n";
		out << "		Link_DeformAcuracy: 50\n";
		out << "	}\n";

		// Clusters per bone
		for (int b = 0; b < boneCount; b++)
		{
			std::vector<int> vIndices;
			std::vector<float> vWeights;

			for (int v = 0; v < obj.vertexCount; v++)
			{
				float w1 = sv[v].w1;
				float w2 = sv[v].w2;
				float sum = w1 + w2;
				if (sum > 1e-5f) { w1 /= sum; w2 /= sum; }
				else { w1 = 1.0f; w2 = 0.0f; }

				if (sv[v].id1 == b && w1 > 1e-4f)
				{
					vIndices.push_back(v);
					vWeights.push_back(w1);
				}
				if (sv[v].id2 == b && w2 > 1e-4f)
				{
					vIndices.push_back(v);
					vWeights.push_back(w2);
				}
			}

			out << "	Deformer: " << ge.clusterIds[b] << ", \"SubDeformer::Cluster\", \"Cluster\" {\n";
			out << "		Version: 100\n";
			out << "		UserData: \"\", \"\"\n";

			out << "		Indexes: *" << vIndices.size() << " {\n";
			out << "			a: ";
			for (size_t k = 0; k < vIndices.size(); k++)
			{
				if (k > 0) out << ",";
				out << vIndices[k];
			}
			out << "\n		}\n";

			out << "		Weights: *" << vWeights.size() << " {\n";
			out << "			a: ";
			for (size_t k = 0; k < vWeights.size(); k++)
			{
				if (k > 0) out << ",";
				out << vWeights[k];
			}
			out << "\n		}\n";

			// Transform (Mesh world transform = identity)
			D3DXMATRIX idMat;
			D3DXMatrixIdentity(&idMat);
			out << "		Transform: *16 {\n";
			out << "			a: " << MatrixToFbxString(idMat) << "\n";
			out << "		}\n";

			// TransformLink (Bone bind pose world matrix)
			out << "		TransformLink: *16 {\n";
			out << "			a: " << MatrixToFbxString(bones[b].TM) << "\n";
			out << "		}\n";

			out << "	}\n";
		}
	}

	// 4.5 Materials
	for (const auto& mi : materials)
	{
		out << "	Material: " << mi.matId << ", \"Material::" << mi.matName << "\", \"\" {\n";
		out << "		Version: 102\n";
		out << "		ShadingModel: \"phong\"\n";
		out << "		Properties70:  {\n";
		out << "			P: \"DiffuseColor\", \"ColorRGB\", \"Color\", \"\","
		    << mi.diffuse[0] << "," << mi.diffuse[1] << "," << mi.diffuse[2] << "\n";
		out << "			P: \"AmbientColor\", \"ColorRGB\", \"Color\", \"\","
		    << mi.ambient[0] << "," << mi.ambient[1] << "," << mi.ambient[2] << "\n";
		out << "			P: \"SpecularColor\", \"ColorRGB\", \"Color\", \"\",0.1,0.1,0.1\n";
		out << "			P: \"Shininess\", \"Number\", \"\", \"A\",20\n";
		out << "		}\n";
		out << "	}\n";
	}

	// 4.6 Textures
	for (const auto& kv : textureMap)
	{
		std::string fn = fs::path(kv.first).stem().string() + ".png";
		out << "	Texture: " << kv.second << ", \"Texture::" << fn << "\", \"\" {\n";
		out << "		Type: \"TextureVideoClip\"\n";
		out << "		Version: 202\n";
		out << "		TextureName: \"Texture::" << fn << "\"\n";
		out << "		Properties70:  {\n";
		out << "			P: \"UVSet\", \"KString\", \"\", \"\", \"UVMap\"\n";
		out << "			P: \"UseMaterial\", \"bool\", \"\", \"\",1\n";
		out << "		}\n";
		out << "		FileName: \"" << fn << "\"\n";
		out << "		RelativeFilename: \"" << fn << "\"\n";
		out << "	}\n";
	}

	// 4.7 Animations (Stacks, Layers, Curves)
	for (const auto& asi : animStacks)
	{
		out << "	AnimationStack: " << asi.stackId << ", \"AnimStack::" << asi.name << "\", \"\" {\n";
		out << "		Properties70:  {\n";
		out << "			P: \"LocalStart\", \"KTime\", \"Time\", \"\",0\n";
		out << "			P: \"LocalStop\", \"KTime\", \"Time\", \"\"," << asi.stopKTime << "\n";
		out << "			P: \"ReferenceStart\", \"KTime\", \"Time\", \"\",0\n";
		out << "			P: \"ReferenceStop\", \"KTime\", \"Time\", \"\"," << asi.stopKTime << "\n";
		out << "		}\n";
		out << "	}\n";

		out << "	AnimationLayer: " << asi.layerId << ", \"AnimLayer::BaseLayer\", \"\" {\n";
		out << "	}\n";

		CMotion mot;
		if (!mot.Load(asi.filepath))
			continue;

		int motBones = (int)asi.boneCurves.size();

		for (int b = 0; b < motBones; b++)
		{
			const auto& bc = asi.boneCurves[b];

			// AnimationCurveNode for Translation
			out << "	AnimationCurveNode: " << bc.tNodeId << ", \"AnimCurveNode::T\", \"\" {\n";
			out << "		Properties70:  {\n";
			out << "			P: \"d|X\", \"Number\", \"\", \"A\",0\n";
			out << "			P: \"d|Y\", \"Number\", \"\", \"A\",0\n";
			out << "			P: \"d|Z\", \"Number\", \"\", \"A\",0\n";
			out << "		}\n";
			out << "	}\n";

			// AnimationCurveNode for Rotation
			out << "	AnimationCurveNode: " << bc.rNodeId << ", \"AnimCurveNode::R\", \"\" {\n";
			out << "		Properties70:  {\n";
			out << "			P: \"d|X\", \"Number\", \"\", \"A\",0\n";
			out << "			P: \"d|Y\", \"Number\", \"\", \"A\",0\n";
			out << "			P: \"d|Z\", \"Number\", \"\", \"A\",0\n";
			out << "		}\n";
			out << "	}\n";

			// Precompute keys for bone b
			std::vector<float> keyTx(asi.frameCount), keyTy(asi.frameCount), keyTz(asi.frameCount);
			std::vector<float> keyRx(asi.frameCount), keyRy(asi.frameCount), keyRz(asi.frameCount);

			for (int f = 0; f < asi.frameCount; f++)
			{
				D3DXMATRIX evalBones[MAX_BONES];
				mot.Animate(evalBones, (float)f, (f + 1) % asi.frameCount);

				D3DXMATRIX localB;
				if (bones[b].parentID != -1 && bones[b].parentID < boneCount)
				{
					D3DXMATRIX invParent;
					D3DXMatrixInverse(&invParent, nullptr, &evalBones[bones[b].parentID]);
					localB = evalBones[b] * invParent;
				}
				else
				{
					localB = evalBones[b];
				}

				D3DXVECTOR3 t, s;
				D3DXQUATERNION r;
				MatrixToTRS(localB, t, r, s);
				float rx, ry, rz;
				MatrixToEulerXYZ(localB, rx, ry, rz);

				keyTx[f] = t.x; keyTy[f] = t.y; keyTz[f] = t.z;
				keyRx[f] = rx;  keyRy[f] = ry;  keyRz[f] = rz;
			}

			// Helper to write an AnimationCurve
			auto writeCurve = [&](int64_t curveId, const std::vector<float>& vals) {
				out << "	AnimationCurve: " << curveId << ", \"AnimCurve\", \"\" {\n";
				out << "		KeyTime: *" << asi.frameCount << " {\n";
				out << "			a: ";
				for (int f = 0; f < asi.frameCount; f++)
				{
					if (f > 0) out << ",";
					out << (f * KTIME_PER_FRAME_30FPS);
				}
				out << "\n		}\n";
				out << "		KeyValueFloat: *" << asi.frameCount << " {\n";
				out << "			a: ";
				for (int f = 0; f < asi.frameCount; f++)
				{
					if (f > 0) out << ",";
					out << vals[f];
				}
				out << "\n		}\n";
				out << "	}\n";
			};

			writeCurve(bc.txCurveId, keyTx);
			writeCurve(bc.tyCurveId, keyTy);
			writeCurve(bc.tzCurveId, keyTz);
			writeCurve(bc.rxCurveId, keyRx);
			writeCurve(bc.ryCurveId, keyRy);
			writeCurve(bc.rzCurveId, keyRz);
		}
	}

	out << "}\n\n";

	// 5. Connections Section
	out << "Connections:  {\n";

	// Connect Geometries -> Mesh Models
	for (const auto& ge : meshes)
	{
		out << "	C: \"OO\"," << ge.geomId << "," << ge.modelId << "\n";
		out << "	C: \"OO\"," << ge.modelId << ",0\n";
	}

	// Connect Materials -> Mesh Models
	for (const auto& ge : meshes)
	{
		const ScenePart* part = scene.GetPart(ge.partIdx);
		const LODGroup* grp = part->model->GetGroup(part->currentLOD);
		const GMObject& obj = grp->objects[ge.objIdx];

		for (int mIdx = 0; mIdx < obj.materialCount; mIdx++)
		{
			std::string matKey = "Mat_p" + std::to_string(ge.partIdx) + "_o" + std::to_string(ge.objIdx) + "_m" + std::to_string(mIdx);
			for (const auto& mi : materials)
			{
				if (mi.matName == matKey)
				{
					out << "	C: \"OO\"," << mi.matId << "," << ge.modelId << "\n";
					if (mi.texId > 0)
					{
						out << "	C: \"OP\"," << mi.texId << "," << mi.matId << ",\"DiffuseColor\"\n";
					}
					break;
				}
			}
		}
	}

	// Connect Bones into hierarchy
	for (int b = 0; b < boneCount; b++)
	{
		int pId = bones[b].parentID;
		if (pId >= 0 && pId < boneCount)
		{
			out << "	C: \"OO\"," << boneNodeIds[b] << "," << boneNodeIds[pId] << "\n";
		}
		else
		{
			out << "	C: \"OO\"," << boneNodeIds[b] << ",0\n";
		}
	}

	// Connect Deformers (Skins & Clusters)
	for (const auto& ge : meshes)
	{
		if (!ge.isSkinned) continue;

		out << "	C: \"OO\"," << ge.skinId << "," << ge.geomId << "\n";

		for (int b = 0; b < boneCount; b++)
		{
			out << "	C: \"OO\"," << ge.clusterIds[b] << "," << ge.skinId << "\n";
			out << "	C: \"OO\"," << boneNodeIds[b] << "," << ge.clusterIds[b] << "\n";
		}
	}

	// Connect Animations
	for (const auto& asi : animStacks)
	{
		out << "	C: \"OO\"," << asi.layerId << "," << asi.stackId << "\n";

		int motBones = (int)asi.boneCurves.size();
		for (int b = 0; b < motBones; b++)
		{
			const auto& bc = asi.boneCurves[b];

			// Connect CurveNodes to Layer
			out << "	C: \"OO\"," << bc.tNodeId << "," << asi.layerId << "\n";
			out << "	C: \"OO\"," << bc.rNodeId << "," << asi.layerId << "\n";

			// Connect CurveNodes to Bone Model properties
			out << "	C: \"OP\"," << bc.tNodeId << "," << boneNodeIds[b] << ",\"Lcl Translation\"\n";
			out << "	C: \"OP\"," << bc.rNodeId << "," << boneNodeIds[b] << ",\"Lcl Rotation\"\n";

			// Connect Curves to CurveNodes
			out << "	C: \"OP\"," << bc.txCurveId << "," << bc.tNodeId << ",\"d|X\"\n";
			out << "	C: \"OP\"," << bc.tyCurveId << "," << bc.tNodeId << ",\"d|Y\"\n";
			out << "	C: \"OP\"," << bc.tzCurveId << "," << bc.tNodeId << ",\"d|Z\"\n";

			out << "	C: \"OP\"," << bc.rxCurveId << "," << bc.rNodeId << ",\"d|X\"\n";
			out << "	C: \"OP\"," << bc.ryCurveId << "," << bc.rNodeId << ",\"d|Y\"\n";
			out << "	C: \"OP\"," << bc.rzCurveId << "," << bc.rNodeId << ",\"d|Z\"\n";
		}
	}

	out << "}\n";

	out.close();
	std::cout << "FbxExporter: Successfully exported Autodesk FBX to: " << filepath
	          << " (" << meshes.size() << " meshes, " << animStacks.size() << " animations)" << std::endl;

	return true;
}

bool FbxExporter::ExportModelFBX(const std::string& filepath, const CObject3D* model,
                                 const std::vector<CMotion*>& motions,
                                 const std::string& modelDirectory)
{
	if (!model) return false;
	SceneManager tempScene;
	tempScene.LoadBaseModel(model->GetLoadedFilename());
	return ExportFBX(filepath, tempScene);
}
