#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "trackracer.h"

/*****************************************************************************/

#define LEN_SUBDIV 8

/*****************************************************************************/

static const mat4 hermiteToBernstein =
{
	{
		{ 2.0f, -2.0f,  1.0f,  1.0f},
		{-3.0f,  3.0f, -2.0f, -1.0f},
		{ 0.0f,  0.0f,  1.0f,  0.0f},
		{ 1.0f,  0.0f,  0.0f,  0.0f}
	}
};

/*****************************************************************************/

static const mat4 derivedToBernstein =
{
	{
		{ 0.0f,  0.0f,  0.0f,  0.0f},
		{ 6.0f, -6.0f,  3.0f,  3.0f},
		{-6.0f,  6.0f, -4.0f, -2.0f},
		{ 0.0f,  0.0f,  1.0f,  0.0f}
	}
};

/*****************************************************************************/

void ResolveHermite(vec3 *pos, vec3 *dir, float t, const vec3 *p0, const vec3 *p1, const vec3 *r0, const vec3 *r1)
{
	vec4 tt;
	vec4 xp = {p0->x, p1->x, r0->x, r1->x};
	vec4 yp = {p0->y, p1->y, r0->y, r1->y};
	vec4 zp = {p0->z, p1->z, r0->z, r1->z};
	tt.w = 1.0f;
	tt.z = t;
	tt.y = t * t;
	tt.x = t * t * t;
	vec4 hB;
	vec4_tform(&hB, &hermiteToBernstein, &tt);
	pos->x = vec4_dot(&hB, &xp);
	pos->y = vec4_dot(&hB, &yp);
	pos->z = vec4_dot(&hB, &zp);
	vec4 dB;
	vec4_tform(&dB, &derivedToBernstein, &tt);
	dir->x = vec4_dot(&dB, &xp);
	dir->y = vec4_dot(&dB, &yp);
	dir->z = vec4_dot(&dB, &zp);
}

/*****************************************************************************/

float TR_ComputeKnotValues(float *knots, const vec3 *points, int nPoints)
{
	float totalLength = 0.0f;
	vec3 prev = points[nPoints - 1];
	for(int i = 0; i < nPoints; ++i)
	{
		vec3 curvePts[LEN_SUBDIV];
		vec3 p0 = points[i];
		vec3 p1 = points[(i + 1) % nPoints];
		vec3 p2 = points[(i + 2) % nPoints];

		vec3 r0;
		vec3 r1;
		vec3_sub(&r0, &prev, &p1);
		vec3_sub(&r1, &p0, &p2);
		vec3_scale(&r0, &r0, 0.5f);
		vec3_scale(&r1, &r1, 0.5f);
		prev = p0;
		vec4 xp = {p0.x, p1.x, r0.x, r1.x};
		vec4 yp = {p0.y, p1.y, r0.y, r1.y};
		vec4 zp = {p0.z, p1.z, r0.z, r1.z};

		for(int j = 0; j < LEN_SUBDIV; ++j)
		{
			vec4 t, tt;
			t.w = 1.0f;
			t.z = (float)j / (LEN_SUBDIV - 1);
			t.y = t.z * t.z;
			t.x = t.z * t.y;
			vec4_tform(&tt, &hermiteToBernstein, &t);
			curvePts[j].x = vec4_dot(&tt, &xp);
			curvePts[j].y = vec4_dot(&tt, &yp);
			curvePts[j].z = vec4_dot(&tt, &zp);
		}
		float length = 0.0f;
		for(int j = 0; j < LEN_SUBDIV - 1; ++j)
		{
			vec3 v;
			vec3_sub(&v, &curvePts[j], &curvePts[j + 1]);
			length += vec3_len(&v);
		}
		totalLength += length;
		knots[i] = totalLength;
	}
	return totalLength;
}

/*****************************************************************************/

static void ResolvePoint(vec3 *point, vec3 *direction, float t, const float *knots, const vec3 *points, int nPoints)
{
	float prevKnot = 0.0f;
	for(int i = 0; i < nPoints; ++i)
	{
		if(knots[i] > t)
		{
			int next = (i + 1) % nPoints;
			int prev = i - 1;
			if(prev < 0)
				prev += nPoints;

			int nextNext = (next + 1) % nPoints;

			float tt = (t - prevKnot) / (knots[i] - prevKnot);
			if(tt < 0.0f)
			{
				tt = 0.0f;
			}
			if(tt > 1.0f)
			{
				tt = 1.0f;
			}

			vec3 r0, r1;
			vec3_sub(&r0, &points[next], &points[prev]);
			vec3_sub(&r1, &points[nextNext], &points[i]);
			vec3_scale(&r0, &r0, 0.5f);
			vec3_scale(&r1, &r1, 0.5f);

			ResolveHermite(point, direction, tt, &points[i], &points[next], &r0, &r1);

			return;
		}
		prevKnot = knots[i];
	}
	*point = points[0];
	vec3_sub(direction, &points[1], &points[nPoints - 1]);
}

/*****************************************************************************/

void TR_ResolvePoint(vec3 *pos, vec3 *dir, tr_track *track, float t)
{
	ResolvePoint(pos, dir, t, track->knots, track->curve, track->nCurvePoints);
}

/*****************************************************************************/

int TR_GetTrackPoints(vec3 *points, vec3 *directions, float *distances, int maxPoints, const vec3 *trackPoints, int nTrackPoints, float pointDistance)
{
	float totalLength = TR_ComputeKnotValues(distances, trackPoints, nTrackPoints);
	int nPoints = (int)(totalLength / pointDistance);

	for(int i = 0; i < nPoints; ++i)
	{
		float t = totalLength * i / (nPoints);
		ResolvePoint(&points[i], &directions[i], t, distances, trackPoints, nTrackPoints);
	}
	points[nPoints] = points[0];
	directions[nPoints] = directions[0];
	return nPoints;
}
