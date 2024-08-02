#ifndef MAGGIE_H_INCLUDED
#define MAGGIE_H_INCLUDED

/*****************************************************************************/

#include <exec/types.h>

/*****************************************************************************/

#define MAGGIE_XRES 640
#define MAGGIE_YRES 360
#define MAGGIE_PIXSIZE 2
#define MAGGIE_NFRAMES 3

#define MAGGIE_SCREENSIZE (MAGGIE_XRES * MAGGIE_YRES * MAGGIE_PIXSIZE)

/*****************************************************************************/

typedef struct
{
	APTR	texture;			/* 32bit texture source */
	APTR	pixDest;			/* 32bit Destination Screen Addr */
	APTR 	depthDest;			/* 32bit ZBuffer Addr */
	UWORD	unused0;
	UWORD	startLength;		/* 16bit LEN and START */
	UWORD	texSize;			/* 16bit MIP texture size (9=512/8=256/7=128/6=64) */
	UWORD	mode;				/* 16bit MODE (Bit0=Bilienar) (Bit1=Zbuffer) (Bit2=16bit output) */
	UWORD	unused1;
	UWORD	modulo;				/* 16bit Destination Step */
	ULONG	unused2;
	ULONG	unused3;
	ULONG	uCoord;				/* 32bit U (8:24 normalised) */
	ULONG	vCoord;				/* 32bit V (8:24 normalised) */
	ULONG	uDelta;				/* 32bit dU (8:24 normalised) */
	ULONG	vDelta;				/* 32bit dV (8:24 normalised) */
	UWORD	light;				/* 16bit Light Ll (8:8) */
	UWORD	lightDelta;			/* 16bit Light dLl (8:8) */
	ULONG	lightRGBA;			/* 32bit Light color (ARGB) */
	ULONG	depthStart;			/* 32bit Z (16:16) */
	ULONG	depthDelta;			/* 32bit Delta (16:16) */
} __attribute__((packed)) MaggieRegs;

static int HasMaggie()
{
	UWORD *vampVersion = (UWORD *)0xdff3fc;
	UWORD version = (*vampVersion);
	switch(version >> 8)
	{
		case 0:
		case 1:
		case 2:
		case 6:
		{
			return 0;
		}
	}
	return 1;
}

#if (MAGGIE_PIXSIZE==2)

# define MAGGIE_MODE 0x0b02		// 640x360x16bpp
	typedef UWORD MaggieFormat;

#elif (MAGGIE_PIXSIZE==4)

# define MAGGIE_MODE 0x0b05		// 640x360x32bpp
	typedef ULONG MaggieFormat;

#else
# error "Only 16 or 32 bpp output for Maggie!"
#endif

static volatile MaggieRegs * const maggieRegs = (MaggieRegs *)0xdff250;

#endif // MAGGIE_H_INCLUDED
