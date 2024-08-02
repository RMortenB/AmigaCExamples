#include "apolloImage.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef AMIGA
#include <proto/exec.h>
#endif
/*****************************************************************************/

#define MIPMAP_TEST 0

/*****************************************************************************/

typedef struct
{
	uint8_t  idlength;
	uint8_t  colourmaptype;
	uint8_t  datatypecode;
	uint16_t colourmaporigin;
	uint16_t colourmaplength;
	uint8_t  colourmapdepth;
	uint16_t x_origin;
	uint16_t y_origin;
	uint16_t width;
	uint16_t height;
	uint8_t  bitsperpixel;
	uint8_t  imagedescriptor;
} tgaHeader;

/*****************************************************************************/

static uint16_t SwapShort(uint16_t v)
{
	return (v << 8) | (v >> 8);
}

/*****************************************************************************/

static uint8_t apollo_ReadByte(FILE *fp)
{
	uint8_t byte;
	fread(&byte, 1, sizeof(uint8_t), fp);
	return byte;
}

/*****************************************************************************/

static uint16_t apollo_ReadShort(FILE *fp)
{
	uint16_t word;
	fread(&word, 1, sizeof(uint16_t), fp);
	return word;
}

/*****************************************************************************/

static uint32_t apollo_ReadInt(FILE *fp)
{
	uint32_t lword;
	fread(&lword, 1, sizeof(uint32_t), fp);
	return lword;
}

/*****************************************************************************/

static void apollo_readTGAHeader(tgaHeader *header, FILE *fp)
{
#ifdef AMIGA
	header->idlength = apollo_ReadByte(fp);
	header->colourmaptype = apollo_ReadByte(fp);
	header->datatypecode = apollo_ReadByte(fp);
	header->colourmaporigin = SwapShort(apollo_ReadShort(fp));
	header->colourmaplength = SwapShort(apollo_ReadShort(fp));
	header->colourmapdepth = apollo_ReadByte(fp);
	header->x_origin = SwapShort(apollo_ReadShort(fp));
	header->y_origin = SwapShort(apollo_ReadShort(fp));
	header->width = SwapShort(apollo_ReadShort(fp));
	header->height = SwapShort(apollo_ReadShort(fp));
	header->bitsperpixel = apollo_ReadByte(fp);
	header->imagedescriptor = apollo_ReadByte(fp);
#else
	header->idlength = apollo_ReadByte(fp);
	header->colourmaptype = apollo_ReadByte(fp);
	header->datatypecode = apollo_ReadByte(fp);
	header->colourmaporigin = apollo_ReadShort(fp);
	header->colourmaplength = apollo_ReadShort(fp);
	header->colourmapdepth = apollo_ReadByte(fp);
	header->x_origin = apollo_ReadShort(fp);
	header->y_origin = apollo_ReadShort(fp);
	header->width = apollo_ReadShort(fp);
	header->height = apollo_ReadShort(fp);
	header->bitsperpixel = apollo_ReadByte(fp);
	header->imagedescriptor = apollo_ReadByte(fp);
#endif
	fseek(fp, header->idlength, SEEK_CUR);
}

/*****************************************************************************/

static void apollo_readCompressedTGAPixelData(uint8_t *data, const tgaHeader *header, FILE *fp)
{
	int bpp = header->bitsperpixel / 8;
	int linelen = header->width * bpp;
	uint8_t pixdata[4];

	for(int line = (header->height - 1) * linelen; line >= 0; line -= linelen)
	{
		int pixel = 0;
		while(pixel < header->width)
		{
			uint8_t token = apollo_ReadByte(fp);
			int pixcount = (token & 0x7f) + 1;
			if(token & 0x80)
			{
				for(int j = 0; j < bpp; ++j)
				{
					pixdata[j] = apollo_ReadByte(fp);
				}
				for(int j = 0; j < pixcount; ++j)
				{
					for(int k = 0; k < bpp; ++k)
					{
						data[line + (pixel + j) * bpp + k] = pixdata[k];
					}
				}
			}
			else
			{
				fread(&data[line + pixel * bpp], 1, bpp * pixcount, fp);
			}
			pixel += pixcount;
		}
	}
}

