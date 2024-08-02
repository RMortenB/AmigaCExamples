#include "ObjMesh.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "vec3.h"

#include "dynarray.h"

/*****************************************************************************/

void FreeMesh(obj_mesh *mesh);
void MeshOutputFormat(obj_mesh *mesh);

/*****************************************************************************/

struct Vertex;
typedef struct Vertex Vertex;

struct Vertex
{
	vec3 pos;
	struct
	{
		float x, y;
	} uv;
	vec3 normal;
};

/*****************************************************************************/

typedef struct
{
	int pos;
	int texcoord;
	int normal;
	int index;
} IndexedVertex;

/*****************************************************************************/

typedef struct
{
	float r, g, b;
} model_rgb;

#define MAX_TEXTURES 4

/*****************************************************************************/

typedef struct
{
	char name[64];
	model_rgb ambient;
	model_rgb diffuse;
	model_rgb specular;
	float transp;
	float spec_coeff;
	int illumination_model;

	char map_kd[64];
	unsigned int txtr[MAX_TEXTURES];
} model_material;

/*****************************************************************************/

static void StripWhites(char *dest, char *src)
{
	int spos = 0;

	while(src[spos] && isspace(src[spos]))
		spos++;

	strcpy(dest, src + spos);

	int dlen = strlen(dest) - 1;
	while(dest[dlen] && isspace(dest[dlen]))
	{
		dest[dlen] = 0;
		dlen--;
	}
}

/*****************************************************************************/

static int getFace(IndexedVertex *ivtx, const char *line)
{
	int spos = 0;
	int nverts = 0;

	while(line[spos] != 0)
	{
		if(line[spos] == '\n')
			break;
		if(line[spos] == '\r')
			break;
		int epos = spos;
		while(line[epos] && !isspace(line[epos]))
			epos++;
		if(!line[epos])
			break;
		ivtx[nverts].pos = 1;
		ivtx[nverts].texcoord = 1;
		ivtx[nverts].normal = 1;
		int nvals = sscanf(line + spos, "%d/%d/%d", &ivtx[nverts].pos, &ivtx[nverts].texcoord, &ivtx[nverts].normal);
		assert(nvals);

		if(nvals == 2)
		{
			ivtx[nverts].normal = 0;
		}
		if(nvals == 1)
		{
			sscanf(line + spos, "%d//%d", &ivtx[nverts].pos, &ivtx[nverts].normal);
		}
		ivtx[nverts].pos -= 1;
		ivtx[nverts].normal -= 1;
		ivtx[nverts].texcoord -= 1;
		nverts++;
		spos = epos + 1;
	}
	return nverts;
}

/*****************************************************************************/

static int FindMaterial(const char *mtrlName, DynamicArray *materials)
{
	int nMtrls = VA_Size(materials);

	for(int i = 0; i < nMtrls; ++i)
	{
		model_material *mtrl = VA_GetObject(materials, i);
		if(!strcmp(mtrl->name, mtrlName))
		{
			return i;
		}
	}
	printf("Unknown material %s\n", mtrlName);
	return 0;
}

/*****************************************************************************/

