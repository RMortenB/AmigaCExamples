#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <float.h>

#include "trackracer.h"
#include "Maggie.h"

#include "apolloImage.h"

/*****************************************************************************/

static const vec3 testTrack3[] =
{

	{   50.0f, 0.0f,   0.0f }, {  75.0f, 3.0f, 25.0f },
	{  100.0f, 0.0f,  50.0f }, {  75.0f, 3.0f, 75.0f },
	{   50.0f, 0.0f, 100.0f }, {  25.0f, 3.0f, 75.0f },
	{    0.0f, 0.0f,  50.0f }, {  25.0f, 3.0f, 25.0f }
};

static const int nPoints3 = sizeof(testTrack3) / sizeof(vec3);
static const char *textureName3 = "trackTxtr.dds";

/*****************************************************************************/

static const vec3 testTrack2[] =
{
	{  9.0f * 2.0f, 0.0f, 26.0f * 2.0f }, {  7.0f * 2.0f, 0.0f, 29.0f * 2.0f }, {  4.0f * 2.0f, 0.0f, 31.0f * 2.0f },
	{  2.0f * 2.0f, 0.1f, 35.0f * 2.0f }, {  2.0f * 2.0f, 0.2f, 48.0f * 2.0f }, {  4.0f * 2.0f, 0.4f, 51.0f * 2.0f },
	{  7.0f * 2.0f, 0.5f, 52.0f * 2.0f }, { 16.0f * 2.0f, 0.6f, 52.0f * 2.0f }, { 22.0f * 2.0f, 0.5f, 49.0f * 2.0f },
	{ 19.0f * 2.0f, 0.4f, 40.0f * 2.0f }, { 26.0f * 2.0f, 0.3f, 30.0f * 2.0f }, { 18.0f * 2.0f, 0.2f, 24.0f * 2.0f },
	{ 25.0f * 2.0f, 0.1f, 16.0f * 2.0f }, { 20.0f * 2.0f, 0.0f, 11.0f * 2.0f }, { 18.0f * 2.0f, 0.0f,  8.0f * 2.0f },
	{ 14.0f * 2.0f, 0.0f,  7.0f * 2.0f }, { 11.0f * 2.0f, 0.0f,  9.0f * 2.0f }, {  9.0f * 2.0f, 0.0f, 13.0f * 2.0f },
	{  9.0f * 2.0f, 0.0f, 19.5f * 2.0f }
};

static const int nPoints2 = sizeof(testTrack2) / sizeof(vec3);
static const char *textureName2 = "trackTxtrS_DXT1.dds";

/*****************************************************************************/

static const vec3 testTrack1[] =
{
	{ 4.0f * 2.0f, 0.0f, 22.0f * 2.0f},{ 4.0f * 2.0f, 0.0f, 34.0f * 2.0f}, {26.0f * 2.0f, 0.0f, 49.0f * 2.0f}, {34.0f * 2.0f, 0.0f, 46.0f * 2.0f},
	{40.0f * 2.0f, 0.0f, 49.0f * 2.0f}, {47.0f * 2.0f, 0.0f, 45.0f * 2.0f}, {55.0f * 2.0f, 0.0f, 48.0f * 2.0f}, {65.0f * 2.0f, 0.0f, 45.0f * 2.0f},
	{64.0f * 2.0f, 0.0f, 18.0f * 2.0f}, {57.0f * 2.0f, 1.0f, 14.0f * 2.0f}, {51.0f * 2.0f, 2.0f, 15.0f * 2.0f}, {48.0f * 2.0f, 3.0f, 20.0f * 2.0f},
	{40.0f * 2.0f, 2.0f, 22.0f * 2.0f}, {32.0f * 2.0f, 2.0f, 18.0f * 2.0f}, {26.0f * 2.0f, 1.0f,  7.0f * 2.0f}, {19.0f * 2.0f, 0.0f,  4.0f * 2.0f},
	{ 5.0f * 2.0f, 0.0f,  7.0f * 2.0f}
};

static const int nPoints1 = sizeof(testTrack1) / sizeof(vec3);
static const char *textureName1 = "trackTxtrFuture.dds";

/*****************************************************************************/

