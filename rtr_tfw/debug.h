#ifndef RTR_TFW_DEBUG_H
#define RTR_TFW_DEBUG_H

#include <errno.h>
#include <stdbool.h>

struct debug_trace;

typedef struct debug_trace debug_trace_t;

struct debug_trace* _rtr_tfw_debug_trace_new();

void _rtr_tfw_debug_trace_add(struct debug_trace *trace, char *msg,
			      const char *file, const char *function,
			      unsigned int line, int errcode);

void _rtr_tfw_debug_trace_print(struct debug_trace *trace);

bool _rtr_tfw_debug_trace_has_data(struct debug_trace *trace);

void _rtr_tfw_debug_trace_free(struct debug_trace *trace);

#endif
