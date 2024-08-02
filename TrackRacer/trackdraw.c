#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Maggie.h"
#include "trackracer.h"

#include "vec3.h"

#define MAX_VERTICES 4096

/*****************************************************************************/

typedef enum
{
	CLIP_OUTSIDE,
	CLIP_INSIDE,
	CLIP_PARTIAL
} clipResult;

/*****************************************************************************/

// static uint16_t zbuffer[1280*720] __attribute__ ((aligned(16))); // Maybe..

typedef struct
{
	float xPos;
	float oow;
	float uow;
	float vow;
//	float zow;
	float iow;
} EdgePos;

/*****************************************************************************/

static EdgePos leftEdge[720];
static EdgePos rightEdge[720];

/*****************************************************************************/

typedef struct
{
	vec4 pos;
	float u, v;
	float intensity;
} tr_finalVertex;

/*****************************************************************************/

#define PIXEL_RUN 16

static tr_finalVertex transformBuffer[MAX_VERTICES];
static tr_finalVertex drawBuffer[MAX_VERTICES];
static uint8_t clipCodeBuffer[MAX_VERTICES];

/*****************************************************************************/

static void SetupHWSpans(const apolloImage *texture)
{
	uint32_t *imageData = APOLLO_GetImageData(texture);
	uint16_t texSize;
	switch(texture->width)
	{
		case 512 : texSize = 9; break;
		case 256 : texSize = 8; break;
		case 128 : texSize = 7; break;
		case  64 : texSize = 6; break;
		default  : texSize = 6; break;
	}
	maggieRegs->texture = imageData;
	maggieRegs->texSize = texSize;
	maggieRegs->modulo = MAGGIE_PIXSIZE;
	maggieRegs->lightRGBA = 0x00ffffff;
	maggieRegs->mode = (MAGGIE_PIXSIZE == 2) ? 5 : 1;
}

/*****************************************************************************/

static void SetTexture(const apolloImage *texture, int level)
{
	void *imageData = APOLLO_GetImageMipMapData(texture, level);
	uint16_t texSize;
	switch(texture->width >> level)
	{
		case 512 : texSize = 9; break;
		case 256 : texSize = 8; break;
		case 128 : texSize = 7; break;
		case  64 : texSize = 6; break;
		default  : texSize = 6; break;
	}
	maggieRegs->texture = imageData;
	maggieRegs->texSize = texSize;
}

/*****************************************************************************/

static void DrawHardwareSpanZBuffered(	MaggieFormat *dest,
										uint16_t *zDest,
										int len,
										uint32_t ZZzz,
										uint32_t UUuu,
										uint32_t VVvv,
										int32_t Ii,
										int32_t dZZzz,
										uint32_t dUUuu,
										uint32_t dVVvv,
										int32_t dIi)
{
	maggieRegs->depthDest = zDest;
	maggieRegs->depthStart = ZZzz;
	maggieRegs->depthDelta = dZZzz;
	maggieRegs->pixDest = dest;
	maggieRegs->uCoord = UUuu;
	maggieRegs->vCoord = VVvv;
	maggieRegs->light = Ii;
	maggieRegs->uDelta = dUUuu;
	maggieRegs->vDelta = dVVvv;
	maggieRegs->lightDelta = dIi;
	maggieRegs->startLength = len;
}

/*****************************************************************************/

static void DrawHardwareSpan(   MaggieFormat *dest,
                                int len,
                                uint32_t UUuu,
                                uint32_t VVvv,
                                int32_t Ii,
                                uint32_t dUUuu,
                                uint32_t dVVvv,
                                int32_t dIi)
{
	maggieRegs->pixDest = dest;
	maggieRegs->uCoord = UUuu;
	maggieRegs->vCoord = VVvv;
	maggieRegs->light = Ii;
	maggieRegs->uDelta = dUUuu;
	maggieRegs->vDelta = dVVvv;
	maggieRegs->lightDelta = dIi;
	maggieRegs->startLength = len;
}

/*****************************************************************************/

static void DrawSpanZBuffer(MaggieFormat *destCol, uint16_t *zBuffer, int len, uint32_t Zz, int32_t Ii, int32_t dZz, int32_t dIi)
{
	for(int i = 0; i < len; ++i)
	{
		uint16_t z = (Zz >> 16);
		if(*zBuffer > z)
		{
			int ti = Ii >> 8;
			if(ti > 255)
				ti = 255;
			*zBuffer = z;
			*destCol = ti * 0x010101;
		}
		destCol++;
		zBuffer++;
		Ii += dIi;
		Zz += dZz;
	}
}

