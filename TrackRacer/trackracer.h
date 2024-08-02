#ifndef TRACKRACER_H_INCLUDED
#define TRACKRACER_H_INCLUDED

#include "vec3.h"
#include "Maggie.h"
#include "apolloImage.h"
#include "ObjMesh.h"

struct tr_player;
typedef struct tr_player tr_player;
struct tr_collision;
typedef struct tr_collision tr_collision;
struct tr_aabb;
typedef struct tr_aabb tr_aabb;
struct tr_collvertex;
typedef struct tr_collvertex tr_collvertex;

/*****************************************************************************/

struct tr_aabb
{
	vec3 bbmin;
	vec3 bbmax;
};

typedef struct
{
	int startIndx;
	int nIndx;
	int startVtx;
	int nVtx;
	vec3 bbmin;
	vec3 bbmax;
} tr_submesh;

typedef struct
{
	tr_submesh *subMeshes;
	int nSubMeshes;

	obj_vertex *vtx;
	int nVerts;
	int *indx;
	int nIndx;

	int nCurvePoints;
	vec3 *curve;
	float *knots;
	float curveLength;

	apolloImage *txtr;

	tr_player *player;

	tr_collision *collision;
} tr_track;

struct tr_collvertex
{
	vec3 pos;
	float spos;
};

void TR_ResolvePoint(vec3 *pos, vec3 *dir, tr_track *track, float t);
int TR_GetTrackPoints(vec3 *points, vec3 *directions, float *distances, int maxPoints, const vec3 *trackPoints, int nTrackPoints, float pointDistance);
float TR_ComputeKnotValues(float *knots, const vec3 *points, int nPoints);

tr_track *TR_GenerateTestTrack(int track);
tr_track *TR_GenerateTrack(const vec3 *curve, int nTrackPoints, const char *textureName);

void TR_PlotTrack(MaggieFormat *pixels, tr_track *track);
void TR_DrawTrack(MaggieFormat *pixels, tr_track *track, mat4 *modelView, mat4 *projection);

tr_player *TR_CreatePlayer(tr_track *track);
void TR_UpdatePlayer(tr_track *track, tr_player *player);
void TR_DrawPlayer(MaggieFormat *pixels, tr_player *player, mat4 *modelView, mat4 *projection);
void TR_GetPlayerPos(tr_player *player, vec3 *pos);
void TR_GetPlayerOrientation(const tr_player *player, vec3 *orientation);

tr_collision *TR_GenerateCollision(const tr_track *track);
int TR_GetTrianglesInAABB(tr_track *track, tr_collvertex *triangles, int maxTriangles, tr_aabb *bbox);

// Debug functions
void TextOut(MaggieFormat *screen, int xpos, int ypos, const char *fmt, ...);


#endif // TRACKRACER_H_INCLUDED
