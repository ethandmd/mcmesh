#ifndef NL_UTILITIES_H
#define NL_UTILITIES_H

typedef struct {
    int nl80211_id;
    struct nl_sock *sk;
} nl_handle;

struct phy_info {
    int phy_id;
    const char *phy_name;
    int hard_mon;
    int soft_mon;
    int new_if;
    int set_if;
};

struct if_info {
    const char *if_name;
    int if_index;
    int wdev;
    int wiphy;
    int if_type;
};

void nl_init(nl_handle *nl);

void nl_cleanup(nl_handle *nl);

int get_if_index(const char *if_name);

int get_phy_info(nl_handle *nl, struct phy_info *info, int phy_id, int mon);

int get_if_info(nl_handle *nl, struct if_info *info, int if_index, int phy_id);

int compare_if_type(int cmp_iftype, const char *base_iftype);

/*int set_if_type(nl_handle *nl, const char *iftype, int if_index, const char *if_name);*/

int set_if_up(const char *if_name);

int set_if_down(const char *if_name);

int delete_if(nl_handle *nl, int if_index);

int create_new_if(nl_handle *nl, const char *if_type, int wiphy, const char *if_name);

#endif