/*****************************************************************************/

static void DrawSoftSpan(MaggieFormat *destCol, int len, int32_t Ii, int32_t dIi)
{
	for(int i = 0; i < len; ++i)
	{
		int ti = Ii >> 8;
		if(ti > 255)
			ti = 255;
		*destCol = ti * 0x010101;
		destCol++;
		Ii += dIi;
	}
}

/*****************************************************************************/

static void DrawSpans(MaggieFormat *pixels, const apolloImage * restrict txtr, const EdgePos * restrict left, const EdgePos * restrict right, int miny, int maxy)
{
	if(txtr)
	{
		MaggieFormat *colPtr = &pixels[miny * MAGGIE_XRES];
		for(int i = miny; i < maxy; ++i)
		{
			int x0 = left[i].xPos;
			int x1 = right[i].xPos;

			MaggieFormat *dstColPtr = colPtr + x0;
			colPtr += MAGGIE_XRES;

			if(x0 >= x1)
				continue;

			int len = x1 - x0;
			float oolen = 1.0f / len;
			float xFrac0 = left[i].xPos - x0;
			float xFrac1 = right[i].xPos - x1;
			float preStep0 = 1.0f - xFrac0;
			float preStep1 = 1.0f - xFrac1;

			float xLen = right[i].xPos - left[i].xPos;
			// float zLen = right[i].zow - left[i].zow;
			float wLen = right[i].oow - left[i].oow;
			float uLen = right[i].uow - left[i].uow;
			float vLen = right[i].vow - left[i].vow;
			float iLen = right[i].iow - left[i].iow;

			float preRatio0 = preStep0 / xLen;
			float preRatio1 = preStep1 / xLen;
			float preRatioDiff = preRatio0 - preRatio1;

			// float zPreStep = zLen * preRatioDiff;
			float wPreStep = wLen * preRatioDiff;
			float uPreStep = uLen * preRatioDiff;
			float vPreStep = vLen * preRatioDiff;
			float iPreStep = iLen * preRatioDiff;

			// uint16_t *dstZPtr =  (uint16_t *)&zbuffer[i * MAGGIE_XRES + x0];

			// float zDDA = (zLen - zPreStep) * oolen;
			float wDDA = (wLen - wPreStep) * oolen;
			float uDDA = (uLen - uPreStep) * oolen;
			float vDDA = (vLen - vPreStep) * oolen;
			float iDDA = (iLen - iPreStep) * oolen;

			// float zPos = left[i].zow + preStep0 * zDDA;
			float wPos = left[i].oow + preStep0 * wDDA;
			float uPos = left[i].uow + preStep0 * uDDA;
			float vPos = left[i].vow + preStep0 * vDDA;
			float iPos = left[i].iow + preStep0 * iDDA;

			// zDDA *= PIXEL_RUN;
			wDDA *= PIXEL_RUN;
			uDDA *= PIXEL_RUN;
			vDDA *= PIXEL_RUN;
			iDDA *= PIXEL_RUN;

			float w = 1.0 / wPos;
			int32_t uStart = uPos * w;
			int32_t vStart = vPos * w;
			// float zStart = zPos;
			int iStart = iPos;
			// int nRuns = len / PIXEL_RUN;
			float ooLen = 1.0f / PIXEL_RUN;

			int runLength = PIXEL_RUN;
			if(runLength > len)
				runLength = len;
			while(len > 0)
			{
				wPos += wDDA;
				uPos += uDDA;
				vPos += vDDA;
				// zPos += zDDA;
				iPos += iDDA;

				w = 1.0 / wPos;

				// float zEnd = zPos;
				float iEnd = iPos;

				int32_t uEnd = uPos * w;
				int32_t vEnd = vPos * w;

				int dUUuu = (int)((uEnd - uStart) * ooLen);
				int dVVvv = (int)((vEnd - vStart) * ooLen);
				// int dZz = (int)((zEnd - zStart) * ooLen);
				int dIi = (int)((iEnd - iStart) * ooLen);

				if(len <= (PIXEL_RUN * 3 / 2))
				{
					runLength = len;
				}

//				DrawHardwareSpanZBuffered(dstColPtr, dstZPtr, runLength, (uint32_t)zStart, (int)(uStart), (int)(vStart), (int)iStart, dZz, dUUuu, dVVvv, dIi);
				DrawHardwareSpan(dstColPtr, runLength, (int)(uStart), (int)(vStart), (int)iStart, dUUuu, dVVvv, dIi);

				dstColPtr += runLength;
				// dstZPtr += runLength;
				uStart = uEnd;
				vStart = vEnd;
				// zStart = zEnd;
				iStart = iEnd;
				len -= runLength;
			}
		}
	}
	else
	{
		for(int i = miny; i < maxy; ++i)
		{
			int x0 = left[i].xPos;
			int x1 = right[i].xPos;
			int len = x1 - x0;
			float ooLen = 1.0f / len;

			if(x0 == x1)
				continue;

			if(x0 < 0)
				x0 = 0;
			if(x1 >= MAGGIE_XRES)
				x1 = MAGGIE_XRES - 1;

			MaggieFormat *dstColPtr = &pixels[(i * MAGGIE_XRES + x0)];
//			uint16_t *dstZPtr =  (uint16_t *)&zbuffer[i * MAGGIE_XRES + x0];

//			uint32_t zPos = left[i].zow;
			float iPos = left[i].iow;
//			int32_t zDDA = (right[i].zow - left[i].zow) * ooLen;
			float iDDA = (right[i].iow - left[i].iow) * ooLen;

//			DrawSpanZBuffer(dstColPtr, dstZPtr, len, zPos, (int)iPos, zDDA, (int)iDDA);
			DrawSoftSpan(dstColPtr, len, (int)iPos, (int)iDDA);
		}
	}
}

