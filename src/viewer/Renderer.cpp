#include "Renderer.h"
#include "rlgl.h"
#include "raymath.h"
#include "../model/TextureMng.h"
#include "../model/Motion.h"
#include <algorithm>
#include <vector>
#include <cmath>

SceneRenderer::SceneRenderer()
{
}

SceneRenderer::~SceneRenderer()
{
}

void SceneRenderer::DrawGrid(int slices, float spacing)
{
	int halfSlices = slices / 2;

	rlBegin(RL_LINES);
	for (int i = -halfSlices; i <= halfSlices; i++)
	{
		if (i == 0)
		{
			// Highlight center lines
			rlColor4ub(100, 200, 100, 255); // Z axis line in green-ish
			rlVertex3f((float)i * spacing, 0.0f, (float)-halfSlices * spacing);
			rlVertex3f((float)i * spacing, 0.0f, (float)halfSlices * spacing);

			rlColor4ub(200, 100, 100, 255); // X axis line in red-ish
			rlVertex3f((float)-halfSlices * spacing, 0.0f, (float)i * spacing);
			rlVertex3f((float)halfSlices * spacing, 0.0f, (float)i * spacing);
		}
		else
		{
			rlColor4ub(80, 85, 95, 180);
			rlVertex3f((float)i * spacing, 0.0f, (float)-halfSlices * spacing);
			rlVertex3f((float)i * spacing, 0.0f, (float)halfSlices * spacing);

			rlVertex3f((float)-halfSlices * spacing, 0.0f, (float)i * spacing);
			rlVertex3f((float)halfSlices * spacing, 0.0f, (float)i * spacing);
		}
	}
	rlEnd();
}

void SceneRenderer::DrawAxes(float length)
{
	rlBegin(RL_LINES);
	// X axis: Red
	rlColor4ub(230, 40, 40, 255);
	rlVertex3f(0.0f, 0.0f, 0.0f);
	rlVertex3f(length, 0.0f, 0.0f);

	// Y axis: Green
	rlColor4ub(40, 230, 40, 255);
	rlVertex3f(0.0f, 0.0f, 0.0f);
	rlVertex3f(0.0f, length, 0.0f);

	// Z axis: Blue
	rlColor4ub(40, 120, 240, 255);
	rlVertex3f(0.0f, 0.0f, 0.0f);
	rlVertex3f(0.0f, 0.0f, length);
	rlEnd();
}

void SceneRenderer::DrawBoundingBox(const Bounds& bounds, Color color)
{
	float minX = bounds.Min.x, minY = bounds.Min.y, minZ = bounds.Min.z;
	float maxX = bounds.Max.x, maxY = bounds.Max.y, maxZ = bounds.Max.z;

	rlBegin(RL_LINES);
	rlColor4ub(color.r, color.g, color.b, color.a);

	// Bottom 4 edges
	rlVertex3f(minX, minY, minZ); rlVertex3f(maxX, minY, minZ);
	rlVertex3f(maxX, minY, minZ); rlVertex3f(maxX, minY, maxZ);
	rlVertex3f(maxX, minY, maxZ); rlVertex3f(minX, minY, maxZ);
	rlVertex3f(minX, minY, maxZ); rlVertex3f(minX, minY, minZ);

	// Top 4 edges
	rlVertex3f(minX, maxY, minZ); rlVertex3f(maxX, maxY, minZ);
	rlVertex3f(maxX, maxY, minZ); rlVertex3f(maxX, maxY, maxZ);
	rlVertex3f(maxX, maxY, maxZ); rlVertex3f(minX, maxY, maxZ);
	rlVertex3f(minX, maxY, maxZ); rlVertex3f(minX, maxY, minZ);

	// 4 Vertical pillars
	rlVertex3f(minX, minY, minZ); rlVertex3f(minX, maxY, minZ);
	rlVertex3f(maxX, minY, minZ); rlVertex3f(maxX, maxY, minZ);
	rlVertex3f(maxX, minY, maxZ); rlVertex3f(maxX, maxY, maxZ);
	rlVertex3f(minX, minY, maxZ); rlVertex3f(minX, maxY, maxZ);

	rlEnd();
}

