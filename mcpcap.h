#ifndef PCAP_H
#define PCAP_H

int create_pack_socket();

int bind_pack_socket(int sockfd, int if_index);

int recvpackets(int sockfd, void* buffer);