#include "wpcap.h"
#include <linux/if_ether.h>
#include <stdint.h>
#include <assert.h>

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
 *  Frame Control Field for figuring out what is happening.
 */
typedef struct {
    unsigned char fc[2];      /* First two bytes of the MAC header */
} frame_ctrl;

/*
 *  802.11 mgmt frame.
*/
typedef struct {                        /* Typically:   */
    frame_ctrl frame_control;           /* (ver/type/subtype) + (flags)*/
    unsigned char frame_duration[2];    /* (TO DS = 0/FROM DS = 0/....) + (....) */
    unsigned char address1[6];          /* Source mac addr */
    unsigned char address2[6];          /* Dest mac addr */ 
    unsigned char address3[6];          /* Tx mac addr */
    unsigned char seq_ctrl[2];          /* frame sequence */
} mgmt_frame;


/*
 *  Wikipedia 802.11 frame subtypes. Only choosing to care about a few of these.
 *  Frame control field is 2 bytes. First byte is ordered:
 *  |B0 B1| |B2 B3| |B4 B5 B6 B7|
 *  |.ver.| |.type| |..subtype..|
 * 
 *  So, for mgmt frames, type & version is always zero...
 *  Second byte has TO DS and FROM DS as B0, B1. We will only care about these bits.
 */

enum frame_type {
    MGMT,
    CTRL,
    DATA
};

/*
 * Yes, redundant use of enum-ing. Bear with me.
 */
enum mgmt_frame_type {
    ASSOCIATION_REQEUST = 0x0,
    ASSOCIATION_RESPONSE = 0X1,
    REASSOCIATION_REQUEST = 0x2,
    REASSOCIATION_RESPONSE = 0X3,
    PROBE_REQUEST = 0x4,
    PROBE_RESPONSE = 0X5,
    TIMING_ADVERTISEMENT = 0x6,
    RESERVED = 0X7,
    BEACON = 0X8,
    DISASSOCIATION = 0XA,
    AUTHENTICATION = 0XB,
    DEAUTHENTICATION = 0XC,
    ACTION = 0XE,
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
 *  Find the frame type.
 */
void get_frame_type(const u_char *byte, enum frame_type *type) {
    if (*byte & 0x8) {
        *type = DATA;
    } 
    if (*byte & 0x4) {
        *type = CTRL;
    }
    assert(!(*byte & 0x8) && !(*byte & 0x4));
    *type = MGMT;
}


void print_mgmt_subtype(const u_char byte) {
    printf("\nMGMT FRAME SUBTYPE: %s", mgmt_subtype_strings[(int)(byte)]);
}
/*
 *  Could write data to somewhere interesting...but instead
 *  this function prints packet data as appropriate.
 *  Should have TO DS = 0, FROM DS = 0 (STA to STA)
 */
void handle_mgmt_frame(const u_char *pkt) {
    mgmt_frame *frame = (mgmt_frame *)(pkt);
    assert(!(frame->frame_control.fc[1] & 0x80) && !(frame->frame_control.fc[1] & 0x40));   /* Sanity check for TO/FROM DS. */
    print_mgmt_subtype(frame->frame_control.fc[0]);
    printf("DEST MAC:\t");
    print_bytes_hex(&(frame->address1), 6);
    printf("SRC MAC:\t");
    print_bytes_hex(&(frame->address2), 6);
    printf("BSSID MAC:\t");
    print_bytes_hex(&(frame->address3), 6);
    printf("\n");
}

/*
 *  Not yet implemented.
 */
void handle_ctrl_frame(const u_char *pkt) {
    printf("CTRL FRAME\n");
}
void handle_data_frame(const u_char *pkt) {
    printf("DATA FRAME\n");
}

/*
 *  Pass frame off to appropriate type handler.
 */
void handle_frame(const u_char *pkt, enum frame_type *type) {
    switch (*type) {
        case MGMT:
            handle_mgmt_frame(pkt);
        case CTRL:
            handle_ctrl_frame(pkt);
        case DATA:
            handle_data_frame(pkt);
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
    enum frame_type type;
    frame_ctrl *fctrl = (frame_ctrl *)pkt;
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