/*****************************************************************************/

static void DrawLine(EdgePos * restrict edge, const tr_finalVertex * restrict v0, const tr_finalVertex * restrict v1)
{
	int y0 = (int)v0->pos.y;
	int y1 = (int)v1->pos.y;

	int lineLen = (int)(y1 - y0);
	if(lineLen <= 0)
		return;
	float ooLineLen = 1.0f / lineLen;

	float yFrac0 = v0->pos.y - y0;
	float yFrac1 = v1->pos.y - y1;
	float preStep0 = 1.0f - yFrac0;
	float preStep1 = 1.0f - yFrac1;

	float xLen = v1->pos.x - v0->pos.x;
	float yLen = v1->pos.y - v0->pos.y;
//	float zLen = v1->pos.z - v0->pos.z;
	float wLen = v1->pos.w - v0->pos.w;
	float uLen = v1->u - v0->u;
	float vLen = v1->v - v0->v;
	float iLen = v1->intensity - v0->intensity;

	float preRatio0 = preStep0 / yLen;
	float preRatio1 = preStep1 / yLen;

	float xPreStep = xLen * (preRatio0 - preRatio1);
//	float zPreStep = zLen * (preRatio0 - preRatio1);
	float wPreStep = wLen * (preRatio0 - preRatio1);
	float uPreStep = uLen * (preRatio0 - preRatio1);
	float vPreStep = vLen * (preRatio0 - preRatio1);
	float iPreStep = iLen * (preRatio0 - preRatio1);
 
	float xDDA = (xLen - xPreStep) * ooLineLen;
//	float zDDA = (zLen - zPreStep) * ooLineLen;
	float wDDA = (wLen - wPreStep) * ooLineLen;
	float uDDA = (uLen - uPreStep) * ooLineLen;
	float vDDA = (vLen - vPreStep) * ooLineLen;
	float iDDA = (iLen - iPreStep) * ooLineLen;

	float xVal = v0->pos.x + preStep0 * xDDA;
	float oow = v0->pos.w + preStep0 * wDDA;
//	float zow = v0->pos.z + preStep0 * zDDA;
	float uow = v0->u + preStep0 * uDDA;
	float vow = v0->v + preStep0 * vDDA;
	float iow = v0->intensity + preStep0 * iDDA;

	for(int i = 0; i < lineLen; ++i)
	{
		edge[i + y0].xPos = xVal;
		edge[i + y0].oow = oow;
		edge[i + y0].uow = uow;
		edge[i + y0].vow = vow;
//		edge[i + y0].zow = zow;
		edge[i + y0].iow = iow;
		xVal += xDDA;
		oow += wDDA;
//		zow += zDDA;
		uow += uDDA;
		vow += vDDA;
		iow += iDDA;
	}
}

/*****************************************************************************/

