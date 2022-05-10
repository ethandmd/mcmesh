#ifndef MCPCAP_H
#define MCPCAP_H

typedef struct {
    int sockfd;
    char *buffer;
} sk_handle;

/*
 *  Frame Control Field for figuring out what is happening.
 */
typedef struct {
    unsigned char fc[2];      /* First two bytes of the MAC header */
} frame_ctrl;

/*
 *  Three _types_ of 802.11 frames to deal with.
 */
enum frame_type {
    MGMT,
    CTRL,
    DATA
};

void get_frame_type(const u_char *byte, enum frame_type *type);

void handle_mgmt_frame(const u_char *pkt);

void handle_frame(const u_char *pkt, enum frame_type *type);

void create_pack_socket(sk_handle *skh);

void cleanup_mcpap(sk_handle *skh);

int bind_pack_socket(sk_handle *skh, int if_index);

int set_if_promisc(sk_handle *skh, int if_index);

// void allocate_packet_buffer(packet_buffer *pb);

void handle_buffer(sk_handle *skh);

int read_socket(sk_handle *skh);

#endif