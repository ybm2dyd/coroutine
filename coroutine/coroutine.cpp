#include "coroutine.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct schedule* coroutine_open()
{
	struct schedule* S = (struct schedule*)malloc(sizeof(*S));
	bzero(S, sizeof(*S));
	S->cap = DEFAULT_COROUTINE;
	S->running = -1;
	S->co = (struct coroutine**)malloc(sizeof(struct coroutine*) * S->cap);
	return S;
}

int create(struct schedule* S, coroutine_func func, void* arg)
{
	struct coroutine* co = (struct coroutine*)malloc(sizeof(*co));
	bzero(co, sizeof(*co));
	co->func = func;
	co->status = COROUTINE_READY;
	co->arg = arg;

	if (S->cap == S->size)
	{
		S->co = (struct coroutine**)realloc(S->co, sizeof(struct coroutine*) * S->cap * 2);
		bzero(S->co + S->cap * sizeof(struct coroutine*), S->cap * sizeof(struct coroutine*));
		int id = S->cap;
		S->cap = S->cap * 2;
		++S->size;
		S->co[id] = co;
		return id;
	}
	for (int i = 0; i < S->cap; i++)
	{
		if (S->co[(i + S->size) % S->cap] == NULL)
		{
			S->co[i] = co;
			++S->size;
			return i;
		}
	}
	assert(-1);
	return -1;
}

int status(struct schedule * S, int id)
{
	struct coroutine* co = S->co[id];
	if (co == NULL)
	{
		return COROUTINE_DIED;
	}
	return co->status;
}

void Run(struct schedule* S)
{
	struct coroutine* co = S->co[S->running];
	co->func(co->arg);
	free(co->stack);
	free(co);
	co = NULL;
	S->size--;
	S->running = -1;
}

void resume(struct schedule * S, int id)
{
	struct coroutine* co = S->co[id];

	switch (co->status)
	{
	case COROUTINE_READY:
		getcontext(&co->ctx);
		co->ctx.uc_stack.ss_sp = S->stack;
		co->ctx.uc_stack.ss_size = sizeof(S->stack);
		co->ctx.uc_link = &(S->ctx);
		makecontext(&co->ctx, (void(*)(void))(&Run), 1, S);
		co->status = COROUTINE_RUNNING;
		S->running = id;
		swapcontext(&S->ctx, &co->ctx);
		break;
	case COROUTINE_SUSPEND:
		co->status = COROUTINE_RUNNING;
		S->running = id;
		bzero(S->stack, sizeof(S->stack));
		memcpy(S->stack + STACK_SIZE - co->size, co->stack, co->size);
		swapcontext(&S->ctx, &co->ctx);
		break;
	}
}

void yield(schedule * S)
{
	assert(S->running >= 0);
	struct coroutine* co = S->co[S->running];
	char top;
	if (co->cap < S->stack + STACK_SIZE - &top)
	{
		free(co->stack);
		co->cap = S->stack + STACK_SIZE - &top;
		co->stack = (char*)malloc(co->cap);
		bzero(co->stack, sizeof(co->stack));
	}
	memcpy(co->stack, &top, co->cap);
	co->size = co->cap;
	co->status = COROUTINE_SUSPEND;
	S->running = -1;
	swapcontext(&co->ctx, &S->ctx);
}

