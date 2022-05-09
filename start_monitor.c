#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nl_utilities.h"
#include "mcpcap.h"

void print_interface(struct if_info *info) {
    printf("Interface: %s\n", info->if_name);
    printf("\tIFINDEX: %d\n", info->if_index);
    printf("\tWDEV: %d\n", info->wdev);
    printf("\tWIPHY: %d\n", info->wiphy);
    printf("\tIFTYPE: %d\n", info->if_type);
    printf("\tIFFREQ: %d\n", info->if_freq);
}

int main(int argc, char **argv) {
    nl_handle nl;
    sk_handle skh;
    struct if_info keep_if;
    struct if_info new_if;

    /*
     *  STEP 0:
     */
    if (argc < 3) {
        fprintf(stderr, "Require exactly 2 arguments, interface name and ITER.\n");
    }
    keep_if.if_name = argv[1];
    keep_if.if_index = get_if_index(keep_if.if_name);
    //int ITER = strtol(argv[2], NULL, 10);

    /*
     *  STEP 1:
     */
    printf("Initializing netlink socket...\n");
    nl_init(&nl);
    if (!nl.sk) {
        fprintf(stderr, "Failed to initialize netlink socket.\n");
        return -1;
    }

    /*
     *  STEP 2: Store existing interface config.
     */
    printf("Storing %s configuration...\n", keep_if.if_name);
    if (get_interface_config(&nl, &keep_if, keep_if.if_index) < 0) {
        fprintf(stderr, "Could not collect configuration for %s.\n", keep_if.if_name);
        return -1;
    }
    print_interface(&keep_if);


    /*
     *  STEP 3: Create new / set existing interface typer monitor mode (+ flags).
     *  Contingent: Delete existing interface from phy.
     */
    printf("Creating new monitor mode interface on phy%d...\n", keep_if.wiphy);
    strcpy(new_if.if_name, keep_if.if_name);
    strcat(new_if.if_name, "mon0");     //Need to find the next highest number for phy. Not just 0.
    if (create_new_interface(&nl, new_if.if_name, NL80211_IFTYPE_MONITOR, keep_if.wiphy) < 0) {
        printf("Could not create new monitor interface...\n");
        printf("Attempting to reset existing interface type...\n");
        set_if_down(keep_if.if_name);
        if (set_interface_type(&nl, NL80211_IFTYPE_MONITOR, keep_if.if_index) < 0) {
            fprintf(stderr, "Could not set existing interface to monitor mode...\n");
            return -1;
        }
        set_if_up(keep_if.if_name);
    } else {
        set_if_up(new_if.if_name);
        new_if.if_index = get_if_index(new_if.if_name);
        get_interface_config(&nl, &new_if, new_if.if_index);
        printf("New monitor mode interface configuration...\n");
        print_interface(&new_if);
        printf("Deleting %s...\n", keep_if.if_name);
        if (delete_interface(&nl, keep_if.if_index) < 0) {
            fprintf(stderr, "Could not delete %s...proceeding.\n", keep_if.if_name);
        }
    }

    /*
     * STEP 4: Create & configure packet socket.
     */
    create_pack_socket(&skh);
    if (!skh.sockfd) {
        fprintf(stderr, "Could not allocate new packet socket. Are you running this as root/sudo?\n");
        return -1;
    }
    if (bind_pack_socket(&skh, new_if.if_index) < 0) {
        fprintf(stderr, "Could not bind packet socket to %s", new_if.if_name);
        return -1;
    }
    //Optional...?
    if (set_if_promisc(&skh, new_if.if_index) < 0) {
        printf("Could not set %s to promiscuous mode.\n", new_if.if_name);
    }

    /* 
     *  STEP 5: Receive and parse data from the socket.
     */

    /*
     *  STEP 6: Recreate network interface setup and free resources used.
     */

    if (create_new_interface(&nl, keep_if.if_name, keep_if.if_index, keep_if.wiphy) < 0) {
        fprintf(stderr, "Could not recreate prior existing network interface.\n");
    }
    print_interface(&keep_if);
    if (delete_interface(&nl, new_if.if_index) < 0) {
        fprintf(stderr, "Could not delete %s...\n", new_if.if_name);
    }
    printf("LINE 111\n");
    set_if_up(keep_if.if_name);
    printf("LINE 113\n");
    nl_cleanup(&nl);
    cleanup_mcpap(&skh);

    return 0;
}