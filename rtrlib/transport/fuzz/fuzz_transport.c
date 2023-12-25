/*
 * This file is part of RTRlib.
 *
 * This file is subject to the terms and conditions of the MIT license.
 * See the file LICENSE in the top level directory for more details.
 *
 * Website: http://rtrlib.realmv6.org/
 */

#include "fuzz_transport.h"

#include "rtrlib/lib/alloc_utils_private.h"
#include "rtrlib/lib/log_private.h"
#include "rtrlib/rtrlib_export_private.h"
#include "rtrlib/transport/transport.h"
#include "rtrlib/transport/transport_private.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define TCP_DBG(fmt, sock, ...)                                                                            \
	do {                                                                                               \
		const struct rt_fuzz_socket *tmp = sock;                                                    \
		lrtr_dbg("TCP Transport(%s:%s): " fmt, tmp->config.host, tmp->config.port, ##__VA_ARGS__); \
	} while (0)
#define TCP_DBG1(a, sock) TCP_DBG(a, sock)

struct rt_fuzz_socket {
	struct fuzz_transport_config *config;
	char *ident;
};

static int rt_fuzz_open(void *rt_fuzz_sock);
static void rt_fuzz_close(void *rt_fuzz_sock);
static void rt_fuzz_free(struct tr_socket *tr_sock);
static int rt_fuzz_recv(const void *rt_fuzz_sock, void *pdu, const size_t len, const time_t timeout);
static int rt_fuzz_send(const void *rt_fuzz_sock, const void *pdu, const size_t len, const time_t timeout);
static const char *rt_fuzz_ident(void *socket);

static int set_socket_blocking(int socket)
{
		return TR_SUCCESS;
}

static int set_socket_non_blocking(int socket)
{
		return TR_SUCCESS;
}

static int get_socket_error(int socket)
{
	return TR_SUCCESS;
}

/* WARNING: This function has cancelable sections! */
int rt_fuzz_open(void *tr_socket)
{
	return TR_SUCCESS;
}

void rt_fuzz_close(void *rt_fuzz_sock)
{
}

void rt_fuzz_free(struct tr_socket *tr_sock)
{
	struct rt_fuzz_socket *socket = tr_sock->socket;
	lrtr_free(socket);
}

int rt_fuzz_recv(const void *rt_fuzz_sock, void *pdu, const size_t len, const time_t timeout)
{
	struct rt_fuzz_socket *socket = (struct rt_fuzz_socket *) rt_fuzz_sock;
	size_t bytes_consumed = socket->config->input_size - socket->config->bytes_left;

	if (socket->config->bytes_left) {
		size_t packet_size;
		if (socket->config->bytes_left >= len) 
			packet_size = len;
		else
			packet_size = socket->config->bytes_left;

		memcpy(pdu, &socket->config->input[bytes_consumed], packet_size);
		socket->config->bytes_left -= packet_size;
		return packet_size;
	} else {
		if (socket->config->blocked++ < 4) {
			return TR_WOULDBLOCK;
		} else {
			exit(1);
		}
	}
}

int rt_fuzz_send(const void *rt_fuzz_sock, const void *pdu, const size_t len, const time_t timeout)
{
	return len;
}

const char *rt_fuzz_ident(void *socket)
{
	size_t len;
	struct rt_fuzz_socket *sock = socket;
	return sock->ident;
}

RTRLIB_EXPORT int tr_fuzz_init(struct fuzz_transport_config *config, struct tr_socket *tr_socket)
{
	tr_socket->close_fp = &rt_fuzz_close;
	tr_socket->free_fp = &rt_fuzz_free;
	tr_socket->open_fp = &rt_fuzz_open;
	tr_socket->recv_fp = &rt_fuzz_recv;
	tr_socket->send_fp = &rt_fuzz_send;
	tr_socket->ident_fp = &rt_fuzz_ident;

	tr_socket->socket = lrtr_malloc(sizeof(struct rt_fuzz_socket));
	struct rt_fuzz_socket *fuzz_socket = tr_socket->socket;
	config->blocked = 0;
	fuzz_socket->config = config;
	fuzz_socket->ident = config->ident;

	return TR_SUCCESS;
}
