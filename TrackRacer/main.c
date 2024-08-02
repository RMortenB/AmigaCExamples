#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#include <proto/timer.h>
#include <proto/exec.h>
#include <proto/lowlevel.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>

#include <devices/timer.h>

#include "Maggie.h"
#include "trackracer.h"
#include "vec3.h"

#include "font8x8.h"

/*****************************************************************************/

static struct Screen *graphicsScreen = NULL;
static struct Window *graphicsWindow = NULL;
static APTR graphicsKeyboardIrq = NULL;

/*****************************************************************************/

int KeyboardIrq()
{
	return 0;
}

/*****************************************************************************/

void EnterGraphicsMode()
{
	ULONG mode = BestModeID(BIDTAG_DesiredWidth, MAGGIE_XRES,
							BIDTAG_NominalWidth, MAGGIE_XRES,
							BIDTAG_DesiredHeight, MAGGIE_YRES,
							BIDTAG_NominalHeight, MAGGIE_YRES,
							BIDTAG_Depth, 16,
							TAG_END);

	graphicsScreen = OpenScreenTags(NULL,
									SA_Width, MAGGIE_XRES,
									SA_Height, MAGGIE_YRES,
									SA_Depth, 16,
									SA_ShowTitle, FALSE,
									SA_Quiet, TRUE,
									SA_Type, CUSTOMSCREEN,
									SA_DisplayID, mode,
									TAG_END);
	if(!graphicsScreen)
		printf("No screen!\n");
	struct NewWindow newWindow;
	memset(&newWindow, 0, sizeof(newWindow));
	newWindow.Width = MAGGIE_XRES;
	newWindow.Height = MAGGIE_YRES;
	newWindow.Flags = WFLG_SIMPLE_REFRESH | WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_ACTIVATE;
	newWindow.Screen = graphicsScreen;
	newWindow.Type = CUSTOMSCREEN;
	newWindow.IDCMPFlags = IDCMP_RAWKEY | IDCMP_ACTIVEWINDOW;

	graphicsWindow = OpenWindowTags(&newWindow, TAG_END);

	if(!graphicsWindow)
		printf("No Window!\n");

	graphicsKeyboardIrq = AddKBInt(KeyboardIrq, NULL);

	if(!graphicsKeyboardIrq)
		printf("No irq!\n");
}

/*****************************************************************************/

void HandleWindowMessages()
{
	struct IntuiMessage *msg;
	while(msg = (struct IntuiMessage *)GetMsg (graphicsWindow->UserPort))
	{
		switch (msg->Class)
		{
			case IDCMP_ACTIVEWINDOW :printf("IDCMP_ACTIVEWINDOW\n"); break;
		}
		ReplyMsg ((struct Message*)msg);
	}
}

/*****************************************************************************/

void ExitGraphicsMode()
{
	RemKBInt(graphicsKeyboardIrq);

	if(graphicsWindow)
		CloseWindow(graphicsWindow);
	if(graphicsScreen)
		CloseScreen(graphicsScreen);
}

/*****************************************************************************/

MaggieFormat textColour = ~0;

/*****************************************************************************/
#define TEXT_XRES MAGGIE_XRES
#define TEXT_YRES MAGGIE_YRES

void TextOut(MaggieFormat *screen, int xpos, int ypos, const char *fmt, ...)
{
	va_list vl;
	char textBuffer[256];

	va_start(vl, fmt);
	vsnprintf(textBuffer, 256, fmt, vl);
	va_end(vl);

	int org_xpos = xpos;

	for(int i = 0; textBuffer[i]; ++i)
	{
		switch(textBuffer[i])
		{
			case '\n':
				xpos = org_xpos;
				ypos += 8;
				break;
			case ' ':
				xpos += 8;
				break;
			case '\t':
				xpos = (xpos + 8) & ~7;
				break;
			default: 
			{
				int dpos = textBuffer[i] * 64;
				int y = ypos * TEXT_XRES;

				for(int j = 0; j < 8; ++j)
				{
					for(int k = 0; k < 8; ++k)
					{
						if(font8x8[dpos])
						{
							screen[y + (xpos + k)] = textColour;
						}
						dpos++;
					}
					y += TEXT_XRES;
				}
				xpos += 8;
			} break;
		}
	}
}

