#include <pcap.h>

typedef struct {
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
} wifi_pcap_t;

void init_pcap(wifi_pcap_t *wpt, char *dev, char *filter);


