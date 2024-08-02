#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include <image.h>

#include "DrawBuffers.h"
#include "vec3.h"
#include <apolloImage.h>

/*****************************************************************************/

#ifndef OUTPUT16
# define OUTPUT16 1
#endif

/*****************************************************************************/

#if OUTPUT16
typedef uint16_t PixelFormat;
#else
typedef uint32_t PixelFormat;
#endif

/*****************************************************************************/

struct SubMesh;
struct Mesh;
struct Material;
typedef struct SubMesh SubMesh;
typedef struct Mesh Mesh;
typedef struct Material Material;

/*****************************************************************************/

struct Material
{
	apolloImage *txtr;
};

/*****************************************************************************/

struct SubMesh
{
	VertexBuffer *vb;
	IndexBuffer *ib;
	int material;
};

/*****************************************************************************/

struct Mesh
{
	Material *materials;
	int nMaterials;

	SubMesh *subMesh;
	int nSubMeshes;

	VertexBuffer *transformedVB;
	VertexBuffer *unClippedVB;
};

/*****************************************************************************/

struct Vertex;
struct DrawVertex;
typedef struct Vertex Vertex;
typedef struct DrawVertex DrawVertex;

struct Vertex
{
	vec3 pos;
	vec2 uv;
	vec3 normal;
};

/*****************************************************************************/

struct DrawVertex
{
	vec4 pos;
	vec3 uvw;
	float intensity;
};

/*****************************************************************************/

#define RENDERFLAGS_SHOWDEPTH		0x0001
#define RENDERFLAGS_WIREFRAME		0x0002
#define RENDERFLAGS_DISABLE_TEXTURE	0x0004
#define RENDERFLAGS_SHOW_CCC_COUNTS	0x0008

/*****************************************************************************/

void DrawMesh(img_image *img, Mesh *mesh, mat4 *matrix, mat4 *modelview, uint32_t flags);

/*****************************************************************************/

void FreeMesh(Mesh *mesh);

/*****************************************************************************/

Mesh *LoadObjModel(const char *modelfile);

/*****************************************************************************/

void BeginFrame(img_image *img);
void EndFrame();

/*****************************************************************************/

#endif // MESH_H_INCLUDED
