#ifndef MCPCAP_H
#define MCPCAP_H

typedef struct {
    int sockfd;
} sk_handle;

typedef struct {
    void *buffer;
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
    unsigned char two_two[8];   //32 octets enough?
};

void create_pack_socket(sk_handle *skh);

int bind_pack_socket(sk_handle *skh, int if_index);

void allocate_packet_buffer(packet_buffer *pb);

int recvpacket(sk_handle *skh, packet_buffer *pb);

#endif