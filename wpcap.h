#include <pcap.h>

typedef struct {
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
} wifi_pcap_t;

void init_pcap(wifi_pcap_t *wpt, const char *dev_name, const char *filter);

void view_packets(wifi_pcap_t *wpt, int ITER);
