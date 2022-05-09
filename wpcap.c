#include "wpcap.h"

int init_pcap(wifi_pcap_t *wpt, char *dev_name, char *filter) {
    char *dev = pcap_lookupdev(dev_name);
    if (!dev) {fprintf(stderr, "Couldn't find %s", dev_name, wpt->errbuf);} //Quick additional check.

    wpt->handle = pcap_open_live(dev, BUFSIZ, 0, 1000, wpt->errbuf);
    if (!wpt->handle) {
        fprintf(stderr, "Could not init pcap handle.\n", wpt->errbuf);
        return -1;
    }

    if (pcap_datalink(handle) != DLT_IEEE802_11_RADIO) {
        fprintf(stderr, "%s is not a compatibly link layer type.\n", dev_name, wpt->errbuf);
        return -1;
    }

    struct bpf_program fp;
    bpf_int32 netmask;
    if (pcap_compile(wpt->handle, &fp, filter, 0, netmask) < 0) {
        fprintf(stderr, "Could not compile BPF expression:\n\t%s.\n", filter, wpt->errbuf);
        return -1;
    }
    if (pcap_setfilter(wpt->handle, &fp) < 0) {
        fprintf(stderr, "Could not apply BPF fitler.\n", wpt->errbuf);
        //return -1;
    }
}