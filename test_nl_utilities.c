#include "nl_utilities.h"
#include "mcpcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
 * Test netlink utilities for fetching an interface's config, creating a new interface,
 * and deleting an interface.
 */
static int test_get_create_del_interface(nl_handle *nl) {
    struct if_info test_info;
    struct if_info info = {
        .if_name = "MCTEST007",
        .if_index = 1001,
        .if_type = 2,           /* 2 == "managed" in enum nl80211_iftype ... */
        .wdev = 0x6d,
        .wiphy = 0,
        .if_freq = 2442         /* 2.4Ghz should be a safe bet... */
    };
    if (create_new_interface(nl, info.if_name, info.if_index, info.wiphy) < 0) {
        fprintf(stderr, "Could not create new test interface.\n");   /* Having tests rely on yet-tested functions :o */
        return -1;
    }

    /* Now the actual test of "get_interface_config" */
    if (get_interface_config(nl, &test_info, info.if_index) < 0) {
        fprintf(stderr, "Could not get test interface info.\n");
        return -1;
    }
    assert((info.if_name == test_info.if_name) && "Names do not match.");
    assert((info.if_index == test_info.if_index) && "Ifindices do not match.");
    assert((info.if_type == test_info.if_type) && "Iftypes do not match.");
    assert((info.wdev == test_info.wdev) && "Wdevs do not match.");
    assert((info.wiphy == test_info.wiphy) && "Wiphys do not match.");
    assert((info.if_freq == test_info.if_freq) && "Iffreq do not match.");

    /* Cleanup */
    delete_interface(nl, info.if_index);
}

