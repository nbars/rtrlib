#include "rtr_test_framework.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "debug.h"

struct elem {
	struct rtr_tfw_context *val;
	struct elem *next;
};

struct context_list {
	struct elem *head;
} ctx_list;

static int _rtr_tfw_send(struct rtr_tfw_context *ctx, void *src, size_t size, unsigned int timeout_ms)
{
	ssize_t bytes_send;
	struct timeval timeout;

	if (ctx->tcp_config.rtr_client_socket_fd < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "It seems like there is no router connected!");
		return RTR_TFW_ERROR;
	}

	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;

	if (setsockopt (ctx->tcp_config.rtr_client_socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
			sizeof(timeout)) < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while setting send timeout on rtrclient socket!");
		return RTR_TFW_ERROR;
	}

	bytes_send = send(ctx->tcp_config.rtr_client_socket_fd, src, size, 0);

	if (bytes_send < size && bytes_send >= 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Only %li bytes of %li requested bytes where send to the rtrclient", bytes_send, size);
		return RTR_TFW_ERROR;
	} else if (bytes_send < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while sending data to the rtrclient!");
		return RTR_TFW_ERROR;
	}

	return RTR_TFW_SUCCESS;
}


static int _rtr_tfw_receive(struct rtr_tfw_context *ctx, void *dst, size_t size, unsigned int timeout_ms)
{
	ssize_t bytes_read;
	struct timeval timeout;

	if (ctx->tcp_config.rtr_client_socket_fd < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "It seems like there is no rtrclient connected!");
		return RTR_TFW_ERROR;
	}

	timeout.tv_sec = timeout_ms / 1000;
	timeout.tv_usec = (timeout_ms % 1000) * 1000;


	if (setsockopt (ctx->tcp_config.rtr_client_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
			sizeof(timeout)) < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while setting receive timeout on rtrclient socket!");
		return RTR_TFW_ERROR;
	}


	bytes_read = read(ctx->tcp_config.rtr_client_socket_fd, dst, size);
	if (bytes_read < size && bytes_read >= 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false, "Only %li bytes of %li requested bytes where received from the rtrclient", bytes_read, size);
		return RTR_TFW_ERROR;
	} else if (bytes_read < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while receiving data from rtrclient!");
		return RTR_TFW_ERROR;
	}

	return RTR_SUCCESS;
}

int _rtr_tfw_send_pdu(struct rtr_tfw_context *ctx, void *pdu, size_t size, unsigned int timeout_ms)
{
	int ret;

	ret = _rtr_tfw_send(ctx, pdu, size, timeout_ms);
	if (ret != RTR_TFW_SUCCESS) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while sending PDU to the rtrclient!");
		return RTR_TFW_ERROR;
	}
	return RTR_TFW_SUCCESS;
}

int _rtr_tfw_receive_pdu(struct rtr_tfw_context *ctx, void **pdu, unsigned int timeout_ms)
{
	void *w_ptr = NULL;
	struct pdu_header header;
	int ret = RTR_TFW_ERROR;

	*pdu = NULL;

	ret = _rtr_tfw_receive(ctx, &header, sizeof(header), timeout_ms);
	if (ret != RTR_TFW_SUCCESS) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while reading pdu header!");
		return RTR_TFW_ERROR;
	}

	rtr_tfw_pdu_header_to_host_byte_order(&header);

	*pdu = malloc(header.len);
	if (*pdu == NULL) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while allocating memory!!");
		return RTR_TFW_ERROR;
	}

	w_ptr = mempcpy(*pdu, &header, sizeof(header));

	//Receive rest
	ret = _rtr_tfw_receive(ctx, w_ptr, header.len - sizeof(header), timeout_ms);
	if (ret != RTR_TFW_SUCCESS) {
		free(*pdu);
		*pdu = NULL;
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while reading rest of pdu!");
		return RTR_TFW_ERROR;
	}

	if (!rtr_pdu_check_size(*pdu, ctx->error_trace)) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Received PDU is malformed!");
		free(*pdu);
		*pdu = NULL;
		return RTR_TFW_ERROR;
	}

	rtr_tfw_pdu_footer_to_host_byte_order(ctx, *pdu);
	if (_rtr_tfw_debug_has_pending_error(ctx)) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while converting PDU to host byte order!");
		free(*pdu);
		*pdu = NULL;
		return RTR_TFW_ERROR;
	}

	return RTR_TFW_SUCCESS;
}