static void ReadMaterials(const char *filename, DynamicArray *materials)
{
	model_material mat;
	int material_started = 0;

	FILE *fp = fopen(filename, "rt");
	if(!fp)
	{
		printf("No material file \"%s\"!\n", filename);
		return;
	}

	while(!feof(fp))
	{
		static char inputLine[2048];
		static char line[2048];

		fgets(inputLine, sizeof(inputLine), fp);
		StripWhites(line, inputLine);
		if(!strncmp(line, "newmtl ", 7))
		{
			if(material_started)
				VA_AddObject(materials, &mat);
			memset(&mat, 0, sizeof(mat));
			int values = sscanf(line + 7, "%s", mat.name);
			assert(values == 1);
			material_started = 1;
		}
		else if(!strncmp(line, "Ka ", 3))
		{
			assert(material_started);
			int values = sscanf(line + 3, "%f %f %f", &mat.ambient.r, &mat.ambient.g, &mat.ambient.b);
			assert(values == 3);
		}
		else if(!strncmp(line, "Kd ", 3))
		{
			assert(material_started);
			int values = sscanf(line + 3, "%f %f %f", &mat.diffuse.r, &mat.diffuse.g, &mat.diffuse.b);
			assert(values == 3);
		}
		else if(!strncmp(line, "Ks ", 3))
		{
			assert(material_started);
			int values = sscanf(line + 3, "%f %f %f", &mat.specular.r, &mat.specular.g, &mat.specular.b);
			assert(values == 3);
		}
		else if(!strncmp(line, "d ", 2))
		{
			assert(material_started);
			int values = sscanf(line + 2, "%f", &mat.transp);
			assert(values == 1);
		}
		else if(!strncmp(line, "Tr ", 3))
		{
			assert(material_started);
			int values = sscanf(line + 3, "%f", &mat.transp);
			assert(values == 1);
		}
		else if(!strncmp(line, "Ns ", 3))
		{
			assert(material_started);
			int values = sscanf(line + 3, "%f", &mat.spec_coeff);
			assert(values == 1);
		}
		else if(!strncmp(line, "map_Kd ", 7))
		{
			assert(material_started);
			int values = sscanf(line + 7, "%s", mat.map_kd);
			assert(values == 1);
		}
	}
	if(material_started)
		VA_AddObject(materials, &mat);

}

/*****************************************************************************/

static DynamicArray *ExplodeSubMesh(DynamicArray *indexvertices, int startIndx, int endIndx, DynamicArray *positions, DynamicArray *normals, DynamicArray *texcoords)
{
	DynamicArray *rawMesh = VA_Alloc(sizeof(Vertex), VA_Size(indexvertices));
	Vertex v;
	memset(&v, 0, sizeof(v));

	for(int i = startIndx; i < endIndx; ++i)
	{
		IndexedVertex *indxVtx = (IndexedVertex *)VA_GetObject(indexvertices, i);
		vec3 *pos = (vec3 *)VA_GetObject(positions, indxVtx->pos);
		v.pos = *pos;
		if(VA_Size(normals))
		{
			vec3 *norm = (vec3 *)VA_GetObject(normals, indxVtx->normal);
			v.normal = *norm;
		}
		if(VA_Size(texcoords))
		{
			vec3 *texcoord = (vec3 *)VA_GetObject(texcoords, indxVtx->texcoord);
			v.uv.x = texcoord->x;
			v.uv.y = texcoord->y;
		}
		VA_AddObject(rawMesh, &v);
	}
	return rawMesh;
}

/*****************************************************************************/

typedef struct
{
	Vertex *vtx;
	int indx;
} TempVertex;

/*****************************************************************************/

static __stdargs int tmpVertexCompare(const void *p0, const void *p1)
{
	TempVertex *tv0 = (TempVertex *)p0;
	TempVertex *tv1 = (TempVertex *)p1;
	return memcmp(tv0->vtx, tv1->vtx, sizeof(Vertex));
}

/*****************************************************************************/

static void IndexSubmesh(DynamicArray *rawMesh, DynamicArray *vb, DynamicArray *ib)
{
	int nVerts = VA_Size(rawMesh);
	Vertex *vtx = (Vertex *)VA_GetData(rawMesh);

	TempVertex *tmpVertex = malloc(sizeof(TempVertex) * nVerts);
	int *remap = malloc(sizeof(int) * nVerts);

	for(int i = 0; i < nVerts; ++i)
	{
		tmpVertex[i].vtx = &vtx[i];
		tmpVertex[i].indx = i;
	}
	qsort(tmpVertex, nVerts, sizeof(TempVertex), tmpVertexCompare);

	int currentIndex = 0;
	VA_AddObject(vb, (uint8_t *)tmpVertex[0].vtx);
	remap[tmpVertex[0].indx] = currentIndex;
	for(int i = 1; i < nVerts; ++i)
	{
		if(memcmp(tmpVertex[i - 1].vtx, tmpVertex[i].vtx, sizeof(Vertex)))
		{
			currentIndex++;
			VA_AddObject(vb, (uint8_t *)tmpVertex[i].vtx);
		}
		remap[tmpVertex[i].indx] = currentIndex;
	}
	for(int i = 0; i < nVerts; i += 3)
	{
		if(remap[i + 0] == remap[i + 1])
			continue;
		if(remap[i + 0] == remap[i + 2])
			continue;
		if(remap[i + 1] == remap[i + 2])
			continue;
		VA_AddObject(ib, &remap[i + 0]);
		VA_AddObject(ib, &remap[i + 1]);
		VA_AddObject(ib, &remap[i + 2]);
	}
	free(remap);
	free(tmpVertex);
}
/*****************************************************************************/

