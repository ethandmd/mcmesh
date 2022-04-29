#include <stdio.h>
#include <stdlib.h>     
#include <string.h>     /* memcpy */

#include <sys/socket.h>
#include <sys/ioctl.h>      /* SIOCGIFINDEX, ioctl */
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>      /* htons */
#include <net/if.h>         /* struct ifreq */
#include <net/ethernet.h>    /* L2 protocols, ETH_P_* */

/* 
*   Compile with:
*       $ gcc raw_sock_trial.c -o raw_sock_trial
*
*   Run:
*       $ sudo raw_sock_trial IF_NAME   #Works with RTL8812au in monitor mode via:
*                                       # $ sudo ip link set IFNAME down
*                                       # $ sudo iw dev IFNAME set type monitor
*                                       # $ sudo ip link set IFNAME up
*/


int main(int argc, char *argv[]) {

    char *if_name;
    int sockfd;
    struct ifreq ifr = {0};
    //const unsigned char ether_broadcast_addr[]= {0xff,0xff,0xff,0xff,0xff,0xff};

    if (argc < 1) {
        printf("Error, require one argumente: dev.");
        return -1;
    } else {
        if_name = argv[1];
        printf("Selected: %s\n", if_name);
    }

    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("Error socket().\n");
        return -1;
    }
    
    if (sizeof(if_name) < IFNAMSIZ) {
        memcpy(ifr.ifr_name, if_name, IFNAMSIZ);
    } else {
        printf("Error, net if name too long.\n");
        return -1;
    }

    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        printf("Error, unable to get net if index.\n");
        return -1;
    }

    struct sockaddr_ll addr = {0};          //Zero out all fields for link layer sock addr
    addr.sll_family = AF_PACKET;            //Set family to packet socket
    addr.sll_ifindex = ifr.ifr_ifindex;     //Set interface to ifindex
    addr.sll_protocol = htons(ETH_P_ALL);   //Network byte order

    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("Error, unable to bind socket to %s\n", if_name);
        return -1;
    }
    
    if (if_name == "wlan0") {
        ifr.ifr_flags = IFF_PROMISC;
        if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
            perror("Unable to set if to promisc.\n");
            return -1;
        }
    }

    /*struct packet_mreq mr;
    memcpy(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifr.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;

    if (setsockopt(sockfd, SOL_SOCKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
        printf("Error setting if to promisc.\n");
        return -1;
    }*/
    
    void* buffer = (void *)malloc(ETH_FRAME_LEN);

    while (1) {
        int receivedBytes = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
        printf("%d bytes received\n", receivedBytes);
        int i;
        for (i = 0; i < receivedBytes; i++)
        {
            printf("%X ", ((unsigned char*)buffer)[i]);
        }
        printf("\n");
    }
    return 0;
}