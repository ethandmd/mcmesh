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
        fprintf(stderr, "Unable to bind socket to if index %d.\n", if_index);
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
    int n = recvfrom(skh->sockfd, pb->buffer, ETH_FRAME_LEN, 0, NULL, NULL);
    printf("Received %d bytes.\n", n);
    struct dumb_cast *pkt = (struct dumb_cast *)(pb->buffer);
    printf("%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\t",pkt->one_one[0],pkt->one_one[1],pkt->one_one[2],pkt->one_one[3],pkt->one_one[4],pkt->one_one[5],pkt->one_one[6],pkt->one_one[7]);
    printf("%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",pkt->one_two[0],pkt->one_two[1],pkt->one_two[2],pkt->one_two[3],pkt->one_two[4],pkt->one_two[5],pkt->one_two[6],pkt->one_two[7]);
    printf("%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\t",pkt->two_one[0],pkt->two_one[1],pkt->two_one[2],pkt->two_one[3],pkt->two_one[4],pkt->two_one[5],pkt->two_one[6],pkt->one_two[7]);
    printf("%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",pkt->two_two[0],pkt->two_two[1],pkt->two_two[2],pkt->two_two[3],pkt->two_two[4],pkt->two_two[5],pkt->two_two[6],pkt->one_two[7]);
    printf("\n");
}