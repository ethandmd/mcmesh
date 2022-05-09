#include <stdio.h>
#include <stdlib.h>     
#include <string.h>             /* memcpy */
#include <assert.h>
#include <stdint.h>

#include <sys/socket.h>
#include <linux/if_packet.h>    /* sockaddr_ll */
#include <linux/if_ether.h>     /* ethhdr, eth protos */
#include <net/if.h>
#include <arpa/inet.h>          /* htons */
#include <net/ethernet.h>       /* L2 protocols, ETH_P_* */

#include "mcpcap.h"


/*
 *  From radiotap.org
 *  De facto (linux?) standard for 80211 rx/tx.
 *  Mostly just want the offset, don't care as much about fine details at the moment.
 */
struct radiotap_header {
    uint8_t it_rev; //Radiotap version, set to 0?
    uint8_t it_pad; //Alignment padding -- word boundaries
    uint8_t it_len; //Get entire radiotap header
    uint8_t it_present; //Fields present
};

/*
*   Create socket file descriptor with:
*       -(Protocol family _ packet) versus AF_PACKET (linux specific)
*       -type: raw (vs dgram).
*       -protocol: 0 (receive no packets until you bind with non-zero proto) vs ETH_P_ALL (no filtering)
*/
void create_pack_socket(sk_handle *skh) {
    skh->sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (skh->sockfd < 0) {
        fprintf(stderr, "Unable to create packet socket.\n");
    }
}

void cleanup_mcpap(sk_handle *skh) {//, packet_buffer *pktbuff) {
    shutdown(skh->sockfd, SHUT_RDWR);
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
    printf("Bound socket to ifindex: %d.\n", if_index);
    return 0;
}

int set_if_promisc(sk_handle *skh, int if_index) {
    struct packet_mreq mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.mr_ifindex = if_index;
    mreq.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(skh->sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        fprintf(stderr, "Failed to put socket into promiscuous mode.\n");
        return -1;
    }
    printf("Set ifindex: %d to promiscuous mode.\n", if_index);
    return 0;
}


// void allocate_packet_buffer(packet_buffer *pb) {
//     pb->buffer = (void *)malloc(ETH_FRAME_LEN);
// }

/*
*   Assumes we are just grabbing mgmt frames.
*/
void handle_buffer(sk_handle *skh) {
    struct radiotap_header *rthdr = (struct radiotap_header *)skh->buffer;
    int offset = rthdr->it_len;
    mgmt_frame *frame = (mgmt_frame *)skh->buffer+offset;
    if (frame->frame_control != 0) {
        printf("Uh oh, not a mgmt frame. :/ Let's right every kind of 80211 parser :/...\n");
    } else {
        printf("%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",frame->source[0],frame->source[1],frame->source[2],frame->source[3],frame->source[4],frame->source[5]);
        printf("%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",frame->destination[0],frame->destination[1],frame->destination[2],frame->destination[3],frame->destination[4],frame->destination[5]);
        printf("%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",frame->bssid[0],frame->bssid[1],frame->bssid[2],frame->bssid[3],frame->bssid[4],frame->bssid[5]);
    }
}


/*
 *  Gone through some back and forth on a good way to read packets.
 *  Between memory mapped ring buffer, recvmsg w/ msghdr and cmsghdr, 
 *  or just a recv/recvfrom.
 *  msghdr: http://stackoverflow.com/questions/32593697/ddg#32594071
 *  
 */
int read_socket(sk_handle *skh) {
    //Requires buffer to be malloc'd
    skh->buffer = malloc(1514); //eth MTU size of 1500...
    struct sockaddr src;
    socklen_t src_len = sizeof(src);
    //Read "n" bytes into "buffer" from socket.
    int n = recvfrom(skh->sockfd, skh->buffer, sizeof(skh->buffer), 0, (struct sockaddr *)&src, &src_len);
    if (n < 0) {
        fprintf(stderr, "Could not read msg from socket.\n");
        return -1;
    }
    printf("Received %d bytes.\n", n);

    return 0;
}