#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <linux/nl80211.h>

/* Build:
*  $ gcc $(pkg-config --cflags --libs libnl-genl-3.0) scan.c -o scan
*/

int main() {
    int if_index = if_nametoindex("wlan1");
    printf("ifindex: %d\n", if_index);
    return 0;
}