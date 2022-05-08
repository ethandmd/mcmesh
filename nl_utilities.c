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
int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

// /*
// *   Callback for NL_CB_ACK.
// *   https://git.kernel.org/pub/scm/linux/kernel/git/jberg/iw.git :: genl.c
// */
int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

/*
*   Callback for NL_CB_FINISH.
*/
int finish_handler(struct nl_msg *msg, void *arg) {
      int *ret = arg;
      *ret = 0;
      return NL_SKIP;
}

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
*   Callback function for getting interface type. 
*   Notice that nla_parse iterates over the attr stream and stores a ptr to
*   each attr in the index array. To retrieve the value, you index the array 
*   by index type. The maxtype is for backwards compatibility. This is a moment
*   where I miss how simple ioctl() is, yet can appreciate nl80211.
*/
int callback_if_info(struct nl_msg *msg, void *arg) {
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

    if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
        int freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
        info->if_freq = freq;
    }

    return NL_SKIP;
}

/*
*   Send GET_INTERFACE cmd, specifying if_index or phy_id for dump of all interfaces on a wiphy, 
*   and use custom callback to filter the resulting stream of attributes for the iftype.
*/

int handler_get_if_info(nl_handle *nl, struct if_info *info, int if_index, int phy_id) {
    int ret;
    int err;
    int nl_flag = 0;
    if (if_index < 0 && phy_id >= 0) {
        printf("Setting phy dump flag...\n");
        nl_flag = NLM_F_DUMP;
    }
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
        nl_flag,                    //flags
        NL80211_CMD_GET_INTERFACE,  //command
        0                           //version
    );

    //Allocate netlink callback.
    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        fprintf(stderr, "Could not allocate netlink callback.\n");
        nlmsg_free(msg);
        return -1;
    }

    //Specify results for if_index or phy.
    if (if_index < 0 && phy_id >= 0) {
        //If phy, this is a dump request for all interfaces on phy!
        nla_put_u32(msg, NL80211_ATTR_WIPHY, phy_id);
    } else {
        nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);
    }

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
    if (if_index < 0  && phy_id >= 0) {
        nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    }
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_if_info, info);
    
    //Receive message (callback handles recv data)
    if (if_index < 0 && phy_id >= 0) {
        err = 1;
        while (err > 0) {
            ret = nl_recvmsgs(nl->sk, cb);
            if (err < 0) {
                fprintf(stderr, "Could not pull info from dump.\n");
            }
        }
    }
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
 *  Phy info -- find supported interface modes.
 *  
*/
int callback_phy_info(struct nl_msg *msg, void *arg) {
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *nl_mode;
    struct nlattr *nl_cmd;
    struct phy_info *info = arg; 
    int rem_mode;
    int rem_cmd;

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if (tb_msg[NL80211_ATTR_WIPHY]) {
        info->phy_id = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
        info->phy_name = nla_get_string(tb_msg[NL80211_ATTR_WIPHY_NAME]);
    }

    if (tb_msg[NL80211_ATTR_SUPPORTED_IFTYPES]) {
        info->hard_mon = 0;
        nla_for_each_nested(nl_mode, tb_msg[NL80211_ATTR_SUPPORTED_IFTYPES], rem_mode) {
            if (nla_type(nl_mode) == NL80211_IFTYPE_MONITOR) {
                info->hard_mon = 1;
            }
        }
    }

    if (tb_msg[NL80211_ATTR_SOFTWARE_IFTYPES]) {
        info->hard_mon = 0;
        nla_for_each_nested(nl_mode, tb_msg[NL80211_ATTR_SOFTWARE_IFTYPES], rem_mode) {
            if (nla_type(nl_mode) == NL80211_IFTYPE_MONITOR) {
                info->soft_mon = 1;
            }
        }
    }

    if (tb_msg[NL80211_ATTR_SUPPORTED_COMMANDS]) {
        info->new_if = 0;
        info->set_if = 0;
        nla_for_each_nested(nl_cmd, tb_msg[NL80211_ATTR_SUPPORTED_COMMANDS], rem_cmd) {
            if (nla_get_u32(nl_cmd) == NL80211_CMD_NEW_INTERFACE) {
                info->new_if = 1;
            }
            if (nla_get_u32(nl_cmd) == NL80211_CMD_SET_INTERFACE) {
                info->set_if = 1;
            }
        }
    }

    return NL_SKIP;
}

int handler_get_phy_info(nl_handle *nl, struct phy_info *info, int phy_id){
    int nl_flag = 0;
    if (phy_id < 0) {
        nl_flag = NLM_F_DUMP;
    }
    int ret;
    int err;
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allocate netlink message for phy info.\n");
        return -1;
    }
    //Add generic netlink header to netlink message.
    genlmsg_put(
        msg,                        //nl msg object
        0,                          //nl port / auto (0 is kernel)
        0,                          //seq no / auto 
        nl->nl80211_id,             //numeric family id
        0,                          //header length in bytes
        nl_flag,                    //flags
        NL80211_CMD_GET_WIPHY,      //command
        0                           //version
    );

    //Allocate netlink callback.
    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        fprintf(stderr, "Could not allocate netlink callback for phy info.\n");
        nlmsg_free(msg);
        return -1;
    }

    //Specify results for if_index
    if (phy_id >= 0) {
        nla_put_u32(msg, NL80211_ATTR_WIPHY, phy_id);
    }

    //Send message
    ret = nl_send_auto(nl->sk, msg);
    if (ret < 0) {
        fprintf(stderr, "Could not send netlink message to get phy info.\n");
        nl_cb_put(cb);
        return -1;
    }
    ret = 1;

    //Set custom netlink callback.
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
    if (phy_id < 0) {
        nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
    }
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_phy_info, info);
    
    //Receive message (callback handles recv data)
    if (phy_id < 0) {
        err = 1;
        while (err > 0) {
            ret = nl_recvmsgs(nl->sk, cb);
            if (err < 0) {
                fprintf(stderr, "Could not read results from phy dump.\n");
            }
            if (info->soft_mon == 1 || info->hard_mon == 1) {
                printf("Found monitor mode capability on %s...\n", info->phy_name);
                nlmsg_free(msg);
                nl_cb_put(cb);
                return 0;
            }
        }
    } else {
        while (ret > 0) {
            nl_recvmsgs(nl->sk, cb);
            if (info->soft_mon == 1 || info->hard_mon == 1) {
                    printf("Found monitor mode capability on %s...\n", info->phy_name);
                    nlmsg_free(msg);
                    nl_cb_put(cb);
                    return 0;
                }
        }
    }

    //Clean up resources.
    nlmsg_free(msg);
    nl_cb_put(cb);

    return ret;
}