void DrawPolygon(MaggieFormat *pixels, apolloImage *txtr, tr_finalVertex *verts, int nVerts)
{
	float x0 = verts[1].pos.x - verts[0].pos.x;
	float y0 = verts[1].pos.y - verts[0].pos.y;
	float x1 = verts[2].pos.x - verts[0].pos.x;
	float y1 = verts[2].pos.y - verts[0].pos.y;

	if((x0 * y1 - x1 * y0) < 0.0f)
		return;

	int miny = verts[0].pos.y;
	int maxy = verts[0].pos.y;
	for(int i = 1; i < nVerts; ++i)
	{
		if(miny > verts[i].pos.y)
			miny = verts[i].pos.y;
		if(maxy < verts[i].pos.y)
			maxy = verts[i].pos.y;
	}

	if(miny < 0) return;
	if(maxy > MAGGIE_YRES) return;

	for(int i = 0; i < nVerts; ++i)
	{
		tr_finalVertex *v0 = &verts[i];
		tr_finalVertex *v1 = &verts[(i + 1) % nVerts];
		if(v0->pos.y < v1->pos.y)
		{
			DrawLine(rightEdge, v0, v1);
		}
		else
		{
			DrawLine(leftEdge, v1, v0);
		}
	}
	DrawSpans(pixels, txtr, leftEdge, rightEdge, miny, maxy);
}

/*****************************************************************************/

static void DrawTriangle(MaggieFormat *pixels, apolloImage *txtr, const tr_finalVertex * restrict v0, const tr_finalVertex * restrict v1, const tr_finalVertex * restrict v2)
{
	float x0 = v1->pos.x - v0->pos.x;
	float y0 = v1->pos.y - v0->pos.y;
	float x1 = v2->pos.x - v0->pos.x;
	float y1 = v2->pos.y - v0->pos.y;

	if((x0 * y1 - x1 * y0) < 0.0f)
		return;

	int miny = v0->pos.y;
	if(miny > v1->pos.y)
		miny = v1->pos.y;
	if(miny > v2->pos.y)
		miny = v2->pos.y;

	int maxy = v0->pos.y;
	if(maxy < v1->pos.y)
		maxy = v1->pos.y;
	if(maxy < v2->pos.y)
		maxy = v2->pos.y;

	if(v0->pos.y < v1->pos.y)
	{
		DrawLine(rightEdge, v0, v1);
	}
	else
	{
		DrawLine(leftEdge, v1, v0);
	}
	if(v1->pos.y < v2->pos.y)
	{
		DrawLine(rightEdge, v1, v2);
	}
	else
	{
		DrawLine(leftEdge, v2, v1);
	}
	if(v2->pos.y < v0->pos.y)
	{
		DrawLine(rightEdge, v2, v0);
	}
	else
	{
		DrawLine(leftEdge, v0, v2);
	}
	DrawSpans(pixels, txtr, leftEdge, rightEdge, miny, maxy);
}

/*****************************************************************************/

static uint8_t ClipCode(const vec4 *v)
{
	uint8_t code = 0;

	if(-v->w >= v->x) code |= 0x01;
	if( v->w <= v->x) code |= 0x02;
	if(-v->w >= v->y) code |= 0x04;
	if( v->w <= v->y) code |= 0x08;
	if( 0.0f >= v->z) code |= 0x10;
	if( v->w <= v->z) code |= 0x20;

	return code;
}

/*****************************************************************************/

clipResult GetBBClipCodes(vec3 *bb0, vec3 *bb1, mat4 *projection)
{
	vec3 bbVerts[8] =
	{
		{ bb0->x, bb0->y, bb0->z },
		{ bb1->x, bb0->y, bb0->z },
		{ bb0->x, bb1->y, bb0->z },
		{ bb1->x, bb1->y, bb0->z },
		{ bb0->x, bb0->y, bb1->z },
		{ bb1->x, bb0->y, bb1->z },
		{ bb0->x, bb1->y, bb1->z },
		{ bb1->x, bb1->y, bb1->z }
	};

	uint32_t clipCodes = 0;

	for(int i = 0; i < 8; ++i)
	{
		vec4 res;
		vec3_tformh(&res, projection, &bbVerts[i], 1.0f);
		if(-res.w >= res.x) clipCodes += 0x00000001;
		if( res.w <= res.x) clipCodes += 0x00000010;
		if(-res.w >= res.y) clipCodes += 0x00000100;
		if( res.w <= res.y) clipCodes += 0x00001000;
		if(  0.0f >= res.z) clipCodes += 0x00010000;
		if( res.w <= res.z) clipCodes += 0x00100000;
	}
	if(clipCodes & 0x00888888)
		return CLIP_OUTSIDE;
	if(!clipCodes)
		return CLIP_INSIDE;
	return CLIP_PARTIAL;
}