void SceneRenderer::DrawBoundingBoxOriented(const Bounds& bounds, const D3DXMATRIX& mat, Color color)
{
	D3DXVECTOR3 corners[8] = {
		{ bounds.Min.x, bounds.Min.y, bounds.Min.z },
		{ bounds.Max.x, bounds.Min.y, bounds.Min.z },
		{ bounds.Max.x, bounds.Min.y, bounds.Max.z },
		{ bounds.Min.x, bounds.Min.y, bounds.Max.z },
		{ bounds.Min.x, bounds.Max.y, bounds.Min.z },
		{ bounds.Max.x, bounds.Max.y, bounds.Min.z },
		{ bounds.Max.x, bounds.Max.y, bounds.Max.z },
		{ bounds.Min.x, bounds.Max.y, bounds.Max.z }
	};

	D3DXVECTOR3 tc[8];
	for (int i = 0; i < 8; i++)
		D3DXVec3TransformCoord(&tc[i], &corners[i], &mat);

	rlBegin(RL_LINES);
	rlColor4ub(color.r, color.g, color.b, color.a);

	// Bottom
	rlVertex3f(tc[0].x, tc[0].y, tc[0].z); rlVertex3f(tc[1].x, tc[1].y, tc[1].z);
	rlVertex3f(tc[1].x, tc[1].y, tc[1].z); rlVertex3f(tc[2].x, tc[2].y, tc[2].z);
	rlVertex3f(tc[2].x, tc[2].y, tc[2].z); rlVertex3f(tc[3].x, tc[3].y, tc[3].z);
	rlVertex3f(tc[3].x, tc[3].y, tc[3].z); rlVertex3f(tc[0].x, tc[0].y, tc[0].z);

	// Top
	rlVertex3f(tc[4].x, tc[4].y, tc[4].z); rlVertex3f(tc[5].x, tc[5].y, tc[5].z);
	rlVertex3f(tc[5].x, tc[5].y, tc[5].z); rlVertex3f(tc[6].x, tc[6].y, tc[6].z);
	rlVertex3f(tc[6].x, tc[6].y, tc[6].z); rlVertex3f(tc[7].x, tc[7].y, tc[7].z);
	rlVertex3f(tc[7].x, tc[7].y, tc[7].z); rlVertex3f(tc[4].x, tc[4].y, tc[4].z);

	// Pillars
	rlVertex3f(tc[0].x, tc[0].y, tc[0].z); rlVertex3f(tc[4].x, tc[4].y, tc[4].z);
	rlVertex3f(tc[1].x, tc[1].y, tc[1].z); rlVertex3f(tc[5].x, tc[5].y, tc[5].z);
	rlVertex3f(tc[2].x, tc[2].y, tc[2].z); rlVertex3f(tc[6].x, tc[6].y, tc[6].z);
	rlVertex3f(tc[3].x, tc[3].y, tc[3].z); rlVertex3f(tc[7].x, tc[7].y, tc[7].z);

	rlEnd();
}

void SceneRenderer::DrawCollisionMesh(const GMObject* collObj, const RenderSettings& settings)
{
	if (!collObj || collObj->vertexCount <= 0 || collObj->indexCount < 3)
		return;

	rlBegin(RL_LINES);
	rlColor4ub(50, 255, 100, 220);

	NormalVertex* nv = (collObj->type == GMT_NORMAL) ? (NormalVertex*)collObj->vertices : nullptr;
	SkinVertex* sv = (collObj->type == GMT_SKIN) ? (SkinVertex*)collObj->vertices : nullptr;

	for (int i = 0; i < collObj->indexCount / 3; i++)
	{
		ushort i0 = collObj->indices[i * 3 + 0];
		ushort i1 = collObj->indices[i * 3 + 1];
		ushort i2 = collObj->indices[i * 3 + 2];

		if (i0 >= collObj->vertexCount || i1 >= collObj->vertexCount || i2 >= collObj->vertexCount)
			continue;

		D3DXVECTOR3 p0 = nv ? nv[i0].p : sv[i0].p;
		D3DXVECTOR3 p1 = nv ? nv[i1].p : sv[i1].p;
		D3DXVECTOR3 p2 = nv ? nv[i2].p : sv[i2].p;

		rlVertex3f(p0.x, p0.y, p0.z); rlVertex3f(p1.x, p1.y, p1.z);
		rlVertex3f(p1.x, p1.y, p1.z); rlVertex3f(p2.x, p2.y, p2.z);
		rlVertex3f(p2.x, p2.y, p2.z); rlVertex3f(p0.x, p0.y, p0.z);
	}
	rlEnd();
}

