#include "wpcap.h"

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