/*****************************************************************************/

typedef struct
{
	float key;
	int subMesh;
	clipResult clipCode;
} subSortEntry;

/*****************************************************************************/

int compareSubMeshes(const void *a, const void *b)
{
	const subSortEntry *sa = (const subSortEntry *)a;
	const subSortEntry *sb = (const subSortEntry *)b;
	if(sa->key > sb->key)
		return -1;
	if(sa->key < sb->key)
		return 1;
	return 0;
}

/*****************************************************************************/
/*****************************************************************************/

static void ClipFinalVertex(tr_finalVertex *res, const tr_finalVertex *v0, const tr_finalVertex *v1, float t)
{
	res->pos.x = (v1->pos.x - v0->pos.x) * t + v0->pos.x;
	res->pos.y = (v1->pos.y - v0->pos.y) * t + v0->pos.y;
	res->pos.z = (v1->pos.z - v0->pos.z) * t + v0->pos.z;
	res->pos.w = (v1->pos.w - v0->pos.w) * t + v0->pos.w;
	res->u = (v1->u - v0->u) * t + v0->u;
	res->v = (v1->v - v0->v) * t + v0->v;
	res->intensity = (v1->intensity - v0->intensity) * t + v0->intensity;
}

/*****************************************************************************/

static int ClipTriangleToPoly(tr_finalVertex *dst, const tr_finalVertex *src, uint16_t indx0, uint16_t indx1, uint16_t indx2)
{
	tr_finalVertex tmpPoly0[16];
	tr_finalVertex tmpPoly1[16];

	tmpPoly0[0] = src[indx0];
	tmpPoly0[1] = src[indx1];
	tmpPoly0[2] = src[indx2];

	const tr_finalVertex *inBuffer = tmpPoly0;
	tr_finalVertex *outBuffer = tmpPoly1;
	int nVerts = 3;
	int prevVertex = nVerts - 1;
	const tr_finalVertex *vLast;
	vLast = &inBuffer[prevVertex];
	int lastOut = -vLast->pos.w > vLast->pos.x;
	int nOutput = 0;

	for(int i = 0; i < nVerts; ++i)
	{
		const tr_finalVertex *v0 = &inBuffer[prevVertex];
		const tr_finalVertex *v1 = &inBuffer[i];
		int out = -v1->pos.w > v1->pos.x;
		if(out != lastOut)
		{
			// Intersect
			float t = (v0->pos.w + v0->pos.x) / (v0->pos.x - v1->pos.x + v0->pos.w - v1->pos.w);
			if(t < 0.0f) t = 0.0f;
			if(t > 1.0f) t = 1.0f;
			ClipFinalVertex(&outBuffer[nOutput++], v0, v1, t);
		}
		if(!out)
		{
			outBuffer[nOutput++] = inBuffer[i];
		}
		prevVertex = i;
		lastOut = out;
	}
	if(nOutput < 3)
		return 0;

	inBuffer = tmpPoly1;
	outBuffer = tmpPoly0;
	nVerts = nOutput;
	prevVertex = nVerts - 1;
	vLast = &inBuffer[prevVertex];
	lastOut = vLast->pos.w < vLast->pos.x;
	nOutput = 0;

	for(int i = 0; i < nVerts; ++i)
	{
		const tr_finalVertex *v0 = &inBuffer[prevVertex];
		const tr_finalVertex *v1 = &inBuffer[i];
		int out = v1->pos.w < v1->pos.x;
		if(out != lastOut)
		{
			// Intersect
			float t = (v0->pos.w - v0->pos.x) / (v1->pos.x - v0->pos.x - v1->pos.w + v0->pos.w);
			if(t < 0.0f) t = 0.0f;
			if(t > 1.0f) t = 1.0f;
			ClipFinalVertex(&outBuffer[nOutput++], v0, v1, t);
		}
		if(!out)
		{
			outBuffer[nOutput++] = inBuffer[i];
		}
		prevVertex = i;
		lastOut = out;
	}
	if(nOutput < 3)
		return 0;

	inBuffer = tmpPoly0;
	outBuffer = tmpPoly1;
	nVerts = nOutput;
	prevVertex = nVerts - 1;
	vLast = &inBuffer[prevVertex];
	lastOut = -vLast->pos.w > vLast->pos.y;
	nOutput = 0;

	for(int i = 0; i < nVerts; ++i)
	{
		const tr_finalVertex *v0 = &inBuffer[prevVertex];
		const tr_finalVertex *v1 = &inBuffer[i];
		int out = -v1->pos.w > v1->pos.y;
		if(out != lastOut)
		{
			// Intersect
			float t = (v0->pos.w + v0->pos.y) / (v0->pos.y - v1->pos.y + v0->pos.w - v1->pos.w);
			if(t < 0.0f) t = 0.0f;
			if(t > 1.0f) t = 1.0f;
			ClipFinalVertex(&outBuffer[nOutput++], v0, v1, t);
		}
		if(!out)
		{
			outBuffer[nOutput++] = inBuffer[i];
		}
		prevVertex = i;
		lastOut = out;
	}
	if(nOutput < 3)
		return 0;

	inBuffer = tmpPoly1;
	outBuffer = tmpPoly0;
	nVerts = nOutput;
	prevVertex = nVerts - 1;
	vLast = &inBuffer[prevVertex];
	lastOut = vLast->pos.w < vLast->pos.y;
	nOutput = 0;

	for(int i = 0; i < nVerts; ++i)
	{
		const tr_finalVertex *v0 = &inBuffer[prevVertex];
		const tr_finalVertex *v1 = &inBuffer[i];
		int out = v1->pos.w < v1->pos.y;
		if(out != lastOut)
		{
			// Intersect
			float t = (v0->pos.w - v0->pos.y) / (v1->pos.y - v0->pos.y - v1->pos.w + v0->pos.w);
			if(t < 0.0f) t = 0.0f;
			if(t > 1.0f) t = 1.0f;
			ClipFinalVertex(&outBuffer[nOutput++], v0, v1, t);
		}
		if(!out)
		{
			outBuffer[nOutput++] = inBuffer[i];
		}
		prevVertex = i;
		lastOut = out;
	}
	if(nOutput < 3)
		return 0;

	inBuffer = tmpPoly0;
	outBuffer = tmpPoly1;
	nVerts = nOutput;
	prevVertex = nVerts - 1;
	vLast = &inBuffer[prevVertex];
	lastOut = vLast->pos.z < 0.0f;
	nOutput = 0;

	for(int i = 0; i < nVerts; ++i)
	{
		const tr_finalVertex *v0 = &inBuffer[prevVertex];
		const tr_finalVertex *v1 = &inBuffer[i];
		int out = v1->pos.z < 0.0f;
		if(out != lastOut)
		{
			// Intersect
			float t = v0->pos.z / (v0->pos.z - v1->pos.z);
			if(t < 0.0f) t = 0.0f;
			if(t > 1.0f) t = 1.0f;
			ClipFinalVertex(&outBuffer[nOutput++], v0, v1, t);
		}
		if(!out)
		{
			outBuffer[nOutput++] = inBuffer[i];
		}
		prevVertex = i;
		lastOut = out;
	}
	if(nOutput < 3)
		return 0;

	inBuffer = tmpPoly1;
	outBuffer = dst;
	nVerts = nOutput;
	prevVertex = nVerts - 1;
	vLast = &inBuffer[prevVertex];
	lastOut = vLast->pos.w < vLast->pos.z;
	nOutput = 0;

	for(int i = 0; i < nVerts; ++i)
	{
		const tr_finalVertex *v0 = &inBuffer[prevVertex];
		const tr_finalVertex *v1 = &inBuffer[i];
		int out = v1->pos.w < v1->pos.z;
		if(out != lastOut)
		{
			// Intersect
			float t = (v0->pos.w - v0->pos.z) / (v1->pos.z - v0->pos.z - v1->pos.w + v0->pos.w);
			if(t < 0.0f) t = 0.0f;
			if(t > 1.0f) t = 1.0f;
			ClipFinalVertex(&outBuffer[nOutput++], v0, v1, t);
		}
		if(!out)
		{
			outBuffer[nOutput++] = inBuffer[i];
		}
		prevVertex = i;
		lastOut = out;
	}
	if(nOutput < 3)
		return 0;

	return nOutput;
}

