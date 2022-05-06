#include "nl_utilities.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    char *if_name = argv[1];
    int if_index = get_ifindex(if_name);

    nl_handle nl;
    nl_init(&nl);

    //int t = get_iftype(&nl, if_index);

    set_iftype_monitor(&nl, if_index);
    
    return 0;
}