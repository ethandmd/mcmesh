#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>             //if_nametoindex
#include <sys/ioctl.h>          //ioctl
#include <sys/socket.h>

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
int get_if_index(const char *if_name) {
    int if_index = if_nametoindex(if_name);

    if (if_index < 0) {
        fprintf(stderr, "Could not find interface index.\n");
        return -1;
    }

    return if_index;
}

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
*   Convert char* to nl80211 enum type.
*/
enum nl80211_iftype convert_iftype(const char *base_iftype) {
    if (strcmp(base_iftype, "monitor") == 0) {
        return NL80211_IFTYPE_MONITOR;
    } else if (strcmp(base_iftype, "managed") == 0) {
        return NL80211_IFTYPE_STATION;
    } else if (strcmp(base_iftype, "mesh") == 0) {
        return NL80211_IFTYPE_MESH_POINT;
    } else {
        return NL80211_IFTYPE_MAX + 1;
    }
}

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

/* 
*   Callback function for getting interface type. 
*   Notice that nla_parse iterates over the attr stream and stores a ptr to
*   each attr in the index array. To retrieve the value, you index the array 
*   by index type. The maxtype is for backwards compatibility. This is a moment
*   where I miss how simple ioctl() is, yet can appreciate nl80211.
*/
int if_info_callback(struct nl_msg *msg, void *arg) {
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

    if (tb_msg[NL80211_ATTR_IFNAME]) {
        info->if_name = nla_get_string(tb_msg[NL80211_ATTR_IFNAME]);
    }
    if (tb_msg[NL80211_ATTR_IFINDEX]) {
        info->if_index = (enum nl80211_iftype)nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
    }
    if (tb_msg[NL80211_ATTR_WDEV]) {
        info->wdev = nla_get_u64(tb_msg[NL80211_ATTR_WDEV]);
    }
    if (tb_msg[NL80211_ATTR_WIPHY]) {
        info->wiphy = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
    }
    if (tb_msg[NL80211_ATTR_IFTYPE]) {
        info->if_type = nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]);
    }

    return NL_SKIP;
}

/*
*   Send GET_INTERFACE cmd, specifying if_index, and use custom callback
*   to filter the resulting stream of attributes for the iftype. Then
*   call a helper function that compares the found type against the desired type.
*/

int handler_get_if_info(nl_handle *nl, struct if_info *info, int if_index) {
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
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, if_info_callback, info);
    
    //Receive message (callback handles recv data)
    while (ret > 0) {
        nl_recvmsgs(nl->sk, cb);
    }

    // if (ret == 0) {
    //     ret = info->iftype;
    //}

    //Clean up resources.
    nlmsg_free(msg);
    nl_cb_put(cb);

    return ret;
}

/*
*   Helper functions to set if down/up before changing if type.
*   Ref. interface flags in <net/if.h>
*/
int set_if_up(const char *if_name, short up_down) {
    int sockfd = socket(AF_UNIX, SOCK_RAW, 0);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        fprintf(stderr, "Could not get device flags.\n");
        return -1;
    }

    if (up_down) {
        ifr.ifr_flags = ifr.ifr_flags | IFF_UP;
    } else {
        ifr.ifr_flags = ifr.ifr_flags & ~IFF_UP;
    }

    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        printf("Could not set device flags.\n");
        return -1;
    }
    printf("Successfully set device flags.\n");
    return 0;
}

/*
* Helper function that sets the REQUIRED flags (nl80211 attrs)  
* to a netlink message.
*/
int add_monitor_flags(struct nl_msg *msg) {
    struct nl_msg *flag_msg = nlmsg_alloc();
    if (!flag_msg) {
        return -1;
    }

    //nla_put_flag(flag_msg, NL80211_MNTR_FLAG_FCSFAIL);
    nla_put_flag(flag_msg, NL80211_MNTR_FLAG_CONTROL);
    nla_put_flag(flag_msg, NL80211_MNTR_FLAG_OTHER_BSS);

    return nla_put_nested(msg, NL80211_ATTR_MNTR_FLAGS, flag_msg);
}