void RecalcNormals(obj_submesh *subMesh)
{
	int nVerts = VA_Size(subMesh->vb);
	Vertex *vtx = (Vertex *)subMesh->vb->data;
	for(int i = 0; i < nVerts; ++i)
	{
		vtx[i].normal.x = 0.0f;
		vtx[i].normal.y = 0.0f;
		vtx[i].normal.z = 0.0f;
	}

	int nIndex = VA_Size(subMesh->ib);
	short *indx = (short *)subMesh->ib->data;
	for(int i = 0; i < nIndex; i += 3)
	{
		vec3 v0, v1, v0x1;
		vec3_sub(&v0, &vtx[indx[i + 1]].pos, &vtx[indx[i + 0]].pos);
		vec3_sub(&v1, &vtx[indx[i + 2]].pos, &vtx[indx[i + 0]].pos);
		vec3_cross(&v0x1, &v0, &v1);
		vec3_add(&vtx[indx[i + 0]].normal, &vtx[indx[i + 0]].normal, &v0x1);
		vec3_add(&vtx[indx[i + 1]].normal, &vtx[indx[i + 1]].normal, &v0x1);
		vec3_add(&vtx[indx[i + 2]].normal, &vtx[indx[i + 2]].normal, &v0x1);
	}
	for(int i = 0; i < nVerts; ++i)
	{
		if(vec3_dot(&vtx[i].normal, &vtx[i].normal) <= 0.0f)
		{
			vtx[i].normal.x = 0.0f;
			vtx[i].normal.y = 0.0f;
			vtx[i].normal.z = 1.0f;
		}
		else
		{
			vec3_normalise(&vtx[i].normal, &vtx[i].normal);
			vtx[i].normal.x = -vtx[i].normal.x;
			vtx[i].normal.y = -vtx[i].normal.y;
			vtx[i].normal.z = -vtx[i].normal.z;
		}
	}
}

/*****************************************************************************/

static void GetBasePath(char *basePath, int maxLen, const char *modelName)
{
	strncpy(basePath, modelName, maxLen - 1);
	int pathLen = strlen(basePath);
	if(pathLen > maxLen)
		pathLen = maxLen;
	while(pathLen)
	{
		if(basePath[pathLen] == '/')
		{
			basePath[pathLen] = 0;
			return;
		}
		pathLen--;
	}
	basePath[0] = 0;
}

/*****************************************************************************/

void RescaleMesh(obj_mesh *mesh)
{
	vec3 bbmin, bbmax;
	Vertex *vtx = (Vertex *)mesh->subMesh[0].vb->data;
	bbmin = vtx[0].pos;
	bbmax = bbmin;

	for(int i = 0; i < mesh->nSubMeshes; ++i)
	{
		vtx = (Vertex *)mesh->subMesh[i].vb->data;
		int nVerts = VA_Size(mesh->subMesh[i].vb);
		for(int j = 0; j < nVerts; ++j)
		{
			vec3_max(&bbmax, &bbmax, &vtx[j].pos);
			vec3_min(&bbmin, &bbmin, &vtx[j].pos);
		}
	}

	float scale;
	vec3 size, offset;
	vec3_add(&offset, &bbmax, &bbmin);
	vec3_scale(&offset, &offset, 0.5f);
	vec3_sub(&size, &bbmax, &bbmin);
	scale = size.x;
	if(scale < size.y)
		scale = size.y;
	if(scale < size.z)
		scale = size.z;

	scale = 2.0f / scale;

	for(int i = 0; i < mesh->nSubMeshes; ++i)
	{
		vtx = (Vertex *)mesh->subMesh[i].vb->data;
		int nVerts = VA_Size(mesh->subMesh[i].vb);
		for(int j = 0; j < nVerts; ++j)
		{
			vec3_sub(&vtx[j].pos, &vtx[j].pos, &offset);
			vec3_scale(&vtx[j].pos, &vtx[j].pos, scale);
		}
	}
}

