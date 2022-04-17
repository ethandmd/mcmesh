#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

int main() {
    int s;
    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (s == -1) { return -1; }

    void* buffer = (void *)malloc(ETH_FRAME_LEN);
    int receivedBytes = recvfrom(s, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
    printf("%d bytes received\n", receivedBytes);
    int i;
    for (i = 0; i < receivedBytes; i++)
    {
        printf("%X ", ((unsigned char*)buffer)[i]);
    }
    printf("\n");
    return 0;
}