/*****************************************************************************/

static void apollo_readUnCompressedTGAPixelData(uint8_t *data, tgaHeader *header, FILE *fp)
{
	int bpp = header->bitsperpixel / 8;
	int linelen = header->width * bpp;
	int line = (header->height - 1) * linelen;

	for(int i = 0; i < header->height; ++i)
	{
		fread(&data[line], 1, linelen, fp);
		line -= linelen;
	}
}

/*****************************************************************************/

apolloImage *APOLLO_Create(uint32_t xres, uint32_t yres, uint32_t format)
{
	int imageSize = APOLLO_CalcImageSize(xres, yres, format);
#ifdef AMIGA
	apolloImage *image = AllocMem(sizeof(apolloImage) + imageSize, MEMF_ANY | MEMF_CLEAR);
#else
	apolloImage *image = malloc(sizeof(apolloImage) + imageSize);
#endif
	return image;
}

/*****************************************************************************/

void APOLLO_SwapRedBlue(apolloImage *img)
{
	uint8_t *imgData = APOLLO_GetImageData(img);
	int bpp = (img->format == FMT_RGB24 ? 3 : 4);

	for(int i = 0; i < img->height; ++i)
	{
		for(int j = 0; j < img->width; ++j)
		{
			int tmp = imgData[i * img->modulo + j * bpp + 0];
			imgData[i * img->modulo + j * bpp + 0] = imgData[i * img->modulo + j * bpp + 2];
			imgData[i * img->modulo + j * bpp + 2] = tmp;
		}
	}
}

/*****************************************************************************/

apolloImage *APOLLO_LoadTGA(const char *filename)
{
	FILE *fp;
	apolloImage *img = NULL;
	tgaHeader header;

	fp = fopen(filename, "rb");

	if(!fp)
	{
		printf("TGA File '%s' not found!\n", filename);
		return 0;
	}

	apollo_readTGAHeader(&header, fp);

	uint32_t imageFormat = (header.bitsperpixel == 24 ? FMT_RGB24 : FMT_RGBA32);

	switch(header.datatypecode)
	{
		case 2 :
			img = APOLLO_Create(header.width, header.height, imageFormat);
			 if(img)
			 {
			 	apollo_readUnCompressedTGAPixelData(APOLLO_GetImageData(img), &header, fp);
				APOLLO_SwapRedBlue(img);
			 }
			break;
		case 10 :
			img = APOLLO_Create(header.width, header.height, imageFormat);
			 if(img)
			 {
				apollo_readCompressedTGAPixelData(APOLLO_GetImageData(img), &header, fp);
				APOLLO_SwapRedBlue(img);
			}
			break;
		case 0 : // Add support for these as needed..
		case 1 :
		case 3 :
		case 9 :
		case 11 :
		case 32 :
		case 33 :
		default :
			printf("Unknown image format!\n");
			break;
	}
	fclose(fp);
	return img;
}

/*****************************************************************************/

uint32_t APOLLO_CalcImageModulo(uint32_t width, uint32_t height, uint32_t format)
{
	switch(format)
	{
		case FMT_RGB16 :
			return width * 2;
		case FMT_RGBA16 :
			return width * 2;
		case FMT_RGB24 :
			return width * 3;
		case FMT_RGBA32 :
			return width * 4;
		case FMT_RGB16_COMP :
			return width / 2;
	}
	return 0;
}

/*****************************************************************************/

uint32_t APOLLO_CalcImageSize(uint32_t width, uint32_t height, uint32_t format)
{
	switch(format)
	{
		case FMT_RGB16 :
			return width * height * 2;
		case FMT_RGBA16 :
			return width * height * 2;
		case FMT_RGB24 :
			return width * height * 3;
		case FMT_RGBA32 :
			return width * height * 4;
		case FMT_RGB16_COMP :
			if(width < 4)
				width = 4;
			if(height < 4)
				height = 4;
			return (width * height) / 2;
	}
	return 0;
}

