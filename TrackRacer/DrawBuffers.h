#ifndef DRAWBUFFERS_H_INCLUDED
#define DRAWBUFFERS_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

struct VertexBuffer;
typedef struct VertexBuffer VertexBuffer;
struct IndexBuffer;
typedef struct IndexBuffer IndexBuffer;

/*****************************************************************************/

struct VertexBuffer
{
	uint8_t *data;
	int nVerts;
	int stride;
	int capacity;
};

/*****************************************************************************/

struct IndexBuffer
{
	uint16_t *data;
	int nIndx;
	int capacity;
};

/*****************************************************************************/

VertexBuffer *VB_Create(int stride, int capacity);

void VB_IncreaseCapacity(VertexBuffer *vb);
void VB_AddVertex(VertexBuffer *vb, uint8_t *v);
void VB_CopyBuffer(VertexBuffer *dst, VertexBuffer *src);
void VB_SetCapacity(VertexBuffer *vb, int capacity);
void VB_Free(VertexBuffer *vb);

/*****************************************************************************/

IndexBuffer *IB_Create(int capacity);

void IB_IncreaseCapacity(IndexBuffer *ib);
void IB_AddIndex(IndexBuffer *ib, uint16_t indx);
void IB_CopyBuffer(IndexBuffer *dst, IndexBuffer *src);
void IB_SetCapacity(IndexBuffer *ib, int capacity);
void IB_Free(IndexBuffer *ib);

/*****************************************************************************/

static inline uint8_t *VB_GetVertex(VertexBuffer *vb, int indx)
{
	return &vb->data[indx * vb->stride];
}

/*****************************************************************************/

static inline uint16_t IB_GetIndex(const IndexBuffer *ib, int indx)
{
	return ib->data[indx];
}

/*****************************************************************************/

#endif // DRAWBUFFERS_H_INCLUDED
