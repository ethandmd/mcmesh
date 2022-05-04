#include <libnet.h>
#include <iostream>

/*
*   Compile with:
*       g++ libnet_trial.cc -o libnet_trial -lnet
*   Run with:
*       sudo ./libnet_trial [dev]
*/

/*
*   1. Init network
*   2. Init memory
*   3. Build packet
*   4. Perform checksum
*   5. Inject Packet
*/

int main(int argc, char *argv[]) {
    libnet_t *handle;
    char *dev;
    u_int8_t dst_mac;
    char errbuf[LIBNET_ERRBUF_SIZE];

    if (argc > 1) {
        dev = argv[1];
        std::cout << "Selected: " << dev << "\n";
    } else if (argc > 2) {
        dst_mac = int(argv[2]);
    } else {
        std::cerr << "Need at least one argument: dev.\n";
        return -1;
    }

    handle = libnet_init(LIBNET_LINK, dev, errbuf);
    if (handle == nullptr) {
        std::cerr << "Error libnet_init(): " << errbuf << "\n";
        return -1;
    }
    libnet_ether_addr *src_mac;
    src_mac = libnet_get_hwaddr(handle);

    libnet_destroy(handle);

    return 0;
}