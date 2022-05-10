#include "wpcap.h"
#include "linux/if_ether.h"

/*
 *  TODO: Implement timestamp, appropriate casts & printing bytes.
 *  Callback function for pcap_loop().
 */
void packet_callback(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *pkt) {
    struct radiotap_header *rthdr;
    rthdr = (struct radiotap_header *)pkt;  //If nothing else from this project: I <3 casting.
    int offset = rthdr->it_len;     //Get radiotap offset

    enum frame_type type;
    frame_ctrl *fctrl = (frame_ctrl *)pkt + offset; //To radiotap or to not radiotap...
    get_frame_type(&(fctrl->fc[0]), &type);
    handle_frame(pkt, &type);
}

int init_wpcap(wifi_pcap_t *wpt, const char *dev_name, struct bpf_program *fp, const char *filter, int monitor) {

    //See: https://www.tcpdump.org/linktypes.html for libpcap Link Layer Types.
    //if (!dev) {fprintf(stderr, "Couldn't find %s", dev_name, wpt->errbuf);} //Redundant device check when called explicitly with dev.

    //For flag (3rd arg), nonzero means promiscuous.
    printf("Opening pcap session on %s...\n", dev_name);
    wpt->handle = pcap_open_live(dev_name, BUFSIZ, 1, 1000, wpt->errbuf);
    if (!wpt->handle) {
        fprintf(stderr, "Could not init pcap handle.\n");//, wpt->errbuf);
        return -1;
    }

    if (monitor && pcap_datalink(wpt->handle) != DLT_IEEE802_11_RADIO) {   //versus just DLT_IEEE802_11
        fprintf(stderr, "%s is not compatible with radiotap link layer info followed by 80211 header.\n", dev_name);//, wpt->errbuf);
        return -1;
    }

    printf("Compiling bpf filter: '%s'...\n", filter);
    bpf_int32 netmask = 0; //0xffffff; // ==> C class network.
    if (pcap_compile(wpt->handle, fp, filter, 0, netmask) < 0) {
        fprintf(stderr, "Could not compile BPF expression:\n\t%s.\n", filter);//, wpt->errbuf);
        return -1;
    }
    if (pcap_setfilter(wpt->handle, fp) < 0) {
        fprintf(stderr, "Could not apply BPF fitler.\n");//, wpt->errbuf);
        //return -1;
    }
}

void cleanup_wpcap(wifi_pcap_t *wpt, struct bpf_program *fp) {
    pcap_freecode(fp);
    pcap_close(wpt->handle);
}

void eth_pkt_callback(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *pkt) {
    struct ethhdr *eth = (struct ethhdr *)(pkt);
    printf("\nReceived Ethernet Header:\n");
    printf("Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
    printf("Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
    printf("Protocol : %d\n",eth->h_proto);
    printf("\n");
}

void view_packets(wifi_pcap_t  *wpt, int ITER, int monitor) {
    printf("Waiting for packets...\n");
    int ret;
    if (monitor) {
        pcap_loop(wpt->handle, ITER, packet_callback, NULL);
    } else {
        pcap_loop(wpt->handle, ITER, eth_pkt_callback, NULL);
    }
    if (ret < 0) {
        fprintf(stderr, "Unable to capture %d packets.\n", ITER);//, wpt->errbuf);
    }
}