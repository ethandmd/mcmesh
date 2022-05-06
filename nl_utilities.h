#ifndef NL_UTILITIES_H
#define NL_UTILITIES_H

typedef struct {
    int nl80211_id;
    struct nl_sock *sk;
} nl_handle;

struct if_info {
    const char *ifname;
    int if_index;
    int wiphy;
    int iftype;
};

void nl_init(nl_handle *nl);

void nl_cleanup(nl_handle *nl);

int get_if_index(const char *if_name);

int get_if_info(nl_handle *nl, struct if_info *info, int if_index);

int compare_if_type(int cmp_iftype, const char *base_iftype);

int set_if_type(nl_handle *nl, const char *iftype, int if_index, const char *if_name);

int delete_if(nl_handle *nl, int if_index);

int create_new_if(nl_handle *nl, const char *iftype, int wiphy, const char *ifname);

#endif