/*****************************************************************************/

void *APOLLO_GetImageData(const apolloImage *image)
{
	return (void *)(((char *)image) + sizeof(apolloImage));
}

/*****************************************************************************/

void *APOLLO_GetImageMipMapData(const apolloImage *image, int level)
{
	char *data = APOLLO_GetImageData(image);
	int xres = image->width;
	int yres = image->height;
	for(int i = 0; (i < level) && (xres > 1) && (yres > 1); ++i)
	{
		data += APOLLO_CalcImageSize(xres, yres, image->format);
		xres >>= 1;
		yres >>= 1;
	}
	return data;
}

/*****************************************************************************/

apolloImage *APOLLO_LoadImageFromFilePt(FILE *fp)
{
	char ident[4];
	uint32_t width, height, format;
	uint32_t modulo, imageSize;
	apolloImage *image;
	uint32_t fpos = ftell(fp);
	
	fread(ident, 1, 4, fp);

	if(strncmp(ident, "VAMP", 4))
	{
		fseek(fp, fpos, SEEK_SET);
		return NULL;
	}
	fread(&format, sizeof(uint32_t), 1, fp);
	fread(&width, sizeof(uint32_t), 1, fp);
	fread(&height, sizeof(uint32_t), 1, fp);
	imageSize = APOLLO_CalcImageSize(width, height, format);
	modulo = APOLLO_CalcImageModulo(width, height, format);

#ifdef AMIGA
	image = AllocMem(sizeof(apolloImage) + imageSize, MEMF_ANY | MEMF_CLEAR);
#else
	image = malloc(sizeof(apolloImage) + imageSize);
#endif
	image->width = width;
	image->height = height;
	image->format = format;
	image->modulo = modulo;
	image->imageSize = sizeof(apolloImage) + imageSize;


	fread(APOLLO_GetImageData(image), sizeof(char), imageSize, fp);

	return image;
}

/*****************************************************************************/

apolloImage *APOLLO_LoadImage(const char *filename)
{
	apolloImage *image;

	FILE *fp = fopen(filename, "rb");

	if(!fp)
	{
		printf("File \"%s\" not found\n", filename);
		return NULL;
	}

	image = APOLLO_LoadImageFromFilePt(fp);

	fclose(fp);

	return image;
}

/*****************************************************************************/
#ifdef AMIGA
static uint32_t bswap(uint32_t val)
{
	return (val >> 24) | (val << 24) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000);
}
#else
static uint32_t bswap(uint32_t val)
{
	return val;
}
#endif

/*****************************************************************************/

static uint32_t ReadUInt(FILE *fp)
{
	uint32_t val;
	fread(&val, sizeof(uint32_t), 1, fp);
	return bswap(val);
}

/*****************************************************************************/

typedef struct
{
  uint32_t	dwSize;
  uint32_t	dwFlags;
  uint32_t	dwFourCC;
  uint32_t	dwRGBBitCount;
  uint32_t	dwRBitMask;
  uint32_t	dwGBitMask;
  uint32_t	dwBBitMask;
  uint32_t	dwABitMask;
} DDS_PIXELFORMAT;

/*****************************************************************************/

typedef struct
{
  uint32_t	dwSize;
  uint32_t	dwFlags;
  uint32_t	dwHeight;
  uint32_t	dwWidth;
  uint32_t	dwPitchOrLinearSize;
  uint32_t	dwDepth;
  uint32_t	dwMipMapCount;
  uint32_t	dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  uint32_t dwCaps;
  uint32_t dwCaps2;
  uint32_t dwCaps3;
  uint32_t dwCaps4;
  uint32_t dwReserved2;
} DDS_HEADER;

/*****************************************************************************/