/*****************************************************************************/
/*****************************************************************************/

struct Device *TimerBase;
static struct IORequest timereq;

/*****************************************************************************/

static void InitTime()
{
	OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, &timereq, 0);
	TimerBase = timereq.io_Device;
}

/*****************************************************************************/

uint32_t GetTime()
{
	struct timeval t;
	GetSysTime(&t);
	return t.tv_micro / 1000 + t.tv_secs * 1000;
}

/*****************************************************************************/

uint32_t GetMicroTime()
{
	struct timeval t;
	GetSysTime(&t);
	return t.tv_micro + t.tv_secs * 1000000;
}

/*****************************************************************************/

static void ExitTime()
{
	CloseDevice(&timereq);
	TimerBase = NULL;
}

/*****************************************************************************/

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

void printMatrix(mat4 *mat)
{
	printf("----------------\n");
	printf("%10f %10f %10f %10f\n", mat->m[0][0], mat->m[0][1], mat->m[0][2], mat->m[0][3]);
	printf("%10f %10f %10f %10f\n", mat->m[1][0], mat->m[1][1], mat->m[1][2], mat->m[1][3]);
	printf("%10f %10f %10f %10f\n", mat->m[2][0], mat->m[2][1], mat->m[2][2], mat->m[2][3]);
	printf("%10f %10f %10f %10f\n", mat->m[3][0], mat->m[3][1], mat->m[3][2], mat->m[3][3]);
}

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

static void ClearMaggieScreen(MaggieFormat *pixels)
{
	memset(pixels, 0, MAGGIE_SCREENSIZE);
}

/*****************************************************************************/

void UpdateDebugCamera(mat4 *camera, vec3 *cameraPos, vec3 *cameraDir)
{
	vec3 up = {0.0f, 1.0f, 0.0f};
	vec3 left;
	vec3 camDir;
	vec3_normalise(&camDir, cameraDir);
	camDir.x = camDir.x;
	camDir.y = camDir.y;
	camDir.z = camDir.z;
	vec3_cross(&left, &camDir, &up);
	vec3_normalise(&left, &left);
	vec3_cross(&up, &left, &camDir);
	mat4_identity(camera);
	camera->m[0][0] = left.x;
	camera->m[0][1] = left.y;
	camera->m[0][2] = left.z;
	camera->m[1][0] = up.x;
	camera->m[1][1] = up.y;
	camera->m[1][2] = up.z;
	camera->m[2][0] = camDir.x;
	camera->m[2][1] = camDir.y;
	camera->m[2][2] = camDir.z;
	camera->m[3][0] = cameraPos->x;
	camera->m[3][1] = cameraPos->y + 0.3f;
	camera->m[3][2] = cameraPos->z;
}

/*****************************************************************************/

void UpdateCamera(tr_track *track, mat4 *camera)
{
}

/*****************************************************************************/

void TR_DrawBackgroud(MaggieFormat *pixels, apolloImage *image, mat4 *iCam);

