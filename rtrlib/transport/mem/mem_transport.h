
#ifndef MEM_TRANSPORT_H
#define MEM_TRANSPORT_H

#include "../transport.h"
#include <stdint.h>

/**
 * @brief Initializes the tr_socket struct for a SSH connection.
 * @param[in] config SSH configuration for the connection.
 * @param[out] socket Initialized transport socket.
 * @returns TR_SUCCESS On success.
 * @returns TR_ERROR On error.
 */
int tr_mem_init(struct tr_socket *socket);

bool tr_mem_data_consumed();

void tr_mem_set_data(uint8_t* data, size_t len);

#endif
/* @} */
