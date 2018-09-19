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

int new_coroutine(struct schedule** S, coroutine_func func, void* arg)
{
	struct coroutine* co = (struct coroutine*)malloc(sizeof(*co));
	bzero(co, sizeof(*co));
	co->func = func;
	co->status = COROUTINE_READY;

	if ((*S)->cap == (*S)->size)
	{
		*S = (struct schedule*)realloc(*S, (*S)->cap * 2);
		bzero(S + (*S)->cap * sizeof(struct coroutine*) * (*S)->cap, (*S)->cap * sizeof(struct coroutine*) * (*S)->cap);
		int id = (*S)->cap;
		(*S)->cap = (*S)->cap * 2;
		++(*S)->size;
		(*S)->co[id] = co;
		return id;
	}
	for (int i = 0; i < (*S)->cap; i++)
	{
		if ((*S)->co[(i + (*S)->size) % (*S)->cap] == NULL)
		{
			(*S)->co[i] = co;
			++(*S)->size;
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

		swapcontext();
		break;
	}
}

void yield(schedule * S, int id)
{
	assert(S->running >= 0);
	struct coroutine* co = S->co[S->running];
	char top;
	if (co->cap < S->stack + STACK_SIZE - &top)
	{
		co->cap = S->stack + STACK_SIZE - &top;
		co->stack = (char*)malloc(co->cap);
	}
	memcpy(S->stack, co->stack);
	co->status = COROUTINE_SUSPEND;
	S->running = -1;
}

