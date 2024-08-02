#ifndef APOLLOIMAGE_H_INCLUDED
#define APOLLOIMAGE_H_INCLUDED

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

#ifdef AMIGA
# include <dos/dos.h>
#endif

struct apolloImage
{
	int32_t width;
	int32_t height;
	uint32_t format;
	int32_t modulo;
	int32_t mipmaps;
	uint32_t imageSize;
};
typedef struct apolloImage apolloImage;

enum apollo_format
{
	FMT_RGB16,
	FMT_RGBA16,
	FMT_RGB24,
	FMT_RGBA32,
	FMT_RGB16_COMP, // DXT1
	FMT_RGBA16_COMP,
	FMT_RGB24_COMP,
	FMT_RGBA32_COMP,

	FMT_LAST
};

#define FMT_NUM_FORMATS (int)FMT_LAST + 1

typedef enum apollo_format apollo_format;

# ifdef __cplusplus
extern "C"
{
# endif

apolloImage *APOLLO_LoadImage(const char *filename);
apolloImage *APOLLO_LoadImageFromMem(const void *data);
apolloImage *APOLLO_LoadImageFromFilePt(FILE *fp);

apolloImage *APOLLO_LoadDDSImage(const char *filename); // Only DXT1 in this pipe!

#ifdef AMIGA
apolloImage *APOLLO_LoadImageFromFileHandle(BPTR bptr);
#endif

uint32_t APOLLO_CalcImageModulo(uint32_t width, uint32_t height, uint32_t format);
uint32_t APOLLO_CalcImageSize(uint32_t width, uint32_t height, uint32_t format);
void *APOLLO_GetImageData(const apolloImage *image);
void *APOLLO_GetImageMipMapData(const apolloImage *image, int level);

void APOLLO_FreeImage(apolloImage *image);

# ifdef __cplusplus
extern "C"
{
# endif


#endif // APOLLOIMAGE_H_INCLUDED
