#include <stdlib.h>
#include <string.h>
#include "vec3.h"

#include <proto/dos.h>

#include "trackracer.h"

/*****************************************************************************/

#define MAX_TRIS_PER_LEAF 16

/*****************************************************************************/

struct tr_collision;
typedef struct tr_collision tr_collision;
struct coll_node;
typedef struct coll_node coll_node;

/*****************************************************************************/

typedef struct
{
	tr_collvertex *tris;
	int nTris;
	int maxTris;
} coll_result;

/*****************************************************************************/

struct coll_node
{
	int flags;
	tr_aabb bbox;
	int start;
	int nTris;
	int childNodes;
};

struct tr_collision
{
	tr_collvertex *vtx;
	int nVtx;
	int *indx;
	int nIndx;
	coll_node *nodes;
	int nNodes;
	int maxDepth;
};

/*****************************************************************************/

static void SwapTriangles(tr_collision *tree, vec3 *centroids, int t, int b)
{
	if((t * 3 + 2) >= tree->nIndx || t < 0)
	{
		printf("Bounds fail1\n");
		return;
	}
	if((b * 3 + 2) >= tree->nIndx || b < 0)
	{
		printf("Bounds fail2\n");
		return;
	}
	for(int i = 0; i < 3; ++i)
	{
		int tmp = tree->indx[t * 3 + i];
		tree->indx[t * 3 + i] = tree->indx[b * 3 + i];
		tree->indx[b * 3 + i] = tmp;
	}
	vec3 tmp = centroids[t];
	centroids[t] = centroids[b];
	centroids[b] = tmp;
}

/*****************************************************************************/

static int GenerateBoundingBox(tr_aabb *aabb, const tr_collvertex *vtx, const int *indx, int nIndx)
{
	if(!nIndx)
	{
		aabb->bbmin = vtx[0].pos;
		aabb->bbmax = vtx[0].pos;
		return 0;
	}
	aabb->bbmin = vtx[indx[0]].pos;
	aabb->bbmax = vtx[indx[0]].pos;

	for(int i = 0; i < nIndx; ++i)
	{
		vec3_min(&aabb->bbmin, &aabb->bbmin, &vtx[indx[i]].pos);
		vec3_max(&aabb->bbmax, &aabb->bbmax, &vtx[indx[i]].pos);
	}
	return 1;
}

/*****************************************************************************/

float SplitNode(coll_node *node, coll_node *left, coll_node *right, vec3 *splitAxis)
{
	vec3 size;
	vec3 mid;
	vec3_sub(&size, &node->bbox.bbmax, &node->bbox.bbmin);
	vec3_add(&mid, &node->bbox.bbmax, &node->bbox.bbmin);
	vec3_scale(&mid, &mid, 0.5f);
	left->bbox.bbmin = node->bbox.bbmin;
	left->bbox.bbmax = node->bbox.bbmax;
	right->bbox.bbmin = node->bbox.bbmin;
	right->bbox.bbmax = node->bbox.bbmax;

	if(size.x > size.z)
	{
		if(size.x > size.y)
		{
			// X
			splitAxis->x = 1.0f;
			splitAxis->y = 0.0f;
			splitAxis->z = 0.0f;
			left->bbox.bbmax.x = mid.x;
			right->bbox.bbmin.x = mid.x;
			return mid.x;
		}
		else
		{
			// Y
			splitAxis->x = 0.0f;
			splitAxis->y = 1.0f;
			splitAxis->z = 0.0f;
			left->bbox.bbmax.y = mid.y;
			right->bbox.bbmin.y = mid.y;
			return mid.y;
		}
	}
	else
	{
		if(size.y > size.z)
		{
			// Y
			splitAxis->x = 0.0f;
			splitAxis->y = 1.0f;
			splitAxis->z = 0.0f;
			left->bbox.bbmax.y = mid.y;
			right->bbox.bbmin.y = mid.y;
			return mid.y;
		}
	}
	// Z
	splitAxis->x = 0.0f;
	splitAxis->y = 0.0f;
	splitAxis->z = 1.0f;
	left->bbox.bbmax.z = mid.z;
	right->bbox.bbmin.z = mid.z;
	return mid.z;
}

/*****************************************************************************/

static void GetCentroids(vec3 *centroids, const tr_collvertex *vtx, int *indx, int nTris)
{
	for(int i = 0; i < nTris; ++i)
	{
		vec3_add(&centroids[i], &vtx[indx[i * 3 + 0]].pos, &vtx[indx[i * 3 + 1]].pos);
		vec3_add(&centroids[i], &centroids[i], &vtx[indx[i * 3 + 2]].pos);
		vec3_scale(&centroids[i], &centroids[i], 1.0f / 3.0f);
	}
}

