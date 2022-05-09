#include "wpcap.h"
#include "linux/if_ether.h"

/*
 *  Simple utility for printing bytes in hex format.
 */
void print_bytes_hex(void *data, size_t len) {
    const u_char *bytes = data;
    size_t count;
    for (count=0; count < len; count++) {
        if (count == len-1) {
        printf("%.2X", bytes[count]);
        } else {
            printf("%.2X:", bytes[count]);
        }
    }
}

/*
 *  TODO: Implement timestamp, eth_hdr casts & printing bytes.
 *  Callback function for pcap_loop().
 */
void packet_callback(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *pkt) {
    // struct tm *time;
    // char timestr[16];
    // time_t secs;

    struct ethhdr *hdr = (struct ethhdr *)pkt;
    printf("\nSRC ADDR:\t");
    print_bytes_hex(&(hdr->h_source), 6);
    printf("\nDEST ADDR:\t");
    print_bytes_hex(&(hdr->h_dest), 6);
    printf("\nETH PROTO:\t");
    print_bytes_hex(&(hdr->h_proto), 1);
    printf("\n");
}

int init_wpcap(wifi_pcap_t *wpt, const char *dev_name, struct bpf_program *fp, const char *filter) {

    //See: https://www.tcpdump.org/linktypes.html for libpcap Link Layer Types.
    //if (!dev) {fprintf(stderr, "Couldn't find %s", dev_name, wpt->errbuf);} //Redundant device check when called explicitly with dev.

    //For flag (3rd arg), nonzero means promiscuous.
    printf("Opening pcap session on %s...\n", dev_name);
    wpt->handle = pcap_open_live(dev_name, BUFSIZ, 1, 1000, wpt->errbuf);
    if (!wpt->handle) {
        fprintf(stderr, "Could not init pcap handle.\n");//, wpt->errbuf);
        return -1;
    }

    printf("Verifying data link type...\n");
    if (pcap_datalink(wpt->handle) != DLT_IEEE802_11_RADIO) {   //versus just DLT_IEEE802_11
        fprintf(stderr, "%s is not a compatible with radiotap link layer info followed by 80211 header.\n", dev_name);//, wpt->errbuf);
        return -1;
    }

    printf("Compiling bpf filter: '%s'...\n", filter);
    bpf_int32 netmask = 0; //0xffffff; // ==> C class network.
    if (pcap_compile(wpt->handle, fp, filter, 0, netmask) < 0) {
        fprintf(stderr, "Could not compile BPF expression:\n\t%s.\n", filter);//, wpt->errbuf);
        return -1;
    }
    printf("Setting filter...\n");
    if (pcap_setfilter(wpt->handle, fp) < 0) {
        fprintf(stderr, "Could not apply BPF fitler.\n");//, wpt->errbuf);
        //return -1;
    }
}

void cleanup_wpcap(wifi_pcap_t *wpt, struct bpf_program *fp) {
    pcap_freecode(fp);
    pcap_close(wpt->handle);
}

void view_packets(wifi_pcap_t  *wpt, int ITER) {
    printf("Waiting for packets...\n");
    int ret = pcap_loop(wpt->handle, ITER, packet_callback, NULL);
    if (ret < 0) {
        fprintf(stderr, "Unable to capture %d packets.\n", ITER);//, wpt->errbuf);
    }
}