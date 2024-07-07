#include "fibers.h"
#include <stdlib.h>

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

	int StackSwap(Fiber *fiber __asm("a0"), int rVal __asm("d0"));
	ULONG *RegisterFill(ULONG *ptr __asm("a0"));

#ifdef __cplusplus
}
#endif

/*****************************************************************************/

struct Fiber
{
	char *stackPtr;
	char *stack;
	fiberFn startup;
	void *data;
};

/*****************************************************************************/

int Fiber_Yield(Fiber *fiber, int val)
{
	return StackSwap(fiber, val);
}

/*****************************************************************************/

int Fiber_Run(Fiber *fiber, int val)
{
	return Fiber_Yield(fiber, val);
}

/*****************************************************************************/

static int EntryPoint(Fiber *fiber)
{
	int rVal = fiber->startup(fiber, fiber->data);
	while(1)
		Fiber_Yield(fiber, rVal);
	return 0;
}

/*****************************************************************************/

Fiber *Fiber_Create(fiberFn func, void *data, int stackSize)
{
	Fiber *fiber = (Fiber *)malloc(sizeof(Fiber));

	fiber->stack = (char *)malloc(stackSize);
	memset(fiber->stack, 0, stackSize);
	fiber->stackPtr = fiber->stack + stackSize;
	fiber->startup = func;
	fiber->data = data;

	ULONG *ptr = (ULONG *)fiber->stackPtr;
	*--ptr = (ULONG)fiber;
	*--ptr = (ULONG)fiber;
	*--ptr = (ULONG)EntryPoint;
	ptr = RegisterFill(ptr);
	fiber->stackPtr = (char *)ptr;

	return fiber;
}

/*****************************************************************************/

void Fiber_Free(Fiber *fiber)
{
	delete [] fiber->stack;
	delete fiber;
}

/*****************************************************************************/
