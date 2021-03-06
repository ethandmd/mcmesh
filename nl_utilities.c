#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <net/if.h>             //if_nametoindex
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>             //IFF_UP

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>  //genl_connect, genlmsg_put
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>  //genl_ctrl_resolve
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>      //nl80211 enums


#include "nl_utilities.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////
//                      BEGIN NETLINK INITIALIZATION AND CLEANUP FUNCTIONS
/*  
 *  Prototypical nl_init() function.
 */
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
 *  Prototypical nl_cleanup() function.
 */
void nl_cleanup(nl_handle *nl) {
    nl_socket_free(nl->sk);
}

/*
 *   Use net/if.h name to index conversion.
 */
int get_if_index(char *if_name) {
    int if_index = if_nametoindex(if_name);

    if (if_index < 0) {
        fprintf(stderr, "Could not find interface index.\n");
        return -1;
    }

    return if_index;
}
//                        END NETLINK INITIALIZATION AND CLEANUP FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
//                        BEGIN NETLINK CALLBACK HELPER FUNCTIONS
/*
 *   Callback for error handling.
 */
static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

/*
 *   Callback for NL_CB_ACK.
 */
static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

/*
 *   Callback for NL_CB_FINISH. Only used for NLM_F_DUMP.
 */
// static int finish_handler(struct nl_msg *msg, void *arg) {
//       int *ret = arg;
//       *ret = 0;
//       return NL_SKIP;
// }

/*
 *  Convert char* to nl80211 enum type.
 *  Currently, only concerned with monitor and managed mode.  
 */
enum nl80211_iftype iftype_str_to_num(const char *base_iftype) {
    if (strcmp(base_iftype, "monitor") == 0) {
        return NL80211_IFTYPE_MONITOR;
    } else if (strcmp(base_iftype, "managed") == 0) {
        return NL80211_IFTYPE_STATION;
    } else {
        return NL80211_IFTYPE_MAX + 1;
    }
}

/*
 *  Compare an integer iftype to a string iftype by converting back to
 *  their respective enums.
 */
int compare_if_type(unsigned cmp_iftype, const char *base_iftype) {
    enum nl80211_iftype ct = (enum nl80211_iftype)cmp_iftype;
    enum nl80211_iftype bt = iftype_str_to_num(base_iftype);
    if (bt == NL80211_ATTR_MAX + 1) {
        fprintf(stderr, "Unable to get valid if type.\n");
        return -1;
    }
    return (ct == bt);
}

//                      END NETLINK CALLBACK HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
//                      BEGIN CREATE/DELETE VIRTUAL INTERFACE && HELPERS
/*
 * TODO:
 * Parse mntr flags to add parametrization of which flags to set.
 * Helper function that sets the REQUIRED flags (nl80211 attrs)  
 * to a netlink message.
*/
static int add_monitor_flags(struct nl_msg *msg) {
    printf("Adding monitor flags...\n");
    struct nl_msg *flag_msg = nlmsg_alloc();
    if (!flag_msg) {
        return -1;
    }

    /* These three lines commented == monitor flags := none */
    //nla_put_flag(flag_msg, NL80211_MNTR_FLAG_FCSFAIL);
    //nla_put_flag(flag_msg, NL80211_MNTR_FLAG_CONTROL);
    //printf("\t(control flag)\n");
    nla_put_flag(flag_msg, NL80211_MNTR_FLAG_OTHER_BSS);
    printf("\t(other bss flag)\n");

    return nla_put_nested(msg, NL80211_ATTR_MNTR_FLAGS, flag_msg);
}

/*
 * Request new virtual interface from userspace:
 * @NL80211_CMD_NEW_INTERFACE 
 *   -NL80211_ATTR_WIPHY
 *   -NL80211_ATTR_IFTYPE
 *   -NL80211_ATTR_IFNAME
 */
