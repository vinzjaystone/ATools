#include "DaeExporter.h"
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
	void D3DXMatrixToCollada(const D3DXMATRIX& inMat, float outMat[16])
	{
		// DirectX to OpenGL/COLLADA row-major 4x4 matrix (transpose)
		outMat[0]  = inMat._11; outMat[1]  = inMat._21; outMat[2]  = inMat._31; outMat[3]  = inMat._41;
		outMat[4]  = inMat._12; outMat[5]  = inMat._22; outMat[6]  = inMat._32; outMat[7]  = inMat._42;
		outMat[8]  = inMat._13; outMat[9]  = inMat._23; outMat[10] = inMat._33; outMat[11] = inMat._43;
		outMat[12] = inMat._14; outMat[13] = inMat._24; outMat[14] = inMat._34; outMat[15] = inMat._44;
	}

	std::string MatrixToString(const D3DXMATRIX& mat)
	{
		float m[16];
		D3DXMatrixToCollada(mat, m);
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(6);
		for (int i = 0; i < 16; i++)
		{
			if (i > 0) ss << " ";
			ss << m[i];
		}
		return ss.str();
	}

	std::string SanitizeXmlName(const std::string& name)
	{
		std::string s = name;
		for (char& c : s)
		{
			if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
				c = '_';
		}
		if (s.empty()) s = "item";
		return s;
	}

	struct MatInfo
	{
		std::string id;
		std::string name;
		std::string effectId;
		std::string imageId;
		std::string imageFilename;
		float diffuse[4];
		float ambient[4];
	};
}

