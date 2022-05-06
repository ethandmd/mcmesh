#ifndef MCPCAP_H
#define MCPCAP_H

typedef struct {
    int sockfd;
} sk_handle;

typedef struct {
    void *buffer;
} packet_buffer;

void create_pack_socket(sk_handle *skh);

int bind_pack_socket(sk_handle *skh, int if_index);

void allocate_packet_buffer(packet_buffer *pb);

int recvpacket(sk_handle *skh, packet_buffer *pb);

#endif