void SceneRenderer::DrawBones(const CObject3D* model)
{
	if (!model) return;

	int boneCount = model->GetBoneCount();
	D3DXMATRIX* bones = model->GetEffectiveBones();

	if (boneCount <= 0 || !bones) return;

	static Color boneColors[] = {
		Color{ 60, 255, 60, 255 },
		Color{ 255, 200, 40, 255 },
		Color{ 40, 200, 255, 255 },
		Color{ 255, 80, 80, 255 }
	};

	// Draw Bone Joint crosses
	rlBegin(RL_LINES);
	for (int i = 0; i < boneCount; i++)
	{
		D3DXVECTOR3 pos(0.0f, 0.0f, 0.0f);
		D3DXVec3TransformCoord(&pos, &pos, &bones[i]);

		Color c = boneColors[i % 4];
		rlColor4ub(c.r, c.g, c.b, 255);

		// Small 3D cross for each bone joint
		float s = 0.03f;
		rlVertex3f(pos.x - s, pos.y, pos.z); rlVertex3f(pos.x + s, pos.y, pos.z);
		rlVertex3f(pos.x, pos.y - s, pos.z); rlVertex3f(pos.x, pos.y + s, pos.z);
		rlVertex3f(pos.x, pos.y, pos.z - s); rlVertex3f(pos.x, pos.y, pos.z + s);
	}
	rlEnd();

	// Draw Bone Hierarchy Connections
	const CMotion* mot = model->GetMotion();
	if (mot && mot->GetBones())
	{
		const Bone* boneList = mot->GetBones();
		rlBegin(RL_LINES);
		rlColor4ub(255, 255, 100, 220);

		for (int i = 0; i < boneCount; i++)
		{
			if (boneList[i].parentID >= 0 && boneList[i].parentID < boneCount)
			{
				D3DXVECTOR3 pChild(0.0f, 0.0f, 0.0f), pParent(0.0f, 0.0f, 0.0f);
				D3DXVec3TransformCoord(&pChild, &pChild, &bones[i]);
				D3DXVec3TransformCoord(&pParent, &pParent, &bones[boneList[i].parentID]);

				rlVertex3f(pParent.x, pParent.y, pParent.z);
				rlVertex3f(pChild.x, pChild.y, pChild.z);
			}
		}
		rlEnd();
	}
}