int ReadDDSHeader(DDS_HEADER *hdr, FILE *fp)
{
	if(!fp)
		return 0;
	fread(hdr, sizeof(DDS_HEADER), 1, fp);
	uint32_t *dwHdr = (uint32_t *)hdr;

	int nDWords =(int)sizeof(DDS_HEADER) >> 2;

	for(int i = 0; i < nDWords; ++i)
	{
		dwHdr[i] = bswap(dwHdr[i]);
	}
	if(hdr->dwSize != 124)
		return 0;
	if(hdr->ddspf.dwSize != 32)
		return 0;
	if(!(hdr->dwFlags & ~0x1007))
		return 0;

	return 1;
}

/*****************************************************************************/

typedef struct 
{
	uint16_t col0;
	uint16_t col1;
	uint32_t pixels;
} CompBlock;

/*****************************************************************************/

static void DecompColours(CompBlock *block, uint32_t *cols)
{
	uint16_t col0 = (block->col0 >> 8) | (block->col0 << 8);
	uint16_t col1 = (block->col1 >> 8) | (block->col1 << 8);

	int r0 = (col0 >> 8) & 0xf8; r0 |= r0 >> 5;
	int g0 = (col0 >> 3) & 0xfc; g0 |= g0 >> 6;
	int b0 = (col0 << 3) & 0xf8; b0 |= b0 >> 5;
	int r1 = (col1 >> 8) & 0xf8; r1 |= r1 >> 5;
	int g1 = (col1 >> 3) & 0xfc; g1 |= g1 >> 6;
	int b1 = (col1 << 3) & 0xf8; b1 |= b1 >> 5;

	cols[0] = (r0 << 16) | (g0 << 8) | (b0 << 0);
	cols[1] = (r1 << 16) | (g1 << 8) | (b1 << 0);

	if(col0 > col1)
	{
		cols[2] = (((r0 * 2 + r1) / 3) << 16) | (((g0 * 2 + g1) / 3) << 8) | (((b0 * 2 + b1) / 3) << 0);
		cols[3] = (((r0 + r1 * 2) / 3) << 16) | (((g0 + g1 * 2) / 3) << 8) | (((b0 + b1 * 2) / 3) << 0);
	}
	else
	{
		cols[2] = (((r0 + r1) / 2) << 16) | (((g0 + g1) / 2) << 8) | (((b0 + b1) / 2) << 0);
		cols[3] = 0;
	}
}

/*****************************************************************************/

static void apollo_DecompDXT(apolloImage *image, uint8_t *compdata)
{
	int xBlocks = image->width / 4;
	int yBlocks = image->height / 4;
	uint32_t *imageData = APOLLO_GetImageData(image);

	for(int i = 0; i < yBlocks; ++i)
	{
		for(int j = 0; j < xBlocks; ++j)
		{
			CompBlock *block = (CompBlock *)&compdata[(i * xBlocks + j) * sizeof(CompBlock)];
			uint32_t cols[4];
			DecompColours(block, cols);
			int shifts[4*4] =
			{
				24,26,28,30,
				16,18,20,22,
				 8,10,12,14,
				 0, 2, 4, 6,
			};
			for(int k = 0; k < 4; ++k)
			{
				for(int l = 0; l < 4; ++l)
				{
					int pen = (block->pixels >> shifts[k * 4 + l]) & 0x03;

					imageData[(i * 4 + k) * image->width + j * 4 + l] = cols[pen];
				}
			}
		}
	}
}

/*****************************************************************************/

apolloImage *apollo_Downscale(apolloImage *image)
{
	int imgSize = APOLLO_CalcImageSize(image->width / 2, image->height / 2, FMT_RGBA32);
#ifdef AMIGA
	apolloImage *newImg = AllocMem(sizeof(apolloImage) + imgSize, MEMF_ANY | MEMF_CLEAR);
#else
	apolloImage *newImg = malloc(sizeof(apolloImage) + imgSize);
#endif
	newImg->width = image->width / 2;
	newImg->height = image->height / 2;
	newImg->format = FMT_RGBA32;
	newImg->modulo = 0;

	uint8_t *src = APOLLO_GetImageData(image);
	uint8_t *dst = APOLLO_GetImageData(newImg);

	for(int i = 0; i < (int)newImg->height; ++i)
	{
		for(int j = 0; j < (int)newImg->width; ++j)
		{
			for(int k = 0; k < 4; ++k)
			{
				int col;
				col  = src[((i * 2 + 0) * image->width + (j * 2 + 0)) * 4 + k];
				col += src[((i * 2 + 0) * image->width + (j * 2 + 1)) * 4 + k];
				col += src[((i * 2 + 1) * image->width + (j * 2 + 0)) * 4 + k];
				col += src[((i * 2 + 1) * image->width + (j * 2 + 1)) * 4 + k];
				dst[(i * newImg->width + j) * 4 + k] = col / 4;
			}
		}
	}
	return newImg;
}

