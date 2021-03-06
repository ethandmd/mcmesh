#ifndef HANDLE80211_H
#define HANDLE80211_H

#include <stdint.h>

typedef struct {
    int sockfd;
    u_char *buffer;
} sk_handle;

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