#include "wpcap.h"
#include <linux/if_ether.h>
#include <stdint.h>

/*
 *  From radiotap.org
 *  De facto (linux?) standard for 80211 rx/tx.
 *  Mostly just want the offset, don't care as much about fine details at the moment.
 */
struct radiotap_header {
    uint8_t it_rev; //Radiotap version, set to 0?
    uint8_t it_pad; //Alignment padding -- word boundaries
    uint8_t it_len; //Get entire radiotap header
    uint8_t it_present; //Fields present
};

/*
 *  802.11ac MAC Frame.
             subtype-type-version 
 *  FC mgmt: XXXX   - 00 - 00       (i.e. probe request, beacon, auth/deauth, etc)
 *  FC ctrl: XXXX   - 01 - 00
 *  FC data: XXXX   - 10 - 00
*/
typedef struct {
    unsigned char frame_control[2];     /* mgmt, ctrl, data */
    unsigned char frame_duration[2];
    unsigned char source[6];            /* RX mac addr */
    unsigned char destination[6];       /* TX mac addr */ 
    unsigned char bssid[6];         /* Filtering (BSS?) ID */
    unsigned char seq_ctrl[2];          /* frame sequence */
} ass_req_frame;

/*
 *  Wikipedia 802.11 mgmt frame subtypes. Only choosingto care about a few of these.
 */
enum mgmt_frame_subtype {
    ASSOCIATION_REQEUST = 0x0,
    REASSOCIATION_REQUEST = 0x2,
    PROBE_REQUEST = 0x4,            /* This */
    TIMING_ADVERTISEMENT = 0x6,
    BEACON = 0X8,                   /* This */
    DISASSOCIATION = 0XA,
    DEAUTHENTICATION = 0XC,
    AUTHENTICATION = 0XB,
    ACTION = 0XE,
    ASSOCATION_RESPONSE = 0X1,
    REASSOCIATION_RESPONSE = 0X3,
    PROBE_RESPONSE = 0X5,           /* This */
    RESERVED = 0X7
};

const char * mgmt_subtype_strings[] = {
    "ASSOCIATION_REQEUST",
    "REASSOCIATION_REQUEST",
    "PROBE_REQUEST",
    "TIMING_ADVERTISEMENT",
    "BEACON",
    "DISASSOCIATION",
    "DEAUTHENTICATION",
    "AUTHENTICATION",
    "ACTION",
    "ASSOCATION_RESPONSE",
    "REASSOCIATION_RESPONSE",
    "PROBE_RESPONSE",
    "RESERVED"
};

void print_mgmt_subtype(unsigned char block) {
    printf("\nMGMT FRAME SUBTYPE: %s", mgmt_subtype_strings[(int)(block)]);
}

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
 *  TODO: Implement timestamp, appropriate casts & printing bytes.
 *  Callback function for pcap_loop().
 */
void packet_callback(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *pkt) {
    // struct radiotap_header *rthdr;
    // rthdr = (struct radiotap_header *)pkt;  //If nothing else from this project: I <3 casting.
    // int offset = rthdr->it_len;     //Get radiotap offset


    ass_req_frame *frame = (ass_req_frame *)pkt;
    if (frame->frame_control[0] == 0) {
        printf("802.11 MGMT FRAME:\n");
    }
    print_mgmt_subtype(frame->frame_control[1]);
    printf("\nSRC ADDR:\t");
    print_bytes_hex(&(frame->source), 6);
    printf("\nDEST ADDR:\t");
    print_bytes_hex(&(frame->destination), 6);
    printf("\nBSSID:\t");
    print_bytes_hex(&(frame->bssid), 6);
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