/*
*   Set if type from userspace requires iftype and ifindex.
*/
int handler_set_if_type(nl_handle *nl, enum nl80211_iftype *type, int if_index) {
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
    nla_put_u32(msg, NL80211_ATTR_IFTYPE, *type);
    //Set monitor mode flags.
    add_monitor_flags(msg);

    //Send message
    int ret = nl_send_auto(nl->sk, msg);
    if (ret < 0) {
        fprintf(stderr, "Could not send netlink message to set iftype to monitor.\n");
        nlmsg_free(msg);
        return -1;
    }

    //Clean up
    nlmsg_free(msg);
    
    return 0;
}

/*
*   Delete if from userspace only requires if index.
*/
int handler_delete_if(nl_handle *nl, int if_index) {
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allovate netlink message.\n");
        return -1;
    }
    genlmsg_put(
        msg,
        0,
        0,
        nl->nl80211_id,
        0,
        0,
        NL80211_CMD_DEL_INTERFACE,
        0
    );
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);
    int ret = nl_send_auto(nl->sk, msg);
    if (ret < 0) {
        fprintf(stderr, "Could not send netlink message to del if.\n");
        nlmsg_free(msg);
        return -1;
    }
    nlmsg_free(msg);
    return 0;
}

/*
* Request new virtual interface from userspace:
* @NL80211_CMD_NEW_INTERFACE 
*   -NL80211_ATTR_WIPHY
*   -NL80211_ATTR_IFTYPE
*   -NL80211_ATTR_IFNAME
*/
int handler_create_new_if(nl_handle *nl, enum nl80211_iftype if_type, int wiphy, const char *ifname) {
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allovate netlink message.\n");
        return -1;
    }
    genlmsg_put(
        msg,
        0,
        0,
        nl->nl80211_id,
        0,
        0,
        NL80211_CMD_NEW_INTERFACE,
        0
    );
    nla_put_u32(msg, NL80211_ATTR_WIPHY, wiphy);
    nla_put_string(msg, NL80211_ATTR_IFNAME, ifname);
    nla_put_u32(msg, NL80211_ATTR_IFTYPE, if_type);
    if (compare_if_type(if_type, "monitor")) {
        printf("Adding monitor flags...\n");
        add_monitor_flags(msg);
    }
    
    int ret = nl_send_auto(nl->sk, msg);
    if (ret < 0) {
        fprintf(stderr, "Could not send netlink message to del if.\n");
        nlmsg_free(msg);
        return -1;
    }
    nlmsg_free(msg);
    return 0;
}

/*
*   Fill the data fields of struct info with results.
*/
int get_if_info(nl_handle *nl, struct if_info *info, int if_index) {
    int ret = handler_get_if_info(nl, info, if_index);
    return ret;
}

/*
*   Given two strings, compare their 
*/
int compare_if_type(int cmp_iftype, const char *base_iftype) {
    enum nl80211_iftype ct = (enum nl80211_iftype)cmp_iftype;
    enum nl80211_iftype bt = convert_iftype(base_iftype);
    if (bt == NL80211_ATTR_MAX + 1) {
        fprintf(stderr, "Unable to ascertain valid if type.\n");
        return -1;
    }
    if (ct == bt) {
        return 1;
    } else {
        return 0;
    }
}

int set_if_type(nl_handle *nl, const char *iftype, int if_index, const char *if_name) {
    //Set if down
    set_if_up(if_name, 0);
    enum nl80211_iftype type = convert_iftype(iftype);
    if (type == NL80211_ATTR_MAX + 1) {
        fprintf(stderr, "Unable to ascertain valid if type.\n");
        return -1;
    }
    int ret = handler_set_if_type(nl, &type, if_index);
    //Set if back up
    set_if_up(if_name, 1);
    return ret;
}

int delete_if(nl_handle *nl, int if_index) {
    int ret = handler_delete_if(nl, if_index);
    return ret;
}

int create_new_if(nl_handle *nl, const char *iftype, int wiphy, const char *ifname) {
    enum nl80211_iftype if_type = convert_iftype(iftype);
    //enum nl80211_attrs wiphy_type = (enum nl80211_attrs)wiphy;
    int ret = handler_create_new_if(nl, if_type, wiphy, ifname);
    return ret;
}
