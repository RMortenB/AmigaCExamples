#include "dynarray.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

DynamicArray *VA_Alloc(int objSize, int initialCapacity)
{
	if(initialCapacity < 16)
		initialCapacity = 16;
	DynamicArray *va = (DynamicArray *)malloc(sizeof(DynamicArray));
	va->capacity = initialCapacity;
	va->objSize = objSize;
	va->size = 0;

	va->data = (uint8 *)malloc(va->objSize * va->capacity);

	return va;
}

/*****************************************************************************/

void VA_AddObject(DynamicArray *va, void *obj)
{
	if(va->size == va->capacity)
	{
		va->capacity = va->capacity * 3 / 2;
		uint8 *newData = (uint8 *)malloc(va->objSize * va->capacity);
		memcpy(newData, va->data, va->objSize * va->size);
		free(va->data);
		va->data = newData;
	}
	memcpy(&va->data[va->size * va->objSize], obj, va->objSize);
	va->size++;
}

/*****************************************************************************/

void *VA_GetObject(DynamicArray *va, int indx)
{
	assert(indx < va->size);
	return &va->data[indx * va->objSize];
}

/*****************************************************************************/

void *VA_GetData(DynamicArray *va)
{
	return va->data;
}

/*****************************************************************************/

int VA_Size(DynamicArray *va)
{
	return va->size;
}

/*****************************************************************************/

void VA_SetSize(DynamicArray *va, int size)
{
	if(va->size < size)
	{
		if(va->capacity < size)
		{
			va->capacity = size;
			uint8 *newData = (uint8 *)malloc(va->objSize * va->capacity);
			memcpy(newData, va->data, va->objSize * va->size);
			free(va->data);
			va->data = newData;
		}
	}
	va->size = size;
}

/*****************************************************************************/

void VA_DeleteObject(DynamicArray *va, int indx)
{
	if(indx != va->size - 1)
	{
		memmove(va->data + indx * va->objSize, va->data + (indx + 1) * va->objSize, (va->size - indx - 1) * va->objSize);
	}
	va->size--;

}

/*****************************************************************************/

void VA_Free(DynamicArray *va)
{
	if(va)
	{
		if(va->data)
			free(va->data);
		free(va);
	}
}

/*****************************************************************************/
