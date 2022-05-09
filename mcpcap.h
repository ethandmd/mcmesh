#ifndef MCPCAP_H
#define MCPCAP_H

typedef struct {
    int sockfd;
} sk_handle;

typedef struct {
    char buffer[65537];
    struct msghdr *mh;
} packet_buffer;


// /*
//  * 802.11ac MAC Frame
// */
// typedef struct {
//     unsigned char frame_control[2];
//     unsigned char frame_duration[2];
//     unsigned char source[6];            /* Source mac addr */
//     unsigned char destination[6];       /* Destination mac addr */ 
//     unsigned char address_3[6];         /* Filtering (BSS?) ID */
//     unsigned char seq_ctrl[2];          /* frame sequence */
// } mgmt_frame_hdr;

/*
 *  Dumb cast. Literally.
*/
struct dumb_cast {
    unsigned char one_one[8];
    unsigned char one_two[8];
    unsigned char two_one[8];
    unsigned char two_two[8];
    unsigned char three_one[8];
    unsigned char three_two[8];
};

void create_pack_socket(sk_handle *skh);

void cleanup_mcpap(sk_handle *skh);

int bind_pack_socket(sk_handle *skh, int if_index);

int set_if_promisc(sk_handle *skh, int if_index);

// void allocate_packet_buffer(packet_buffer *pb);

void handle_buffer(packet_buffer *pb, struct msghdr *mh);

int recv_sk_msg(sk_handle *skh, char *buffer, struct msghdr *mh);

#endif