/*****************************************************************************/

static void ExpandBBoxes(tr_collision *tree, int node)
{
	if(!tree->nodes[node].flags)
	{
		int left = tree->nodes[node].childNodes + 0;
		int right = tree->nodes[node].childNodes + 1;
		ExpandBBoxes(tree, left);
		ExpandBBoxes(tree, right);
		vec3_min(&tree->nodes[node].bbox.bbmin, &tree->nodes[left].bbox.bbmin, &tree->nodes[right].bbox.bbmin);
		vec3_max(&tree->nodes[node].bbox.bbmax, &tree->nodes[left].bbox.bbmax, &tree->nodes[right].bbox.bbmax);
	}
	else
	{
		int start = tree->nodes[node].start;
		int nTris = tree->nodes[node].nTris;
		tree->nodes[node].bbox.bbmin = tree->vtx[tree->indx[start]].pos;
		tree->nodes[node].bbox.bbmax = tree->vtx[tree->indx[start]].pos;
		for(int i = 0; i < nTris; ++i)
		{
			for(int j = 0; j < 3; ++j)
			{
				vec3_min(&tree->nodes[node].bbox.bbmin, &tree->nodes[node].bbox.bbmin, &tree->vtx[tree->indx[(start + i) * 3 + j]].pos);
				vec3_max(&tree->nodes[node].bbox.bbmax, &tree->nodes[node].bbox.bbmax, &tree->vtx[tree->indx[(start + i) * 3 + j]].pos);
			}
		}
	}
}

/*****************************************************************************/
static int stack[10000];

tr_collision *TR_GenerateCollision(const tr_track *track)
{
	int nTris = track->nIndx / 3;
	int stackPos = 0;
	int maxStackLen = nTris * 2 / MAX_TRIS_PER_LEAF - 2;

	tr_collision *tree = malloc(sizeof(tr_collision));
	memset(tree, 0, sizeof(tr_collision));
	tree->nodes = malloc(sizeof(coll_node) * nTris * 2);
	memset(tree->nodes, 0, sizeof(coll_node) * nTris * 2);

	tree->vtx = malloc(sizeof(tr_collvertex) * track->nVerts);
	for(int i = 0; i < track->nVerts; ++i)
	{
		tree->vtx[i].pos = track->vtx[i].pos;
		tree->vtx[i].spos = track->vtx[i].v * 4.0f;
	}
	tree->nVtx = track->nVerts;
	tree->indx = malloc(sizeof(int) * track->nIndx);
	memcpy(tree->indx, track->indx, sizeof(int) * track->nIndx);
	tree->nIndx = track->nIndx;

	int nNodes = 0;
	tree->nodes[nNodes].start = 0;
	tree->nodes[nNodes].nTris = nTris;
	nNodes++;

	vec3 *centroids = malloc(sizeof(vec3) * nTris);
	GetCentroids(centroids, tree->vtx, tree->indx, nTris);
	stack[stackPos++] = 0;

	GenerateBoundingBox(&tree->nodes[0].bbox, tree->vtx, tree->indx, tree->nIndx);
	int maxDepth = 1;
	while(stackPos > 0)
	{
		int node = stack[--stackPos];
		if(tree->nodes[node].nTris < MAX_TRIS_PER_LEAF)
		{
			continue;
		}
		int b = tree->nodes[node].start;
		int t = tree->nodes[node].start + tree->nodes[node].nTris - 1;
		tree->nodes[node].childNodes = nNodes;
		vec3 splitAxis;
		float splitVal = SplitNode(&tree->nodes[node], &tree->nodes[nNodes + 0], &tree->nodes[nNodes + 1], &splitAxis);
		int start = t;
		int end = b;
		while(t > b)
		{
			while((vec3_dot(&centroids[b], &splitAxis) < splitVal) && (t > b))
			{
				b++;
			}
			while((vec3_dot(&centroids[t], &splitAxis) > splitVal) && (t > b))
			{
				t--;
			}
			if(t > b)
			{
				SwapTriangles(tree, centroids, t, b);
			}
		}
		if(t <= start)
		{
			tree->nodes[node].bbox.bbmin = tree->nodes[nNodes + 0].bbox.bbmin;
			tree->nodes[node].bbox.bbmax = tree->nodes[nNodes + 0].bbox.bbmax;
			stack[stackPos++] = node;
		}
		else if(b >= end)
		{
			tree->nodes[node].bbox.bbmin = tree->nodes[nNodes + 1].bbox.bbmin;
			tree->nodes[node].bbox.bbmax = tree->nodes[nNodes + 1].bbox.bbmax;
			stack[stackPos++] = node;
		}
		else
		{
			if(stackPos < maxStackLen - 2)
			{
				tree->nodes[nNodes + 0].start = tree->nodes[node].start;
				tree->nodes[nNodes + 0].nTris = t - tree->nodes[node].start;
				tree->nodes[nNodes + 0].flags = 1;
				tree->nodes[nNodes + 1].start = t;
				tree->nodes[nNodes + 1].nTris = tree->nodes[node].nTris - tree->nodes[nNodes + 0].nTris;
				tree->nodes[nNodes + 1].flags = 1;
				tree->nodes[node].flags = 0;
				stack[stackPos++] = nNodes;
				stack[stackPos++] = nNodes + 1;
				nNodes += 2;
				if(maxDepth < stackPos)
					maxDepth = stackPos;
			}
			else
			{
				printf("stack blown!\n");
				return NULL;	
			}
		}
	}
	tree->nNodes = nNodes;
	tree->maxDepth = maxDepth;
	free(centroids);
	ExpandBBoxes(tree, 0);
	return tree;
}

