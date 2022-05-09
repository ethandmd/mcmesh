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
    printf("\tphy id: %d\n", info->phy_id);
    printf("\tHardware mntr mode: %d\n", info->hard_mon);
    printf("\tSoftware mntr mode: %d\n", info->soft_mon);
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

// int set_iftype_mntr(nl_handle *nl, struct if_info *v_info) {
//     struct phy_info p_info;

//     if (get_phy_info(nl, &p_info, -1) < 0) {
//         fprintf(stderr, "Phy info dump failed.\n");
//     }
//     get_if_info(nl, v_info, -1, p_info.phy_id);
//     print_if_info(v_info);
//     set_if_down(v_info->if_name);
//     if (set_if_type(nl, "monitor", v_info->if_index) < 0) {
//         fprintf(stderr, "Failed to put %s into monitor mode.\n", v_info->if_name);
//         return -1;
//     }
//     set_if_up(v_info->if_name);
// }

int set_up_mntr_if(nl_handle *nl, struct if_info *v_info, struct if_info *keep_if_info) {
    printf("-----------------Calling {get_if_info}------------------ \n");
    get_if_info(nl, keep_if_info, keep_if_info->if_index, -1);
    printf("-----------------Calling {print_if_info}------------------ \n");
    print_if_info(keep_if_info);

    //Heuristic to avoid trying to delete an interface that doesn't exist.
    if (keep_if_info->if_index >= 0 && keep_if_info->if_index < 1000) {
        delete_if(nl, keep_if_info->if_index);
    }

    const char *mntr_iftype = "monitor";
    char *mntr_ifname = "mcmon";
    if (start_mntr_if(nl, v_info, mntr_iftype, keep_if_info->wiphy, mntr_ifname) < 0) {
        fprintf(stderr, "Could not start new monitor interface on phy%d.\n", keep_if_info->wiphy);
        return -1;
    }
    get_if_info(nl, v_info, -v_info->if_index, -1);
    return 0;
}

int main(int argc, char** argv) {

    nl_handle nl;
    sk_handle skh;
    packet_buffer pb;
    struct if_info v_info;
    struct if_info keep_if_info;
    char *if_name;
    int ITER = 5;
    if (argc < 3) {
        printf("Need two arguments. ITER and IFNAME.\n");
        return -1;
    }
    ITER = strtol(argv[1], NULL , 10);
    if_name = argv[2];
    keep_if_info.if_index = get_if_index(if_name);

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
    printf("Setting monitor interface channel freq...\n");
    set_if_chan(&nl, v_info.if_index, CHANNEL_6);
    printf("\n");
    //print_if_info(&v_info);
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

    if (set_if_promisc(&skh, v_info.if_index) < 0) {
        return -1;
    }
    printf("Waiting to receive packets...\n\n");
    // allocate_packet_buffer(&pb);
    //Just for testing, remove asap.
    for (int i=0; i<ITER; i++) {
        //recvpacket(&skh, &pb);
        
        if (recv_sk_msg(&skh, pb.buffer, pb.mh) < 0) {
            fprintf(stderr, "Could not read from socket.\n");
        }
    }
    
    printf("Removing created virtual interface...\n");
    //set_if_type(&nl, ret_iftype, if_index, if_name);
    delete_if(&nl, v_info.if_index);
    create_new_if(&nl, "managed", keep_if_info.wiphy, keep_if_info.if_name);
    nl_cleanup(&nl);

    return 0;
}