static const vec3 testTrack0[] =
{
	{  8.0f, -3.0f, 29.0f }, {  8.0f, -3.0f, 24.0f }, {  8.0f, -3.0f, 18.0f }, {  9.0f, -3.0f, 13.0f },
	{ 12.0f, -2.0f, 10.0f }, { 17.0f,  0.0f,  6.0f }, { 21.0f,  0.0f,  5.0f }, { 26.0f,  0.0f,  6.0f },
	{ 30.0f,  0.0f,  8.0f }, { 35.0f,  0.0f,  9.0f }, { 40.0f,  0.0f,  8.0f }, { 46.0f,  0.0f, 12.0f },
	{ 47.0f,  0.0f, 16.0f }, { 47.0f,  0.0f, 21.0f }, { 44.0f,  0.0f, 24.0f }, { 39.0f,  0.0f, 24.0f },
	{ 32.0f,  0.0f, 22.0f }, { 27.0f,  0.0f, 20.0f }, { 20.0f,  0.0f, 19.0f }, { 18.0f,  0.0f, 20.0f },
	{ 17.0f,  0.0f, 23.0f }, { 17.0f,  0.0f, 30.0f }, { 18.0f,  0.0f, 37.0f }, { 22.0f,  0.0f, 43.0f },
	{ 26.0f,  0.0f, 44.0f }, { 39.0f,  0.0f, 42.0f }, { 48.0f,  0.0f, 41.0f }, { 54.0f,  0.0f, 41.0f },
	{ 56.0f,  0.0f, 42.0f }, { 57.0f,  0.0f, 44.0f }, { 56.0f,  0.0f, 50.0f }, { 54.0f,  0.0f, 57.0f },
	{ 50.0f,  0.0f, 59.0f }, { 46.0f,  0.0f, 59.0f }, { 39.0f,  0.0f, 55.0f }, { 33.0f,  0.0f, 53.0f },
	{ 27.0f,  0.0f, 53.0f }, { 14.0f,  0.0f, 53.0f }, { 11.0f,  0.0f, 51.0f }, {  9.0f, -2.0f, 45.0f },
	{  8.0f, -3.0f, 35.0f }
};

static const int nPoints0 = sizeof(testTrack0) / sizeof(vec3);
static const char *textureName0 = "trackTxtrS_DXT1.dds";

/*****************************************************************************/

static void SetPixel(MaggieFormat *pixels, int x, int y, int col)
{
	if(x < 0)
		return;
	if(y < 0)
		return;
	if(x >= MAGGIE_XRES)
		return;
	if(y >= MAGGIE_YRES)
		return;
	pixels[y * MAGGIE_XRES + x] = col;
}

/*****************************************************************************/

static const float trackSegmentLength = 1.4f;

/*****************************************************************************/

#define MAX_TRACK_POINTS 2048

static int nPointsTmp;
static vec3 trackPointsTmp[MAX_TRACK_POINTS];
static vec3 trackVelsTmp[MAX_TRACK_POINTS];
static float trackDistancesTmp[MAX_TRACK_POINTS];

/*****************************************************************************/

