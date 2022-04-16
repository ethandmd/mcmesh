#include <iostream>
#include <pcap.h>
#include <string>

int main () {
    pcap_if_t *alldevsp;
    pcap_if_t *d;
    char errbuf[PCAP_ERRBUF_SIZE];
    
    /*
    *   writes all net if devs to linked list.
    */
    if (pcap_findalldevs(&alldevsp, errbuf) != 0) {
        std::cout << "Error reading devs.\n";
        return -1;
    }

    for (d = alldevsp; d != nullptr; d = d->next) {
        char *desc = d->name;
        std::cout << desc << "\n";
    }

    return 0;
}