void SceneRenderer::_drawGMObject(const GMObject& obj, const D3DXMATRIX& transform, int lod,
                                  const RenderSettings& settings, const CObject3D* model,
                                  int objIdx, bool isSelectedPart)
{
	if (obj.vertexCount <= 0 || obj.indexCount < 3 || !obj.vertices || !obj.indices)
		return;

	NormalVertex* vertices = (NormalVertex*)obj.vertices;
	ushort* indices = obj.indices;

	// Light direction vector (normalized)
	D3DXVECTOR3 lightDir(0.5f, 1.0f, 0.7f);
	D3DXVec3Normalize(&lightDir, &lightDir);

	float scrollU = model ? model->GetScrollU() : 0.0f;
	float scrollV = model ? model->GetScrollV() : 0.0f;

	Texture2D whiteTex = TextureMng->GetDefaultWhiteTexture();

	for (int b = 0; b < obj.materialBlockCount; b++)
	{
		const MaterialBlock& block = obj.materialBlocks[b];
		if (block.primitiveCount <= 0) continue;

		bool isSelected = isSelectedPart && (settings.selectedObjIndex == objIdx &&
		                  (settings.selectedBlockIndex == -1 || settings.selectedBlockIndex == b));

		// Texture setup
		Texture2D tex = whiteTex;
		if (settings.enableTextures && obj.material && obj.materials && block.materialID < obj.materialCount)
		{
			if (obj.materials[block.materialID].textureLoaded && obj.materials[block.materialID].texture.id > 0)
				tex = obj.materials[block.materialID].texture;
		}

		// Material and effects setup
		bool twoSided = (block.effect & XE_2SIDE) != 0;
		if (twoSided)
			rlDisableBackfaceCulling();
		else
		{
			rlEnableBackfaceCulling();
			rlSetCullFace(RL_CULL_FACE_BACK);
		}

		if (block.effect & XE_HIGHLIGHT_OBJ)
			rlSetBlendMode(RL_BLEND_ADDITIVE);
		else
			rlSetBlendMode(RL_BLEND_ALPHA);

		float alpha = 1.0f;
		if (block.effect & XE_OPACITY)
			alpha = (float)block.amount / 255.0f;

		bool unlit = (block.effect & XE_SELF_ILLUMINATE) || !settings.enableLighting;

		// Bind texture
		rlSetTexture(tex.id);

		if (settings.fillMode == FILL_WIREFRAME)
		{
			rlSetTexture(whiteTex.id);
			rlBegin(RL_LINES);
			for (int j = 0; j < block.primitiveCount; j++)
			{
				int idxBase = block.startVertex + j * 3;
				if (idxBase + 2 >= obj.indexCount) break;

				ushort idx[3] = {
					indices[idxBase + 0],
					indices[idxBase + 1],
					indices[idxBase + 2]
				};

				D3DXVECTOR3 p[3];
				for (int k = 0; k < 3; k++)
				{
					if (idx[k] < obj.vertexCount)
						D3DXVec3TransformCoord(&p[k], &vertices[idx[k]].p, &transform);
				}

				Color wireCol = isSelected ? Color{ 255, 220, 60, 255 } : Color{ 200, 220, 240, 220 };
				rlColor4ub(wireCol.r, wireCol.g, wireCol.b, wireCol.a);

				rlVertex3f(p[0].x, p[0].y, p[0].z); rlVertex3f(p[1].x, p[1].y, p[1].z);
				rlVertex3f(p[1].x, p[1].y, p[1].z); rlVertex3f(p[2].x, p[2].y, p[2].z);
				rlVertex3f(p[2].x, p[2].y, p[2].z); rlVertex3f(p[0].x, p[0].y, p[0].z);
			}
			rlEnd();
		}
		else if (settings.fillMode == FILL_POINT)
		{
			rlSetTexture(whiteTex.id);
			rlBegin(RL_LINES);
			rlColor4ub(255, 255, 255, 255);
			for (int j = 0; j < obj.vertexCount; j++)
			{
				D3DXVECTOR3 p;
				D3DXVec3TransformCoord(&p, &vertices[j].p, &transform);
				rlVertex3f(p.x - 0.01f, p.y, p.z);
				rlVertex3f(p.x + 0.01f, p.y, p.z);
				rlVertex3f(p.x, p.y - 0.01f, p.z);
				rlVertex3f(p.x, p.y + 0.01f, p.z);
			}
			rlEnd();
		}
		else // Solid
		{
			rlBegin(RL_TRIANGLES);

			for (int j = 0; j < block.primitiveCount; j++)
			{
				int idxBase = block.startVertex + j * 3;
				if (idxBase + 2 >= obj.indexCount) break;

				ushort idx[3] = {
					indices[idxBase + 0],
					indices[idxBase + 1],
					indices[idxBase + 2]
				};

				for (int k = 0; k < 3; k++)
				{
					if (idx[k] >= obj.vertexCount) continue;

					const NormalVertex& v = vertices[idx[k]];

					D3DXVECTOR3 p, n;
					D3DXVec3TransformCoord(&p, &v.p, &transform);
					D3DXVec3TransformNormal(&n, &v.n, &transform);
					D3DXVec3Normalize(&n, &n);

					float diffuse = 1.0f;
					if (!unlit)
					{
						float dot = D3DXVec3Dot(&n, &lightDir);
						diffuse = std::max(0.35f, std::min(1.0f, dot * 0.65f + 0.35f));
					}

					float r = diffuse;
					float g = diffuse;
					float b = diffuse;

					if (isSelected)
					{
						r = std::min(1.0f, r * 1.3f);
						g = std::min(1.0f, g * 1.2f);
						b = b * 0.7f;
					}

					unsigned char cr = (unsigned char)(r * 255.0f);
					unsigned char cg = (unsigned char)(g * 255.0f);
					unsigned char cb = (unsigned char)(b * 255.0f);
					unsigned char ca = (unsigned char)(alpha * 255.0f);

					rlColor4ub(cr, cg, cb, ca);
					rlTexCoord2f(v.t.x + scrollU, v.t.y + scrollV);
					rlNormal3f(n.x, n.y, n.z);
					rlVertex3f(p.x, p.y, p.z);
				}
			}
			rlEnd();
		}

		// Restore default blend/cull mode
		rlSetBlendMode(RL_BLEND_ALPHA);
		rlEnableBackfaceCulling();
	}

	rlSetTexture(0);
}

