#ifndef RTR_TEST_FRAMEWORK_H
#define RTR_TEST_FRAMEWORK_H

#include "pdu.h"
#include "debug.h"

#include "rtrlib/rtrlib.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define RTR_TFW_SUCCESS 0
#define RTR_TFW_ERROR -1
#define RTR_TFW_NO_DATA_AVAIL -2

#define RTR_VERSION_0 0
#define RTR_VERSION_1 1

//Terminal colors
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"

//TODO: Add braces to macro expressions
//TODO: Add debug output?
//TODO: Check what macros are really needed

/*
 * Debug and error report macros
 */

//Assert the given expression. If the assertion fail the
//given message msg will be added to the trace log that belongs to the context
#define _rtr_tfw_debug_assert_add_debug_trace(ctx, expr, msg, ...) \
	if (!(expr)) { \
	char *msg_buf = malloc(1024); \
	snprintf(msg_buf, 1024, msg, ## __VA_ARGS__); \
	_rtr_tfw_debug_trace_add(ctx->error_trace, msg_buf, __FILE__, __FUNCTION__, __LINE__, errno); \
	errno = 0;\
	}

//Private
//Check if error occurred in the last function call
#define _rtr_tfw_debug_check_error(ctx) \
	if (_rtr_tfw_debug_trace_has_data(ctx->error_trace)) { \
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false, "ERROR"); \
		_rtr_tfw_debug_trace_print(ctx->error_trace); \
		fflush(NULL); \
		rtr_tfw_tear_down(); \
		return -1; \
	}

#define _rtr_tfw_debug_has_pending_error(ctx) \
	_rtr_tfw_debug_trace_has_data(ctx->error_trace)

