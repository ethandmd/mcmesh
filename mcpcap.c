#include <stdio.h>
#include <stdlib.h>     
#include <string.h>             /* memcpy */
#include <assert.h>

#include <sys/socket.h>
#include <linux/if_packet.h>    /* sockaddr_ll */
#include <linux/if_ether.h>     /* ethhdr, eth protos */
#include <arpa/inet.h>          /* htons */
#include <net/ethernet.h>       /* L2 protocols, ETH_P_* */


/*
*   Create socket file descriptor with:
*       -family: packet socket
*       -type: raw (not packet!)
*       -protocol: ETH_P_ALL (every packet!)
*/
int create_pack_socket() {
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))
    if (sockfd < 0) {
        fprintf(stderr, "Unable to create packet socket.\n");
        return -1;
    }
    return sockfd;
}

/*
*   Bind packet socket to interface by if_index.
*/
int bind_pack_socket(int sockfd, int if_index) {
    //Link layer socket data fields.
    struct sockaddr_ll addr = {
        .sll_family = AF_PACKET;
        .sll_ifindex = if_index;
        .sll_protocol = htons(ETH_P_ALL);
    };

    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Unable to bind socket to interface.\n");
        return -1
    }
    
    return 0;
}