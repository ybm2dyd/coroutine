#include <ucontext.h>

#define COROUTINE_READY 0
#define COROUTINE_RUNNING 1
#define COROUTINE_SUSPEND 2
#define COROUTINE_DIED 3

#define DEFAULT_COROUTINE 2000
#define STACK_SIZE 1024 * 1024

typedef void(*coroutine_func)(void*);

struct coroutine;

struct schedule
{
	ucontext_t ctx;
	struct coroutine** co;
	char stack[STACK_SIZE];
	int cap;
	int size;
	int running;
};

struct coroutine
{
	int status;
	ucontext_t ctx;
	coroutine_func func;
	void* arg;
	char* stack;
	int cap;
	int size;
};

struct schedule* coroutine_open();

int new_coroutine(struct schedule** S, coroutine_func func, void* arg);

int status(struct schedule* S, int id);

void resume(struct schedule* S, int id);

void yield(struct schedule* S, int id);