/*****************************************************************************/

static const float offsetScaleX = (MAGGIE_XRES + 0.5f) / 2.0f;
static const float offsetScaleY = (MAGGIE_YRES + 0.5f) / 2.0f;

/*****************************************************************************/

static void NormaliseBuffer(tr_finalVertex *dst, tr_finalVertex *src, int nVerts, const uint8_t *clipCodes)
{
	for(int i = 0; i < nVerts; ++i)
	{
		if(clipCodes[i])
			continue;
		float oow = 1.0f / src[i].pos.w;

		dst[i].pos.x = offsetScaleX * (src[i].pos.x * oow + 1.0f);
		dst[i].pos.y = offsetScaleY * (src[i].pos.y * oow + 1.0f);
		dst[i].pos.z = src[i].pos.z * oow * 65536.0f * 65535.0f;
		dst[i].pos.w = oow;
		dst[i].u = src[i].u * 256.0f * 65536.0f * oow;
		dst[i].v = src[i].v * 256.0f * 65536.0f * oow;
		dst[i].intensity = src[i].intensity * 65535.0f;
	}
}

/*****************************************************************************/

static void NormaliseClippedBuffer(tr_finalVertex *dst, int nVerts)
{
	for(int i = 0; i < nVerts; ++i)
	{
		float oow = 1.0f / dst[i].pos.w;

		dst[i].pos.x = offsetScaleX * (dst[i].pos.x * oow + 1.0f);
		dst[i].pos.y = offsetScaleY * (dst[i].pos.y * oow + 1.0f);
		dst[i].pos.z = dst[i].pos.z * oow * 65536.0f * 65535.0f;
		dst[i].pos.w = oow;
		dst[i].u = dst[i].u * 256.0f * 65536.0f * oow;
		dst[i].v = dst[i].v * 256.0f * 65536.0f * oow;
		dst[i].intensity = dst[i].intensity * 65535.0f;
	}
}

