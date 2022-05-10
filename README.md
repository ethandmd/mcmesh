# mcmesh
This program is designed to sniff 802.11 frames "in the air" or ethernet frames from a wifi interface on a local network.
To run this program you will need:
- Linux. 
- A wifi chipset that:
    - has a driver which can support nl80211 (ref. [drivers](https://wireless.wiki.kernel.org/en/users/drivers)).
    - is capable of supporting a monitor mode interface (this program can detect if you have one or not).
- [Dependencies](#Install-Dependencies)


## Build & Run
```
$ make
$ sudo ./mcmesh [-interface] {interface name} [-count] {No. of packets to capture} [-type] {capture interface type (i.e. "monitor")}
```
*For interface type, only support station (managed) mode, or monitor mode.

### Install-Dependencies
1) Install [libnl](https://www.infradead.org/~tgr/libnl/) with headers for dev (or in the future possibly go to libmnl, libnl tiny (OpenWRT)) in order to use the ```nl80211.h``` enums.
Most linux distributions have libnl via their respective package managers.

+ Example (Ubuntu 21.04):
```
$ sudo apt install libnl-genl-3-dev
```
+ Example (Fedora 35):
```
$ sudo dnf install libnl3-devel
```

2) Install [libpcap](https://github.com/the-tcpdump-group/libpcap) (dev). After building up a version using just packet sockets, for the sake of development speed, it was significantly faster to rely on some of the pre-built ```pcap``` functions.

+ Example (Ubuntu 21.04):
```
$ sudo apt install lipcap-dev
```
+ Example (Fedora 35):
```
$ sudo dnf install libpcap-devel
```

    
 
#### Reference
+ Libnl: [Libnl developer docs](https://www.infradead.org/~tgr/libnl/doc/core.html)
+ iw (J. Berg.): [iw git repo](http://git.kernel.org/?p=linux/kernel/git/jberg/iw.git)
+ Libpcap nl80211 examples: [pcap-linux](https://github.com/the-tcpdump-group/libpcap/blob/master/pcap-linux.c)
+ Kismet Wireless [rf_kill linux](https://github.com/kismetwireless/kismet/blob/master/capture_linux_wifi/linux_wireless_rfkill.c)
+ Link layer headers from [libpcap](https://www.tcpdump.org/linktypes.html)
+ Documentation for [radiotap](https://radiotap.org)
+ Working with [radiotap](https://www.oreilly.com/library/view/network-security-tools/0596007949/ch10s03.html)
+ Wireshark [Capture Setup](https://gitlab.com/wireshark/wireshark/-/wikis/CaptureSetup/WLAN#linux)
+ Netlink: [RFC 3549](https://datatracker.ietf.org/doc/html/rfc3549) 
+ Driver for Alfa wireless chipset (RTL8812au) for RPI3 from aircrack-ng:
```
$ git clone -b v5.6.4.2 https://github.com/aircrack-ng/rtl8812au.git
$ cd rtl*
$ sudo apt-get install raspberrypi-kernel-headers
#$ sed -i 's/CONFIG_PLATFORM_I386_PC = y/CONFIG_PLATFORM_I386_PC = n/g' Makefile    #No workie
$ make && sudo make install   #then reboot
```