struct rtr_tfw_context* _rtr_tfw_context_init(unsigned int port)
{
	int ret;
	struct elem *current_elem = NULL;
	struct sockaddr_in serv_addr;
	struct rtr_tfw_context* ctx = malloc(sizeof(*ctx));
	_rtr_tfw_assert_wo_ctx(ctx != NULL, "Error while calling malloc!");

	memset(ctx, 0, sizeof(*ctx));
	ctx->tcp_config.rtr_client_socket_fd = -1;
	ctx->tcp_config.cache_sockfd = -1;
	ctx->tcp_config.cache_port = port;
	ctx->error_trace = _rtr_tfw_debug_trace_new();

	//Append context to global list. This list will be used for cleanup operations
	//when the test is finished
	if (ctx_list.head == NULL) {
		ctx_list.head = malloc(sizeof(struct elem));
		_rtr_tfw_assert_wo_ctx(ctx_list.head != NULL,
					 "Error while calling malloc!");
		ctx_list.head->val = ctx;
		ctx_list.head->next = NULL;
	} else {
		current_elem = ctx_list.head;
		while (current_elem->next != NULL) {
			current_elem = current_elem->next;
		}
		current_elem->next = malloc(sizeof(struct elem));
		_rtr_tfw_assert_wo_ctx(current_elem->next != NULL,
					 "Error while calling malloc!");
		current_elem->next->val = ctx;
		current_elem->next->next = NULL;
	}

	//Create a cache socket
	ctx->tcp_config.cache_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (ctx->tcp_config.cache_sockfd < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while opening socket!");
		return ctx;
	}

	//Reuse socket
	if (setsockopt(ctx->tcp_config.cache_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error setting socket options!");
		return ctx;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(ctx->tcp_config.cache_port);
	if (bind(ctx->tcp_config.cache_sockfd, (struct sockaddr *) &serv_addr,
		 sizeof(serv_addr)) < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while binding!");
		return ctx;
	}

	//Only put max one client in the backlog
	ret = listen(ctx->tcp_config.cache_sockfd, 1);

	if (ret != 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while starting to listen on socket!");
		return ctx;
	}

	return ctx;
}

/**
 * @brief This function blocks until a rtrclient has connected to the cache
 * that belongs to the given context ctx.
 * @param ctx
 * @return
 */
int _rtr_tfw_context_wait_for_rtrclient(struct rtr_tfw_context *ctx)
{
	//TODO: Handle already connected client
	socklen_t clilen;
	struct sockaddr_in cli_addr;
	clilen = sizeof(cli_addr);

	if (ctx->tcp_config.rtr_client_socket_fd != -1) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx,
						      false,
						      "There is a open rtrclient socket assigned to this context,"
						      "please call shutdown first!")
		return RTR_TFW_ERROR;
	}

	ctx->tcp_config.rtr_client_socket_fd = accept(ctx->tcp_config.cache_sockfd,
						      (struct sockaddr *) &cli_addr,
						      &clilen);

	if (ctx->tcp_config.rtr_client_socket_fd < 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx,
						      false,
						      "Error while accepting rtrclient")
		return RTR_TFW_ERROR;
	}

	return RTR_TFW_SUCCESS;
}

