/*
 * This file is part of RTRlib.
 *
 * This file is subject to the terms and conditions of the MIT license.
 * See the file LICENSE in the top level directory for more details.
 *
 * Website: http://rtrlib.realmv6.org/
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "mem_transport.h"

uint8_t* data_buffer;
size_t bytes_left;

uint8_t ctr = 0;

static int tr_mem_open(void *tr_ssh_sock);
static void tr_mem_close(void *tr_ssh_sock);
static void tr_mem_free(struct tr_socket *tr_sock);
static int tr_mem_recv(const void *tr_ssh_sock, void *buf, const size_t buf_len, const time_t timeout);
static int tr_mem_send(const void *tr_ssh_sock, const void *pdu, const size_t len, const time_t timeout);
static const char *tr_mem_ident(void *tr_ssh_sock);

int tr_mem_open(void *socket)
{
    return TR_SUCCESS;
}

void tr_mem_close(void *tr_ssh_sock)
{

}

void tr_mem_free(struct tr_socket *tr_sock)
{

}

int tr_mem_recv(const void* tr_ssh_sock, void* buf, const size_t buf_len, const time_t timeout){
    //printf("Bytes left: %lu\n", bytes_left);
    size_t tmp;
    if (bytes_left > buf_len) {
        memcpy(buf, data_buffer, buf_len);
        data_buffer += buf_len;
        bytes_left -= buf_len;
        return buf_len;
    } else if (bytes_left > 0) {
        memcpy(buf, data_buffer, bytes_left);
        tmp = bytes_left;
        bytes_left = 0;
        return tmp;
    } else {
        return TR_WOULDBLOCK;
    }
}

int tr_mem_send(const void *tr_ssh_sock, const void *pdu, const size_t len, const time_t timeout __attribute__((unused)))
{
    return len;
}

const char *tr_mem_ident(void *tr_ssh_sock)
{
    return "MEM0";
}

bool tr_mem_data_consumed() {
    return bytes_left == 0;
}

void tr_mem_set_data(uint8_t* data, size_t len) {
    data_buffer = data;
    bytes_left = len;
}

int tr_mem_init(struct tr_socket *socket)
{
    ctr = 0;
    socket->close_fp = &tr_mem_close;
    socket->free_fp = &tr_mem_free;
    socket->open_fp = &tr_mem_open;
    socket->recv_fp = &tr_mem_recv;
    socket->send_fp = &tr_mem_send;;
    socket->ident_fp = &tr_mem_ident;

    return TR_SUCCESS;
}
