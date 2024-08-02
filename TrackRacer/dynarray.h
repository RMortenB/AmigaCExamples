#ifndef DYNARRAY_H_INCLUDED
#define DYNARRAY_H_INCLUDED

#include "coretypes.h"

/*****************************************************************************/

typedef struct DynamicArray
{
	int capacity;
	int size;
	int objSize;
	uint8 *data;
} DynamicArray;

/*****************************************************************************/

DynamicArray *VA_Alloc(int objSize, int initialCapacity);
void VA_Free(DynamicArray *va);

void VA_AddObject(DynamicArray *va, void *data);
void VA_DeleteObject(DynamicArray *va, int indx);

void *VA_GetObject(DynamicArray *va, int indx);
int VA_Size(DynamicArray *va);

void VA_SetSize(DynamicArray *va, int size);

void *VA_GetData(DynamicArray *va);

#endif // DYNARRAY_H_INCLUDED
