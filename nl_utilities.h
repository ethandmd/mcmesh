#ifndef NL_UTILITIES_H
#define NL_UTILITIES_H

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
    int phy_id;
    char *phy_name;
    int hard_mon;
    int soft_mon;
    int new_if;
    int set_if;
    struct phy_info *next;
};

struct if_info {
    char *if_name;
    int if_index;
    int wdev;
    int wiphy;
    int if_type;
    int if_freq;
};



void nl_init(nl_handle *nl);

void nl_cleanup(nl_handle *nl);

int get_if_index(const char *if_name);

enum nl80211_iftype iftype_str_to_num(const char *base_iftype);

int create_new_interface(nl_handle *nl, char *if_name, int if_index, int wiphy);

int delete_interface(nl_handle *nl, int if_index);

int get_interface_config(nl_handle *nl, struct if_info *info, int if_index);

int set_if_up(const char *if_name);

int set_if_down(const char *if_name);

int set_interface_channel(nl_handle *nl, int if_index, enum wifi_chan_freqs freq);

#endif