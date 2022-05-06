#include "nl_utilities.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    char *if_name = argv[1];
    int if_index = get_ifindex(if_name);

    nl_handle nl;
    nl_init(&nl);

    if (check_if_monitor(&nl, if_index) != 1) {
        printf("Setting device to monitor mode.\n");
        set_iftype_monitor(&nl, if_index);
    } else {
        printf("Device already in monitor mode.\n");
    }
    
    printf("Restoring device to managed mode.\n");
    set_iftype_managed(&nl, if_index);
    nl_cleanup(&nl);

    return 0;
}