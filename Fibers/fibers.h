#pragma once
#include <string.h>
#include <stdlib.h>

#include <exec/types.h>

/*****************************************************************************/
//
// To create a fiber, call Fiber_Create with entry point and stack size.
//
// When the fiber needs to run, call Fiber_Run()
// The fiber will run until it returns, or it calls Fiber_Yield().
// The parameter passed to Fiber_Yield() or Fiber_Run() is returned in the other fiber.
// Exception handing is not supported past the entry point. It has never been tested in any way.
// Also memory allocated on the fiber is not freed when the fiber gets freed.
//
// Only tested with gcc6.5
//
/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

struct Fiber;
typedef struct Fiber Fiber;

/*****************************************************************************/

typedef int (*fiberFn)(Fiber *fiber, void *data);

/*****************************************************************************/
// Fiber_Yield() is called from the "thread", and Fiber_Run() from the "main thread"
// They are the same now, but it didn't "feel" right to have only the one..

/*****************************************************************************/

int Fiber_Yield(Fiber *fiber, int val);
int Fiber_Run(Fiber *fiber, int val);

/*****************************************************************************/

Fiber *Fiber_Create(fiberFn func, void *data, int stackSize);
void Fiber_Free(Fiber *fiber);

/*****************************************************************************/
#ifdef __cplusplus
}
#endif
