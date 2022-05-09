#include "wpcap.h"

/*
 *  TODO: Implement timestamp, eth_hdr casts & printing bytes.
 *  Callback function for pcap_loop().
 */
void packet_callback(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *pkt) {
    // struct tm *time;
    // char timestr[16];
    // time_t secs;

    int len = 0;
    while (len < pkthdr->len) {
        printf("%.2X:", *pkt++);
        //Find new lines for 16-byte width packets.
        if (!(++len % 16)) printf("\n");
    }
    printf("\n");
}

int init_pcap(wifi_pcap_t *wpt, const char *dev_name, const char *filter) {
    char *dev = pcap_lookupdev(dev_name);

    //See: https://www.tcpdump.org/linktypes.html for libpcap Link Layer Types.
    if (!dev) {fprintf(stderr, "Couldn't find %s", dev_name, wpt->errbuf);} //Quick additional check.

    //For flag (3rd arg), nonzero means promiscuous.
    wpt->handle = pcap_open_live(dev, BUFSIZ, 1, 1000, wpt->errbuf);
    if (!wpt->handle) {
        fprintf(stderr, "Could not init pcap handle.\n", wpt->errbuf);
        return -1;
    }

    if (pcap_datalink(wpt->handle) != DLT_IEEE802_11_RADIO) {   //versus just DLT_IEEE802_11
        fprintf(stderr, "%s is not a compatible with radiotap link layer info followed by 80211 header.\n", dev_name, wpt->errbuf);
        return -1;
    }

    struct bpf_program fp;
    bpf_int32 netmask; //=0xffffff // ==> C class network.
    if (pcap_compile(wpt->handle, &fp, filter, 0, netmask) < 0) {
        fprintf(stderr, "Could not compile BPF expression:\n\t%s.\n", filter, wpt->errbuf);
        return -1;
    }
    if (pcap_setfilter(wpt->handle, &fp) < 0) {
        fprintf(stderr, "Could not apply BPF fitler.\n", wpt->errbuf);
        //return -1;
    }
}

void view_packets(wifi_pcap_t  *wpt, int ITER) {
    int ret = pcap_loop(wpt->handle, ITER, packet_callback, NULL);
    if (ret < 0) {
        fprintf(stderr, "Unable to capture %d packets.\n", ITER, wpt->errbuf);
    }
}