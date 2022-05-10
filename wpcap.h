#include <pcap.h>
#include "mcpcap.h"


typedef struct {
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
} wifi_pcap_t;

int init_wpcap(wifi_pcap_t *wpt, const char *dev_name, struct bpf_program *fp, const char *filter);

void cleanup_wpcap(wifi_pcap_t *wpt, struct bpf_program *fp);

void view_packets(wifi_pcap_t *wpt, int ITER);