int main(int argc, char *argv[])
{
	tr_track *tracks[4];

	for(int i = 0; i < 4; ++i)
	{
		printf("Generating track %d\n", i);
		tracks[i] = TR_GenerateTestTrack(i);
		printf("Track done\n");
		tracks[i]->player = TR_CreatePlayer(tracks[i]);
		printf("Player done\n");
	}

	if(!HasMaggie())
	{
		printf("TrackRacer requires a Maggie chipset.\n");
		return 0;
	}

	InitTime();

	EnterGraphicsMode();

	SystemControl(SCON_TakeOverSys, TRUE, TAG_DONE);
	OwnBlitter();
	WaitBlit();
	WaitBlit();

	UBYTE *screenMem = AllocMem(MAGGIE_SCREENSIZE * MAGGIE_NFRAMES, MEMF_ANY | MEMF_CLEAR);

	UBYTE *screenPixels[MAGGIE_NFRAMES];

	screenPixels[0] = screenMem;
	for(LONG i = 1; i < MAGGIE_NFRAMES; ++i)
	{
		screenPixels[i] = screenPixels[i - 1] + MAGGIE_SCREENSIZE;
	}

	volatile UWORD *SAGA_ScreenModeRead = (UWORD *)0xdfe1f4;
	volatile ULONG *SAGA_ChunkyDataRead = (ULONG *)0xdfe1ec;
	volatile UWORD *SAGA_ScreenMode = (UWORD *)0xdff1f4;
	volatile ULONG *SAGA_ChunkyData = (ULONG *)0xdff1ec;

	UWORD oldMode = *SAGA_ScreenModeRead;
	ULONG oldScreen = *SAGA_ChunkyDataRead;

	*SAGA_ScreenMode = MAGGIE_MODE;

	void *vblHandle = AddVBlankInt(VBLHandler, (APTR)&frameSig);

	LONG currentBufferNumber = 0;

	apolloImage *background = APOLLO_LoadImage("background.aif");

	mat4 projection, camera;
	
	mat4_perspective(&projection, 60.0f, (float)MAGGIE_YRES / MAGGIE_XRES, 0.1f, 25.0f);
	projection.m[1][1] *= -1.0;
	projection.m[0][0] *= 1.0;

	float phase = 0.0f;
	int currentTrack = 0;
	int lapCount = 0;

	vec3 cameraDir;
	vec3 cameraPos;
	TR_ResolvePoint(&cameraPos, &cameraDir, tracks[currentTrack], 0.0f);
	vec3_sub(&cameraPos, &cameraPos, &cameraDir);

	while(!(ReadJoyPort(0) & JPF_BUTTON_RED)) // While left mouse button not pressed
	{
		tr_track *track = tracks[currentTrack];

		*SAGA_ChunkyData = (ULONG)screenPixels[currentBufferNumber]; // Set current display frame.

		currentBufferNumber = (currentBufferNumber + 1) % MAGGIE_NFRAMES; // Cycle through the frame buffers..
		MaggieFormat *pixels = (MaggieFormat *)screenPixels[currentBufferNumber]; // Current screen buffer

		HandleWindowMessages();

		WaitVBLPassed();
		
		uint32_t startTime = GetTime();

		ClearMaggieScreen(pixels);

		TR_ResolvePoint(&cameraPos, &cameraDir, track, phase);
		UpdateDebugCamera(&camera, &cameraPos, &cameraDir);

		mat4 iCamera;
		mat4_inverseLight(&iCamera, &camera);
//		mat4_sync(&iCamera);

		TR_DrawBackgroud(pixels, background, &camera);
		TR_DrawTrack(pixels, track, &iCamera, &projection);

//		TR_DrawPlayer(pixels, track->player, &iCamera, &projection);
		TR_PlotTrack(pixels, track);

		float scale = 1.0f;

		SetPixel(pixels, cameraPos.x * scale + 0.5f, cameraPos.z * scale + 0.5f, 0xf100);

		uint32_t drawTime = GetTime() - startTime;

		TextOut(pixels, 0, TEXT_YRES - 8, "Frame time   %dms", drawTime);

		if(!(ReadJoyPort(0) & JPF_BUTTON_BLUE))
		{
			phase += 0.2f;
		}
		else
		{
			phase += 0.6f;
		}
		if(phase >= track->curveLength)
		{
			lapCount++;
			if(lapCount == 2)
			{
				phase = 0.0f;
				lapCount = 0;
				currentTrack++;
				if( currentTrack >= 4)
					currentTrack = 0;
				TR_ResolvePoint(&cameraPos, &cameraDir, track, 0.0f);
				vec3_sub(&cameraPos, &cameraPos, &cameraDir);
			}
			else
			{
				phase -= track->curveLength;
			}
		}
	}

	WaitBlit();
	WaitBlit();
	DisownBlitter();

	*SAGA_ScreenMode = oldMode;
	*SAGA_ChunkyData = oldScreen;

	ExitGraphicsMode();

	ExitTime();

	RemVBlankInt(vblHandle);

	SystemControl(SCON_TakeOverSys, FALSE, TAG_DONE); // Restore system

	FreeMem(screenMem, MAGGIE_SCREENSIZE * MAGGIE_NFRAMES);

	return 0;
}
