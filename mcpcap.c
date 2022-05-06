#include <stdio.h>
#include <stdlib.h>     
#include <string.h>             /* memcpy */
#include <assert.h>

#include <sys/socket.h>
#include <linux/if_packet.h>    /* sockaddr_ll */
#include <linux/if_ether.h>     /* ethhdr, eth protos */
#include <arpa/inet.h>          /* htons */
#include <net/ethernet.h>       /* L2 protocols, ETH_P_* */

#include "mcpcap.h"


/*
*   Create socket file descriptor with:
*       -family: packet socket
*       -type: raw (not packet!)
*       -protocol: ETH_P_ALL (every packet!)
*/
void create_pack_socket(sk_handle *skh) {
    skh->sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (skh->sockfd < 0) {
        fprintf(stderr, "Unable to create packet socket.\n");
    }
}

/*
*   Bind packet socket to interface by if_index.
*/
int bind_pack_socket(sk_handle *skh, int if_index) {
    //Link layer device independent address structure.
    struct sockaddr_ll addr = {
        .sll_family = AF_PACKET,
        .sll_ifindex = if_index,
        .sll_protocol = htons(ETH_P_ALL),
    };

    if (bind(skh->sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Unable to bind socket to interface.\n");
        return -1;
    }

    return 0;
}

void allocate_packet_buffer(packet_buffer *pb) {
    pb->buffer = (void *)malloc(ETH_FRAME_LEN);
}

/*
*   Fill buffer with binary data from socket.
*/
int recvpacket(sk_handle *skh, packet_buffer *pb) {
    return recvfrom(skh->sockfd, pb->buffer, ETH_FRAME_LEN, 0, NULL, NULL);
}