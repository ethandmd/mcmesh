#include <stdio.h>
#include <stdlib.h>     
#include <string.h>     /* memcpy */
#include <assert.h>

#include <sys/socket.h>
#include <sys/ioctl.h>          /* ioctl */
#include <linux/if_packet.h>    /* sockaddr_ll */
#include <linux/if_ether.h>     /* ethhdr */
#include <arpa/inet.h>          /* htons */
#include <net/if.h>             /* struct ifreq */
#include <net/ethernet.h>       /* L2 protocols, ETH_P_* */

/* 
*   Compile with:
*       $ gcc raw_sock_trial.c -o raw_sock_trial
*
*   Run:
*       $ sudo raw_sock_trial IF_NAME   #Works with RTL8812au in monitor mode via:
*                                       # $ sudo ip link set IFNAME down
*                                       # $ sudo iw dev IFNAME set type monitor
*                                       # $ sudo ip link set IFNAME up
*
*   Read layer 2 (link layer) frames:
*       
*   [x] Initialize and create socket
*   [x] Bind socket to interface (in this case, a monitor mode capable wifi interface)
*   [~] Set interface to monitor mode (Currently using ioctl call, instead of setsockopt() or just rewrite with netlink.)
*   [x] Read bytes from socket into buffer (memset buffer to appropriate size per frame type)
*   [ ] Type cast link layer header, discover IP layer protocol.    
*   [ ] Access payload data ( data := begin packet + header size)
*/


int main(int argc, char *argv[]) {

    char *if_name;
    int sockfd;

    printf("Configuring scan...\n");

    /*Capture CLI arg for net if dev.*/
    if (argc < 1) {
        printf("Error, require one argumente: dev.");
        return -1;
    } else {
        if_name = argv[1];
        printf("Selected: %s\n", if_name);
    }

    /*Step 1: Create raw socket.*/
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("Error socket().\n");
        return -1;
    }
    
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    /*Step 2a: Load interface name to struct ifreq.*/
    if (sizeof(if_name) < IFNAMSIZ) {
        memcpy(ifr.ifr_name, if_name, IFNAMSIZ);
    } else {
        printf("Error, net if name too long.\n");
        return -1;
    }

    /* Check interface MAC address. */
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        printf("Unable to get hw addr.\n");
    } else {
        printf("This interface Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",ifr.ifr_hwaddr.sa_data[0], \
        ifr.ifr_hwaddr.sa_data[1], \
        ifr.ifr_hwaddr.sa_data[2], \
        ifr.ifr_hwaddr.sa_data[3], \
        ifr.ifr_hwaddr.sa_data[4], \
        ifr.ifr_hwaddr.sa_data[5]);
    }

    /*Step 2b: Get net if dev index in order to bind socket.*/
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        printf("Error, unable to get net if index.\n");
        return -1;
    }



    /*Step 2c: Set appropriate link layer struct info for socket bind() call.*/
    struct sockaddr_ll addr;                //Zero out all fields for link layer sock addr.
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;            //Set family to packet socket
    addr.sll_ifindex = ifr.ifr_ifindex;     //Set interface to ifindex
    addr.sll_protocol = htons(ETH_P_ALL);   //Network byte order

    /*Step 2d: Bind socket to specified device*/
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("Error, unable to bind socket to %s\n", if_name);
        return -1;
    }

    /*struct ifreq pifr;
    memset(&pifr, 0, sizeof(pifr));
    memcpy(pifr.ifr_name, if_name, IFNAMSIZ);
    */
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        printf("Error getting device flags.\n");
        return -1;
    }

    /* Promiscuous mode is not what we're going for. */
    /*if (ifr.ifr_flags & IFF_PROMISC) {
        printf("Device is in promiscuous mode.\n");
    } else {
        printf("Putting device in promiscuous mode...\n");
        ifr.ifr_flags |= IFF_PROMISC;
        
        if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
            printf("Could not put device into promiscuous mode.\n");
            return -1;
        }
        printf("Successfully put device into promiscuous mode.\n");        
    }
    printf("Setup complete.\n\n");

    /*struct packet_mreq mr = {0};    //Legal?
    //memcpy(&mr, 0, sizeof(mr));
    mr.mr_ifindex = if_nametoindex(if_name);
    assert(mr.mr_ifindex == ifr.ifr_ifindex);   //Sanity check.
    mr.mr_type = PACKET_MR_PROMISC;

    if (setsockopt(sockfd, SOL_SOCKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
        printf("Error setting if to promisc.\n");
        return -1;
    }*/
    
    void* buffer = (void *)malloc(ETH_FRAME_LEN);

    while (1) {
        int receivedBytes = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
        printf("%d bytes received\n", receivedBytes);
        struct ethhdr *eth = (struct ethhdr *)(buffer);
        printf("\nReceived Ethernet Header:\n");
        printf("Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
        printf("Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
        printf("Protocol : %d\n",eth->h_proto);
        printf("\n");
    }
    return 0;
}