int create_new_interface(nl_handle *nl, char *if_name, enum nl80211_iftype if_type, unsigned wiphy) {
    assert((wiphy >= 0) && "Can't create interface; wiphy out of bounds.\n");

    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allocate netlink message.\n");
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
    
    nla_put_string(msg, NL80211_ATTR_IFNAME, if_name);
    nla_put_u32(msg, NL80211_ATTR_IFTYPE, if_type);
    nla_put_u32(msg, NL80211_ATTR_WIPHY, wiphy);

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

/*
 *  Request delete virtual interface from userspace:
 *  @NL80211_CMD_DEL_INTERFACE:
 *      -NL80211_ATTR_IFINDEX
 */
int delete_interface(nl_handle *nl, unsigned if_index) {
    assert((if_index >= 0) && "Can't delete if; ifindex out of bounds.\n");
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allocate netlink message.\n");
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
//                      END CREATE/DELETE VIRTUAL INTERFACE && HELPERS
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
//                      BEGIN GET INTERFACE CONFIGURATION && HELPERS

static int callback_if_info(struct nl_msg *msg, void *arg) {
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
 *  Get interface info from userspace. Relies on standard callbacks, plus
 *  custom callback to parse & store desired interface data fields.
 *  @NL80211_CMD_GET_INTERFACE:
 *      NL80211_ATTR_IFINDEX
 */
int get_interface_config(nl_handle *nl, struct if_info *info, unsigned if_index) {
    assert((if_index >= 0) && "Can't get interface config; ifindex out of bounds.\n");
    int ret;
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
        0,                    //flags
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

    //Specify results for if_index.
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);

    //Send message
    ret = nl_send_auto(nl->sk, msg);
    if (ret < 0) {
        fprintf(stderr, "Could not send netlink message to get if config.\n");
        nl_cb_put(cb);
        return -1;
    }
    ret = 1;

    //Set custom netlink callback.
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_if_info, info);
    
    //Receive message (callback handles recv data)
    while (ret > 0) {
        nl_recvmsgs(nl->sk, cb);
    }

    //Clean up resources.
    nlmsg_free(msg);
    nl_cb_put(cb);

    return ret;
}

//                      END GET INTERFACE CONFIGURATION && HELPERS
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
//                      BEGIN IOCTL CALLS
/*
 *  TODO: Make one function to set if flags.
 *
 *  Helper functions to set if down/up before changing if type.
 */
int set_if_up(const char *if_name) {
    int sockfd = socket(AF_NETLINK, SOCK_RAW, 0);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    /* Read the tea leaves. */
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        fprintf(stderr, "Could not get device flags for %s.\n", if_name);
        return -1;
    }
    /* Do the tea leaves say this interface is arleady up and running? */
    if (ifr.ifr_flags & IFF_UP) {
        printf("%s is already up.\n", if_name);
        return 0;
    }
    /* If not, we need to set the interface flags! */
    ifr.ifr_flags |= IFF_UP;
    
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        printf("Could not set device flags up for %s.\n", if_name);
        return -1;
    }
    printf("Successfully set %s up.\n", if_name);
    return 0;
}

int set_if_down(const char *if_name) {
    int sockfd = socket(AF_NETLINK, SOCK_RAW, 0);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    /* Read the tea leaves. */
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        fprintf(stderr, "Could not get device flags for %s.\n", if_name);
        return -1;
    }
    /* Do the tea leaves say this interface is arleady up and running? */
    if (ifr.ifr_flags & ~IFF_UP) {
        printf("%s is already down.\n", if_name);
        return 0;
    }
    /* If not, we need to set the interface flags! */
    ifr.ifr_flags &= ~IFF_UP;
    
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        printf("Could not set device flags down for %s.\n", if_name);
        return -1;
    }
    printf("Successfully set interface down.\n");
    return 0;
}
//                  END IOCTL CALLS
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
//                  BEGIN SET INTERFACE ATTRS

/*
 *  @NL80211_CMD_SET_CHANNEL:
 *      -NL80211_ATTR_WIPHY_FREQ
 *      -attrs for channel width (NL80211_ATTR_CHANNEL_WIDTH)
 */
int set_interface_channel(nl_handle *nl, unsigned if_index, enum wifi_chan_freqs freq) {
    assert((if_index >= 0) && "Can't set interface channel; ifindex out of bounds.\n");
    assert((CHANNEL_1 <= freq) && (freq <= CHANNEL_165) && "Could not set channel; frequency out of bounds.\n");
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allocate netlink message.\n");
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
        fprintf(stderr, "Could not send netlink message to set if channel.\n");
        nlmsg_free(msg);
        return -1;
    }
    nlmsg_free(msg);

    printf("Successfully set ifindex %d to %dMHz.\n", if_index, freq);
    return 0; 
}


/*
 *  @NL80211_CMD_SET_INTERFACE:
 *      -NL80211_ATTR_IFINDEX
 *      -NL80211_ATTR_IFTYPE
 */
int set_interface_type(nl_handle *nl, enum nl80211_iftype type, unsigned if_index) {
    assert((if_index >= 0) && "Can't set interface channel; ifindex out of bounds.\n");
    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Could not allocate netlink message.\n");
        return -1;
    }
    genlmsg_put(
        msg,
        0,
        0,
        nl->nl80211_id,
        0,
        0,
        NL80211_CMD_SET_INTERFACE,
        0
    );
    nla_put_u32(msg, NL80211_ATTR_IFTYPE, type);
    nla_put_u32(msg, NL80211_ATTR_IFINDEX, if_index);
    
    int ret = nl_send_auto(nl->sk, msg);
    if (ret < 0) {
        fprintf(stderr, "Could not send netlink message to set iftype.\n");
        nlmsg_free(msg);
        return -1;
    }
    nlmsg_free(msg);
    return 0; 
}
//                  END SET INTERFACE ATTRS
////////////////////////////////////////////////////////////////////////////////////////////////////////

