#include "nl_utilities.h"
#include "mcpcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/if_ether.h>     /* ethhdr */


/*
*   $ gcc nl_utilities.c mcpcap.c sample.c -o sample $(pkg-config --cflags --libs libnl-genl-3.0)
*
*   Example test:
*   $ sudo ./sample wlan1 1000
*/


void print_if_info(struct if_info *info) {
    printf("Interface: %s\n", info->if_name);
    printf("\tIFINDEX: %d\n", info->if_index);
    printf("\tWDEV: %d\n", info->wdev);
    printf("\tWIPHY: %d\n", info->wiphy);
    printf("\tIFTYPE: %d\n", info->if_type);
    printf("\n");
}

void print_phy_info(struct phy_info *info) {
    //printf("%s:\n", info->phy_name);
    //printf("\tPHY ID: %d\n", info->phy_id);
    printf("\tCan do hardware mntr mode: %d\n", info->hard_mon);
    printf("\tCan do software mntr mode: %d\n", info->soft_mon);
    printf("\n");
}

int find_mntr_phy(nl_handle *nl, struct phy_info *info, int phy_id) {
    printf("Getting phy info dump...\n");
    return get_phy_info(nl, info, phy_id, 1);
}

int start_mntr_if(nl_handle *nl, struct if_info *info, const char *if_type, int phy_id, const char *if_name) {
    printf("Creating new virtual interface on phy%d\n", phy_id);
    if (create_new_if(nl, if_type, phy_id, if_name) < 0 ) {
        fprintf(stderr, "Could not create %s interface.\n", if_name);
        return -1;
    }
    printf("Setting new virtual interface up...\n");
    if (set_if_up(if_name) < 0) {
        fprintf(stderr, "Could not set %s up.\n", if_name);
        return -1;
    }
    printf("Verifying new virtual interface configuration...\n");
    int if_index = get_if_index(if_name);
    if (get_if_info(nl, info, if_index, -1) < 0) {
        fprintf(stderr, "Could not retrieve %s information.\n", if_name);
        return -1;
    }
    return 0;
}

int set_up_mntr_if(nl_handle *nl, struct if_info *v_info) {
    struct phy_info p_info;

    //Iterating through possibly the first 4 wiphy's is not a long term solution.
    for (int phyid = 0; phyid < 4; phyid++) {
        if (find_mntr_phy(nl, &p_info, phyid) < 0) {
            fprintf(stderr, "wiphy%d to support monitor mode.\n", phyid);
        }
        p_info.phy_id = phyid;
        break;
    }
    printf("Turning off interfaces on phy%d...\n", p_info.phy_id);
    struct if_info tmp;
    get_if_info(nl, &tmp, -1, p_info.phy_id);

    const char *mntr_iftype = "monitor";
    const char *mntr_ifname = "mcmon";
    if (start_mntr_if(nl, v_info, mntr_iftype, p_info.phy_id, mntr_ifname) < 0) {
        fprintf(stderr, "Could not start new monitor interface on phy%d.\n", p_info.phy_id);
        return -1;
    }
}

int main(int argc, char** argv) {

    nl_handle nl;
    sk_handle skh;
    packet_buffer pb;
    struct if_info v_info;
    //info.if_name = argv[1];
    int ITER = 5;
    if (argc > 1) {
        ITER = strtol(argv[1], NULL , 10);
    }
    //int if_index = get_if_index(b_info.if_name);

    printf("Initializing netlink socket...\n");
    nl_init(&nl);
    if (!nl.sk) {
        return -1;
    }

    printf("Starting set up of a new virtual monitor mode interface...\n");
    printf("\n");
    if (set_up_mntr_if(&nl, &v_info) < 0) {
        fprintf(stderr, "Aborting...\n");
        return -1;
    }
    print_if_info(&v_info);

    printf("Creating new packet socket...\n");
    create_pack_socket(&skh);
    if (skh.sockfd < 0) {
        return -1;
    }

    printf("Binding packet socket to new monitor mode virtual interface...\n");
    if (bind_pack_socket(&skh, v_info.if_index) < 0) {
        return -1;
    }

    printf("Waiting to receive packets...\n");
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
    delete_if(&nl, v_info.if_index);
    //create_new_if(&nl, info.if_type, info.wiphy, info.if_name);
    nl_cleanup(&nl);

    return 0;
}
