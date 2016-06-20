#include "debug.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//Terminal colors
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"

struct debug_trace_item {
	char *msg;
	const char *file;
	const char *function;
	unsigned int line;
	const char *err_code;
	int err_code_int;
	struct debug_trace_item *next;
};

struct debug_trace {
	struct debug_trace_item *head;
};

struct debug_trace* _rtr_tfw_debug_trace_new()
{
	struct debug_trace *trace = malloc(sizeof(*trace));
	trace->head = NULL;
	return trace;
}

void _rtr_tfw_debug_trace_print(struct debug_trace *trace)
{
	struct debug_trace_item *curr = trace->head;
	while(curr != NULL) {
		printf(KRED "%s:%u(%s):%s" KNRM,  curr->file, curr->line, curr->function, curr->msg);
		if (curr->err_code_int != 0) {
			printf("(%s)", curr->err_code); //TODO: Print to stderr
		}
		printf("\n");
		curr = curr->next;
	}

}

void _rtr_tfw_debug_trace_add(struct debug_trace *trace, char *msg,
			      const char *file, const char *function,
			      unsigned int line, int errcode)
{
	struct debug_trace_item *item = malloc(sizeof(*item));
	item->msg = msg;
	item->file = file;
	item->function = function;
	item->line = line;
	item->err_code = strerror(errcode);
	item->err_code_int = errcode;
	item->next = NULL;

	if (trace->head == NULL)
		trace->head = item;
	else {
		item->next = trace->head;
		trace->head = item;
	}
}

bool _rtr_tfw_debug_trace_has_data(struct debug_trace *trace)
{
	return trace->head != NULL;
}


//TODO: Also free the trace struct
void _rtr_tfw_debug_trace_free(struct debug_trace *trace)
{
	struct debug_trace_item *prev = trace->head;
	struct debug_trace_item *curr = trace->head;

	if (trace->head == NULL)
		return;

	while (curr->next != NULL) {
		curr = curr->next;
		free(prev->msg);
		free(prev);
		prev = curr;
	}
	free(curr->msg);
	free(curr);
	trace->head = NULL;
}
//TODO: Add debug trace purge