void SceneRenderer::_drawSkinGMObject(const GMObject& obj, const D3DXMATRIX& rootTransform, int lod,
                                      const RenderSettings& settings, const CObject3D* model,
                                      int objIdx, bool isSelectedPart)
{
	if (obj.vertexCount <= 0 || obj.indexCount < 3 || !obj.vertices || !obj.indices)
		return;

	SkinVertex* vertices = (SkinVertex*)obj.vertices;
	ushort* indices = obj.indices;

	D3DXMATRIX* bones = model ? model->GetEffectiveBones() : nullptr;
	D3DXMATRIX* invBones = model ? model->GetEffectiveInvBones() : nullptr;

	D3DXVECTOR3 lightDir(0.5f, 1.0f, 0.7f);
	D3DXVec3Normalize(&lightDir, &lightDir);

	float scrollU = model ? model->GetScrollU() : 0.0f;
	float scrollV = model ? model->GetScrollV() : 0.0f;

	Texture2D whiteTex = TextureMng->GetDefaultWhiteTexture();

	// Precompute transformed skin vertices
	std::vector<D3DXVECTOR3> transPos(obj.vertexCount);
	std::vector<D3DXVECTOR3> transNorm(obj.vertexCount);

	int boneCount = model ? model->GetBoneCount() : 0;

	for (int v = 0; v < obj.vertexCount; v++)
	{
		const SkinVertex& sv = vertices[v];

		if (bones && invBones && obj.physiqueVertices && obj.IIB && v < obj.vertexCount)
		{
			ushort iibIdx = obj.IIB[v];
			int boneID = (iibIdx < obj.vertexListCount) ? obj.physiqueVertices[iibIdx] : 0;

			if (boneID >= 0 && boneID < boneCount)
			{
				D3DXMATRIX tm = invBones[boneID] * bones[boneID];
				tm = tm * rootTransform;
				D3DXVec3TransformCoord(&transPos[v], &sv.p, &tm);
				D3DXVec3TransformNormal(&transNorm[v], &sv.n, &tm);
				D3DXVec3Normalize(&transNorm[v], &transNorm[v]);
			}
			else
			{
				D3DXVec3TransformCoord(&transPos[v], &sv.p, &rootTransform);
				D3DXVec3TransformNormal(&transNorm[v], &sv.n, &rootTransform);
				D3DXVec3Normalize(&transNorm[v], &transNorm[v]);
			}
		}
		else
		{
			D3DXVec3TransformCoord(&transPos[v], &sv.p, &rootTransform);
			D3DXVec3TransformNormal(&transNorm[v], &sv.n, &rootTransform);
			D3DXVec3Normalize(&transNorm[v], &transNorm[v]);
		}
	}

	for (int b = 0; b < obj.materialBlockCount; b++)
	{
		const MaterialBlock& block = obj.materialBlocks[b];
		if (block.primitiveCount <= 0) continue;

		bool isSelected = isSelectedPart && (settings.selectedObjIndex == objIdx &&
		                  (settings.selectedBlockIndex == -1 || settings.selectedBlockIndex == b));

		Texture2D tex = whiteTex;
		if (settings.enableTextures && obj.material && obj.materials && block.materialID < obj.materialCount)
		{
			if (obj.materials[block.materialID].textureLoaded && obj.materials[block.materialID].texture.id > 0)
				tex = obj.materials[block.materialID].texture;
		}

		bool twoSided = (block.effect & XE_2SIDE) != 0;
		if (twoSided)
			rlDisableBackfaceCulling();
		else
		{
			rlEnableBackfaceCulling();
			rlSetCullFace(RL_CULL_FACE_BACK);
		}

		if (block.effect & XE_HIGHLIGHT_OBJ)
			rlSetBlendMode(RL_BLEND_ADDITIVE);
		else
			rlSetBlendMode(RL_BLEND_ALPHA);

		float alpha = 1.0f;
		if (block.effect & XE_OPACITY)
			alpha = (float)block.amount / 255.0f;

		bool unlit = (block.effect & XE_SELF_ILLUMINATE) || !settings.enableLighting;

		rlSetTexture(tex.id);

		if (settings.fillMode == FILL_WIREFRAME)
		{
			rlSetTexture(whiteTex.id);
			rlBegin(RL_LINES);
			for (int j = 0; j < block.primitiveCount; j++)
			{
				int idxBase = block.startVertex + j * 3;
				if (idxBase + 2 >= obj.indexCount) break;

				ushort idx[3] = {
					indices[idxBase + 0],
					indices[idxBase + 1],
					indices[idxBase + 2]
				};

				Color wireCol = isSelected ? Color{ 255, 220, 60, 255 } : Color{ 200, 220, 240, 220 };
				rlColor4ub(wireCol.r, wireCol.g, wireCol.b, wireCol.a);

				if (idx[0] < obj.vertexCount && idx[1] < obj.vertexCount && idx[2] < obj.vertexCount)
				{
					const D3DXVECTOR3& p0 = transPos[idx[0]];
					const D3DXVECTOR3& p1 = transPos[idx[1]];
					const D3DXVECTOR3& p2 = transPos[idx[2]];

					rlVertex3f(p0.x, p0.y, p0.z); rlVertex3f(p1.x, p1.y, p1.z);
					rlVertex3f(p1.x, p1.y, p1.z); rlVertex3f(p2.x, p2.y, p2.z);
					rlVertex3f(p2.x, p2.y, p2.z); rlVertex3f(p0.x, p0.y, p0.z);
				}
			}
			rlEnd();
		}
		else if (settings.fillMode == FILL_POINT)
		{
			rlSetTexture(whiteTex.id);
			rlBegin(RL_LINES);
			rlColor4ub(255, 255, 255, 255);
			for (int j = 0; j < obj.vertexCount; j++)
			{
				const D3DXVECTOR3& p = transPos[j];
				rlVertex3f(p.x - 0.01f, p.y, p.z);
				rlVertex3f(p.x + 0.01f, p.y, p.z);
				rlVertex3f(p.x, p.y - 0.01f, p.z);
				rlVertex3f(p.x, p.y + 0.01f, p.z);
			}
			rlEnd();
		}
		else // Solid
		{
			rlBegin(RL_TRIANGLES);

			for (int j = 0; j < block.primitiveCount; j++)
			{
				int idxBase = block.startVertex + j * 3;
				if (idxBase + 2 >= obj.indexCount) break;

				ushort idx[3] = {
					indices[idxBase + 0],
					indices[idxBase + 1],
					indices[idxBase + 2]
				};

				for (int k = 0; k < 3; k++)
				{
					if (idx[k] >= obj.vertexCount) continue;

					const SkinVertex& sv = vertices[idx[k]];
					const D3DXVECTOR3& p = transPos[idx[k]];
					const D3DXVECTOR3& n = transNorm[idx[k]];

					float diffuse = 1.0f;
					if (!unlit)
					{
						float dot = D3DXVec3Dot(&n, &lightDir);
						diffuse = std::max(0.35f, std::min(1.0f, dot * 0.65f + 0.35f));
					}

					float r = diffuse;
					float g = diffuse;
					float b = diffuse;

					if (isSelected)
					{
						r = std::min(1.0f, r * 1.3f);
						g = std::min(1.0f, g * 1.2f);
						b = b * 0.7f;
					}

					unsigned char cr = (unsigned char)(r * 255.0f);
					unsigned char cg = (unsigned char)(g * 255.0f);
					unsigned char cb = (unsigned char)(b * 255.0f);
					unsigned char ca = (unsigned char)(alpha * 255.0f);

					rlColor4ub(cr, cg, cb, ca);
					rlTexCoord2f(sv.t.x + scrollU, sv.t.y + scrollV);
					rlNormal3f(n.x, n.y, n.z);
					rlVertex3f(p.x, p.y, p.z);
				}
			}
			rlEnd();
		}

		rlSetBlendMode(RL_BLEND_ALPHA);
		rlEnableBackfaceCulling();
	}

	rlSetTexture(0);
}

