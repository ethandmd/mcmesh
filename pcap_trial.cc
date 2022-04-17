#include <iostream>
#include <pcap.h>
#include <string.h>
#include <stdio.h>

/*
*   Compile with:
*   g++ pcap_trial.cc -o pcap_trial -lpcap
*   
*   Run with:
*   sudo ./pcap_trial wlan0    #For socket permissions
*/

int main (int argc, char *argv[]) {

    pcap_if_t *alldevsp;            //Linked list of all devs.
    pcap_if_t *itd;                 //Interface type for scanning devs.
    char *dev;                      //Capture device.
    char errbuf[PCAP_ERRBUF_SIZE];
    //struct bpf_program fp;        //Compiled BPF filter expression.
    pcap_t *handle;     //Session handler.
    struct pcap_pkthdr header;      //Packet header from pcap.
    const u_char *packet;                 //Actual packet.

    if (argc > 1) {
        dev = argv[1];
        std::cout << "Starting pcap session on: " <<dev << "\n";
    } else {
        std::cerr << "Expected one CLI arg for net if dev name.\n";
        return -1;
    }
    
    /*
    *   Write all netword interface devices to linked list.
    */
    std::cout << "Checking all network interface devices...\n";
    if (pcap_findalldevs(&alldevsp, errbuf) != 0) {
        std::cout << "Error reading devs.\n";
        return -1;
    }

    /*
    *   Iterate through found devs.
    */
    for (itd = alldevsp; itd != nullptr; itd = itd->next) {
        char *name = itd->name;
        if (!strcmp(name, dev)) {
            std::cout << "Found target device: "<< name << "\n";
            continue;
        } else {
            std::cout << "Not the device we're looking for: " << name << "\n";
        }
    }
    pcap_freealldevs(alldevsp);     //Delete devs data.

    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);      //Start session.

    if (handle == nullptr) {
        std::cerr << "Couldn't open device: " << dev << "\n";
        std::cout << errbuf << "\n";
        return -1;
    }

    packet = pcap_next(handle, &header);
    std::cout << "Capd packet with length: " << header.len << "\n";
    for (int i = 0; i < header.len; i++) {
        if (isprint(packet[i])) {
        printf("%c", packet[i]);
        } else {
            printf(" . ", packet[i]);
        }
        if ((i%16 == 0 && i != 0) || i == header.len-1) {
            printf("\n");
        }
    }
    pcap_close(handle);
    return 0;
}