bool DaeExporter::ExportDAE(const std::string& filepath, const SceneManager& scene,
                            const DaeExportOptions& options)
{
	const CObject3D* baseModel = scene.GetBaseModel();
	if (!baseModel)
	{
		std::cerr << "DaeExporter: No base model loaded in scene to export." << std::endl;
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
		std::cerr << "DaeExporter: Failed to open output file: " << filepath << std::endl;
		return false;
	}

	int boneCount = baseModel->GetBoneCount();
	const CSkeleton* skeleton = baseModel->GetSkeleton();
	const Bone* bones = skeleton ? skeleton->GetBones() : nullptr;

	std::vector<std::string> boneNames;
	std::vector<std::string> boneNodeIds;
	for (int b = 0; b < boneCount; b++)
	{
		std::string bName = (bones && bones[b].name[0] != '\0') ? bones[b].name : ("Bone_" + std::to_string(b));
		bName = SanitizeXmlName(bName);
		boneNames.push_back(bName);
		boneNodeIds.push_back("bone_node_" + std::to_string(b));
	}

	// 1. Collect Materials & Images from all parts
	std::vector<MatInfo> materialList;
	std::map<std::string, std::string> textureToImageMap;

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
				std::string matKey = "mat_p" + std::to_string(pIdx) + "_o" + std::to_string(oIdx) + "_m" + std::to_string(mIdx);

				MatInfo mi;
				mi.id = matKey;
				mi.name = "Mat_" + std::to_string(pIdx) + "_" + std::to_string(oIdx) + "_" + std::to_string(mIdx);
				mi.effectId = matKey + "-effect";

				mi.diffuse[0] = (mm.material.Diffuse.r > 0.0f) ? mm.material.Diffuse.r : 0.8f;
				mi.diffuse[1] = (mm.material.Diffuse.g > 0.0f) ? mm.material.Diffuse.g : 0.8f;
				mi.diffuse[2] = (mm.material.Diffuse.b > 0.0f) ? mm.material.Diffuse.b : 0.8f;
				mi.diffuse[3] = (mm.material.Diffuse.a > 0.0f) ? mm.material.Diffuse.a : 1.0f;

				mi.ambient[0] = (mm.material.Ambient.r > 0.0f) ? mm.material.Ambient.r : 0.2f;
				mi.ambient[1] = (mm.material.Ambient.g > 0.0f) ? mm.material.Ambient.g : 0.2f;
				mi.ambient[2] = (mm.material.Ambient.b > 0.0f) ? mm.material.Ambient.b : 0.2f;
				mi.ambient[3] = 1.0f;

				if (mm.textureName[0] != '\0')
				{
					std::string rawTexName = mm.textureName;
					if (textureToImageMap.find(rawTexName) == textureToImageMap.end())
					{
						std::string imgId = "image_" + std::to_string(textureToImageMap.size());
						textureToImageMap[rawTexName] = imgId;

						// Copy / convert texture to export folder
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
					mi.imageId = textureToImageMap[rawTexName];
					mi.imageFilename = fs::path(rawTexName).stem().string() + ".png";
				}

				materialList.push_back(mi);
			}
		}
	}

	// Write XML Header
	out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
	out << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n";
	out << "  <asset>\n";
	out << "    <contributor>\n";
	out << "      <authoring_tool>ATools Flyff Model Viewer COLLADA DAE Exporter</authoring_tool>\n";
	out << "    </contributor>\n";
	out << "    <created>2026-10-26T00:00:00Z</created>\n";
	out << "    <modified>2026-10-26T00:00:00Z</modified>\n";
	out << "    <unit name=\"meter\" meter=\"1\"/>\n";
	out << "    <up_axis>Y_UP</up_axis>\n";
	out << "  </asset>\n";

	// 2. Library Images
	if (!textureToImageMap.empty())
	{
		out << "  <library_images>\n";
		for (const auto& kv : textureToImageMap)
		{
			std::string fn = fs::path(kv.first).stem().string() + ".png";
			out << "    <image id=\"" << kv.second << "\" name=\"" << kv.second << "\">\n";
			out << "      <init_from>" << fn << "</init_from>\n";
			out << "    </image>\n";
		}
		out << "  </library_images>\n";
	}

	// 3. Library Materials & Effects
	if (!materialList.empty())
	{
		out << "  <library_materials>\n";
		for (const auto& mi : materialList)
		{
			out << "    <material id=\"" << mi.id << "\" name=\"" << mi.name << "\">\n";
			out << "      <instance_effect url=\"#" << mi.effectId << "\"/>\n";
			out << "    </material>\n";
		}
		out << "  </library_materials>\n";

		out << "  <library_effects>\n";
		for (const auto& mi : materialList)
		{
			out << "    <effect id=\"" << mi.effectId << "\" name=\"" << mi.name << "-effect\">\n";
			out << "      <profile_COMMON>\n";
			if (!mi.imageId.empty())
			{
				out << "        <newparam sid=\"" << mi.id << "-surface\">\n";
				out << "          <surface type=\"2D\">\n";
				out << "            <init_from>" << mi.imageId << "</init_from>\n";
				out << "          </surface>\n";
				out << "        </newparam>\n";
				out << "        <newparam sid=\"" << mi.id << "-sampler\">\n";
				out << "          <sampler2D>\n";
				out << "            <source>" << mi.id << "-surface</source>\n";
				out << "          </sampler2D>\n";
				out << "        </newparam>\n";
			}
			out << "        <technique sid=\"common\">\n";
			out << "          <phong>\n";
			out << "            <emission><color sid=\"emission\">0 0 0 1</color></emission>\n";
			out << "            <ambient><color sid=\"ambient\">"
			    << mi.ambient[0] << " " << mi.ambient[1] << " " << mi.ambient[2] << " 1</color></ambient>\n";
			out << "            <diffuse>\n";
			if (!mi.imageId.empty())
			{
				out << "              <texture texture=\"" << mi.id << "-sampler\" texcoord=\"TEX0\"/>\n";
			}
			else
			{
				out << "              <color sid=\"diffuse\">"
				    << mi.diffuse[0] << " " << mi.diffuse[1] << " " << mi.diffuse[2] << " 1</color>\n";
			}
			out << "            </diffuse>\n";
			out << "            <specular><color sid=\"specular\">0.1 0.1 0.1 1</color></specular>\n";
			out << "            <shininess><float sid=\"shininess\">20</float></shininess>\n";
			out << "          </phong>\n";
			out << "        </technique>\n";
			out << "      </profile_COMMON>\n";
			out << "    </effect>\n";
		}
		out << "  </library_effects>\n";
	}

	// 4. Library Geometries
	out << "  <library_geometries>\n";

	struct GeomEntry
	{
		int partIdx;
		int objIdx;
		std::string geomId;
		std::string skinId;
		bool isSkinned;
		int vertexCount;
		std::vector<std::pair<int, int>> matBlocks; // matIndex, primCount
	};

	std::vector<GeomEntry> exportedGeoms;

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

			std::string geomId = "geom-p" + std::to_string(pIdx) + "-o" + std::to_string(oIdx);
			std::string skinId = geomId + "-skin";
			bool isSkinned = (obj.type == GMT_SKIN && boneCount > 0);

			GeomEntry ge;
			ge.partIdx = pIdx;
			ge.objIdx = oIdx;
			ge.geomId = geomId;
			ge.skinId = skinId;
			ge.isSkinned = isSkinned;
			ge.vertexCount = obj.vertexCount;

			out << "    <geometry id=\"" << geomId << "\" name=\"" << geomId << "\">\n";
			out << "      <mesh>\n";

			// Position source
			out << "        <source id=\"" << geomId << "-positions\">\n";
			out << "          <float_array id=\"" << geomId << "-positions-array\" count=\"" << (obj.vertexCount * 3) << "\">";
			out << std::fixed << std::setprecision(6);

			for (int v = 0; v < obj.vertexCount; v++)
			{
				D3DXVECTOR3 p = (obj.type == GMT_SKIN) ? static_cast<const SkinVertex*>(obj.vertices)[v].p
				                                       : static_cast<const NormalVertex*>(obj.vertices)[v].p;
				out << (v > 0 ? " " : "") << p.x << " " << p.y << " " << p.z;
			}
			out << "</float_array>\n";
			out << "          <technique_common>\n";
			out << "            <accessor source=\"#" << geomId << "-positions-array\" count=\"" << obj.vertexCount << "\" stride=\"3\">\n";
			out << "              <param name=\"X\" type=\"float\"/>\n";
			out << "              <param name=\"Y\" type=\"float\"/>\n";
			out << "              <param name=\"Z\" type=\"float\"/>\n";
			out << "            </accessor>\n";
			out << "          </technique_common>\n";
			out << "        </source>\n";

			// Normal source
			out << "        <source id=\"" << geomId << "-normals\">\n";
			out << "          <float_array id=\"" << geomId << "-normals-array\" count=\"" << (obj.vertexCount * 3) << "\">";
			for (int v = 0; v < obj.vertexCount; v++)
			{
				D3DXVECTOR3 n = (obj.type == GMT_SKIN) ? static_cast<const SkinVertex*>(obj.vertices)[v].n
				                                       : static_cast<const NormalVertex*>(obj.vertices)[v].n;
				out << (v > 0 ? " " : "") << n.x << " " << n.y << " " << n.z;
			}
			out << "</float_array>\n";
			out << "          <technique_common>\n";
			out << "            <accessor source=\"#" << geomId << "-normals-array\" count=\"" << obj.vertexCount << "\" stride=\"3\">\n";
			out << "              <param name=\"X\" type=\"float\"/>\n";
			out << "              <param name=\"Y\" type=\"float\"/>\n";
			out << "              <param name=\"Z\" type=\"float\"/>\n";
			out << "            </accessor>\n";
			out << "          </technique_common>\n";
			out << "        </source>\n";

			// UV source
			out << "        <source id=\"" << geomId << "-map-0\">\n";
			out << "          <float_array id=\"" << geomId << "-map-0-array\" count=\"" << (obj.vertexCount * 2) << "\">";
			for (int v = 0; v < obj.vertexCount; v++)
			{
				D3DXVECTOR2 t = (obj.type == GMT_SKIN) ? static_cast<const SkinVertex*>(obj.vertices)[v].t
				                                       : static_cast<const NormalVertex*>(obj.vertices)[v].t;
				out << (v > 0 ? " " : "") << t.x << " " << (1.0f - t.y); // COLLADA V inverted
			}
			out << "</float_array>\n";
			out << "          <technique_common>\n";
			out << "            <accessor source=\"#" << geomId << "-map-0-array\" count=\"" << obj.vertexCount << "\" stride=\"2\">\n";
			out << "              <param name=\"S\" type=\"float\"/>\n";
			out << "              <param name=\"T\" type=\"float\"/>\n";
			out << "            </accessor>\n";
			out << "          </technique_common>\n";
			out << "        </source>\n";

			// Vertices
			out << "        <vertices id=\"" << geomId << "-vertices\">\n";
			out << "          <input semantic=\"POSITION\" source=\"#" << geomId << "-positions\"/>\n";
			out << "        </vertices>\n";

			// Triangles per Material Block
			int blockCount = (obj.materialBlockCount > 0 && obj.materialBlocks) ? obj.materialBlockCount : 1;

			for (int mb = 0; mb < blockCount; mb++)
			{
				int startFace = (obj.materialBlocks && mb < obj.materialBlockCount) ? obj.materialBlocks[mb].startVertex / 3 : 0;
				int primFaces = (obj.materialBlocks && mb < obj.materialBlockCount) ? obj.materialBlocks[mb].primitiveCount : obj.faceListCount;
				int matId = (obj.materialBlocks && mb < obj.materialBlockCount) ? obj.materialBlocks[mb].materialID : 0;

				if (primFaces <= 0) continue;

				std::string matSymbol = "mat_symbol_p" + std::to_string(pIdx) + "_o" + std::to_string(oIdx) + "_m" + std::to_string(matId);
				ge.matBlocks.push_back({ matId, primFaces });

				out << "        <triangles count=\"" << primFaces << "\" material=\"" << matSymbol << "\">\n";
				out << "          <input semantic=\"VERTEX\" source=\"#" << geomId << "-vertices\" offset=\"0\"/>\n";
				out << "          <input semantic=\"NORMAL\" source=\"#" << geomId << "-normals\" offset=\"1\"/>\n";
				out << "          <input semantic=\"TEXCOORD\" source=\"#" << geomId << "-map-0\" offset=\"2\" set=\"0\"/>\n";
				out << "          <p>";

				for (int f = 0; f < primFaces; f++)
				{
					int fIdx = (startFace + f) * 3;
					ushort i0 = obj.indices[fIdx];
					ushort i1 = obj.indices[fIdx + 1];
					ushort i2 = obj.indices[fIdx + 2];

					if (f > 0) out << " ";
					out << i0 << " " << i0 << " " << i0 << " "
					    << i1 << " " << i1 << " " << i1 << " "
					    << i2 << " " << i2 << " " << i2;
				}
				out << "</p>\n";
				out << "        </triangles>\n";
			}

			out << "      </mesh>\n";
			out << "    </geometry>\n";

			exportedGeoms.push_back(ge);
		}
	}
	out << "  </library_geometries>\n";

	// 5. Library Controllers (Skins)
	if (boneCount > 0 && bones)
	{
		out << "  <library_controllers>\n";

		for (const auto& ge : exportedGeoms)
		{
			if (!ge.isSkinned) continue;

			const ScenePart* part = scene.GetPart(ge.partIdx);
			const LODGroup* grp = part->model->GetGroup(part->currentLOD);
			const GMObject& obj = grp->objects[ge.objIdx];
			const SkinVertex* sv = static_cast<const SkinVertex*>(obj.vertices);

			out << "    <controller id=\"" << ge.skinId << "\" name=\"" << ge.skinId << "\">\n";
			out << "      <skin source=\"#" << ge.geomId << "\">\n";
			out << "        <bind_shape_matrix>1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</bind_shape_matrix>\n";

			// Joints source
			out << "        <source id=\"" << ge.skinId << "-joints\">\n";
			out << "          <Name_array id=\"" << ge.skinId << "-joints-array\" count=\"" << boneCount << "\">";
			for (int b = 0; b < boneCount; b++)
			{
				out << (b > 0 ? " " : "") << boneNames[b];
			}
			out << "</Name_array>\n";
			out << "          <technique_common>\n";
			out << "            <accessor source=\"#" << ge.skinId << "-joints-array\" count=\"" << boneCount << "\" stride=\"1\">\n";
			out << "              <param name=\"JOINT\" type=\"Name\"/>\n";
			out << "            </accessor>\n";
			out << "          </technique_common>\n";
			out << "        </source>\n";

			// Bind Poses (Inverse Bind Matrices)
			out << "        <source id=\"" << ge.skinId << "-bind_poses\">\n";
			out << "          <float_array id=\"" << ge.skinId << "-bind_poses-array\" count=\"" << (boneCount * 16) << "\">";
			for (int b = 0; b < boneCount; b++)
			{
				if (b > 0) out << " ";
				out << MatrixToString(bones[b].inverseTM);
			}
			out << "</float_array>\n";
			out << "          <technique_common>\n";
			out << "            <accessor source=\"#" << ge.skinId << "-bind_poses-array\" count=\"" << boneCount << "\" stride=\"16\">\n";
			out << "              <param name=\"TRANSFORM\" type=\"float4x4\"/>\n";
			out << "            </accessor>\n";
			out << "          </technique_common>\n";
			out << "        </source>\n";

			// Unique weights source
			std::vector<float> weights;
			for (int v = 0; v < obj.vertexCount; v++)
			{
				float w1 = sv[v].w1;
				float w2 = sv[v].w2;
				float sum = w1 + w2;
				if (sum > 1e-5f) { w1 /= sum; w2 /= sum; }
				else { w1 = 1.0f; w2 = 0.0f; }

				weights.push_back(w1);
				weights.push_back(w2);
			}

			out << "        <source id=\"" << ge.skinId << "-weights\">\n";
			out << "          <float_array id=\"" << ge.skinId << "-weights-array\" count=\"" << weights.size() << "\">";
			for (size_t w = 0; w < weights.size(); w++)
			{
				out << (w > 0 ? " " : "") << weights[w];
			}
			out << "</float_array>\n";
			out << "          <technique_common>\n";
			out << "            <accessor source=\"#" << ge.skinId << "-weights-array\" count=\"" << weights.size() << "\" stride=\"1\">\n";
			out << "              <param name=\"WEIGHT\" type=\"float\"/>\n";
			out << "            </accessor>\n";
			out << "          </technique_common>\n";
			out << "        </source>\n";

			// Joints element
			out << "        <joints>\n";
			out << "          <input semantic=\"JOINT\" source=\"#" << ge.skinId << "-joints\"/>\n";
			out << "          <input semantic=\"INV_BIND_MATRIX\" source=\"#" << ge.skinId << "-bind_poses\"/>\n";
			out << "        </joints>\n";

			// Vertex weights element
			out << "        <vertex_weights count=\"" << obj.vertexCount << "\">\n";
			out << "          <input semantic=\"JOINT\" source=\"#" << ge.skinId << "-joints\" offset=\"0\"/>\n";
			out << "          <input semantic=\"WEIGHT\" source=\"#" << ge.skinId << "-weights\" offset=\"1\"/>\n";

			out << "          <vcount>";
			for (int v = 0; v < obj.vertexCount; v++)
				out << (v > 0 ? " 2" : "2");
			out << "</vcount>\n";

			out << "          <v>";
			for (int v = 0; v < obj.vertexCount; v++)
			{
				uint16_t b0 = (sv[v].id1 < boneCount) ? sv[v].id1 : 0;
				uint16_t b1 = (sv[v].id2 < boneCount) ? sv[v].id2 : 0;
				int wIdx0 = v * 2;
				int wIdx1 = v * 2 + 1;

				if (v > 0) out << " ";
				out << b0 << " " << wIdx0 << " " << b1 << " " << wIdx1;
			}
			out << "</v>\n";

			out << "        </vertex_weights>\n";
			out << "      </skin>\n";
			out << "    </controller>\n";
		}

		out << "  </library_controllers>\n";
	}

	// 6. Library Animations
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

		if (!animList.empty())
		{
			out << "  <library_animations>\n";

			for (const auto& item : animList)
			{
				CMotion motion;
				if (!motion.Load(item.second) || motion.GetFrameCount() <= 1 || motion.GetBoneCount() <= 0)
					continue;

				int frameCount = motion.GetFrameCount();
				float fps = 30.0f;
				float frameTime = 1.0f / fps;
				std::string animSanitized = SanitizeXmlName(item.first);

				int motBones = std::min(boneCount, motion.GetBoneCount());

				for (int b = 0; b < motBones; b++)
				{
					std::string animId = "anim-" + animSanitized + "-bone" + std::to_string(b);

					out << "    <animation id=\"" << animId << "\" name=\"" << animId << "\">\n";

					// Time input source
					out << "      <source id=\"" << animId << "-input\">\n";
					out << "        <float_array id=\"" << animId << "-input-array\" count=\"" << frameCount << "\">";
					for (int f = 0; f < frameCount; f++)
					{
						out << (f > 0 ? " " : "") << (f * frameTime);
					}
					out << "</float_array>\n";
					out << "        <technique_common>\n";
					out << "          <accessor source=\"#" << animId << "-input-array\" count=\"" << frameCount << "\" stride=\"1\">\n";
					out << "            <param name=\"TIME\" type=\"float\"/>\n";
					out << "          </accessor>\n";
					out << "        </technique_common>\n";
					out << "      </source>\n";

					// Transform matrix output source
					out << "      <source id=\"" << animId << "-output\">\n";
					out << "        <float_array id=\"" << animId << "-output-array\" count=\"" << (frameCount * 16) << "\">";

					for (int f = 0; f < frameCount; f++)
					{
						D3DXMATRIX evalBones[MAX_BONES];
						motion.Animate(evalBones, (float)f, (f + 1) % frameCount);

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

						if (f > 0) out << " ";
						out << MatrixToString(localB);
					}
					out << "</float_array>\n";
					out << "        <technique_common>\n";
					out << "          <accessor source=\"#" << animId << "-output-array\" count=\"" << frameCount << "\" stride=\"16\">\n";
					out << "            <param name=\"TRANSFORM\" type=\"float4x4\"/>\n";
					out << "          </accessor>\n";
					out << "        </technique_common>\n";
					out << "      </source>\n";

					// Interpolation source
					out << "      <source id=\"" << animId << "-interpolation\">\n";
					out << "        <Name_array id=\"" << animId << "-interpolation-array\" count=\"" << frameCount << "\">";
					for (int f = 0; f < frameCount; f++)
						out << (f > 0 ? " LINEAR" : "LINEAR");
					out << "</Name_array>\n";
					out << "        <technique_common>\n";
					out << "          <accessor source=\"#" << animId << "-interpolation-array\" count=\"" << frameCount << "\" stride=\"1\">\n";
					out << "            <param name=\"INTERPOLATION\" type=\"Name\"/>\n";
					out << "          </accessor>\n";
					out << "        </technique_common>\n";
					out << "      </source>\n";

					// Sampler & Channel
					out << "      <sampler id=\"" << animId << "-sampler\">\n";
					out << "        <input semantic=\"INPUT\" source=\"#" << animId << "-input\"/>\n";
					out << "        <input semantic=\"OUTPUT\" source=\"#" << animId << "-output\"/>\n";
					out << "        <input semantic=\"INTERPOLATION\" source=\"#" << animId << "-interpolation\"/>\n";
					out << "      </sampler>\n";
					out << "      <channel source=\"#" << animId << "-sampler\" target=\"" << boneNodeIds[b] << "/transform\"/>\n";

					out << "    </animation>\n";
				}
			}

			out << "  </library_animations>\n";
		}
	}

	// 7. Library Visual Scenes
	out << "  <library_visual_scenes>\n";
	out << "    <visual_scene id=\"Scene\" name=\"Scene\">\n";

	// Write Armature Bone Tree
	if (boneCount > 0 && bones)
	{
		out << "      <node id=\"Armature\" name=\"Armature\" type=\"NODE\">\n";

		// Helper lambda for recursive bone nodes
		auto writeBoneNode = [&](auto& self, int bIdx, int indent) -> void {
			std::string ind(indent, ' ');
			out << ind << "<node id=\"" << boneNodeIds[bIdx] << "\" sid=\"" << boneNames[bIdx]
			    << "\" name=\"" << boneNames[bIdx] << "\" type=\"JOINT\">\n";
			out << ind << "  <matrix sid=\"transform\">" << MatrixToString(bones[bIdx].localTM) << "</matrix>\n";

			for (int c = 0; c < boneCount; c++)
			{
				if (bones[c].parentID == bIdx)
				{
					self(self, c, indent + 2);
				}
			}

			out << ind << "</node>\n";
		};

		for (int b = 0; b < boneCount; b++)
		{
			if (bones[b].parentID == -1 || bones[b].parentID >= boneCount)
			{
				writeBoneNode(writeBoneNode, b, 8);
			}
		}

		out << "      </node>\n";
	}

	// Write Geometry / Controller Instances
	for (const auto& ge : exportedGeoms)
	{
		std::string nodeName = "node-p" + std::to_string(ge.partIdx) + "-o" + std::to_string(ge.objIdx);

		out << "      <node id=\"" << nodeName << "\" name=\"" << nodeName << "\" type=\"NODE\">\n";

		if (ge.isSkinned && boneCount > 0)
		{
			out << "        <instance_controller url=\"#" << ge.skinId << "\">\n";
			out << "          <skeleton>#" << boneNodeIds[0] << "</skeleton>\n";
			out << "          <bind_material>\n";
			out << "            <technique_common>\n";
			for (const auto& mb : ge.matBlocks)
			{
				std::string matSymbol = "mat_symbol_p" + std::to_string(ge.partIdx) + "_o" + std::to_string(ge.objIdx) + "_m" + std::to_string(mb.first);
				std::string matTarget = "mat_p" + std::to_string(ge.partIdx) + "_o" + std::to_string(ge.objIdx) + "_m" + std::to_string(mb.first);
				out << "              <instance_material symbol=\"" << matSymbol << "\" target=\"#" << matTarget << "\"/>\n";
			}
			out << "            </technique_common>\n";
			out << "          </bind_material>\n";
			out << "        </instance_controller>\n";
		}
		else
		{
			out << "        <instance_geometry url=\"#" << ge.geomId << "\">\n";
			out << "          <bind_material>\n";
			out << "            <technique_common>\n";
			for (const auto& mb : ge.matBlocks)
			{
				std::string matSymbol = "mat_symbol_p" + std::to_string(ge.partIdx) + "_o" + std::to_string(ge.objIdx) + "_m" + std::to_string(mb.first);
				std::string matTarget = "mat_p" + std::to_string(ge.partIdx) + "_o" + std::to_string(ge.objIdx) + "_m" + std::to_string(mb.first);
				out << "              <instance_material symbol=\"" << matSymbol << "\" target=\"#" << matTarget << "\"/>\n";
			}
			out << "            </technique_common>\n";
			out << "          </bind_material>\n";
			out << "        </instance_geometry>\n";
		}

		out << "      </node>\n";
	}

	out << "    </visual_scene>\n";
	out << "  </library_visual_scenes>\n";

	// 8. Scene
	out << "  <scene>\n";
	out << "    <instance_visual_scene url=\"#Scene\"/>\n";
	out << "  </scene>\n";

	out << "</COLLADA>\n";
	out.close();

	std::cout << "DaeExporter: Successfully exported COLLADA .dae to: " << filepath
	          << " (" << exportedGeoms.size() << " geometries)" << std::endl;

	return true;
}

bool DaeExporter::ExportModelDAE(const std::string& filepath, const CObject3D* model,
                                 const std::vector<CMotion*>& motions,
                                 const std::string& modelDirectory)
{
	if (!model) return false;
	SceneManager tempScene;
	tempScene.LoadBaseModel(model->GetLoadedFilename());
	return ExportDAE(filepath, tempScene);
}
