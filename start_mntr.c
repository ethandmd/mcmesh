#include "nl_utilities.h"
#include "mcpcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/if_ether.h>     /* ethhdr */


/*
*   $ gcc nl_utilities.c mcpcap.c start_mntr.c -o start_mntr $(pkg-config --cflags --libs libnl-genl-3.0)
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
    printf("\tIFFREQ: %d\n", info->if_freq);
    printf("\n");
}

void print_phy_info(struct phy_info *info) {
    printf("%s:\n", info->phy_name);
    printf("\tPHY ID: %d\n", info->phy_id);
    printf("\tCan do hardware mntr mode: %d\n", info->hard_mon);
    printf("\tCan do software mntr mode: %d\n", info->soft_mon);
    printf("\n");
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
    return 0;
}

int set_iftype_mntr(nl_handle *nl, struct if_info *v_info) {
    struct phy_info p_info;

    if (get_phy_info(nl, &p_info, -1) < 0) {
        fprintf(stderr, "Phy info dump failed.\n");
    }
    get_if_info(nl, v_info, -1, p_info.phy_id);
    print_if_info(v_info);
    set_if_down(v_info->if_name);
    if (set_if_type(nl, "monitor", v_info->if_index) < 0) {
        fprintf(stderr, "Failed to put %s into monitor mode.\n", v_info->if_name);
        return -1;
    }
    set_if_up(v_info->if_name);
}

int set_up_mntr_if(nl_handle *nl, struct if_info *v_info, struct if_info *keep_if_info) {
    struct phy_info p_info;
    //Arbitrarily attempt to scan 4 wiphys looking for a monitor mode capable wiphy...Hacky.
    for (int phyid = 0; phyid < 4; phyid++) {
        if (get_phy_info(nl, &p_info, phyid) < 0) {
            fprintf(stderr, "Phy%d info dump failed.\n", phyid);
        }
        if (p_info.hard_mon == 1 && p_info.soft_mon == 1) {
            break;
        }
    }

    get_if_info(nl, keep_if_info, -1, p_info.phy_id);
    
    //Heuristic to avoid trying to delete an interface that doesn't exist.
    if (keep_if_info->if_index >= 0 && keep_if_info->if_index < 100) {
        delete_if(nl, keep_if_info->if_index);
    }

    const char *mntr_iftype = "monitor";
    char *mntr_ifname = "mcmon";
    if (start_mntr_if(nl, v_info, mntr_iftype, p_info.phy_id, mntr_ifname) < 0) {
        fprintf(stderr, "Could not start new monitor interface on phy%d.\n", p_info.phy_id);
        return -1;
    }
    get_if_info(nl, v_info, -1, p_info.phy_id);
    return 0;
}

int main(int argc, char** argv) {

    nl_handle nl;
    sk_handle skh;
    packet_buffer pb;
    struct if_info v_info;
    struct if_info keep_if_info;
    //info.if_name = argv[1];
    int ITER = 5;
    if (argc > 1) {
        ITER = strtol(argv[1], NULL , 10);
    }

    printf("Initializing netlink socket...\n");
    nl_init(&nl);
    if (!nl.sk) {
        return -1;
    }

    printf("Starting set up of a new virtual monitor mode interface...\n");
    printf("\n");
    if (set_up_mntr_if(&nl, &v_info, &keep_if_info) < 0) {
        fprintf(stderr, "Aborting...\n");
        return -1;
    }
    print_if_info(&keep_if_info);
    printf("Setting monitor interface channel freq...\n");
    set_if_chan(&nl, v_info.if_index, CHANNEL_11);
    printf("\n");
    //set_iftype_mntr(&nl, &v_info);

    printf("Creating new packet socket...\n");
    create_pack_socket(&skh);
    if (skh.sockfd < 0) {
        return -1;
    }

    printf("Binding packet socket to new monitor mode virtual interface %s...\n", v_info.if_name);
     if (bind_pack_socket(&skh, v_info.if_index) < 0) {
         return -1;
    }

    printf("Waiting to receive packets...\n\n");
    allocate_packet_buffer(&pb);
    //Just for testing, remove asap.
    for (int n=0; n<ITER; n++) {
        recvpacket(&skh, &pb);
    }
    
    printf("Removing created virtual interface...\n");
    //set_if_type(&nl, ret_iftype, if_index, if_name);
    delete_if(&nl, v_info.if_index);
    create_new_if(&nl, "managed", keep_if_info.wiphy, keep_if_info.if_name);
    nl_cleanup(&nl);

    return 0;
}