/*****************************************************************************/

void DrawClippedTriangle(MaggieFormat *pixels, apolloImage *txtr, const tr_finalVertex * restrict srcVtx, int i0, int i1, int i2)
{
	static tr_finalVertex clippedVtx[16];

	int clipVerts = ClipTriangleToPoly(clippedVtx, srcVtx, i0, i1, i2);
	if(clipVerts < 3)
		return;
	NormaliseClippedBuffer(clippedVtx, clipVerts);
	DrawPolygon(pixels, txtr, clippedVtx, clipVerts);
}

/*****************************************************************************/

void TR_DrawTrack(MaggieFormat *pixels, tr_track *track, mat4 *modelView, mat4 *projection)
{
	mat4 viewProj;

	mat4_mul(&viewProj, projection, modelView);

	for(int i = 0; i < track->nVerts; ++i)
	{
		vec3_tformh(&transformBuffer[i].pos, &viewProj, &track->vtx[i].pos, 1.0f);
		transformBuffer[i].u = track->vtx[i].u;
		transformBuffer[i].v = track->vtx[i].v;
		transformBuffer[i].intensity = track->vtx[i].normal.y;

		clipCodeBuffer[i] = ClipCode(&transformBuffer[i].pos);
	}

	NormaliseBuffer(drawBuffer, transformBuffer, track->nVerts, clipCodeBuffer);

	SetupHWSpans(track->txtr);

	subSortEntry sortSubTable[track->nSubMeshes];
	int nVisibleSubMeshes = 0;

	for(int i = 0; i < track->nSubMeshes; ++i)
	{
		tr_submesh *subMesh = &track->subMeshes[i];
		vec3 bb0, bb1;
		vec3_tform(&bb0, modelView, &subMesh->bbmin, 1.0f);
		vec3_tform(&bb1, modelView, &subMesh->bbmax, 1.0f);

		clipResult clipCode = GetBBClipCodes(&subMesh->bbmin, &subMesh->bbmax, &viewProj);

		if(clipCode != CLIP_OUTSIDE)
		{
			sortSubTable[nVisibleSubMeshes].key = bb0.z + bb1.z;
			sortSubTable[nVisibleSubMeshes].clipCode = clipCode;
			sortSubTable[nVisibleSubMeshes].subMesh = i;
			nVisibleSubMeshes++;
		}
	}

	qsort(sortSubTable, nVisibleSubMeshes, sizeof(subSortEntry), compareSubMeshes);

	for(int i = 0; i < nVisibleSubMeshes; ++i)
	{
		tr_submesh *subMesh = &track->subMeshes[sortSubTable[i].subMesh];
		if(sortSubTable[i].clipCode == CLIP_PARTIAL)
		{
			int startIndx = subMesh->startIndx;
			for(int j = 0; j < subMesh->nIndx; j += 3)
			{
				int i0 = track->indx[startIndx++];
				int i1 = track->indx[startIndx++];
				int i2 = track->indx[startIndx++];
				if(clipCodeBuffer[i0] & clipCodeBuffer[i1] & clipCodeBuffer[i2])
					continue;

				if(clipCodeBuffer[i0] | clipCodeBuffer[i1] | clipCodeBuffer[i2])
				{
					DrawClippedTriangle(pixels, track->txtr, transformBuffer, i0, i1, i2);
//					DrawClippedTriangle(pixels, NULL, transformBuffer, i0, i1, i2);
				}
				else
				{
					DrawTriangle(pixels, track->txtr, &drawBuffer[i0], &drawBuffer[i1], &drawBuffer[i2]);
				}
			}
		}
		else
		{
			int startIndx = subMesh->startIndx;
			int nTriangles = subMesh->nIndx / 3;
			for(int j = 0; j < nTriangles; ++j)
			{
				int i0 = track->indx[startIndx++];
				int i1 = track->indx[startIndx++];
				int i2 = track->indx[startIndx++];
				DrawTriangle(pixels, track->txtr, &drawBuffer[i0], &drawBuffer[i1], &drawBuffer[i2]);
			}
		}
	}
}