/*****************************************************************************/

obj_mesh *OBJ_LoadModel(const char *modelName)
{
	FILE *objfp;
	char basePath[256];

	obj_mesh *mesh = malloc(sizeof(obj_mesh));
	memset(mesh, 0, sizeof(obj_mesh));

	objfp = fopen(modelName, "rt");

	if(!objfp)
		return 0;

	memset(basePath, 0, sizeof(basePath));

	GetBasePath(basePath, sizeof(basePath)-1, modelName);

	DynamicArray *positions = VA_Alloc(sizeof(vec3), 0);
	DynamicArray *texcoords = VA_Alloc(sizeof(vec3), 0);
	DynamicArray *normals = VA_Alloc(sizeof(vec3), 0);
	DynamicArray *materials = VA_Alloc(sizeof(model_material), 0);

	typedef struct
	{
		int start;
		int end;
		int material;
	} submesh_index;

	DynamicArray *indexvertices = VA_Alloc(sizeof(IndexedVertex), 384);
	DynamicArray *subMeshStarts = VA_Alloc(sizeof(submesh_index), 1249);

	vec3 vector;

	while(!feof(objfp))
	{
		static char line[2048];
		static char paramline[2048];
		memset(line, 0, sizeof(line));
		memset(paramline, 0, sizeof(paramline));
	
		fgets(line, 2048, objfp);
		if(!strncmp(line, "v ", 2))
		{
			StripWhites(paramline, line + 1);
		
			int values = sscanf(paramline, "%f %f %f", &vector.x, &vector.y, &vector.z);
			assert(values == 3);
			VA_AddObject(positions, &vector);
		}
		else if(!strncmp(line, "vn ", 3))
		{
			StripWhites(paramline, line + 2);
			int values = sscanf(paramline, "%f %f %f", &vector.x, &vector.y, &vector.z);
			assert(values == 3);
			VA_AddObject(normals, &vector);
		}
		else if(!strncmp(line, "vt ", 3))
		{
			StripWhites(paramline, line + 2);
			int values = sscanf(paramline, "%f %f %f", &vector.x, &vector.y, &vector.z);
			if(values == 2)
				vector.z = 0.0f;
			assert(values >= 2);
			vector.x = 256.0f *  vector.x * 65536.0f;
			vector.y = 256.0f * -vector.y * 65536.0f;
			VA_AddObject(texcoords, &vector);
		}
		else if(!strncmp(line, "f ", 2))
		{
			StripWhites(paramline, line + 1);

			IndexedVertex indxvertex[256];
			memset(indxvertex, 0, sizeof(indxvertex));
			int nverts = getFace(indxvertex, line + 2);
			for(int i = 1; i < nverts - 1; ++i)
			{
				VA_AddObject(indexvertices, &indxvertex[0]);
				VA_AddObject(indexvertices, &indxvertex[i + 1]);
				VA_AddObject(indexvertices, &indxvertex[i]);
			}
		}
		else if(!strncmp(line, "usemtl ", 7))
		{
			submesh_index smi;
			StripWhites(paramline, line + 6);
			smi.start = VA_Size(indexvertices);
			smi.end = 0;
			smi.material = FindMaterial(paramline, materials);
			VA_AddObject(subMeshStarts, &smi);
		}
		else if(!strncmp(line, "mtllib ", 7))
		{
			char materialFileName[256];
			StripWhites(paramline, line + 6);
			snprintf(materialFileName, sizeof(materialFileName), "%s/%s", basePath, paramline);
			ReadMaterials(materialFileName, materials);
		}
	}

	fclose(objfp);

	for(int i = 0; i < VA_Size(subMeshStarts) - 1; ++i)
	{
		submesh_index *smi_curr = (submesh_index *)VA_GetObject(subMeshStarts, i);
		submesh_index *smi_next = (submesh_index *)VA_GetObject(subMeshStarts, i + 1);
		smi_curr->end = smi_next->start;
	}
	submesh_index *smi_last = VA_GetObject(subMeshStarts, VA_Size(subMeshStarts) - 1);
	smi_last->end = VA_Size(indexvertices);

	mesh->nSubMeshes = VA_Size(subMeshStarts);
	mesh->subMesh = malloc(sizeof(obj_submesh) * mesh->nSubMeshes);
	memset(mesh->subMesh, 0, sizeof(obj_submesh) * mesh->nSubMeshes);

	for(int i = 0; i < VA_Size(subMeshStarts); ++i)
	{
		submesh_index *smi = (submesh_index *)VA_GetObject(subMeshStarts, i);

		DynamicArray *rawMesh = ExplodeSubMesh(indexvertices, smi->start, smi->end, positions, normals, texcoords);

		mesh->subMesh[i].material = smi->material;
		mesh->subMesh[i].vb = VA_Alloc(sizeof(Vertex), 0);
		mesh->subMesh[i].ib = VA_Alloc(sizeof(short), VA_Size(indexvertices));

		IndexSubmesh(rawMesh, mesh->subMesh[i].vb, mesh->subMesh[i].ib);
		if(!VA_Size(normals))
			RecalcNormals(&mesh->subMesh[i]);

		VA_Free(rawMesh);
	}

	VA_Free(positions);
	VA_Free(texcoords);
	VA_Free(normals);
	VA_Free(materials);
	VA_Free(indexvertices);
	VA_Free(subMeshStarts);

	mesh->nMaterials = VA_Size(materials);
	mesh->materials = malloc(sizeof(obj_material) * mesh->nMaterials);
	memset(mesh->materials, 0, sizeof(obj_material) * mesh->nMaterials);
	for(int i = 0; i < mesh->nMaterials; ++i)
	{
		model_material *mtrl = VA_GetObject(materials, i);
		char textureFileName[256];
		snprintf(textureFileName, sizeof(textureFileName), "%s/%s", basePath, mtrl->map_kd);
		mesh->materials[i].txtr = APOLLO_LoadDDSImage(textureFileName);
	}
	RescaleMesh(mesh);

	int maxVerts = 0;
	int totalVerts = 0;
	int totalFaces = 0;

	for(int i = 0; i < mesh->nSubMeshes; ++i)
	{
		if(maxVerts < VA_Size(mesh->subMesh[i].vb))
		{
			maxVerts = VA_Size(mesh->subMesh[i].vb);
		}
		totalVerts += VA_Size(mesh->subMesh[i].vb);
		totalFaces += VA_Size(mesh->subMesh[i].ib) / 3;
	}
	printf("Model %s\nVerts %d\nFaces %d\n", modelName, totalVerts, totalFaces);

	MeshOutputFormat(mesh);

	return mesh;
}