int _rtr_tfw_cache_router_socket_shutdown(struct rtr_tfw_context *ctx, int how)
{
	int ret;
	if (ctx->tcp_config.rtr_client_socket_fd < 0)
		return RTR_TFW_SUCCESS;

	ret = shutdown(ctx->tcp_config.rtr_client_socket_fd, how);
	if (ret != 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while shutting down router socket!");
		return RTR_TFW_ERROR;
	}
	ret = close(ctx->tcp_config.rtr_client_socket_fd);
	if (ret != 0) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while closing router socket!");
		return RTR_TFW_ERROR;
	}

	ctx->tcp_config.rtr_client_socket_fd = -1;
	return RTR_TFW_SUCCESS;
}

/**
 * @brief Tear down the passed context ctx. This leads to closing all
 * sockets and freeing the ressources claimed by the context and rtrlib.
 * @param ctx
 */
void _rtr_tfw_context_tear_down(struct rtr_tfw_context **ctx)
{
	struct rtr_tfw_context *c = *ctx;
	int ret;
	if (c->cache_thread != 0) {
		pthread_cancel(c->cache_thread);
		pthread_join(c->cache_thread, NULL);
	}
	if (c->tcp_config.rtr_client_socket_fd >= 0) {
		_rtr_tfw_cache_router_socket_shutdown(c, 2);
	}
	if (c->registerd_rtrclient != NULL) {
		rtr_mgr_stop(c->registerd_rtrclient);
		rtr_mgr_free(c->registerd_rtrclient);
	}
	if (c->tcp_config.cache_sockfd >= 0) {
		ret = close(c->tcp_config.cache_sockfd);
		_rtr_tfw_assert_wo_ctx(ret == 0,
					 "Error while shutting down cache socket! (%s)", strerror(errno));
	}
	_rtr_tfw_debug_trace_free(c->error_trace);
	free(c->error_trace);
	c->error_trace = NULL;
	free(c);
	c = NULL;
}

/**
 * @brief This functions tears down all test contexts that where created
 * by the calling process. This also includes the closing of the sockets
 * and freeing of all ressources claimed by the assignd rtrclient.
 */
void rtr_tfw_tear_down()
{
	struct elem *prev;
	struct elem *curr = ctx_list.head;
	printf("Starting tear down!\n");
	while (curr != NULL) {
		_rtr_tfw_context_tear_down(&curr->val);
		prev = curr;
		curr = curr->next;
		free(prev);
	}
	ctx_list.head = NULL;
}

int _rtr_tfw_cache_register_rtrclient(struct rtr_tfw_context *ctx,
				      struct rtr_mgr_group *groups,
				      unsigned int group_size,
				      struct rtr_mgr_config **mgr_config,
				      unsigned int refresh_interval_s,
				      unsigned int expire_interval_s,
				      unsigned int retry_interval_s)
{
	int ret = rtr_mgr_init(mgr_config, groups, group_size,
			       30, 600, 600, NULL, NULL, NULL, NULL);
	if (ret != RTR_SUCCESS) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while initializing rtr_mgr_config");
		return RTR_TFW_ERROR;
	}

	//HACK!
	//We are accessing internal datastrctures of the rtrlib
	//This could fail if the rtrlib changes!
	for (unsigned int i = 0; i < group_size; ++i) {
		for (unsigned int j = 0; j < groups[i].sockets_len; ++j) {
			groups[i].sockets[j]->refresh_interval = refresh_interval_s;
			groups[i].sockets[j]->retry_interval = retry_interval_s;
			groups[i].sockets[j]->expire_interval = expire_interval_s;
		}
	}

	ret = rtr_mgr_start(*mgr_config);
	if (ret != RTR_SUCCESS) {
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Error while starting rtr_mgr_config");
		return RTR_TFW_ERROR;
	}

	ctx->registerd_rtrclient = *mgr_config;
	return RTR_TFW_SUCCESS;
}

/**
 * @brief Returns a pointer to an allocated memory area of size size.
 * The returned memory area is set to zero by this function.
 * @param size
 * @return
 */
void *rtr_tfw_zmalloc(size_t size)
{
	void *ptr = malloc(size);
	_rtr_tfw_assert_wo_ctx(ptr != NULL, "Error in malloc!");
	memset(ptr, 0, size);
	return ptr;
}