void SceneRenderer::_drawModelInternal(const CObject3D* model, int lod, const D3DXMATRIX& rootTransform,
                                       const RenderSettings& settings, bool isSelectedPart)
{
	if (!model) return;

	const LODGroup* group = model->GetGroup(lod);
	if (!group || !group->objects) return;

	for (int i = 0; i < group->objectCount; i++)
	{
		const GMObject& obj = group->objects[i];
		D3DXMATRIX objTrans = group->updates ? group->updates[i] : obj.transform;
		D3DXMATRIX finalTrans = objTrans * rootTransform;

		if (obj.type == GMT_SKIN)
			_drawSkinGMObject(obj, rootTransform, lod, settings, model, i, isSelectedPart);
		else
			_drawGMObject(obj, finalTrans, lod, settings, model, i, isSelectedPart);

		if (settings.showSubBounds)
			DrawBoundingBoxOriented(obj.bounds, finalTrans, Color{ 80, 140, 255, 180 });
	}
}

void SceneRenderer::Render(const CObject3D* model, int lod, const OrbitCamera& camera,
                           Rectangle viewportRect, const RenderSettings& settings)
{
	rlEnableScissorTest();
	rlScissor((int)viewportRect.x, (int)(GetScreenHeight() - (viewportRect.y + viewportRect.height)),
	          (int)viewportRect.width, (int)viewportRect.height);

	DrawRectangleRec(viewportRect, settings.backgroundColor);

	Camera3D cam = camera.GetRaylibCamera();
	BeginMode3D(cam);

	if (settings.showGrid)
		DrawGrid(32, 1.0f);

	if (settings.showAxes)
		DrawAxes(2.0f);

	if (model)
	{
		D3DXMATRIX identity;
		D3DXMatrixIdentity(&identity);
		_drawModelInternal(model, lod, identity, settings, true);

		if (settings.showBounds)
			DrawBoundingBox(model->GetBounds(), Color{ 0, 225, 240, 240 });

		if (settings.showCollision)
			DrawCollisionMesh(model->GetCollisionObj(), settings);

		if (settings.showBones)
			DrawBones(model);
	}

	EndMode3D();
	rlDisableScissorTest();
}

