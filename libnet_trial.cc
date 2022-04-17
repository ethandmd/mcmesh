#include <libnet.h>
#include <iostream>

/*
*   Compile with:
*       g++ libnet_trial.cc -o libnet_trial -lnet
*   Run with:
*       sudo ./libnet_trial [dev]
*/

int main(int argc, char *argv[]) {
    libnet_t *handle;
    char *dev;
    char errbuf[LIBNET_ERRBUF_SIZE];

    if (argc > 1) {
        dev = argv[1];
        std::cout << "Selected: " << dev << "\n";
    } else {
        std::cerr << "Need at least one argument: dev.\n";
        return -1;
    }

    handle = libnet_init(LIBNET_LINK, dev, errbuf);
    if (handle == nullptr) {
        std::cerr << "Error libnet_init(): " << errbuf << "\n";
        return -1;
    }
    libnet_destroy(handle);

    return 0;
}