/*****************************************************************************/

void apollo_ComputeMipMaps(apolloImage *image)
{

}

/*****************************************************************************/

apolloImage *APOLLO_LoadDDSImage(const char *filename)
{
	FILE *fp = fopen(filename, "rb");

	if(!fp)
	{
		return NULL;
	}

	uint32_t magic;
	fread(&magic, sizeof(magic), 1, fp);

	if(bswap(magic) != 0x20534444)
	{
		fclose(fp);
		return NULL;
	}

	DDS_HEADER hdr;
	if(!ReadDDSHeader(&hdr, fp))
	{
		fclose(fp);
		return NULL;
	}

	int imgSize = APOLLO_CalcImageSize(hdr.dwWidth, hdr.dwHeight, FMT_RGB16_COMP);
	if(hdr.dwMipMapCount > 4)
		hdr.dwMipMapCount = 4;
	for(uint32_t i = 1; i < hdr.dwMipMapCount; ++i)
	{
		int mipWidth = hdr.dwWidth >> i;
		int mipHeight = hdr.dwHeight >> i;

		imgSize += APOLLO_CalcImageSize(mipWidth, mipHeight, FMT_RGB16_COMP);
	}

#ifdef AMIGA
	apolloImage *image = AllocMem(sizeof(apolloImage) + imgSize, MEMF_ANY | MEMF_CLEAR);
#else
	apolloImage *image = malloc(sizeof(apolloImage) + imgSize);
#endif
	if(!image)
	{
		fclose(fp);
		return NULL;
	}
	image->width = hdr.dwWidth;
	image->height = hdr.dwHeight;
	image->format = FMT_RGB16_COMP;
	image->modulo = 0;
	image->imageSize = sizeof(apolloImage) + imgSize;
	image->mipmaps = hdr.dwMipMapCount;

	fseek(fp, 128, SEEK_SET);
	fread(APOLLO_GetImageData(image), 1, imgSize, fp);
#if MIPMAP_TEST
	CompBlock *blocks = (CompBlock *)((uint8_t *)APOLLO_GetImageData(image));

	uint16_t mipmapColours[4] = {0x001f, 0x07e0, 0xf100, 0xf11f};

	for(uint32_t i = 0; i < hdr.dwMipMapCount; ++i)
	{
		int height = hdr.dwHeight >> i;
		int width = hdr.dwWidth >> i;

		for(int j = 0; j < height; j += 4)
		{
			for(int k = 0; k < width; k += 4)
			{
				if(blocks->col0 > blocks->col1)
					blocks->col0 = mipmapColours[i];
				else
					blocks->col1 = mipmapColours[i];
				blocks++;
			}
		}
	}
#endif

	fclose(fp);

	return image;
}

/*****************************************************************************/

apolloImage *APOLLO_LoadImageFromMem(const void *data) // TODO
{
	return NULL;
}

/*****************************************************************************/

#ifdef AMIGA
apolloImage *APOLLO_LoadImageFromFileHandle(BPTR bptr) // TODO
{
	return NULL;
}
#endif

/*****************************************************************************/

void APOLLO_FreeImage(apolloImage *image)
{
	if(image)
	{
#ifdef AMIGA
		FreeMem(image, image->imageSize);
#else
		free(image);
#endif
	}
}

/*****************************************************************************/
