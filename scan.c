#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>  //genl_connect, genlmsg_put
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>  //genl_ctrl_resolve
#include <linux/nl80211.h>      //nl80211 enums

/* Build:
*  $ gcc $(pkg-config --cflags --libs libnl-genl-3.0) scan.c -o scan
*/

typedef struct {
    int nl80211_id;
    struct nl_sock *sk;
    struct nl_cb *cb;
} nl_handle;

/* See: https://github.com/cozybit/iw/blob/master/iw.c : nl80211_init() */
static int nl_init(nl_handle *nl){
    //First, allocate a new nl socket.
    nl->sk = nl_socket_alloc();
    if (!nl->sk) {
        fprintf(stderr, "Could not allocate new netlink socket.\n");
    }

    //Set socket buffer size: *nl_sock, rx: 2**13, tx: 2**13.
    nl_socket_set_buffer_size(nl->sk, 8192, 8192);

    //Create fd, bind socket.
    if (genl_connect(nl->sk)) {
        fprintf(stderr, "Could not connect to generic netlink.\n");
        //Deallocate nl socket.
        nl_socket_free(nl->sk);
        return -1;
    }

    //Get nl80211 interface driver id.
    nl->nl80211_id = genl_ctrl_resolve(nl->sk, "nl80211");
    if (nl->nl80211_id < 0) {
        fprintf(stderr, "Could not resolve nl80211 interface.\n");
        return -1;
    }

    return 0;
}

int main() {
    nl_handle nl;  //Create nl80211 handle.
    if (nl_init(&nl) < 0) {
        fprintf(stderr, "Failed to initialize netlink handle.\n");
        return -1;
    }

    return 0;
}