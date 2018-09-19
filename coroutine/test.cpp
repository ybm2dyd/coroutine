#include "coroutine.h"
#include <stdio.h>

struct args
{
	struct schedule* s;
	int val;
};

void foo(void* arg)
{
	struct schedule* S = ((struct args*)(arg))->s;
	int val = ((struct args*)(arg))->val;
	for (int i = 0; i < 10; i++)
	{
		fprintf(stdout, "%d\n", i + val);
		yield(S);
	}
}

int main(int argc, char* argv[])
{
	struct schedule* S = coroutine_open();
	struct args arg1;
	arg1.s = S;
	arg1.val = 100;
	int co = create(S, foo, &arg1);
	resume(S, co);
	resume(S, co);
	resume(S, co);
	resume(S, co);
	return 0;
}