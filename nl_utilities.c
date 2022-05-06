#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>             //if_nametoindex
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>  //genl_connect, genlmsg_put
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>  //genl_ctrl_resolve
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>      //nl80211 enums


#include "nl_utilities.h"


/* Build:
*  $ gcc scan.c -o scan $(pkg-config --cflags --libs libnl-genl-3.0) 
*/

/* See: https://git.kernel.org/pub/scm/linux/kernel/git/jberg/iw.git :: iw.c :: nl80211_init() */
void nl_init(nl_handle *nl){
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
    }

    //Get nl80211 interface driver id.
    nl->nl80211_id = genl_ctrl_resolve(nl->sk, "nl80211");
    if (nl->nl80211_id < 0) {
        fprintf(stderr, "Could not resolve nl80211 interface.\n");
    }
}

/*
*   Deallocate nl socket resources.
*/
void nl_cleanup(nl_handle *nl) {
    nl_socket_free(nl->sk);
}

/*
*   Use net/if.h name to index conversion.
*/
int get_ifindex(char *if_name) {
    int if_index = if_nametoindex(if_name);

    if (if_index < 0) {
        fprintf(stderr, "Could not find interface index.\n");
        return -1;
    }

    return if_index;
}
/*
*   Helpers:
*/

/*
*   Callback for error handling.
*   https://git.kernel.org/pub/scm/linux/kernel/git/jberg/iw.git :: genl.c
*/
static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

// /*
// *   Callback for NL_CB_ACK.
// *   https://git.kernel.org/pub/scm/linux/kernel/git/jberg/iw.git :: genl.c
// */
static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

/*
*   Callback for NL_CB_FINISH.
*/
// static int finish_handler(struct nl_msg *msg, void *arg) {
//       int *ret = arg;
//       *ret = 0;
//       return NL_SKIP;
// }


/*
*   Allocates a netlink message. Fills generic netlink message header for nl80211
*   family (hardcoded), and flags, command params. Freeing memory is caller's responsibility. 
*/
// static int build_genlmsg(nl_handle *nl, struct nl_msg *msg, enum nl80211_commands cmd, int flags) {
//     msg = nlmsg_alloc();
//     if (!msg) {
//         fprintf(stderr, "Could not allocate netlink message.\n");
//         return -1;
//     }
//     //Add generic netlink header to netlink message.
//     genlmsg_put(
//         msg,                        //nl msg object
//         0,                          //nl port / auto (0 is kernel)
//         0,                          //seq no / auto 
//         nl->nl80211_id,             //numeric family id
//         0,                          //header length in bytes
//         flags,                      //flags
//         cmd,                        //command
//         0                           //version
//     );
//     return 0;
// }

struct if_info {
    int iftype;
};

static int iftype_compare(int iftype, enum nl80211_iftype type) {
    enum nl80211_iftype ciftype = (enum nl80211_iftype)iftype;
    if (ciftype == type) {
        return 1;
    } else {
        return 0;
    }
}
/* 
*   Callback function for getting interface type. 
*   Notice that nla_parse iterates over the attr stream and stores a ptr to
*   each attr in the index array. To retrieve the value, you index the array 
*   by index type. The maxtype is for backwards compatibility. This is a moment
*   where I miss how simple ioctl() is, yet can appreciate nl80211.
*/
static int iftype_handler(struct nl_msg *msg, void *arg) {
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];    //Highest attr num currently defined +1.
    struct if_info *info = arg;

    //Create attr index from stream of attrs
    nla_parse(
        tb_msg,                     //Idx array; filled with maxtype+1 elts
        NL80211_ATTR_MAX,           //Maxtype expected
        genlmsg_attrdata(gnlh, 0),  //Head of attr stream
        genlmsg_attrlen(gnlh, 0),   //Length of attr stream
        NULL                        //Attribute validation policy
    );

    if (tb_msg[NL80211_ATTR_IFTYPE]) {
        info->iftype = nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]);
    }

    return NL_SKIP;
}

