#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "rtrlib/rtrlib.h"
#include "rtrlib/transport/mem/mem_transport.h"

static volatile bool stop;
static void status_fp(const struct rtr_mgr_group *group __attribute__((unused)),
              enum rtr_mgr_status mgr_status,
              const struct rtr_socket *rtr_sock,
              void *data __attribute__((unused)))
{
    if (mgr_status == RTR_MGR_CLOSED)
        stop = true;
}


int main(int argc, char **argv)
{

    spki_update_fp spki_update_fp = NULL;
    pfx_update_fp pfx_update_fp = NULL;


    struct tr_socket sock;
    struct rtr_socket rtr;
    struct rtr_mgr_config *conf;
    struct rtr_mgr_group groups[1];

    tr_mem_init(&sock);

    rtr.tr_socket = &sock;
    groups[0].sockets_len = 1;
    groups[0].sockets = malloc(1 * sizeof(rtr));
    groups[0].sockets[0] = &rtr;
    groups[0].preference = 1;


    uint8_t buffer[1024*32];
    size_t len;

    stop = false;

    FILE* fp;
    fp = fopen(argv[1], "r");
    if (!fp) {
        free(groups[0].sockets);
        return -1;
    }


    len = fread(buffer, 1, sizeof(buffer), fp);
    //len = read(STDIN_FILENO, buffer, sizeof(buffer));
    tr_mem_set_data(buffer, len);

    rtr_mgr_init(&conf, groups, 1, 30, 600, 600, pfx_update_fp,
                    spki_update_fp, status_fp, NULL);
    rtr_mgr_start(conf);

    while (!stop)
        usleep(1000);

    rtr_mgr_stop(conf);
    rtr_mgr_free(conf);


    free(groups[0].sockets);

    return EXIT_SUCCESS;
}

