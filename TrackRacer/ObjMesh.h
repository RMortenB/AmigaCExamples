#ifndef OBJMESH_H_INCLUDED
#define OBJMESH_H_INCLUDED

#include "vec3.h"
#include "apolloImage.h"
#include "dynarray.h"

struct obj_mesh;
typedef struct obj_mesh obj_mesh;
struct obj_submesh;
typedef struct obj_submesh obj_submesh;


obj_mesh *OBJ_LoadModel(const char *modelName);

typedef struct
{
	vec3 pos;
	vec3 normal; // WTAF!
	float u, v; // WTAF!!
} obj_vertex;

typedef struct
{
	apolloImage *txtr;
} obj_material;

typedef struct
{
	vec3 bbmin;
	vec3 bbmax;
} obj_aabb;

struct obj_submesh
{
	DynamicArray *vb;
	DynamicArray *ib;
	int material;
};

struct obj_mesh
{
	obj_material *materials;
	int nMaterials;

	obj_submesh *subMesh;
	int nSubMeshes;
};

#endif // OBJMESH_H_INCLUDED
