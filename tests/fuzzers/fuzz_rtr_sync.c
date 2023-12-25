#include "rtrlib/rtr/packets_private.h"
#include "rtrlib/transport/fuzz/fuzz_transport.h"
#include "rtrlib/rtr/rtr.h"
#include "rtrlib/rtrlib.h"
#include <bits/stdint-uintn.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	struct rtr_mgr_config *conf;
	struct rtr_mgr_group groups[1];

    struct tr_socket tr_fuzz;
    struct fuzz_transport_config config;

    FILE *f;
    if (argc < 2) {
        return 1;
    }
    f = fopen(argv[1], "r");
    if (!f) {
        return 1;
    }

    uint8_t *buffer = malloc(1024*1024*12);
    size_t bytes_read = fread(buffer, 1, 1024*1024*12, f);
    config.input_size = bytes_read;
    config.bytes_left = bytes_read;
    config.input = buffer;
    config.ident = strdup("abcd");

    tr_fuzz_init(&config, &tr_fuzz);

    struct rtr_socket socket;
    socket.tr_socket = &tr_fuzz;

    struct rtr_socket* sockets[] = {
        &socket
    };

	groups[0].sockets_len = 1;
	groups[0].sockets = sockets;
	groups[0].preference = 1;


	int ret = rtr_mgr_init(&conf, groups, 1, 
        30, 600, 600,
        NULL, NULL, NULL, NULL);
    if(ret != RTR_SUCCESS) {
        return 1;
    }

    rtr_mgr_start(conf);

    while (!rtr_mgr_conf_in_sync(conf)) {
        usleep(1000*2);
    }

    rtr_mgr_stop(conf);
	rtr_mgr_free(conf);
}