#include "nl_utilities.h"
#include "mcpcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <linux/if_ether.h>     /* ethhdr */


/*
*   $ gcc nl_utilities.c mcpcap.c sample.c -o sample $(pkg-config --cflags --libs libnl-genl-3.0)
*
*   Example test:
*   $ sudo ./sample wlan1 1000
*/
void print_if_info(struct if_info *info) {
    printf("%s:\n", info->if_name);
    printf("\tIFINDEX: %d\n", info->if_index);
    printf("\tWDEV: %d\n", info->wdev);
    printf("\tWIPHY: %d\n", info->wiphy);
    printf("\tIFTYPE: %d\n", info->if_type);
    printf("\n");
}

int start_monitor(nl_handle *nl, struct if_info *info, const char *if_type, int wiphy, const char *if_name) {
    if (create_new_if(nl, if_type, wiphy, if_name) < 0 ) {
        fprintf(stderr, "Could not create %s interface.\n", if_name);
        return -1;
    }
    if (set_if_up(if_name) < 0) {
        fprintf(stderr, "Could not set %s up.\n", if_name);
        return -1;
    }
    int if_index = get_if_index(if_name);
    if (get_if_info(nl, info, if_index) < 0) {
        fprintf(stderr, "Could not retrieve %s information.\n", if_name);
        return -1;
    }
    return 0;
}



int main(int argc, char** argv) {

    nl_handle nl;
    sk_handle skh;
    packet_buffer pb;
    struct if_info info;
    struct if_info v_info;
    info.if_name = argv[1];
    int ITER = strtol(argv[2], NULL , 10);
    int if_index = get_if_index(info.if_name);

    nl_init(&nl);
    if (!nl.sk) {
        return -1;
    }

    if (get_if_info(&nl, &info, if_index) < 0) {
        fprintf(stderr, "Could not retrieve interface information.\n");
        return -1;
    }
    print_if_info(&info);

    const char *new_iftype = "monitor";
    const char *new_ifname = "mcmesh0";
    int bind_if_index;
    if (compare_if_type(info.if_type, new_iftype) == 0) {
        printf("Device not currently in monitor mode.\n");
         if (start_monitor(&nl, &v_info, new_iftype, info.wiphy, new_ifname) < 0) {
             fprintf(stderr, "Could not create new monitor mode interface.\n");
         }
        printf("Created new monitor mode interface.\n");
        bind_if_index = get_if_index(new_ifname);
        printf("New if index: %d\n", bind_if_index);
    } else {
        bind_if_index = info.if_index;
        printf("Device is already in monitor mode.\n");
    }
    printf("\n");

    struct if_info vinfo;
    if (get_if_info(&nl, &vinfo, bind_if_index) < 0) {
        fprintf(stderr, "Newly created virtual interface is not fuctioning.");
        return 0;
    }
    printf("New virtual interface info:\n");
    printf("\tIFNAME: %s\n", vinfo.if_name);
    printf("\tIFINDEX: %d\n", vinfo.if_index);
    printf("\tWDEV: %d\n", vinfo.wdev);
    printf("\tWIPHY: %d\n", vinfo.wiphy);
    printf("\tIFTYPE: %d\n", vinfo.if_type);
    printf("\n");   

    create_pack_socket(&skh);
    if (skh.sockfd < 0) {
        return -1;
    }
    
    if (bind_pack_socket(&skh, bind_if_index) < 0) {
        return -1;
    }

    allocate_packet_buffer(&pb);

    //Just for testing, remove asap.
    for (int n=0; n<ITER; n++) {
        int n = recvpacket(&skh, &pb);
        printf("Received %d bytes.\n", n);
        struct ethhdr *eth = (struct ethhdr *)(pb.buffer);
        printf("\nReceived Ethernet Header:\n");
        printf("Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
        printf("Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
        printf("Protocol : %d\n",eth->h_proto);
        printf("\n");
    }
    
    printf("Removing created virtual interface...\n");
    const char *ret_iftype = "managed";
    //set_if_type(&nl, ret_iftype, if_index, if_name);
    delete_if(&nl, bind_if_index);
    //create_new_if(&nl, info.if_type, info.wiphy, info.if_name);
    nl_cleanup(&nl);

    return 0;
}
