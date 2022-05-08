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

int get_phy_info(nl_handle *nl, struct phy_info *info, int phy_id);

int get_if_info(nl_handle *nl, struct if_info *info, int if_index, int phy_id);

int compare_if_type(int cmp_iftype, const char *base_iftype);

int set_if_type(nl_handle *nl, const char *iftype, int if_index);

int set_if_up(const char *if_name);

int set_if_down(const char *if_name);

int delete_if(nl_handle *nl, int if_index);

int create_new_if(nl_handle *nl, const char *if_type, int wiphy, const char *if_name);

int set_if_chan(nl_handle *nl, int if_index, enum wifi_chan_freqs freq);

#endif