//Assertion for places where no context is available
#define _rtr_tfw_assert_wo_ctx(expr, msg, ...) \
	if (!(expr)) {\
		printf(KRED "Error:%s:%i: " KNRM, __FILE__, __LINE__); \
		printf(KRED msg KNRM, ## __VA_ARGS__);\
		printf("\n"); \
		rtr_tfw_tear_down(); \
	}

//Public
#define rtr_tfw_assert(expr, msg, ...) \
	if (!(expr)) {\
		printf(KRED "Error:%s:%i: " KNRM, __FILE__, __LINE__); \
		printf(KRED msg KNRM "\n", ## __VA_ARGS__);\
		rtr_tfw_tear_down(); \
		return RTR_TFW_ERROR; \
	}

/*
 * State Quarys
 * The following macros can be used to quary the state
 * of a rtr_socket that belongs to a given test context.
 */

/*
 * State management macros for the cache server
 */
#define rtr_tfw_context_init(ctx, cache_port_tcp) \
	rtr_tfw_assert(cache_port_tcp >= 0 && cache_port_tcp <= 65535, "Illegal cache_port"); \
	ctx = NULL; \
	ctx = _rtr_tfw_context_init(cache_port_tcp); \
	rtr_tfw_assert(ctx != NULL, "Error while creating cache! Maybe no access rights to open the port?"); \
	_rtr_tfw_debug_check_error(ctx);

//TODO: Implement timeout
//Accept a rtrclient if its connect within timeout_ms ms.
//If no client connects, an error is raised.
#define rtr_tfw_context_wait_for_rtrclient(ctx, timeout_ms); \
	_rtr_tfw_context_wait_for_rtrclient(ctx); \
	_rtr_tfw_debug_check_error(ctx);

//If an rtrclient is connected to the cache server that belongs to
//context ctx, the connection will be closed by the cache.
#define rtr_tfw_context_rtrclient_close(ctx)

/*
 * State management macros for the rtrclient
 */

//TODO: How we should handle the configurations of the interval parameters?
//Currently its works, because we're accessing the rtr_socket structure directly.


//Init the client and set the cbs
#define rtr_tfw_rtrclient_register_with_cache(ctx, groups, group_size, mgr_config, refresh_interval_s, expire_interval, retry_interval_s) \
	_rtr_tfw_cache_register_rtrclient(ctx, groups, group_size, &mgr_config, refresh_interval_s, expire_interval, retry_interval_s); \
	_rtr_tfw_debug_check_error(ctx);

//This will stop/shutdown the client that belongs to the given ctx.
//This function can be used to check if the client shutdown gracefully.
#define rtr_tfw_rtrclient_stop(ctx, timeout)

/*
 * Transport macros (sending and receiving PDUs/Data)
 */
#define rtr_tfw_try_receive_pdu(ctx, pdu, timeout_ms) \
	rtr_tfw_assert(timeout_ms >= 0, "Negative values for timeout are not allowed!"); \
	rtr_tfw_assert(ctx != NULL, "The context must not be null!"); \
	pdu = NULL; \
	_rtr_tfw_receive_pdu(ctx, &pdu, timeout_ms); \
	_rtr_tfw_debug_trace_free(ctx->error_trace);

#define rtr_tfw_pdu_send(ctx, pdu, real_size_in_byte, timeout_ms) \
	_rtr_tfw_send_pdu(ctx, pdu, real_size_in_byte, timeout_ms); \
	_rtr_tfw_debug_check_error(ctx);

//Receive a PDU and store it in the variable named pdu.
#define rtr_tfw_receive_pdu(ctx, pdu, timeout_ms) \
	rtr_tfw_assert(timeout_ms >= 0, "Negative values for timeout are not allowed!"); \
	rtr_tfw_assert(ctx != NULL, "The context must not be null!"); \
	pdu = NULL; \
	_rtr_tfw_receive_pdu(ctx, &pdu, timeout_ms); \
	_rtr_tfw_debug_check_error(ctx);

#define rtr_tfw_free_pdu(pdu) \
	rtr_tfw_assert(pdu != NULL, "Cannot free an empty PDU (NULL)!"); \
	free(pdu); \
	pdu = NULL;

//TODO: check error
#define rtr_tfw_pdu_hton(ctx, pdu) \
	rtr_pdu_to_network_byte_order(ctx, pdu);

/*
 * PDU validation macros
 */

//TODO: What else do we need?

//Valid arguments for type are "enum pdu_type"
#define rtr_tfw_pdu_assert_type(pdu, type) \
	rtr_tfw_assert(_rtr_tfw_pdu_get_type(pdu) == type, \
		       "Type doesn't match! (Expected:%u, Received:%u)", \
			type, _rtr_tfw_pdu_get_version(pdu))

#define rtr_tfw_pdu_assert_version(pdu, version) \
	rtr_tfw_assert(_rtr_tfw_pdu_get_version(pdu) == version, \
		       "Version doesn't match! (Expected:%u, Received:%u)", \
			version, _rtr_tfw_pdu_get_version(pdu))

#define rtr_tfw_pdu_assert_error_code(pdu, expected_error_code) \
	rtr_tfw_pdu_assert_type(pdu, ERROR); \
	rtr_tfw_assert(((struct pdu_error*)pdu)->error_code == expected_error_code, \
			"Error PDU contains unexpected error code! (Expected:%u, Received:%u)", \
			expected_error_code, ((struct pdu_error*)pdu)->error_code);

/*
 * PDU validation shorthands
 */
#define rtr_tfw_pdu_rassert(ctx, pdu_type, rtr_version, timeout) \
{ \
	void *pdu_tmp_name_53421; \
	rtr_tfw_receive_pdu(ctx, pdu_tmp_name_53421, timeout); \
	rtr_tfw_pdu_assert_type(pdu_tmp_name_53421, pdu_type); \
	rtr_tfw_pdu_assert_version(pdu_tmp_name_53421, rtr_version); \
	rtr_tfw_free_pdu(pdu_tmp_name_53421); \
}

struct tcp_config {
	unsigned int cache_port;
	int cache_sockfd;
	int rtr_client_socket_fd;
};

struct rtr_tfw_context {
	pthread_rwlock_t lock;
	struct tcp_config tcp_config;
	pthread_t cache_thread;
	struct rtr_mgr_config *registerd_rtrclient;
	struct debug_trace *error_trace;
};

/*
 * Gloabal vars
 */


/*
 * Functions ----------------------------------------------------------------------------
 */

/**
 * @brief
 * Tear down all test contexts that where created by the calling process.
 * When this call returns, all ressources are freed and the test framework
 * is reset to its initial state.
 */
void rtr_tfw_tear_down();

/**
 * @brief
 * Create a new test context and start a cache server on the passed port.
 * After this function was called, a rtrclient can connect to the cache on the
 * given port.
 * @param port The port where the cache server, that belongs to the test context,
 * will listen.
 * @return A pointer to a new rtr_tfw_context. On error NULL is returned
 */
struct rtr_tfw_context* _rtr_tfw_context_init(unsigned int port);

int _rtr_tfw__context_cache_rtrclient_socket_shutdown(struct rtr_tfw_context *ctx, int how);

/**
 * @brief
 * Tear down the given context and the rtrclient that was
 * associated with _rtr_tfw_cache_register_rtrclient
 * @param ctx
 */
void _rtr_tfw_context_tear_down(struct rtr_tfw_context **ctx);

/**
 * @brief
 * Register a rtrclient with the given context.
 * @param ctx
 * @param groups
 * @param group_size
 * @param mgr_config
 * @param refresh_interval_s
 * @param expire_interval_s
 * @param retry_interval_s
 * @return
 */
int _rtr_tfw_cache_register_rtrclient(struct rtr_tfw_context *ctx, struct rtr_mgr_group *groups,
				      unsigned int group_size, struct rtr_mgr_config **mgr_config,
				      unsigned int refresh_interval_s, unsigned int expire_interval_s,
				      unsigned int retry_interval_s);

/**
 * @brief
 * Block until a rtrclient connects to the cache server that belongs to
 * the test context ctx.
 * @param ctx
 * @return
 */
int _rtr_tfw_context_wait_for_rtrclient(struct rtr_tfw_context *ctx);

/**
 * @brief _rtr_tfw_receive_pdu
 * If this function succesfully returns, *pdu points to a well formed PDU.
 * All fields of the returned pdu are in host-byte-order!.
 * Well formed means that the length field was compared with the expected length,
 * thus the buffer *pdu is at least ((struct pdu_header*)(*pdu))->len byte long.
 * @param ctx
 * @param pdu
 * @param timeout_ms
 * @return
 */
int _rtr_tfw_receive_pdu(struct rtr_tfw_context *ctx, void **pdu, unsigned int timeout_ms);

/**
 * @brief
 * Send the PDU pdu to the rtrclient that was registered with the given test context
 * @param ctx
 * @param pdu
 * @param size
 * @param timeout_ms
 * @return
 */
int _rtr_tfw_send_pdu(struct rtr_tfw_context *ctx, void *pdu, size_t size, unsigned int timeout_ms);

void *rtr_tfw_zmalloc(size_t size);

#endif
