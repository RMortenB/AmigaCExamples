#include <string.h>

#include <stdlib.h>
#include "DrawBuffers.h"

/*****************************************************************************/

VertexBuffer *VB_Create(int stride, int capacity)
{
	VertexBuffer *vb = malloc(sizeof(VertexBuffer));
	vb->capacity = capacity;
	vb->stride = stride;
	vb->nVerts = 0;
	vb->data = malloc(vb->stride * vb->capacity);

	return vb;
}

/*****************************************************************************/

void VB_IncreaseCapacity(VertexBuffer *vb)
{
	if(!vb->capacity)
		vb->capacity = 16;

	vb->capacity *= 2;

	uint8_t *newBuffer = malloc(vb->stride * vb->capacity);

	if(vb->data)
	{
		memcpy(newBuffer, vb->data, vb->nVerts * vb->stride);
		free(vb->data);
	}
	vb->data = newBuffer;
}

/*****************************************************************************/

void VB_SetCapacity(VertexBuffer *vb, int capacity)
{
	vb->capacity = capacity;

	if(vb->nVerts < capacity)
		vb->nVerts = capacity;

	uint8_t *newBuffer = malloc(vb->stride * vb->capacity);

	if(vb->data)
	{
		memcpy(newBuffer, vb->data, vb->nVerts * vb->stride);
		free(vb->data);
	}
	vb->data = newBuffer;
}

/*****************************************************************************/

void VB_AddVertex(VertexBuffer *vb, uint8_t *v)
{
	if((vb->nVerts + 1) >= vb->capacity)
		VB_IncreaseCapacity(vb);
	memcpy(&vb->data[vb->stride * vb->nVerts++], v, vb->stride);
}

/*****************************************************************************/

void VB_CopyBuffer(VertexBuffer *dst, VertexBuffer *src)
{
	if(dst->capacity < src->nVerts)
		VB_SetCapacity(dst, src->nVerts);
	memcpy(dst->data, src->data, src->stride * src->nVerts);
	dst->nVerts = src->nVerts;
}

/*****************************************************************************/

void VB_Free(VertexBuffer *vb)
{
	if(vb)
	{
		free(vb->data);
		free(vb);
	}
}

/*****************************************************************************/

IndexBuffer *IB_Create(int capacity)
{
	IndexBuffer *ib = malloc(sizeof(IndexBuffer));
	memset(ib, 0, sizeof(IndexBuffer));
	ib->capacity = capacity;
	ib->data = malloc(sizeof(uint16_t) * ib->capacity);

	return ib;
}

/*****************************************************************************/

void IB_IncreaseCapacity(IndexBuffer *ib)
{
	if(!ib->capacity)
		ib->capacity = 16;

	ib->capacity <<= 1;

	uint16_t *newBuffer = malloc(sizeof(uint16_t) * ib->capacity);
	if(ib->data)
	{
		memcpy(newBuffer, ib->data, ib->nIndx * sizeof(uint16_t));
		free(ib->data);
	}
	ib->data = newBuffer;
}

/*****************************************************************************/

void IB_AddIndex(IndexBuffer *ib, uint16_t indx)
{
	if((ib->nIndx + 1) >= ib->capacity)
		IB_IncreaseCapacity(ib);
	ib->data[ib->nIndx++] = indx;
}

/*****************************************************************************/

void IB_SetCapacity(IndexBuffer *ib, int capacity)
{
	ib->capacity = capacity;

	if(ib->nIndx < capacity)
		ib->nIndx = capacity;

	uint16_t *newBuffer = malloc(sizeof(uint16_t) * ib->capacity);

	if(ib->data)
	{
		memcpy(newBuffer, ib->data, ib->nIndx * sizeof(uint16_t));
		free(ib->data);
	}
	ib->data = newBuffer;
}

/*****************************************************************************/

void IB_CopyBuffer(IndexBuffer *dst, IndexBuffer *src)
{
	if(dst->capacity < src->nIndx)
		IB_SetCapacity(dst, src->nIndx);
	memcpy(dst->data, src->data, sizeof(short) * src->nIndx);
	dst->nIndx = src->nIndx;
}

/*****************************************************************************/

void IB_Free(IndexBuffer *ib)
{
	if(ib)
	{
		free(ib->data);
		free(ib);
	}
}