/*
*   Public:
*/

/*
*   Send GET_INTERFACE cmd, specifying if_index, and use custom callback
*   to filter the resulting stream of attributes for the iftype. Then
*   call a helper function that compares the found type against the desired type.
*/
static int get_iftype(nl_handle *nl, int if_index) {
    struct if_info info;
    int ret;
    //int err;
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allocate netlink message.\n");
        return -1;
    }
    //Add generic netlink header to netlink message.
    genlmsg_put(
        msg,                        //nl msg object
        0,                          //nl port / auto (0 is kernel)
        0,                          //seq no / auto 
        nl->nl80211_id,             //numeric family id
        0,                          //header length in bytes
        0,                          //flags
        NL80211_CMD_GET_INTERFACE,  //command
        0                           //version
    );
    //struct nl_msg *msg;
    //if (build_genlmsg(nl, msg, NL80211_CMD_GET_INTERFACE, 0) < 0) {
    //    return -1;
    //}

    //Allocate netlink callback.
    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        fprintf(stderr, "Could not allocate netlink callback.\n");
        nlmsg_free(msg);
        return -1;
    }

    //Specify results for if_index
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);

    //Send message
    ret = nl_send_auto(nl->sk, msg);
    if (ret < 0) {
        fprintf(stderr, "Bad send.\n");
        nl_cb_put(cb);
        return -1;
    }
    ret = 1;

    //Set custom netlink callback.
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
    //nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, iftype_handler, &info);
    
    //Receive message (callback handles recv data)
    while (ret > 0) {
        nl_recvmsgs(nl->sk, cb);
    }

    if (ret == 0) {
        ret = info.iftype;
    }

    //Clean up resources.
    nlmsg_free(msg);
    nl_cb_put(cb);

    return ret;
}

/*
*   Compare an interface's type.
*/
int check_if_monitor(nl_handle *nl, int if_index) {
    // struct if_info info = {
    //     .iftype = -1,
    // };
    int found_iftype = get_iftype(nl, if_index);
    return iftype_compare(found_iftype, NL80211_IFTYPE_MONITOR);
}

/*
* TODO:
* Request new virtual interface from userspace:
* @NL80211_CMD_NEW_INTERFACE 
*   -NL80211_ATTR_WIPHY
*   -NL80211_ATTR_IFTYPE
*   -NL80211_ATTR_IFNAME
*/
static int set_iftype(nl_handle *nl, enum nl80211_iftype type, int if_index) {
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allocate netlink message.\n");
        return -1;
    }
    //Add generic netlink header to netlink message.
    genlmsg_put(
        msg,                        //nl msg object
        0,                          //nl port / auto (0 is kernel)
        0,                          //seq no / auto 
        nl->nl80211_id,             //numeric family id
        0,                          //header length in bytes
        0,                          //flags
        NL80211_CMD_SET_INTERFACE,  //command
        0                           //version
    );
    //Specify ifindex for nl80211 cmd
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);
    //Specify monitor iftype for nl80211 cmd
    nla_put_u32(msg, NL80211_ATTR_IFTYPE, type);

    //Send message
    int ret = nl_send_auto(nl->sk, msg);
    if (ret < 0) {
        fprintf(stderr, "Could not send netlink message to set iftype to monitor.\n");
        return -1;
    }

    //Clean up
    nlmsg_free(msg);
    
    return 0;
}

int set_iftype_monitor(nl_handle *nl, int if_index) {
    enum nl80211_iftype type = NL80211_IFTYPE_MONITOR;
    if (set_iftype(nl, type, if_index) < 0) {
        return -1;
    }

    return 0;
}

int set_iftype_managed(nl_handle *nl, int if_index) {
    enum nl80211_iftype type = NL80211_IFTYPE_STATION;
    if (set_iftype(nl, type, if_index) < 0) {
        return -1;
    }
    return 0;
}