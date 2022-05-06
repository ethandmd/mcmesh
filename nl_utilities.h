#ifndef NL_UTILITIES_H
#define NL_UTILITIES_H

typedef struct {
    int nl80211_id;
    struct nl_sock *sk;
} nl_handle;

int nl_init(nl_handle *nl);

int get_ifindex(char *if_name);

//int get_iftype(nl_handle *nl, int if_index);

int set_iftype_monitor(nl_handle *nl, int if_index);

#endif