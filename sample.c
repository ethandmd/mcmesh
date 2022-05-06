#include "nl_utilities.h"
#include "mcpcap.h"
#include <stdio.h>
#include <stdlib.h>

/*
*   $ gcc nl_utilities.c mcpcap.c sample.c -o sample $(pkg-config --cflags --libs libnl-genl-3.0)
*
*   Example test:
*   $ sudo ./sample wlan1 1000
*/

int main(int argc, char** argv) {
    char *if_name = argv[1];
    int ITER = strtol(argv[2], NULL , 10);
    int if_index = get_ifindex(if_name);
    nl_handle nl;
    sk_handle skh;
    packet_buffer pb;

    nl_init(&nl);
    if (!nl.sk) {
        return -1;
    }

    if (check_if_monitor(&nl, if_index) != 1) {
        printf("Setting device to monitor mode.\n");
        set_iftype_monitor(&nl, if_index);
    } else {
        printf("Device already in monitor mode.\n");
    }

    create_pack_socket(&skh);
    if (skh.sockfd < 0) {
        return -1;
    }
    
    if (bind_pack_socket(&skh, if_index) < 0) {
        return -1;
    }


    allocate_packet_buffer(&pb);

    //Just for testing, remove asap.
    for (int n=0; n<ITER; n++) {
        int n = recvpacket(&skh, &pb);
        printf("Received %d bytes.\n", n);
    }
    
    printf("Restoring device to managed mode.\n");
    set_iftype_managed(&nl, if_index);
    nl_cleanup(&nl);

    return 0;
}