/*****************************************************************************/

void MeshOutputFormat(obj_mesh *mesh)
{
	for(int i = 0; i < mesh->nSubMeshes; ++i)
	{
		DynamicArray *outVtx = VA_Alloc(sizeof(obj_vertex), VA_Size(mesh->subMesh[i].vb));

		Vertex *srcVtx = (Vertex *)mesh->subMesh[i].vb->data;
		obj_vertex *dstVtx = (obj_vertex *)outVtx->data;

		for(int j = 0; j < VA_Size(mesh->subMesh[i].vb); ++j)
		{
			dstVtx[j].pos = srcVtx[j].pos;
			dstVtx[j].normal = srcVtx[j].normal;
			dstVtx[j].u = srcVtx[j].uv.x;
			dstVtx[j].v = srcVtx[j].uv.y;
		}
		VA_Free(mesh->subMesh[i].vb);
		mesh->subMesh[i].vb = outVtx;
	}
}

/*****************************************************************************/

void FreeMesh(obj_mesh *mesh)
{
	for(int i = 0; i < mesh->nSubMeshes; ++i)
	{
		VA_Free(mesh->subMesh[i].vb);
		VA_Free(mesh->subMesh[i].ib);
	}
	free(mesh->subMesh);
	for(int i = 0; i < mesh->nMaterials; ++i)
	{
		if(mesh->materials[i].txtr)
		APOLLO_FreeImage(mesh->materials[i].txtr);
	}
	free(mesh->materials);
}