tr_track *TR_GenerateTrack(const vec3 *curve, int nTrackPoints, const char *textureName)
{
	int nPoints = TR_GetTrackPoints(trackPointsTmp, trackVelsTmp, trackDistancesTmp, MAX_TRACK_POINTS, curve, nTrackPoints, trackSegmentLength);
	trackPointsTmp[nPoints] = trackPointsTmp[0];
	trackVelsTmp[nPoints] = trackVelsTmp[0];
	nPoints++;

	tr_track *track = malloc(sizeof(tr_track));
	memset(track, 0, sizeof(tr_track));

	track->nCurvePoints = nTrackPoints;
	track->curve = malloc(sizeof(vec3) * nTrackPoints);
	memcpy(track->curve, curve, sizeof(vec3) * nTrackPoints);

	for(int i = 0; i < nPoints; ++i)
	{
		vec3_normalise(&trackVelsTmp[i], &trackVelsTmp[i]);
	}

	track->knots = malloc(sizeof(float) * (nTrackPoints));
	memcpy(track->knots, trackDistancesTmp, sizeof(float) * (nTrackPoints));
	track->curveLength = track->knots[nTrackPoints - 1];

	track->nVerts = nPoints * 6;
	track->vtx = malloc(track->nVerts * sizeof(obj_vertex));
	memset(track->vtx, 0, track->nVerts * sizeof(obj_vertex));

	track->nIndx = (nPoints - 1) * 30;
	track->indx = malloc(track->nIndx * sizeof(int));
	memset(track->indx, 0, track->nIndx * sizeof(int));

	vec3 up = {0.0f, 1.0f, 0.0f};
	vec3 prevVel = trackVelsTmp[nPoints - 1];
	float angleAvg = 0.0f;

	for(int i = 0; i < nPoints; ++i)
	{
		vec3 sideOffsets[4], offset, crs;
		float vCoord = (track->curveLength * i / nPoints) * 0.25f;

		vec3_cross(&crs, &trackVelsTmp[i], &up); // This up needs rotating..
#if 0
		vec3 v0, v1;

		vec3_sub(&v1, &trackPointsTmp[(i + 1) % nPoints], &trackPointsTmp[i]);
		vec3_sub(&v0, &trackPointsTmp[(i - 1 + nPoints) % nPoints], &trackPointsTmp[i]);
#else
		vec3 v0 = { trackVelsTmp[(i + 2) % nPoints].x, 0.0f, trackVelsTmp[(i + 2) % nPoints].z };
		vec3 v1 = { prevVel.x, 0.0f, prevVel.z };
#endif
		prevVel = trackVelsTmp[i];

		vec3_normalise(&v0, &v0);
		vec3_normalise(&v1, &v1);
		vec3 devCrs;
		vec3_cross(&devCrs, &v0, &v1);
		float angle = -asinf(devCrs.y) * 0.5f;
		angleAvg = angleAvg * 0.8f + angle * 0.2f;

		quat4 zQuat;
		mat4 zMat;
		quat4_AxisAngle(&zQuat, &trackVelsTmp[i], angleAvg);
		mat4_FromQuat(&zMat, &zQuat);
		vec3_tform(&crs, &zMat, &crs, 0.0f);

		vec3_scale(&offset, &crs, trackSegmentLength * 0.5f);

		vec3_scale(&sideOffsets[0], &crs, trackSegmentLength * 1.0f);
		vec3_scale(&sideOffsets[1], &crs, trackSegmentLength * 0.6f);
		vec3_scale(&sideOffsets[2], &crs, trackSegmentLength * 0.6f);
		vec3_scale(&sideOffsets[3], &crs, trackSegmentLength * 1.0f);

		sideOffsets[0].y += 0.1f;
		sideOffsets[1].y += 0.1f;
		sideOffsets[2].y -= 0.1f;
		sideOffsets[3].y -= 0.1f;

		vec3 norm;
		vec3_cross(&norm, &crs, &trackVelsTmp[i]);
		vec3_normalise(&norm, &norm);

		vec3_add(&track->vtx[i * 6 + 0].pos, &trackPointsTmp[i], &sideOffsets[0]);
		track->vtx[i * 6 + 0].u = 0.0f;
		track->vtx[i * 6 + 0].v = vCoord;
		track->vtx[i * 6 + 0].normal = norm;

		vec3_add(&track->vtx[i * 6 + 1].pos, &trackPointsTmp[i], &sideOffsets[1]);
		track->vtx[i * 6 + 1].u = 0.2f;
		track->vtx[i * 6 + 1].v = vCoord;
		track->vtx[i * 6 + 1].normal = norm;

		vec3_add(&track->vtx[i * 6 + 2].pos, &trackPointsTmp[i], &offset);
		track->vtx[i * 6 + 2].u = 0.25f;
		track->vtx[i * 6 + 2].v = vCoord;
		track->vtx[i * 6 + 2].normal = norm;

		vec3_sub(&track->vtx[i * 6 + 3].pos, &trackPointsTmp[i], &offset);
		track->vtx[i * 6 + 3].u = 0.75f;
		track->vtx[i * 6 + 3].v = vCoord;
		track->vtx[i * 6 + 3].normal = norm;

		vec3_sub(&track->vtx[i * 6 + 4].pos, &trackPointsTmp[i], &sideOffsets[2]);
		track->vtx[i * 6 + 4].u = 0.8f;
		track->vtx[i * 6 + 4].v = vCoord;
		track->vtx[i * 6 + 4].normal = norm;

		vec3_sub(&track->vtx[i * 6 + 5].pos, &trackPointsTmp[i], &sideOffsets[3]);
		track->vtx[i * 6 + 5].u = 1.0f;
		track->vtx[i * 6 + 5].v = vCoord;
		track->vtx[i * 6 + 5].normal = norm;
	}

	for(int i = 0; i < 6; ++i)
	{
		track->vtx[(nPoints - 1) * 6 + i].pos = track->vtx[i].pos;
		track->vtx[(nPoints - 1) * 6 + i].normal = track->vtx[i].normal;
	}
	int currIndx = 0;
	for(int i = 0; i < nPoints - 1; ++i)
	{
		track->indx[currIndx++] = (i + 0) * 6 + 0 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 0 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 0 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 0 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 0 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 0 + 0;

		track->indx[currIndx++] = (i + 0) * 6 + 1 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 1 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 1 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 1 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 1 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 1 + 0;

		track->indx[currIndx++] = (i + 0) * 6 + 4 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 4 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 4 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 4 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 4 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 4 + 0;

		track->indx[currIndx++] = (i + 0) * 6 + 2 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 2 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 2 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 2 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 2 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 2 + 0;

		track->indx[currIndx++] = (i + 0) * 6 + 3 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 3 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 3 + 0;
		track->indx[currIndx++] = (i + 0) * 6 + 3 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 3 + 1;
		track->indx[currIndx++] = (i + 1) * 6 + 3 + 0;
	}

	int indicesPerMesh = 60 * 3;
	track->nSubMeshes = (track->nIndx + (indicesPerMesh - 1)) / indicesPerMesh;
	track->subMeshes = malloc(sizeof(tr_submesh) * track->nSubMeshes);
	memset(track->subMeshes, 0, sizeof(tr_submesh) * track->nSubMeshes);

	int indicesLeft = track->nIndx;

	for(int i = 0; i < track->nSubMeshes; ++i)
	{
		track->subMeshes[i].startIndx = indicesPerMesh * i;
		track->subMeshes[i].nIndx = indicesLeft < indicesPerMesh ? indicesLeft : indicesPerMesh;
		indicesLeft -= indicesPerMesh;
		track->subMeshes[i].bbmin.x = FLT_MAX;
		track->subMeshes[i].bbmin.y = FLT_MAX;
		track->subMeshes[i].bbmin.z = FLT_MAX;
		track->subMeshes[i].bbmax.x = -FLT_MAX;
		track->subMeshes[i].bbmax.y = -FLT_MAX;
		track->subMeshes[i].bbmax.z = -FLT_MAX;
		for(int j = 0; j < track->subMeshes[i].nIndx; ++j)
		{
			int indx = track->indx[track->subMeshes[i].startIndx + j];
			vec3_min(&track->subMeshes[i].bbmin, &track->subMeshes[i].bbmin, &track->vtx[indx].pos);
			vec3_max(&track->subMeshes[i].bbmax, &track->subMeshes[i].bbmax, &track->vtx[indx].pos);
		}
		vec3 bbrel;
		vec3_sub(&bbrel, &track->subMeshes[i].bbmax, &track->subMeshes[i].bbmin);
		vec3_scale(&bbrel, &bbrel, 0.05f);
		vec3_sub(&track->subMeshes[i].bbmin, &track->subMeshes[i].bbmin, &bbrel);
		vec3_add(&track->subMeshes[i].bbmax, &track->subMeshes[i].bbmax, &bbrel);
	}

	for(int i = 0; i < track->nSubMeshes; ++i)
	{
		int begin = track->subMeshes[i].startIndx;
		int end = track->subMeshes[i].startIndx + track->subMeshes[i].nIndx - 3;
		while(begin <= end)
		{
			int i0 = track->indx[begin + 0];
			int i1 = track->indx[begin + 1];
			int i2 = track->indx[begin + 2];
			track->indx[begin + 0] = track->indx[end + 0];
			track->indx[begin + 1] = track->indx[end + 1];
			track->indx[begin + 2] = track->indx[end + 2];
			track->indx[end + 0] = i0;
			track->indx[end + 1] = i1;
			track->indx[end + 2] = i2;
			begin += 3;
			end -= 3;
		}
	}
	track->txtr = APOLLO_LoadDDSImage(textureName);
	if(!track->txtr)
		printf("No texture %s!\n", textureName);

	nPointsTmp = nPoints;

//	printf("Collision\n");
//	track->collision = TR_GenerateCollision(track);
//	printf("Collision Done\n");

	return track;
}

/*****************************************************************************/

tr_track *TR_GenerateTestTrack(int track)
{
	switch(track)
	{
		case 0 : return TR_GenerateTrack(testTrack0, nPoints0, textureName0);
		case 1 : return TR_GenerateTrack(testTrack1, nPoints1, textureName1);
		case 2 : return TR_GenerateTrack(testTrack2, nPoints2, textureName2);
		case 3 : return TR_GenerateTrack(testTrack3, nPoints3, textureName3);
	}
	return NULL;
}

/*****************************************************************************/
/*****************************************************************************/

void TR_PlotTrack(MaggieFormat *pixels, tr_track *track)
{
	// vec3 up = {0.0f, 1.0f, 0.0f};
	float scale = 1.0f;
	for(int i = 0; i < track->nCurvePoints; ++i)
	{
		int x = (track->curve[i].x * scale) + 0.5f;
		int y = (track->curve[i].z * scale) + 0.5f;
		SetPixel(pixels, x, y, ~0);
	}
}

/*****************************************************************************/