void SceneRenderer::RenderScene(const SceneManager& scene, const OrbitCamera& camera,
                                Rectangle viewportRect, const RenderSettings& settings)
{
	rlEnableScissorTest();
	rlScissor((int)viewportRect.x, (int)(GetScreenHeight() - (viewportRect.y + viewportRect.height)),
	          (int)viewportRect.width, (int)viewportRect.height);

	DrawRectangleRec(viewportRect, settings.backgroundColor);

	Camera3D cam = camera.GetRaylibCamera();
	BeginMode3D(cam);

	// 1. Grid
	if (settings.showGrid)
		DrawGrid(32, 1.0f);

	// 2. Coordinate Axes
	if (settings.showAxes)
		DrawAxes(2.0f);

	// 3. Render all scene parts
	const CObject3D* baseModel = scene.GetBaseModel();
	int activeIdx = scene.GetActivePartIndex();

	for (int p = 0; p < scene.GetPartCount(); p++)
	{
		const ScenePart* part = scene.GetPart(p);
		if (!part || !part->visible || !part->model)
			continue;

		D3DXMATRIX rootTransform;
		D3DXMatrixIdentity(&rootTransform);

		// If attached to a specific bone socket on the base model
		if (part->attachBoneID >= 0 && baseModel)
		{
			D3DXMATRIX* baseBones = baseModel->GetBaseBones();
			if (baseBones && part->attachBoneID < baseModel->GetBoneCount())
			{
				rootTransform = baseBones[part->attachBoneID];
			}
		}

		bool isSelectedPart = (p == activeIdx);
		_drawModelInternal(part->model, part->currentLOD, rootTransform, settings, isSelectedPart);
	}

	// 4. Scene Overlays (Bounds, Bones, Collision)
	if (settings.showBounds)
	{
		Bounds combined = scene.GetCombinedBounds();
		DrawBoundingBox(combined, Color{ 0, 225, 240, 240 });
	}

	if (settings.showBones && baseModel)
	{
		DrawBones(baseModel);
	}

	const CObject3D* activeModel = scene.GetActiveModel();
	if (settings.showCollision && activeModel)
	{
		DrawCollisionMesh(activeModel->GetCollisionObj(), settings);
	}

	EndMode3D();
	rlDisableScissorTest();
}
