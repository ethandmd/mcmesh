#ifndef NL_UTILITIES_H
#define NL_UTILITIES_H

#include <linux/nl80211.h>

/* 
* 2.4 (assumes 20MHz width) & 5 (assues 80MHz width) GHz channel frequencies. 
* Yes, these channels may be used without restriction in NA.
*/
enum wifi_chan_freqs {
    CHANNEL_1 = 2412,
    CHANNEL_6 = 2437,
    CHANNEL_11 = 2462,
    CHANNEL_36 = 5180,
    CHANNEL_40 = 5200,
    CHANNEL_44 = 5220,
    CHANNEL_48 = 5240,
    CHANNEL_149 = 5745,
    CHANNEL_153 = 5765,
    CHANNEL_157 = 5785,
    CHANNEL_161 = 5805,
    CHANNEL_165 = 5825
};

typedef struct {
    int nl80211_id;
    struct nl_sock *sk;
} nl_handle;

struct phy_info {
    unsigned phy_id;
    char *phy_name;
    unsigned hard_mon;
    unsigned soft_mon;
    unsigned new_if;
    unsigned set_if;
    struct phy_info *next;
};

struct if_info {
    char *if_name;
    unsigned if_index;
    unsigned wdev;
    unsigned wiphy;
    unsigned if_type;
    unsigned if_freq;
};



void nl_init(nl_handle *nl);

void nl_cleanup(nl_handle *nl);

int get_if_index(char *if_name);

enum nl80211_iftype iftype_str_to_num(const char *base_iftype);

int create_new_interface(nl_handle *nl, char *if_name, enum nl80211_iftype if_type, unsigned wiphy);

int delete_interface(nl_handle *nl, unsigned if_index);

int get_interface_config(nl_handle *nl, struct if_info *info, unsigned if_index);

int set_if_up(const char *if_name);

int set_if_down(const char *if_name);

int set_interface_channel(nl_handle *nl, unsigned if_index, enum wifi_chan_freqs freq);

int set_interface_type(nl_handle *nl, enum nl80211_iftype type, unsigned if_index);

#endif