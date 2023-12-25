/*
 * This file is part of RTRlib.
 *
 * This file is subject to the terms and conditions of the MIT license.
 * See the file LICENSE in the top level directory for more details.
 *
 * Website: http://rtrlib.realmv6.org/
 */
#ifndef RTR_FUZZ_TRANSPORT_H
#define RTR_FUZZ_TRANSPORT_H

#include <stdint.h>
#include "rtrlib/transport/transport.h"


struct fuzz_transport_config {
	uint8_t *input;
	size_t input_size;
	size_t bytes_left;
	char *ident;
	size_t blocked;
};

/**
 * @brief Initializes the tr_socket struct for a TCP connection.
 * @param[in] config TCP configuration for the connection.
 * @param[out] socket Initialized transport socket.
 * @returns TR_SUCCESS On success.
 * @returns TR_ERROR On error.
 */
int tr_fuzz_init(struct fuzz_transport_config *config, struct tr_socket *socket);
#endif
/** @} */