/*****************************************************************************/

void ExpandLine1632(uint32_t *dest __asm("a0"), uint16_t *src __asm("a1"), int len  __asm("d0"));
void Copy16(uint16_t *dest __asm("a0"), uint16_t *src __asm("a1"), int len  __asm("d0"));

void TR_DrawBackgroud(MaggieFormat *pixels, apolloImage *image, mat4 *iCam)
{
	const float pi = 3.1415927;
	vec3 zVec = {0.0f, 0.0f, 1.0f};
	vec3 yVec = {0.0f, 1.0f, 0.0f};

	vec3_tform(&zVec, iCam, &zVec, 0.0f);

	float centerY = -vec3_dot(&zVec, &yVec) * 0.5f;
	zVec.y = 0.0f;
	vec3_normalise(&zVec, &zVec);

	float centerX = core_atan2f(zVec.x, zVec.z) / (2.0f * pi);

	int xpos = (centerX) * image->width + MAGGIE_XRES / 2.0f;
	int ypos = (centerY) * image->height + image->height / 3.0f;
	if(ypos < 0)
		ypos = 0;
	if(ypos >= (image->height - MAGGIE_YRES))
		ypos = image->height - MAGGIE_YRES - 1;

	MaggieFormat *imageData = APOLLO_GetImageData(image);

#if MAGGIE_PIXSIZE == 4
	for(int i = 0; i < MAGGIE_YRES; ++i)
	{
		ExpandLine1632(&pixels[i * MAGGIE_XRES], &imageData[i * image->width], MAGGIE_XRES);
	}
#else
	if(xpos < 0)
	{
		xpos += image->width;
	}
	if(xpos + MAGGIE_XRES <= image->width)
	{
		for(int i = 0; i < MAGGIE_YRES-1; ++i)
		{
			Copy16(&pixels[i * MAGGIE_XRES], &imageData[(i + ypos) * image->width + xpos], MAGGIE_XRES);
		}
	}
	else
	{
		int width0 = image->width - xpos;
		int width1 = MAGGIE_XRES + xpos - image->width;

		for(int i = 0; i < MAGGIE_YRES-1; ++i)
		{
			memcpy(&pixels[i * MAGGIE_XRES], &imageData[(i + ypos) * image->width + xpos], width0 * MAGGIE_PIXSIZE);
			memcpy(&pixels[i * MAGGIE_XRES + width0], &imageData[(i + ypos) * image->width], width1 * MAGGIE_PIXSIZE);
		}
		// TextOut(pixels, 0, 48, "%d %d", width0 + width1);
	}
#endif
	// TextOut(pixels, 0, 32, "%f %f", centerX, centerY);
	// TextOut(pixels, 0, 40, "%d %d", xpos, ypos);
}

/*****************************************************************************/
