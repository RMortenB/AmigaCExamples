#include "fibers.h"
#include <stdio.h>

int value = 0;

extern "C" int fiberFunc(Fiber *fiber);

int fiberFunc(Fiber *fiber, void *data)
{
	while(true)
	{
		if(Fiber_Yield(fiber, value++))
			break;
	}
	return -10;
}

int main(int argc, const char *argv[])
{
	Fiber *fiber = Fiber_Create(fiberFunc, NULL, 100000);
	for(int i = 0; i < 10; ++i)
	{
		printf("fiber rVal %d\n", Fiber_Run(fiber, 0));
	}
	printf("fiber last rVal %d\n", Fiber_Run(fiber, 1));
	Fiber_Free(fiber);

	return 0;
}