/*****************************************************************************/

static int TriangleInBB(const tr_aabb *bbox, const vec3 *p0, const vec3 *p1, const vec3 *p2)
{
	if((p0->x < bbox->bbmin.x) && (p1->x < bbox->bbmin.x) && (p2->x < bbox->bbmin.x)) return 0;
	if((p0->y < bbox->bbmin.y) && (p1->y < bbox->bbmin.y) && (p2->x < bbox->bbmin.y)) return 0;
	if((p0->z < bbox->bbmin.z) && (p1->z < bbox->bbmin.z) && (p2->z < bbox->bbmin.z)) return 0;
	if((p0->x > bbox->bbmax.x) && (p1->x > bbox->bbmax.x) && (p2->x > bbox->bbmax.x)) return 0;
	if((p0->y > bbox->bbmax.y) && (p1->y > bbox->bbmax.y) && (p2->x > bbox->bbmax.y)) return 0;
	if((p0->z > bbox->bbmax.z) && (p1->z > bbox->bbmax.z) && (p2->z > bbox->bbmax.z)) return 0;

	return 1;
}

/*****************************************************************************/

static int AABBOverlaps(const tr_aabb *bb0, const tr_aabb *bb1)
{
	if(bb0->bbmin.x > bb1->bbmax.x) return 0;
	if(bb0->bbmin.y > bb1->bbmax.y) return 0;
	if(bb0->bbmin.z > bb1->bbmax.z) return 0;
	if(bb0->bbmax.x < bb1->bbmin.x) return 0;
	if(bb0->bbmax.y < bb1->bbmin.y) return 0;
	if(bb0->bbmax.z < bb1->bbmin.z) return 0;

	return 1;
}

/*****************************************************************************/

static void getTriangles(const tr_collision *tree, coll_result *result, const tr_aabb *bbox, int node)
{
	if(!AABBOverlaps(bbox, &tree->nodes[node].bbox))
		return;
	if(!tree->nodes[node].flags)
	{
		int left = tree->nodes[node].childNodes + 0;
		int right = left + 1;
		getTriangles(tree, result, bbox, left);
		getTriangles(tree, result, bbox, right);
	}
	else
	{
		coll_node *leaf = &tree->nodes[node];
		for(int i = 0; i < leaf->nTris; ++i)
		{
			int i0 = tree->indx[(leaf->start + i) * 3 + 0];
			int i1 = tree->indx[(leaf->start + i) * 3 + 1];
			int i2 = tree->indx[(leaf->start + i) * 3 + 2];
			if(TriangleInBB(bbox, &tree->vtx[i0].pos, &tree->vtx[i1].pos, &tree->vtx[i2].pos))
			{
				if(result->nTris < result->maxTris)
				{
					result->tris[result->nTris * 3 + 0] = tree->vtx[i0];
					result->tris[result->nTris * 3 + 1] = tree->vtx[i1];
					result->tris[result->nTris * 3 + 2] = tree->vtx[i2];
					result->nTris++;
				}
			}
		}
	}
}

/*****************************************************************************/

int TR_GetTrianglesInAABB(tr_track *track, tr_collvertex *triangles, int maxTriangles, tr_aabb *bbox)
{
	int node = 0;
	coll_result result;
	result.tris = triangles;
	result.nTris = 0;
	result.maxTris = maxTriangles;

	getTriangles(track->collision, &result, bbox, node);

	return result.nTris;
}
