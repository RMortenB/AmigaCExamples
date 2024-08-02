#include <stdio.h>
#include <math.h>

#include <proto/exec.h>
#include <proto/lowlevel.h>
#include <proto/graphics.h>

#include "Maggie.h"
#include "owl.h"

/*****************************************************************************/
/********************** Vertical blank interrupt stuff ***********************/
/*****************************************************************************/

static volatile LONG frameSig = 0; // This is set to 1 by the VBL handler.

LONG VBLHandler(void *data __asm("a1"))
{
	LONG *signal = (LONG *)data;
	*signal = 1;
    return 0;
}

/*****************************************************************************/

void WaitVBLPassed()
{
	while(!frameSig)
		;
	frameSig = 0;
}

/*****************************************************************************/
/****************************** Maggie control *******************************/
/*****************************************************************************/

static void SetupMaggieSpans(void *textureBase)
{
	maggieRegs->modulo = MAGGIE_PIXSIZE;
	maggieRegs->texture = textureBase;
	maggieRegs->lightRGBA = ~0;
	maggieRegs->texSize = 7;
	maggieRegs->light = ~0;
	maggieRegs->lightDelta = 0;

	if(MAGGIE_PIXSIZE==2)
		maggieRegs->mode = 5;
	else
		maggieRegs->mode = 1;
}

/*****************************************************************************/

static void DrawMaggieSpan(	MaggieFormat *destAddr,
								ULONG U, ULONG V,
								ULONG dU, ULONG dV,
								LONG length)
{
	maggieRegs->pixDest = destAddr;
	maggieRegs->uCoord = U;
	maggieRegs->vCoord = V;
	maggieRegs->uDelta = dU;
	maggieRegs->vDelta = dV;
	maggieRegs->startLength = length;
}

/*****************************************************************************/
/*****************************************************************************/

static void DrawZoom(MaggieFormat *screen, float t)
{
	float zoom = (sinf(t) * 0.30f + 0.301f) * 65536.0f;
	float xx = cosf(t) * zoom;
	float xy = sinf(t) * zoom;

	for(LONG i = 0; i < MAGGIE_YRES; ++i)
	{
		DrawMaggieSpan(&screen[i * MAGGIE_XRES],
							i * xx, i * xy,
							-xy, xx,
							MAGGIE_XRES);
	}
}

/*****************************************************************************/
/*****************************************************************************/

int main(int argc, char *argv[])
{
	if(!HasMaggie())
	{
		printf("MaggieZoom requires a Maggie chipset.\n");
		return 0;
	}

	UBYTE *screenMem = AllocMem(MAGGIE_SCREENSIZE * MAGGIE_NFRAMES, MEMF_ANY | MEMF_CLEAR);

	UBYTE *screenPixels[MAGGIE_NFRAMES];

	screenPixels[0] = screenMem;
	for(LONG i = 1; i < MAGGIE_NFRAMES; ++i)
	{
		screenPixels[i] = screenPixels[i - 1] + MAGGIE_SCREENSIZE;
	}

	UBYTE *texture = AllocMem(128 * 128 / 2, MEMF_ANY); // Make sure texture is 8 byte aligned
	CopyMemQuick(owl_dds + 128, texture, 128 * 128 / 2);

	SystemControl(SCON_TakeOverSys, TRUE, TAG_DONE);

	volatile UWORD *SAGA_ScreenModeRead = (UWORD *)0xdfe1f4;
	volatile ULONG *SAGA_ChunkyDataRead = (ULONG *)0xdfe1ec;
	volatile UWORD *SAGA_ScreenMode = (UWORD *)0xdff1f4;
	volatile ULONG *SAGA_ChunkyData = (ULONG *)0xdff1ec;

	UWORD oldMode = *SAGA_ScreenModeRead;
	ULONG oldScreen = *SAGA_ChunkyDataRead;

	*SAGA_ScreenMode = MAGGIE_MODE;

	void *vblHandle = AddVBlankInt(VBLHandler, (APTR)&frameSig);

	LONG currentBufferNumber = 0;

	WaitBlit();

	SetupMaggieSpans(texture);

	float phase = 0.0f;
	while(!(ReadJoyPort(0) & JPF_BUTTON_RED)) // While left mouse button not pressed
	{
		*SAGA_ChunkyData = (ULONG)screenPixels[currentBufferNumber]; // Set current display frame.

		currentBufferNumber = (currentBufferNumber + 1) % MAGGIE_NFRAMES; // Cycle through the frame buffers..
		MaggieFormat *pixels = (MaggieFormat *)screenPixels[currentBufferNumber]; // Current screen buffer

		WaitVBLPassed();

		DrawZoom(pixels, phase);
		phase += 0.01f;
	}

	*SAGA_ScreenMode = oldMode;
	*SAGA_ChunkyData = oldScreen;

	RemVBlankInt(vblHandle);

	SystemControl(SCON_TakeOverSys, FALSE, TAG_DONE); // Restore system

	FreeMem(texture, 128 * 128 / 2);
	FreeMem(screenMem, MAGGIE_SCREENSIZE * MAGGIE_NFRAMES);

	return 0;
}
