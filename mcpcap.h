#ifndef MCPCAP_H
#define MCPCAP_H

typedef struct {
    int sockfd;
    char *buffer;
} sk_handle;

/*
 *  802.11ac MAC Frame.
             subtype-type-version 
 *  FC mgmt: XXXX   - 00 - 00       (i.e. probe request, beacon, auth/deauth, etc)
 *  FC ctrl: XXXX   - 01 - 00
 *  FC data: XXXX   - 10 - 00
*/
typedef struct {
    unsigned char frame_control[2];     /* mgmt, ctrl, data */
    unsigned char frame_duration[2];
    unsigned char source[6];            /* Source mac addr */
    unsigned char destination[6];       /* Destination mac addr */ 
    unsigned char bssid[6];         /* Filtering (BSS?) ID */
    unsigned char seq_ctrl[2];          /* frame sequence */
} mgmt_frame;

// /*
//  *  Dumb cast. Literally. Parsing frames had some ups & downs.
// */
// struct dumb_cast {
//     unsigned char one_one[8];
//     unsigned char one_two[8];
//     unsigned char two_one[8];
//     unsigned char two_two[8];
//     unsigned char three_one[8];
//     unsigned char three_two[8];
// };

void create_pack_socket(sk_handle *skh);

void cleanup_mcpap(sk_handle *skh);

int bind_pack_socket(sk_handle *skh, int if_index);

int set_if_promisc(sk_handle *skh, int if_index);

// void allocate_packet_buffer(packet_buffer *pb);

void handle_buffer(sk_handle *skh);

int read_socket(sk_handle *skh);

#endif