/*
*   Given two strings, compare their 
*/
int compare_if_type(int cmp_iftype, const char *base_iftype) {
    enum nl80211_iftype ct = (enum nl80211_iftype)cmp_iftype;
    enum nl80211_iftype bt = convert_iftype(base_iftype);
    if (bt == NL80211_ATTR_MAX + 1) {
        fprintf(stderr, "Unable to get valid if type.\n");
        return -1;
    }
    if (ct == bt) {
        return 1;
    } else {
        return 0;
    }
}

/*
*   Helper functions to set if down/up before changing if type.
*   Ref. interface flags in <net/if.h>
*/
int set_if_up(const char *if_name) {
    int sockfd = socket(AF_UNIX, SOCK_RAW, 0);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    
    // if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
    //     fprintf(stderr, "Could not get device flags.\n");
    //     return -1;
    // }

    ifr.ifr_flags |= IFF_UP;
    
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        printf("Could not set device flags.\n");
        return -1;
    }
    printf("Successfully set interface up.\n");
    return 0;
}

int set_if_down(const char *if_name) {
    int sockfd = socket(AF_UNIX, SOCK_RAW, 0);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    
    // if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
    //     fprintf(stderr, "Could not get device flags.\n");
    //     return -1;
    // }

    ifr.ifr_flags &= ~IFF_UP;
    
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        printf("Could not set device flags.\n");
        return -1;
    }
    printf("Successfully set interface down.\n");
    return 0;
}

/*
 * TODO:
 * Parse mntr flags to add parametrization of which flags to set.
 * Helper function that sets the REQUIRED flags (nl80211 attrs)  
 * to a netlink message.
*/
int add_monitor_flags(struct nl_msg *msg) {
    printf("Adding monitor flags...\n\t(none)\n");
    struct nl_msg *flag_msg = nlmsg_alloc();
    if (!flag_msg) {
        return -1;
    }

    /* These three lines commented == monitor flags := none */
    //nla_put_flag(flag_msg, NL80211_MNTR_FLAG_FCSFAIL);
    nla_put_flag(flag_msg, NL80211_MNTR_FLAG_CONTROL);
    nla_put_flag(flag_msg, NL80211_MNTR_FLAG_OTHER_BSS);

    return nla_put_nested(msg, NL80211_ATTR_MNTR_FLAGS, flag_msg);
}

/*
*   Set if type from userspace requires iftype and ifindex.
*/
int handler_set_if_type(nl_handle *nl, enum nl80211_iftype type, int if_index) {
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

/**/
int handler_set_if_chan(nl_handle *nl, int if_index, enum wifi_chan_freqs freq) {
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
        NL80211_CMD_SET_CHANNEL,
        0
    );
    nla_put_u32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
    if (freq > CHANNEL_11) {
        nla_put_u32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_80); //Consider 40MHz for 5GHz.
    } else {
        nla_put_u32(msg, NL80211_ATTR_CHANNEL_WIDTH, NL80211_CHAN_WIDTH_20);
    }
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
 *  Fill the data fields of a struct with results.
*/
int get_phy_info(nl_handle *nl, struct phy_info *info, int phy_id) {
    return handler_get_phy_info(nl, info, phy_id);
}


/*
 *   Fill the data fields of struct info with results.
*/
int get_if_info(nl_handle *nl, struct if_info *info, int if_index, int phy_id) {
    int ret = handler_get_if_info(nl, info, if_index, phy_id);
    return ret;
}

/*
 *  Set interface to type.
*/
int set_if_type(nl_handle *nl, const char *iftype, int if_index) {
    printf("Setting interface index %d to %s...\n", if_index, iftype);
    enum nl80211_iftype type = convert_iftype(iftype);
    return handler_set_if_type(nl, type, if_index);
}

/*
*   Delete interface by if index.
*/
int delete_if(nl_handle *nl, int if_index) {
    printf("Deleting ifindex: %d...\n", if_index);
    int ret = handler_delete_if(nl, if_index);
    return ret;
}


/*
*   Create interface by name, wiphy, type (+ flags for monitor mode).
*/
int create_new_if(nl_handle *nl, const char *if_type, int wiphy, const char *if_name) {
    enum nl80211_iftype type = convert_iftype(if_type);
    //enum nl80211_attrs wiphy_type = (enum nl80211_attrs)wiphy;
    printf("Creating interface, ifname: %s, iftype:%s...\n", if_name, if_type);
    int ret = handler_create_new_if(nl, type, wiphy, if_name);
    return ret;
}

/*
*   Set wiphy channel frequency (MHz) and width (MHz)
*/
int set_if_chan(nl_handle *nl, int if_index, enum wifi_chan_freqs freq) {
    return handler_set_if_chan(nl, if_index, freq);
}

