#include <stdio.h>
#include <stdlib.h>     
#include <string.h>             /* memcpy */
#include <assert.h>

#include <sys/socket.h>
#include <linux/if_packet.h>    /* sockaddr_ll */
#include <linux/if_ether.h>     /* ethhdr, eth protos */
#include <net/if.h>
#include <arpa/inet.h>          /* htons */
#include <net/ethernet.h>       /* L2 protocols, ETH_P_* */

#include "handle80211.h"

/*
 *  802.11 mgmt frame.
*/
typedef struct {                        /* Typically:   */
    frame_ctrl frame_control;           /* (ver/type/subtype) + (flags)*/
    unsigned char frame_duration[2];    /* (TO DS = 0/FROM DS = 0/....) + (....) */
    unsigned char address1[6];          /* Source mac addr */
    unsigned char address2[6];          /* Dest mac addr */ 
    unsigned char address3[6];          /* Tx mac addr */
    unsigned char seq_ctrl[2];          /* frame sequence */
} mgmt_frame;


/*
 *  Wikipedia 802.11 frame subtypes. Only choosing to care about a few of these.
 *  Frame control field is 2 bytes. First byte is ordered:
 *  |B0 B1| |B2 B3| |B4 B5 B6 B7|
 *  |.ver.| |.type| |..subtype..|
 * 
 *  So, for mgmt frames, type & version is always zero...
 *  Second byte has TO DS and FROM DS as B0, B1. We will only care about these bits.
 */

/*
 * Yes, redundant use of enum-ing. Bear with me.
 */
enum mgmt_frame_type {
    ASSOCIATION_REQEUST = 0x0,
    ASSOCIATION_RESPONSE = 0X1,
    REASSOCIATION_REQUEST = 0x2,
    REASSOCIATION_RESPONSE = 0X3,
    PROBE_REQUEST = 0x4,
    PROBE_RESPONSE = 0X5,
    TIMING_ADVERTISEMENT = 0x6,
    RESERVED = 0X7,
    BEACON = 0X8,
    DISASSOCIATION = 0XA,
    AUTHENTICATION = 0XB,
    DEAUTHENTICATION = 0XC,
    ACTION = 0XE,
};

const char * mgmt_subtype_strings[] = {
    "ASSOCIATION_REQUEST",
    "REASSOCIATION_REQUEST",
    "PROBE_REQUEST",
    "TIMING_ADVERTISEMENT",
    "BEACON",
    "DISASSOCIATION",
    "DEAUTHENTICATION",
    "AUTHENTICATION",
    "ACTION",
    "ASSOCATION_RESPONSE",
    "REASSOCIATION_RESPONSE",
    "PROBE_RESPONSE",
    "RESERVED"
};

/*
 *  Simple utility for printing bytes in hex format.
 */
void print_bytes_hex(void *data, size_t len) {
    const u_char *bytes = data;
    size_t count;
    for (count=0; count < len; count++) {
        if (count == len-1) {
        printf("%.2X", bytes[count]);
        } else {
            printf("%.2X:", bytes[count]);
        }
    }
}

/*
 *  Find the frame type.
 */
void get_frame_type(const u_char *byte, enum frame_type *type) {
    if (*byte & 0x8) {
        *type = DATA;
        return;
    } 
    if (*byte & 0x4) {
        *type = CTRL;
        return;
    }
    //assert(!(*byte & 0x8) && !(*byte & 0x4));
    *type = MGMT;
}


void print_mgmt_subtype(const u_char byte) {
    printf("\nMGMT FRAME SUBTYPE: %s", mgmt_subtype_strings[(int)(byte)]);
}
/*
 *  Could write data to somewhere interesting...but instead
 *  this function prints packet data as appropriate.
 *  Should have TO DS = 0, FROM DS = 0 (STA to STA)
 */
void handle_mgmt_frame(const u_char *pkt) {
    mgmt_frame *frame = (mgmt_frame *)(pkt);
    //assert(!(frame->frame_control.fc[1] & 0x80) && !(frame->frame_control.fc[1] & 0x40));   /* Sanity check for TO/FROM DS. */
    print_mgmt_subtype(frame->frame_control.fc[0]);
    printf("\nDEST MAC:\t");
    print_bytes_hex(&(frame->address1), 6);
    printf("\nSRC MAC:\t");
    print_bytes_hex(&(frame->address2), 6);
    printf("\nBSSID MAC:\t");
    print_bytes_hex(&(frame->address3), 6);
    printf("\n");
}

/*
 *  Not yet implemented.
 */
void handle_ctrl_frame(const u_char *pkt) {
    printf("CTRL FRAME\n");
}
void handle_data_frame(const u_char *pkt) {
    printf("DATA FRAME\n");
}

/*
 *  Pass frame off to appropriate type handler.
 */
void handle_frame(const u_char *pkt, enum frame_type *type) {
    switch (*type) {
        case MGMT:
            handle_mgmt_frame(pkt);
            break;
        case CTRL:
            handle_ctrl_frame(pkt);
            break;
        case DATA:
            handle_data_frame(pkt);
            break;
    }
}

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
    
    enum frame_type type;
    frame_ctrl *fctrl = (frame_ctrl *)skh->buffer + offset;
    get_frame_type(&(fctrl->fc[0]), &type);
    handle_frame(skh->buffer, &type);
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